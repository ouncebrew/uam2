#include "uam2.h"
#include "abi.h"
#include "nir.h"
#include "nir_spirv.h"
#include "nak.h"
#include "nv_device_info.h"
#include "compiler/glsl_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct nak_compiler* s_nak;

void uam2_init(void)
{
	glsl_type_singleton_init_or_ref();
	s_nak = nak_compiler_create(&uam2_t239_device);
#ifndef UAM2_RUNTIME
	uam2_glsl_init(nak_nir_options(s_nak));
#endif
}

void uam2_exit(void)
{
#ifndef UAM2_RUNTIME
	uam2_glsl_exit();
#endif
	nak_compiler_destroy(s_nak);
	s_nak = NULL;
	glsl_type_singleton_decref();
}

static mesa_shader_stage to_mesa_stage(enum uam2_stage stage)
{
	switch (stage)
	{
		default:
		case Uam2Stage_Vertex:   return MESA_SHADER_VERTEX;
		case Uam2Stage_TessCtrl: return MESA_SHADER_TESS_CTRL;
		case Uam2Stage_TessEval: return MESA_SHADER_TESS_EVAL;
		case Uam2Stage_Geometry: return MESA_SHADER_GEOMETRY;
		case Uam2Stage_Fragment: return MESA_SHADER_FRAGMENT;
		case Uam2Stage_Compute:  return MESA_SHADER_COMPUTE;
	}
}

static void shared_var_info(const struct glsl_type* type, unsigned* size, unsigned* align)
{
	uint32_t comp_size = glsl_type_is_boolean(type) ? 4 : glsl_get_bit_size(type) / 8;
	*size = comp_size * glsl_get_vector_elements(type);
	*align = comp_size;
}

static bool compile_nir(nir_shader* nir, enum uam2_stage stage, bool dump_asm, struct uam2_program* out,
	struct uam2_log* log)
{
	nak_preprocess_nir(nir, s_nak);

	const nir_lower_compute_system_values_options csv_opts =
	{
		.lower_local_invocation_index = true,
	};
	NIR_PASS(_, nir, nir_lower_compute_system_values, &csv_opts);

	if (nir->info.stage == MESA_SHADER_FRAGMENT)
		NIR_PASS(_, nir, nir_lower_fragcolor, 8);

	NIR_PASS(_, nir, nir_opt_large_constants, NULL, 32);
	NIR_PASS(_, nir, nir_lower_load_const_to_scalar);

	uam2_lower_descriptors(nir);
	NIR_PASS(_, nir, nir_lower_ssbo, NULL);
	NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_global, nir_address_format_64bit_global);

	if (!uam2_lower_abi(nir, log))
	{
		ralloc_free(nir);
		return false;
	}

	if (mesa_shader_stage_uses_workgroup(nir->info.stage))
	{
		NIR_PASS(_, nir, nir_lower_vars_to_explicit_types, nir_var_mem_shared, shared_var_info);
		NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_shared, nir_address_format_32bit_offset);
	}

	nak_lower_nir_before_linking(nir, s_nak);
	nir->info.io_lowered = true;

	if (nir->constant_data_size)
	{
		out->data = malloc(nir->constant_data_size);
		memcpy(out->data, nir->constant_data, nir->constant_data_size);
		out->data_size = nir->constant_data_size;
	}

	struct nak_shader_bin* bin = nak_compile_shader(nir, dump_asm, s_nak, 0, NULL, false);
	ralloc_free(nir);
	if (!bin)
	{
		uam2_logf(log, "error: NAK compilation failed\n");
		uam2_program_free(out);
		return false;
	}

	out->stage = stage;
	out->code = malloc(bin->code_size);
	memcpy(out->code, bin->code, bin->code_size);
	out->code_size = bin->code_size;
	out->num_gprs = bin->info.num_gprs;
	out->num_instrs = bin->info.num_instrs;
	out->slm_size = bin->info.slm_size;
	out->num_barriers = bin->info.num_control_barriers;
	if (stage == Uam2Stage_Compute)
	{
		for (unsigned i = 0; i < 3; i ++)
			out->block_dims[i] = bin->info.cs.local_size[i];
		out->shared_mem_size = bin->info.cs.smem_size;
	}
	else if (stage == Uam2Stage_TessCtrl || stage == Uam2Stage_TessEval)
	{
		out->ts.domain = bin->info.ts.domain;
		out->ts.spacing = bin->info.ts.spacing;
		out->ts.ccw = bin->info.ts.ccw;
		out->ts.point_mode = bin->info.ts.point_mode;
	}
	else if (stage == Uam2Stage_Fragment)
	{
		out->fs.writes_depth = bin->info.fs.writes_depth;
		out->fs.early_fragment_tests = bin->info.fs.early_fragment_tests;
		out->fs.post_depth_coverage = bin->info.fs.post_depth_coverage;
		out->fs.uses_sample_shading = bin->info.fs.uses_sample_shading;
	}
	memcpy(out->hdr, bin->info.hdr, sizeof(out->hdr));
	if (bin->asm_str)
		out->asm_str = strdup(bin->asm_str);

	nak_shader_bin_destroy(bin);
	return true;
}

#ifndef UAM2_RUNTIME
bool uam2_compile_glsl(const char* source, enum uam2_stage stage, bool dump_asm, struct uam2_program* out,
	struct uam2_log* log)
{
	memset(out, 0, sizeof(*out));

	nir_shader* nir = uam2_glsl_to_nir(source, stage, log);
	if (!nir)
		return false;

	return compile_nir(nir, stage, dump_asm, out, log);
}
#endif

#ifdef UAM2_RUNTIME
static void spirv_log(void* data, enum nir_spirv_debug_level level, size_t spirv_offset, const char* message)
{
	(void)spirv_offset;
	if (level >= NIR_SPIRV_DEBUG_LEVEL_WARNING)
		uam2_logf((struct uam2_log*)data, "%s\n", message);
}
#endif

bool uam2_compile_spirv(const uint32_t* words, size_t num_words, enum uam2_stage stage,
	const char* entrypoint, const struct uam2_spec_const* spec, unsigned num_spec, bool dump_asm,
	struct uam2_program* out, struct uam2_log* log)
{
	memset(out, 0, sizeof(*out));

	struct nir_spirv_specialization* spec_info = NULL;
	if (num_spec)
	{
		spec_info = vtn_alloc_specialization(num_spec);
		if (!spec_info)
			return false;
		for (unsigned i = 0; i < num_spec; i ++)
			if (!vtn_add_specialization_entry(spec_info, i, spec[i].id, sizeof(spec[i].value), &spec[i].value, false))
			{
				vtn_free_specialization(spec_info);
				return false;
			}
	}

	const struct spirv_to_nir_options spirv_opts =
	{
		.ssbo_addr_format = nir_address_format_32bit_index_offset,
		.phys_ssbo_addr_format = nir_address_format_64bit_global,
		.ubo_addr_format = nir_address_format_32bit_index_offset,
		.shared_addr_format = nir_address_format_32bit_offset,
		.min_ubo_alignment = DK_UNIFORM_BUF_ALIGNMENT,
		.min_ssbo_alignment = UAM2_STORAGE_BUF_ALIGNMENT,
		.skip_os_break_in_debug_build = true,
#ifdef UAM2_RUNTIME
		.debug = { .func = spirv_log, .private_data = log },
#endif
	};

	nir_shader* nir = spirv_to_nir(words, num_words, spec_info, to_mesa_stage(stage), entrypoint,
		&spirv_opts, nak_nir_options(s_nak));
	vtn_free_specialization(spec_info);
	if (!nir)
	{
		uam2_logf(log, "error: SPIR-V to NIR translation failed\n");
		return false;
	}

	NIR_PASS(_, nir, nir_lower_variable_initializers, nir_var_function_temp);
	NIR_PASS(_, nir, nir_lower_returns);
	NIR_PASS(_, nir, nir_inline_functions);
	NIR_PASS(_, nir, nir_opt_copy_prop);
	NIR_PASS(_, nir, nir_opt_deref);
	nir_remove_non_entrypoints(nir);

	// Borrowed from Mesa's vk_spirv_to_nir
	NIR_PASS(_, nir, nir_lower_variable_initializers, ~0);
	NIR_PASS(_, nir, nir_split_var_copies);
	NIR_PASS(_, nir, nir_split_per_member_structs);
	NIR_PASS(_, nir, nir_remove_dead_variables,
		nir_var_shader_in | nir_var_shader_out | nir_var_system_value, NULL);
	nir_gather_clip_cull_distance_sizes_from_vars(nir);
	NIR_PASS(_, nir, nir_merge_clip_cull_distance_vars);

	const nir_opt_access_options access_opts = { .is_vulkan = true };
	NIR_PASS(_, nir, nir_opt_access, &access_opts);

	if (!uam2_check_abi(nir, log))
	{
		ralloc_free(nir);
		return false;
	}

	return compile_nir(nir, stage, dump_asm, out, log);
}

void uam2_program_free(struct uam2_program* prog)
{
	free(prog->code);
	free(prog->data);
	free(prog->asm_str);
	memset(prog, 0, sizeof(*prog));
}
