#include "uam2.h"
#include "nv_device_info.h"
#include "nak.h"

const struct nv_device_info uam2_t239_device =
{
	.type = NV_DEVICE_TYPE_SOC,
	.device_name = "Tegra T239",
	.chipset_name = "GA10F",
	.sm = 86,
	.gpc_count = 1,
	.tpc_count = 6,
	.mp_per_tpc = 2,
	.max_warps_per_mp = 48,
	.cls_copy = 0xC7B5,
	.cls_eng2d = 0x902D,
	.cls_eng3d = 0xC797,
	.cls_m2mf = 0xA140,
	.cls_compute = 0xC7C0,
	.cls_gpfifo = 0xC76F,
};

const struct nak_constant_offset_info nak_const_offsets_base = { 0 };
const struct nak_constant_offset_info nak_const_offsets_turing_graphics = { 0 };
