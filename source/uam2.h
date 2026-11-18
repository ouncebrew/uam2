#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nv_device_info;
struct nir_shader;
struct nir_shader_compiler_options;

enum uam2_stage
{
	Uam2Stage_Vertex,
	Uam2Stage_TessCtrl,
	Uam2Stage_TessEval,
	Uam2Stage_Geometry,
	Uam2Stage_Fragment,
	Uam2Stage_Compute,
};

struct uam2_spec_const
{
	uint32_t id;
	uint32_t value;
};

struct uam2_program
{
	enum uam2_stage stage;
	void* code;
	uint32_t code_size;
	void* data;
	uint32_t data_size;
	uint32_t num_gprs;
	uint32_t num_instrs;
	uint32_t slm_size;
	uint32_t num_barriers;
	uint32_t block_dims[3];
	uint32_t shared_mem_size;
	struct
	{
		bool writes_depth;
		bool early_fragment_tests;
		bool post_depth_coverage;
		bool uses_sample_shading;
	} fs;
	struct
	{
		uint8_t domain;
		uint8_t spacing;
		bool ccw;
		bool point_mode;
	} ts;
	uint32_t hdr[32];
	char* asm_str;
};

extern const struct nv_device_info uam2_t239_device;

void uam2_init(void);
void uam2_exit(void);

void uam2_glsl_init(const struct nir_shader_compiler_options* options);
void uam2_glsl_exit(void);
struct nir_shader* uam2_glsl_to_nir(const char* source, enum uam2_stage stage);

bool uam2_lower_abi(struct nir_shader* nir);

bool uam2_compile_glsl(const char* source, enum uam2_stage stage, bool dump_asm, struct uam2_program* out);
bool uam2_compile_spirv(const uint32_t* words, size_t num_words, enum uam2_stage stage,
	const char* entrypoint, const struct uam2_spec_const* spec, unsigned num_spec, bool dump_asm,
	struct uam2_program* out);

void uam2_program_free(struct uam2_program* prog);

void* uam2_build_dksh(const struct uam2_program* prog, size_t* out_size);
bool uam2_write_dksh(const char* path, const struct uam2_program* prog);

#ifdef __cplusplus
}
#endif
