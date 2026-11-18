#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	Uam2rtStage_Vertex,
	Uam2rtStage_TessCtrl,
	Uam2rtStage_TessEval,
	Uam2rtStage_Geometry,
	Uam2rtStage_Fragment,
	Uam2rtStage_Compute,
} Uam2rtStage;

typedef struct
{
	uint32_t id;
	uint32_t value;
} Uam2rtSpecConst;

bool uam2rt_init(void);
void uam2rt_exit(void);

// Returns a DKSH image allocated with malloc
void* uam2rt_compile_spirv(const uint32_t* words, size_t num_words, Uam2rtStage stage,
	const char* entrypoint, const Uam2rtSpecConst* spec, uint32_t num_spec,
	size_t* out_size, char** out_log);

#ifdef __cplusplus
}
#endif
