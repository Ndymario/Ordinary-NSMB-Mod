#include "extralighting.hpp"

namespace Lighting {

	u32 currentProfileID;

	void setLight(const DirLight& light, GXLightId lightID) {

		Vec3 dir = Vec3(light.direction).normalize();
		NNS_G3dGlbLightVector(lightID, dir.x, dir.y, dir.z);
	  	NNS_G3dGlbLightColor(lightID, light.color);
	  	(*rcast<Vec3(*)[4]>(0x020caa84))[lightID] = dir; 			// s3DLightVector
	  	(*rcast<GXRgb(*)[4]>(0x020caa40))[lightID] = light.color; 	// sLightColor
	}

	void setMatLighting(const StageLighting& lighting) {

		*rcast<GXRgb*>(0x020caa30) = lighting.diffuse; 	// GFX::sDiffuseColor
		*rcast<GXRgb*>(0x020caa2c) = lighting.ambient; 	// GFX::sAmbientColor
		*rcast<GXRgb*>(0x020caa34) = lighting.emission; // GFX::sEmissionColor

		*rcast<GXRgb*>(0x020a3b20) = lighting.specular;

		rcast<void(*)()>(0x020a3ad8)(); // setLightColors (mat colours)
	}

	ncp_jump(0x020a3b24,0)
	void setLightingFromProfile(u32 profileID) {

		currentProfileID = profileID;
		
		u32 n = 0;
		for (const DirLight& light : LightingProfiles[profileID].lights) {
			if (light.active) 
				setLight(light,scast<GXLightId>(n));
	  		n+=1;
		}

		setMatLighting(LightingProfiles[profileID]);
		setGlbLightMask(LightingProfiles[profileID].getLightMask());
	}

	// unused
	void setMatLightMask(NNSG3dResMdl* pMdl, GXLightMask lightMask) {

		if (pMdl->info.numMat == 0) return;

		for (u32 matID = 0; matID < pMdl->info.numMat; matID++) {
			NNSG3dResMatData* matData = NNS_G3dGetMatDataByIdx(NNS_G3dGetMat(pMdl),matID);
			matData->polyAttr = (matData->polyAttr & ~0xf) | lightMask;
		}
	}

}


// replaces polygonAttr call in StageCamera::onCreate to not modify light mask
ncp_call(0x020ce5b0,10)
static void setGlbPolyAttr(int r0, GXPolygonMode r1, GXCull r2, int r3, int r4, int r5) {
	NNS_G3dGlbPolygonAttr(NNS_G3dGlb.prmPolygonAttr & 0xf, r1, r2, r3, r4, r5);
}

ncp_call(0x02019984) // Model::create
static void onModelSetup(NNSG3dRenderObj *pRenderObj,NNSG3dResMdl *pResMdl) {

	rcast<void(*)(NNSG3dRenderObj*,NNSG3dResMdl*)>(0x02057dc4)(pRenderObj,pResMdl); // NNS_G3dRenderObjInit
	
	NNS_G3dMdlUseGlbLightEnableFlag(pResMdl); // use global light mask

	bool useGlbSpecular = true;

	// if the specular of any material in model resource is not 0 don't use global specular
	for (u32 matIdx = 0; matIdx < pResMdl->info.numMat; matIdx++) {
		NNSG3dResMatData* matData = NNS_G3dGetMatDataByIdx(NNS_G3dGetMat(pResMdl),matIdx);
		if ((matData->specEmi & 0xffff) > 0) {
			useGlbSpecular = false;
		}
	}

	if (useGlbSpecular)
		NNS_G3dMdlUseGlbSpec(pResMdl);
}

// don't reset lighting if view does not change
ncp_jump(0x02118a44, 10) // Player::viewTransitState
NTR_NAKED static void noLightSwitchInSameView() {asm(R"(
	ldrb	r2, [r8,#0x2be] 	@ store previous view
	strb 	r0, [r8,#0x2be] 	@ keep replaced instruction
	mov 	r5, r0				@ store next view
	beq 	0x02118a5c
	ldrb	r0, [r8,#0x2be]
	bl 		0x020a16a8
	cmp 	r2, r5 		@ compare previous with next view
	beq 	0x02118a5c 	@ skip Stage::setLighting if the same
	b 		0x02118a54
)");}
