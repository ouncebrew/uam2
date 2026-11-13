#include "uam2.h"
#include "abi.h"
#include "nir.h"
#include "nir_builder.h"
#include "glsl_parser_extras.h"
#include "standalone_scaffolding.h"
#include "gl_nir.h"
#include "gl_nir_linker.h"
#include "linker_util.h"
#include "builtin_functions.h"
#include "main/mtypes.h"
#include "main/shader_types.h"
#include "pipe/p_screen.h"
#include <stdio.h>
#include <string.h>

static struct gl_context s_ctx;

static const GLenum s_glStages[] =
{
	GL_VERTEX_SHADER,
	GL_TESS_CONTROL_SHADER,
	GL_TESS_EVALUATION_SHADER,
	GL_GEOMETRY_SHADER,
	GL_FRAGMENT_SHADER,
	GL_COMPUTE_SHADER,
};

void uam2_glsl_init(const struct nir_shader_compiler_options* options)
{
	struct gl_context* ctx = &s_ctx;
	initialize_context_to_defaults(ctx, API_OPENGL_CORE);
	_mesa_glsl_builtin_functions_init_or_ref();

	for (unsigned i = 0; i < MESA_SHADER_MESH_STAGES; i ++)
		ctx->screen->nir_options[i] = options;

	ctx->Version = 460;
	ctx->Const.GLSLVersion = 460;
	ctx->Const.NativeIntegers = GL_TRUE;
	ctx->Const.GLSLFragCoordIsSysVal = true;
	ctx->Const.GLSLPointCoordIsSysVal = true;
	ctx->Const.GLSLFrontFacingIsSysVal = true;
	ctx->Const.GLSLTessLevelsAsInputs = true;
	ctx->Const.GenerateTemporaryNames = true;
	ctx->Const.PointSizeFixed = true;

	ctx->Extensions.ARB_ES3_compatibility = true;
	ctx->Extensions.ARB_ES3_1_compatibility = true;
	ctx->Extensions.ARB_ES3_2_compatibility = true;
	ctx->Extensions.ARB_compute_variable_group_size = false;
	ctx->Extensions.ARB_bindless_texture = false;
	ctx->Extensions.ARB_shader_ballot = true;
	ctx->Extensions.ARB_shader_group_vote = true;
	ctx->Extensions.ARB_shader_clock = true;
	ctx->Extensions.ARB_shader_draw_parameters = true;
	ctx->Extensions.ARB_shader_viewport_layer_array = true;
	ctx->Extensions.ARB_post_depth_coverage = true;
	ctx->Extensions.ARB_fragment_shader_interlock = false;
	ctx->Extensions.KHR_shader_subgroup = true;

	ctx->Const.MaxPatchVertices = 32;
	ctx->Const.MaxClipPlanes = 8;
	ctx->Const.MaxSamples = 8;
	ctx->Const.MaxImageSamples = 8;
	ctx->Const.MaxColorTextureSamples = 8;
	ctx->Const.MaxDepthTextureSamples = 8;
	ctx->Const.MaxIntegerSamples = 8;
	ctx->Const.MaxViewports = 16;
	ctx->Const.MaxComputeWorkGroupInvocations = 1024;
	ctx->Const.MaxComputeSharedMemorySize = 48 << 10;
	ctx->Const.MaxComputeWorkGroupCount[0] = 0x7fffffff;
	ctx->Const.MaxComputeWorkGroupCount[1] = 65535;
	ctx->Const.MaxComputeWorkGroupCount[2] = 65535;
	ctx->Const.MaxComputeWorkGroupSize[0] = 1024;
	ctx->Const.MaxComputeWorkGroupSize[1] = 1024;
	ctx->Const.MaxComputeWorkGroupSize[2] = 64;

	ctx->Const.MaxDrawBuffers = ctx->Const.MaxColorAttachments = 8;
	ctx->Const.MaxDualSourceDrawBuffers = 1;
	ctx->Const.MaxUniformBlockSize = 65536;
	for (unsigned i = 0; i < MESA_SHADER_MESH_STAGES; i ++)
	{
		struct gl_program_constants* pc = &ctx->Const.Program[i];
		pc->MaxTextureImageUnits = DK_NUM_TEXTURE_BINDINGS;
		pc->MaxAttribs = i == MESA_SHADER_VERTEX ? 16 : (i == MESA_SHADER_FRAGMENT ? (0x1f0 / 16) : (0x200 / 16));
		pc->MaxUniformComponents = 65536/4;
		pc->MaxInputComponents = pc->MaxAttribs*4;
		pc->MaxOutputComponents = 32*4;
		pc->MaxUniformBlocks = DK_NUM_UNIFORM_BUFS;
		pc->MaxCombinedUniformComponents = pc->MaxUniformComponents + uint64_t(ctx->Const.MaxUniformBlockSize) / 4 * pc->MaxUniformBlocks;
		pc->MaxShaderStorageBlocks = DK_NUM_STORAGE_BUFS;
		pc->MaxAtomicCounters = 0;
		pc->MaxAtomicBuffers = 0;
		pc->MaxImageUniforms = DK_NUM_IMAGE_BINDINGS;
	}

	ctx->Const.MaxUserAssignableUniformLocations = 4 * MESA_SHADER_MESH_STAGES * MAX_UNIFORMS;
	ctx->Const.MaxVarying = ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxAttribs;
	ctx->Const.MaxCombinedTextureImageUnits = DK_NUM_TEXTURE_BINDINGS;
	ctx->Const.MaxGeometryOutputVertices = 1024;
	ctx->Const.MaxGeometryTotalOutputComponents = 1024;
	ctx->Const.MaxGeometryShaderInvocations = 32;
	ctx->Const.MaxTessPatchComponents = 30*4;
	ctx->Const.MinProgramTexelOffset = -8;
	ctx->Const.MaxProgramTexelOffset = 7;
	ctx->Const.MaxProgramTextureGatherComponents = 4;
	ctx->Const.MinProgramTextureGatherOffset = -32;
	ctx->Const.MaxProgramTextureGatherOffset = 31;
	ctx->Const.MaxVertexStreams = 4;
	ctx->Const.MaxCombinedUniformBlocks = ctx->Const.MaxUniformBufferBindings = DK_NUM_UNIFORM_BUFS;
	ctx->Const.MaxCombinedShaderStorageBlocks = ctx->Const.MaxShaderStorageBufferBindings = DK_NUM_STORAGE_BUFS;
	ctx->Const.MaxShaderStorageBlockSize = 1 << 27;
	ctx->Const.MaxCombinedImageUniforms = ctx->Const.MaxImageUnits = DK_NUM_IMAGE_BINDINGS;
	ctx->Const.MaxCombinedShaderOutputResources = ctx->Const.MaxDrawBuffers + DK_NUM_STORAGE_BUFS + DK_NUM_IMAGE_BINDINGS;
}

void uam2_glsl_exit(void)
{
	free(s_ctx.screen);
	s_ctx.screen = NULL;
	_mesa_glsl_builtin_functions_decref();
}

static void print_log(const char* what, const char* log)
{
	if (log && log[0])
		fprintf(stderr, "%s:\n%s", what, log);
}

static bool is_opaque_uniform(const nir_variable* var)
{
	const struct glsl_type* type = glsl_without_array(var->type);
	return glsl_type_is_sampler(type) || glsl_type_is_image(type) || glsl_type_is_texture(type);
}

static bool check_variables(nir_shader* nir)
{
	bool ok = true;
	nir_foreach_variable_with_modes(var, nir, nir_var_uniform | nir_var_image | nir_var_mem_ubo | nir_var_mem_ssbo)
	{
		if (var->num_state_slots)
			continue;

		bool needs_binding = var->data.mode != nir_var_uniform || is_opaque_uniform(var);
		if (!needs_binding)
		{
			fprintf(stderr, "error: uniform '%s' outside of a uniform block is not supported\n", var->name);
			ok = false;
		}
		else if (!var->data.explicit_binding)
		{
			const char* name = var->interface_type ? glsl_get_type_name(var->interface_type) : var->name;
			fprintf(stderr, "error: '%s' must have an explicit binding\n", name);
			ok = false;
		}
	}

	if (nir->info.stage == MESA_SHADER_FRAGMENT)
	{
		if (nir->info.fs.pixel_center_integer)
		{
			fprintf(stderr, "error: pixel_center_integer is not supported\n");
			ok = false;
		}
		if (nir->info.fs.origin_upper_left)
			fprintf(stderr, "warning: origin_upper_left has no effect\n");
	}

	if (nir->info.stage == MESA_SHADER_COMPUTE && nir->info.workgroup_size_variable)
	{
		fprintf(stderr, "error: the work group size must be a compile-time constant\n");
		ok = false;
	}

	return ok;
}

struct block_map
{
	unsigned num_ubos, num_ssbos;
	int ubo_binding[DK_NUM_UNIFORM_BUFS];
	int ssbo_binding[DK_NUM_STORAGE_BUFS];
	bool ok;
};

static nir_def* remap_block_index(nir_builder* b, nir_def* index, const int* bindings, unsigned count, struct block_map* map)
{
	if (nir_src_is_const(nir_src_for_ssa(index)))
	{
		unsigned i = nir_src_as_uint(nir_src_for_ssa(index));
		return nir_imm_int(b, bindings[i]);
	}

	int delta = bindings[0];
	for (unsigned i = 1; i < count; i ++)
	{
		if (bindings[i] - (int)i != delta)
		{
			fprintf(stderr, "error: dynamically indexed blocks must be the only blocks of their kind in the shader\n");
			map->ok = false;
			break;
		}
	}
	return nir_iadd_imm(b, index, delta);
}

static bool remap_block_instr(nir_builder* b, nir_intrinsic_instr* intr, void* data)
{
	struct block_map* map = (struct block_map*)data;
	unsigned src;
	bool is_ubo = false;
	switch (intr->intrinsic)
	{
		case nir_intrinsic_load_ubo:
			src = 0;
			is_ubo = true;
			break;
		case nir_intrinsic_load_ssbo:
		case nir_intrinsic_ssbo_atomic:
		case nir_intrinsic_ssbo_atomic_swap:
		case nir_intrinsic_get_ssbo_size:
			src = 0;
			break;
		case nir_intrinsic_store_ssbo:
			src = 1;
			break;
		default:
			return false;
	}

	b->cursor = nir_before_instr(&intr->instr);
	nir_def* index = is_ubo
		? remap_block_index(b, intr->src[src].ssa, map->ubo_binding, map->num_ubos, map)
		: remap_block_index(b, intr->src[src].ssa, map->ssbo_binding, map->num_ssbos, map);
	nir_src_rewrite(&intr->src[src], index);
	return true;
}

static bool lower_blocks_to_bindings(nir_shader* nir, struct gl_shader_program* prg)
{
	struct gl_program* prog = prg->_LinkedShaders[nir->info.stage]->Program;

	struct block_map map = {};
	map.ok = true;
	map.num_ubos = prog->info.num_ubos;
	map.num_ssbos = prog->info.num_ssbos;
	for (unsigned i = 0; i < map.num_ubos; i ++)
		map.ubo_binding[i] = prog->sh.UniformBlocks[i]->Binding;
	for (unsigned i = 0; i < map.num_ssbos; i ++)
		map.ssbo_binding[i] = prog->sh.ShaderStorageBlocks[i]->Binding;

	NIR_PASS(_, nir, gl_nir_lower_buffers, prg);
	NIR_PASS(_, nir, nir_opt_copy_prop);
	NIR_PASS(_, nir, nir_opt_constant_folding);
	NIR_PASS(_, nir, nir_opt_dce);
	NIR_PASS(_, nir, nir_shader_intrinsics_pass, remap_block_instr, nir_metadata_control_flow, &map);
	return map.ok;
}

struct nir_shader* uam2_glsl_to_nir(const char* source, enum uam2_stage stage)
{
	struct gl_context* ctx = &s_ctx;
	struct gl_shader_program* prg = standalone_create_shader_program();
	prg->SeparateShader = true;

	struct gl_shader* shader = standalone_add_shader_source(ctx, prg, s_glStages[stage], source);
	nir_shader* nir = NULL;

	_mesa_glsl_compile_shader(ctx, shader, NULL, false, false, true);
	if (shader->CompileStatus != COMPILE_SUCCESS)
	{
		print_log("Shader failed to compile", shader->InfoLog);
		goto done;
	}
	print_log("Compiler log", shader->InfoLog);

	_mesa_clear_shader_program_data(ctx, prg);
	prg->data->LinkStatus = LINKING_SUCCESS;
	link_shaders_init(ctx, prg);
	gl_nir_link_glsl(ctx, prg);
	if (prg->data->LinkStatus != LINKING_SUCCESS)
	{
		print_log("Shader failed to link", prg->data->InfoLog);
		goto done;
	}
	print_log("Linker log", prg->data->InfoLog);

	{
		struct gl_linked_shader* linked = prg->_LinkedShaders[shader->Stage];
		if (linked->Program->sh.NumSubroutineFunctions || linked->Program->sh.NumSubroutineUniforms)
		{
			fprintf(stderr, "error: shader subroutines are not supported\n");
			goto done;
		}

		nir = linked->Program->nir;
		linked->Program->nir = NULL;
		ralloc_steal(NULL, nir);

		if (!check_variables(nir) || !lower_blocks_to_bindings(nir, prg))
		{
			ralloc_free(nir);
			nir = NULL;
		}
	}

done:
	standalone_destroy_shader_program(prg);
	return nir;
}
