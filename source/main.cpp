#include "uam2.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

static int usage(const char* prog)
{
	fprintf(stderr,
		"Usage: %s [options] file\n"
		"Options:\n"
		"  -o, --out=<file>   Specifies the output deko3d shader module file (.dksh)\n"
		"  -r, --raw=<file>   Specifies the file to which output raw SM 8.6 bytecode\n"
		"  -a, --asm=<file>   Specifies the file to which output NAK assembly\n"
		"  -s, --stage=<name> Specifies the pipeline stage of the shader\n"
		"                     (vert, tess_ctrl, tess_eval, geom, frag, comp)\n"
		"  -v, --version      Displays version information\n"
		, prog);
	return EXIT_FAILURE;
}

static bool writeFile(const char* path, const void* data, size_t size)
{
	FILE* f = fopen(path, "wb");
	if (!f)
	{
		fprintf(stderr, "Could not open output file: %s\n", path);
		return false;
	}

	bool ok = fwrite(data, 1, size, f) == size;
	fclose(f);
	if (!ok)
		fprintf(stderr, "Could not write output file: %s\n", path);
	return ok;
}

int main(int argc, char* argv[])
{
	const char *inFile = nullptr, *outFile = nullptr, *rawFile = nullptr, *asmFile = nullptr, *stageName = nullptr;

	static struct option long_options[] =
	{
		{ "out",     required_argument, NULL, 'o' },
		{ "raw",     required_argument, NULL, 'r' },
		{ "asm",     required_argument, NULL, 'a' },
		{ "stage",   required_argument, NULL, 's' },
		{ "help",    no_argument,       NULL, '?' },
		{ "version", no_argument,       NULL, 'v' },
		{ NULL, 0, NULL, 0 }
	};

	int opt, optidx = 0;
	while ((opt = getopt_long(argc, argv, "o:r:a:s:?v", long_options, &optidx)) != -1)
	{
		switch (opt)
		{
			case 'o': outFile = optarg; break;
			case 'r': rawFile = optarg; break;
			case 'a': asmFile = optarg; break;
			case 's': stageName = optarg; break;
			case '?': usage(argv[0]); return EXIT_SUCCESS;
			case 'v': printf("%s (Mesa %s NAK). Built on %s %s\n", PACKAGE_STRING, UAM2_MESA_VERSION, __DATE__, __TIME__); return EXIT_SUCCESS;
			default:  return usage(argv[0]);
		}
	}

	if ((argc-optind) != 1)
		return usage(argv[0]);
	inFile = argv[optind];

	if (!stageName)
	{
		fprintf(stderr, "Missing pipeline stage argument (--stage)\n");
		return EXIT_FAILURE;
	}

	if (!outFile && !rawFile && !asmFile)
	{
		fprintf(stderr, "No output file specified\n");
		return EXIT_FAILURE;
	}

	uam2_stage stage;
	if (0) ((void)0);
#define TEST_STAGE(_str,_val) else if (strcmp(stageName,(_str))==0) stage = (_val)
	TEST_STAGE("vert", Uam2Stage_Vertex);
	TEST_STAGE("tess_ctrl", Uam2Stage_TessCtrl);
	TEST_STAGE("tess_eval", Uam2Stage_TessEval);
	TEST_STAGE("geom", Uam2Stage_Geometry);
	TEST_STAGE("frag", Uam2Stage_Fragment);
	TEST_STAGE("comp", Uam2Stage_Compute);
#undef TEST_STAGE
	else
	{
		fprintf(stderr, "Unrecognized pipeline stage: `%s'\n", stageName);
		return EXIT_FAILURE;
	}

	FILE* fin = fopen(inFile, "rb");
	if (!fin)
	{
		fprintf(stderr, "Could not open input file: %s\n", inFile);
		return EXIT_FAILURE;
	}

	fseek(fin, 0, SEEK_END);
	long fsize = ftell(fin);
	rewind(fin);

	std::vector<uint32_t> words((fsize + 3) / 4);
	size_t readSize = fread(words.data(), 1, fsize, fin);
	fclose(fin);

	if (readSize != (size_t)fsize || fsize < 20 || (fsize & 3) || words[0] != 0x07230203)
	{
		fprintf(stderr, "Input is not a SPIR-V module (GLSL is not implemented yet)\n");
		return EXIT_FAILURE;
	}

	uam2_init();

	uam2_program prog;
	bool rc = uam2_compile_spirv(words.data(), words.size(), stage, "main", asmFile != nullptr, &prog);
	if (rc)
	{
		if (outFile)
			rc = uam2_write_dksh(outFile, &prog);

		if (rc && rawFile)
			rc = writeFile(rawFile, prog.code, prog.code_size);

		if (rc && asmFile)
			rc = writeFile(asmFile, prog.asm_str ? prog.asm_str : "", prog.asm_str ? strlen(prog.asm_str) : 0);

		uam2_program_free(&prog);
	}

	uam2_exit();
	return rc ? EXIT_SUCCESS : EXIT_FAILURE;
}
