#include "nsmb.hpp"

// Titlescreen lighting 
ncp_jump(0x020D3BD0,9)
NTR_NAKED static void useLightProfilesTS() {asm(R"(
	mov 	r0, #0 			@ set light profile
	bl 		0x020a3b24 		@ call setMain3DLighting
	b 		0x020D3C80 		@ branch back and skip default light set
)");}

ncp_call(0x020D3CA0,9) // TitleScreenCamera::onCreate
static void setPolyAttr(int r0, GXPolygonMode r1, GXCull r2, int r3, int r4, int r5) {
	NNS_G3dGlbPolygonAttr(NNS_G3dGlb.prmPolygonAttr & 0xf, r1, r2, r3, r4, r5);
}
