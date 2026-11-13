#include "uam2.h"
#include "dksh.h"
#include <stdio.h>
#include <string.h>
#include <vector>

namespace
{
	constexpr uint32_t Align256(uint32_t x)
	{
		return (x + 0xFF) &~ 0xFF;
	}

	void Append(std::vector<uint8_t>& buf, const void* data, size_t size)
	{
		const uint8_t* p = (const uint8_t*)data;
		buf.insert(buf.end(), p, p + size);
	}

	void Pad256(std::vector<uint8_t>& buf)
	{
		buf.resize(Align256(buf.size()), 0);
	}

	constexpr unsigned SphImapPositionWWord = 5;
	constexpr uint32_t SphImapPositionWBit = 1u << 31;
	constexpr unsigned SphOmapTargetsWord = 18;

	unsigned NumColorTargets(const uint32_t* sph)
	{
		unsigned count = 0;
		for (unsigned i = 0; i < 8; i ++)
			if ((sph[SphOmapTargetsWord] >> (4*i)) & 0xF)
				count ++;
		return count;
	}

	enum
	{
		TessDomain_Isoline = 0,
		TessPrims_Points   = 0,
		TessPrims_Lines    = 1,
		TessPrims_TriCw    = 2,
		TessPrims_TriCcw   = 3,
	};

	// The clockwise output mode yields counterclockwise triangles under deko3d's origin
	uint32_t TessParams(const uam2_program* prog)
	{
		uint32_t prims;
		if (prog->ts.point_mode)
			prims = TessPrims_Points;
		else if (prog->ts.domain == TessDomain_Isoline)
			prims = TessPrims_Lines;
		else
			prims = prog->ts.ccw ? TessPrims_TriCw : TessPrims_TriCcw;
		return prog->ts.domain | (prog->ts.spacing << 4) | (prims << 8);
	}

	uint32_t ProgramType(uam2_stage stage)
	{
		switch (stage)
		{
			default:
			case Uam2Stage_Vertex:   return DkshProgramType_Vertex;
			case Uam2Stage_TessCtrl: return DkshProgramType_TessCtrl;
			case Uam2Stage_TessEval: return DkshProgramType_TessEval;
			case Uam2Stage_Geometry: return DkshProgramType_Geometry;
			case Uam2Stage_Fragment: return DkshProgramType_Fragment;
			case Uam2Stage_Compute:  return DkshProgramType_Compute;
		}
	}
}

bool uam2_write_dksh(const char* path, const uam2_program* prog)
{
	bool isCompute = prog->stage == Uam2Stage_Compute;
	uint32_t sphSize = isCompute ? 0 : sizeof(prog->hdr);
	uint32_t localPosSize = (prog->slm_size + 0xF) &~ 0xF;

	DkshProgramHeader dkph = {};
	dkph.type = ProgramType(prog->stage);
	dkph.entrypoint = 0;
	dkph.num_gprs = prog->num_gprs;
	if (prog->data_size)
	{
		dkph.constbuf1_off = Align256(sphSize + prog->code_size);
		dkph.constbuf1_sz = prog->data_size;
	}
	dkph.per_warp_scratch_sz = localPosSize * 32;

	if (isCompute)
	{
		for (unsigned i = 0; i < 3; i ++)
			dkph.comp.block_dims[i] = prog->block_dims[i];
		dkph.comp.shared_mem_sz = Align256(prog->shared_mem_size);
		dkph.comp.local_pos_mem_sz = localPosSize;
		dkph.comp.local_neg_mem_sz = 0;
		dkph.comp.crs_sz = 0;
		dkph.comp.num_barriers = prog->num_barriers;
	}

	if (prog->stage == Uam2Stage_TessEval)
		dkph.tess_eval.param_c8 = TessParams(prog);

	uint32_t sph[32];
	memcpy(sph, prog->hdr, sizeof(sph));

	if (prog->stage == Uam2Stage_Fragment)
	{
		sph[SphImapPositionWWord] |= SphImapPositionWBit;

		dkph.frag.early_fragment_tests  = prog->fs.early_fragment_tests;
		dkph.frag.post_depth_coverage   = prog->fs.post_depth_coverage;
		dkph.frag.persample_invocation = prog->fs.uses_sample_shading;
		dkph.frag.param_65b             = prog->fs.writes_depth ? 0x11 : 0x00;
		dkph.frag.param_489             = 0;
		dkph.frag.param_d8              = !prog->fs.writes_depth && NumColorTargets(sph) >= 3 ? 0x20806080 : 0x087F6080;
	}

	std::vector<uint8_t> code;
	if (sphSize)
		Append(code, sph, sphSize);
	Append(code, prog->code, prog->code_size);
	Pad256(code);
	if (prog->data_size)
	{
		Append(code, prog->data, prog->data_size);
		Pad256(code);
	}

	DkshHeader hdr = {};
	hdr.magic        = DKSH_MAGIC;
	hdr.header_sz    = sizeof(DkshHeader);
	hdr.control_sz   = Align256(sizeof(DkshHeader) + sizeof(DkshProgramHeader));
	hdr.code_sz      = code.size();
	hdr.programs_off = sizeof(DkshHeader);
	hdr.num_programs = 1;

	std::vector<uint8_t> file;
	Append(file, &hdr, sizeof(hdr));
	Append(file, &dkph, sizeof(dkph));
	Pad256(file);
	file.insert(file.end(), code.begin(), code.end());

	FILE* f = fopen(path, "wb");
	if (!f)
	{
		fprintf(stderr, "Could not open output file: %s\n", path);
		return false;
	}

	bool ok = fwrite(file.data(), 1, file.size(), f) == file.size();
	fclose(f);
	if (!ok)
		fprintf(stderr, "Could not write output file: %s\n", path);
	return ok;
}
