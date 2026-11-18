#include "uam2.h"
#include "abi.h"
#include "nir.h"
#include "nir_builder.h"
#include <stdio.h>

struct abi_state
{
	bool ok;
};

static void abi_error(struct abi_state* state, const char* msg)
{
	fprintf(stderr, "error: %s\n", msg);
	state->ok = false;
}

static bool is_compute(const nir_builder* b)
{
	return b->shader->info.stage == MESA_SHADER_COMPUTE;
}

static nir_def* ldc(nir_builder* b, unsigned num_components, unsigned bit_size, unsigned cbuf, nir_def* offset)
{
	return nir_ldc_nv(b, num_components, bit_size, nir_imm_int(b, cbuf), offset,
		.align_mul = bit_size / 8, .align_offset = 0);
}

static nir_def* storage_desc_offset(nir_builder* b, nir_def* index)
{
	unsigned base = is_compute(b) ? UAM2_CS_STORAGE_DESC_OFFSET(0) : UAM2_GFX_STORAGE_DESC_OFFSET(b->shader->info.stage, 0);
	return nir_iadd_imm(b, nir_imul_imm(b, index, UAM2_BUF_DESC_SIZE), base);
}

static nir_def* load_desc_addr(nir_builder* b, nir_def* desc_offset)
{
	return nir_pack_64_2x32(b, ldc(b, 2, 32, UAM2_DRIVER_CBUF, desc_offset));
}

static bool lower_load_ubo(nir_builder* b, nir_intrinsic_instr* intr, struct abi_state* state)
{
	nir_def* index = intr->src[0].ssa;
	nir_def* offset = intr->src[1].ssa;
	bool const_index = nir_src_is_const(intr->src[0]);
	unsigned num_cbufs = is_compute(b) ? UAM2_CS_NUM_UBO_CBUFS : DK_NUM_UNIFORM_BUFS;

	nir_def* val;
	if (const_index && nir_src_as_uint(intr->src[0]) < num_cbufs)
	{
		val = nir_ldc_nv(b, intr->def.num_components, intr->def.bit_size,
			nir_imm_int(b, UAM2_FIRST_USER_CBUF + nir_src_as_uint(intr->src[0])), offset,
			.access = nir_intrinsic_access(intr),
			.align_mul = nir_intrinsic_align_mul(intr),
			.align_offset = nir_intrinsic_align_offset(intr));
	}
	else if (is_compute(b))
	{
		nir_def* desc = nir_iadd_imm(b, nir_imul_imm(b, index, UAM2_BUF_DESC_SIZE), UAM2_CS_UNIFORM_DESC_OFFSET(0));
		nir_def* addr = nir_iadd(b, load_desc_addr(b, desc), nir_u2u64(b, offset));
		val = nir_load_global_constant(b, intr->def.num_components, intr->def.bit_size, addr,
			.align_mul = nir_intrinsic_align_mul(intr),
			.align_offset = nir_intrinsic_align_offset(intr));
	}
	else
	{
		abi_error(state, "dynamically indexed uniform blocks are only supported in compute shaders");
		return false;
	}

	nir_def_replace(&intr->def, val);
	return true;
}

static nir_def* deref_binding(nir_builder* b, nir_deref_instr* deref)
{
	if (deref->deref_type == nir_deref_type_var)
		return nir_imm_int(b, deref->var->data.binding);

	assert(deref->deref_type == nir_deref_type_array);
	nir_deref_instr* parent = nir_deref_instr_parent(deref);
	nir_def* base = deref_binding(b, parent);
	nir_def* elem = nir_imul_imm(b, deref->arr.index.ssa, glsl_get_aoa_size(deref->type) ? glsl_get_aoa_size(deref->type) : 1);
	return nir_iadd(b, base, elem);
}

static nir_def* load_handle(nir_builder* b, nir_deref_instr* deref, bool image)
{
	nir_def* binding = deref_binding(b, deref);
	unsigned base = image ? UAM2_IMAGE_HANDLE_OFFSET(0) : UAM2_TEXTURE_HANDLE_OFFSET(0);
	return ldc(b, 1, 32, UAM2_HANDLE_CBUF, nir_iadd_imm(b, nir_imul_imm(b, binding, 8), base));
}

static bool lower_tex(nir_builder* b, nir_tex_instr* tex, struct abi_state* state)
{
	nir_deref_instr* texture = nir_steal_tex_deref(tex, nir_tex_src_texture_deref);
	nir_deref_instr* sampler = nir_steal_tex_deref(tex, nir_tex_src_sampler_deref);
	if (texture && sampler && nir_deref_instr_get_variable(texture) != nir_deref_instr_get_variable(sampler))
	{
		abi_error(state, "separate texture and sampler objects are not supported");
		return false;
	}
	if (!texture)
		texture = sampler;
	if (!texture)
		return false;

	b->cursor = nir_before_instr(&tex->instr);
	nir_tex_instr_add_src(tex, nir_tex_src_texture_handle, load_handle(b, texture, false));
	return true;
}

static bool lower_intrinsic(nir_builder* b, nir_intrinsic_instr* intr, struct abi_state* state)
{
	b->cursor = nir_before_instr(&intr->instr);

	switch (intr->intrinsic)
	{
		case nir_intrinsic_load_ubo:
			return lower_load_ubo(b, intr, state);

		case nir_intrinsic_load_ssbo_address:
		{
			nir_def* addr = load_desc_addr(b, storage_desc_offset(b, intr->src[0].ssa));
			nir_def_replace(&intr->def, nir_iadd(b, addr, nir_u2u64(b, intr->src[1].ssa)));
			return true;
		}

		case nir_intrinsic_get_ssbo_size:
		{
			nir_def* offset = nir_iadd_imm(b, storage_desc_offset(b, intr->src[0].ssa), 8);
			nir_def_replace(&intr->def, ldc(b, 1, 32, UAM2_DRIVER_CBUF, offset));
			return true;
		}

		case nir_intrinsic_load_num_workgroups:
		{
			if (!is_compute(b))
				return false;
			nir_def* val = ldc(b, 3, 32, UAM2_DRIVER_CBUF, nir_imm_int(b, UAM2_CS_GRID_SIZE_OFFSET));
			nir_def_replace(&intr->def, val);
			return true;
		}

		case nir_intrinsic_load_base_vertex:
		case nir_intrinsic_load_first_vertex:
		case nir_intrinsic_load_base_instance:
		case nir_intrinsic_load_draw_id:
		{
			if (is_compute(b))
				return false;
			unsigned offset = intr->intrinsic == nir_intrinsic_load_base_vertex ? UAM2_GFX_BASE_VERTEX_OFFSET
				: intr->intrinsic == nir_intrinsic_load_first_vertex ? UAM2_GFX_FIRST_VERTEX_OFFSET
				: intr->intrinsic == nir_intrinsic_load_base_instance ? UAM2_GFX_BASE_INSTANCE_OFFSET
				: UAM2_GFX_DRAW_ID_OFFSET;
			nir_def_replace(&intr->def, ldc(b, 1, 32, UAM2_DRIVER_CBUF, nir_imm_int(b, offset)));
			return true;
		}

		case nir_intrinsic_load_constant:
		{
			nir_def* offset = nir_iadd_imm(b, intr->src[0].ssa, nir_intrinsic_base(intr));
			nir_def* val = nir_ldc_nv(b, intr->def.num_components, intr->def.bit_size,
				nir_imm_int(b, UAM2_DATA_CBUF), offset,
				.align_mul = nir_intrinsic_align_mul(intr),
				.align_offset = nir_intrinsic_align_offset(intr));
			nir_def_replace(&intr->def, val);
			return true;
		}

		case nir_intrinsic_image_deref_load:
		case nir_intrinsic_image_deref_sparse_load:
		case nir_intrinsic_image_deref_store:
		case nir_intrinsic_image_deref_atomic:
		case nir_intrinsic_image_deref_atomic_swap:
		case nir_intrinsic_image_deref_size:
		case nir_intrinsic_image_deref_samples:
		{
			nir_deref_instr* deref = nir_src_as_deref(intr->src[0]);
			nir_rewrite_image_intrinsic(intr, load_handle(b, deref, true), nir_image_intrinsic_type_bindless);
			return true;
		}

		default:
			return false;
	}
}

static bool lower_instr(nir_builder* b, nir_instr* instr, void* data)
{
	struct abi_state* state = (struct abi_state*)data;
	switch (instr->type)
	{
		case nir_instr_type_tex:
			return lower_tex(b, nir_instr_as_tex(instr), state);
		case nir_instr_type_intrinsic:
			return lower_intrinsic(b, nir_instr_as_intrinsic(instr), state);
		default:
			return false;
	}
}

static bool lower_descriptor(nir_builder* b, nir_intrinsic_instr* intr, void* data)
{
	b->cursor = nir_before_instr(&intr->instr);

	nir_def* val;
	switch (intr->intrinsic)
	{
		case nir_intrinsic_vulkan_resource_index:
			val = nir_vec2(b, nir_iadd_imm(b, intr->src[0].ssa, nir_intrinsic_binding(intr)), nir_imm_int(b, 0));
			break;
		case nir_intrinsic_vulkan_resource_reindex:
			val = nir_vec2(b, nir_iadd(b, nir_channel(b, intr->src[0].ssa, 0), intr->src[1].ssa),
				nir_channel(b, intr->src[0].ssa, 1));
			break;
		case nir_intrinsic_load_vulkan_descriptor:
			val = intr->src[0].ssa;
			break;
		default:
			return false;
	}

	nir_def_replace(&intr->def, val);
	return true;
}

bool uam2_lower_descriptors(nir_shader* nir)
{
	bool progress = false;
	NIR_PASS(progress, nir, nir_shader_intrinsics_pass, lower_descriptor, nir_metadata_control_flow, NULL);
	if (!progress)
		return false;

	NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_ubo | nir_var_mem_ssbo, nir_address_format_32bit_index_offset);
	NIR_PASS(_, nir, nir_opt_copy_prop);
	NIR_PASS(_, nir, nir_opt_constant_folding);
	NIR_PASS(_, nir, nir_opt_dce);
	return true;
}

static const char* var_name(const nir_variable* var)
{
	const struct glsl_type* iface = var->interface_type ? glsl_without_array(var->interface_type) : NULL;
	if (iface && glsl_get_type_name(iface))
		return glsl_get_type_name(iface);
	return var->name ? var->name : "(unnamed)";
}

static bool is_opaque_uniform(const nir_variable* var)
{
	const struct glsl_type* type = glsl_without_array(var->type);
	return glsl_type_is_sampler(type) || glsl_type_is_image(type) || glsl_type_is_texture(type);
}

static unsigned binding_limit(const nir_variable* var)
{
	switch (var->data.mode)
	{
		case nir_var_mem_ubo:  return DK_NUM_UNIFORM_BUFS;
		case nir_var_mem_ssbo: return DK_NUM_STORAGE_BUFS;
		case nir_var_image:    return DK_NUM_IMAGE_BINDINGS;
		default:               return DK_NUM_TEXTURE_BINDINGS;
	}
}

bool uam2_check_abi(nir_shader* nir)
{
	bool ok = true;
	nir_foreach_variable_with_modes(var, nir, nir_var_uniform | nir_var_image | nir_var_mem_ubo | nir_var_mem_ssbo)
	{
		if (var->num_state_slots)
			continue;

		if (var->data.mode == nir_var_uniform && !is_opaque_uniform(var))
		{
			fprintf(stderr, "error: uniform '%s' outside of a uniform block is not supported\n", var_name(var));
			ok = false;
			continue;
		}
		if (!var->data.explicit_binding)
		{
			fprintf(stderr, "error: '%s' must have an explicit binding\n", var_name(var));
			ok = false;
			continue;
		}
		if (var->data.descriptor_set != 0)
		{
			fprintf(stderr, "error: '%s' must be in descriptor set 0\n", var_name(var));
			ok = false;
			continue;
		}

		bool binding_array = glsl_type_is_array(var->type) &&
			(!var->interface_type || var->interface_type == var->type ||
			 var->interface_type == glsl_without_array(var->type));
		unsigned count = binding_array ? MAX2(glsl_get_aoa_size(var->type), 1) : 1;
		if (var->data.binding + count > binding_limit(var))
		{
			fprintf(stderr, "error: binding %u of '%s' is out of range (maximum %u)\n",
				var->data.binding + count - 1, var_name(var), binding_limit(var) - 1);
			ok = false;
		}
	}

	nir_foreach_variable_with_modes(var, nir, nir_var_mem_push_const)
	{
		fprintf(stderr, "error: push constants are not supported\n");
		ok = false;
		break;
	}

	if (nir->info.stage == MESA_SHADER_FRAGMENT && nir->info.fs.pixel_center_integer)
	{
		fprintf(stderr, "error: pixel_center_integer is not supported\n");
		ok = false;
	}

	if (nir->info.stage == MESA_SHADER_COMPUTE && nir->info.workgroup_size_variable)
	{
		fprintf(stderr, "error: the work group size must be a compile-time constant\n");
		ok = false;
	}

	return ok;
}

bool uam2_lower_abi(nir_shader* nir)
{
	struct abi_state state = { .ok = true };
	NIR_PASS(_, nir, nir_shader_instructions_pass, lower_instr, nir_metadata_control_flow, &state);
	return state.ok;
}
