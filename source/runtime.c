#include "uam2rt.h"
#include "uam2.h"
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned s_refs;

bool uam2rt_init(void)
{
	pthread_mutex_lock(&s_lock);
	if (s_refs++ == 0)
		uam2_init();
	pthread_mutex_unlock(&s_lock);
	return true;
}

void uam2rt_exit(void)
{
	pthread_mutex_lock(&s_lock);
	if (s_refs && --s_refs == 0)
		uam2_exit();
	pthread_mutex_unlock(&s_lock);
}

void* uam2rt_compile_spirv(const uint32_t* words, size_t num_words, Uam2rtStage stage,
	const char* entrypoint, const Uam2rtSpecConst* spec, uint32_t num_spec,
	size_t* out_size, char** out_log)
{
	if (out_log)
		*out_log = NULL;

	struct uam2_program prog;
	if (!uam2_compile_spirv(words, num_words, (enum uam2_stage)stage, entrypoint ? entrypoint : "main",
		(const struct uam2_spec_const*)spec, num_spec, false, &prog))
		return NULL;

	void* dksh = uam2_build_dksh(&prog, out_size);
	uam2_program_free(&prog);
	return dksh;
}
