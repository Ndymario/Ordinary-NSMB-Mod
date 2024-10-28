#include "nsmb.hpp"

namespace Lighting {

	NTR_USED static const u32 worldmapLightProfileMap[8] = {

		10, 	// World 1 using profile 10
		10,		// ...
		10,
		10,
		10,
		10,
		10,
		10		// World 8
	};
}

ncp_jump(0x020D1D90,8) // WorldMapCamera::onCreate
NTR_NAKED static void useLightProfilesWM() {asm(R"(
	ldr 	r3, =_ZN8LightingL23worldmapLightProfileMapE
	ldr  	r2, =0x02088BFC
	ldr  	r2, [r2]
	ldr 	r3, [r3, r2, lsl #2]
	mov 	r0, r3
	bl 		0x020a3b24 				@ setLightingFromProfile	
	b 		0x020D1F08 				@ branch back
)");}

ncp_call(0x020D1F28,8) // WorldmapCamera::onCreate
static void setPolyAttr(int r0, GXPolygonMode r1, GXCull r2, int r3, int r4, int r5) {
	NNS_G3dGlbPolygonAttr(NNS_G3dGlb.prmPolygonAttr & 0xf, r1, r2, r3, r4, r5);
}
