#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nv_device_info;

enum uam2_stage
{
	Uam2Stage_Vertex,
	Uam2Stage_TessCtrl,
	Uam2Stage_TessEval,
	Uam2Stage_Geometry,
	Uam2Stage_Fragment,
	Uam2Stage_Compute,
};

struct uam2_program
{
	void* code;
	uint32_t code_size;
	uint32_t num_gprs;
	uint32_t num_instrs;
	uint32_t slm_size;
	uint32_t hdr[32];
	char* asm_str;
};

extern const struct nv_device_info uam2_t239_device;

void uam2_init(void);
void uam2_exit(void);

bool uam2_compile_spirv(const uint32_t* words, size_t num_words, enum uam2_stage stage,
	const char* entrypoint, bool dump_asm, struct uam2_program* out);

void uam2_program_free(struct uam2_program* prog);

#ifdef __cplusplus
}
#endif
