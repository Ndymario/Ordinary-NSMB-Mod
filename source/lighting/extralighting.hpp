#pragma once
#include "nsmb.hpp"

namespace Lighting {

struct DirLight {
	VecFx16 direction;
	GXRgb color;
	bool active;

	constexpr DirLight(VecFx16 dir = {}, GXRgb col = 0, bool on = true):
		direction(dir), color(col), active(on) 
	{} 
};

struct StageLighting {
	GXRgb diffuse;
	GXRgb ambient;
	GXRgb emission;
	GXRgb specular;
	DirLight lights[4];

	constexpr GXLightMask getLightMask() const {
		u32 lightMask = 0;
		for (u32 i=0; i<4; i++) { lightMask |= (lights[i].active ? 1 << i : 0); }
		return scast<GXLightMask>(lightMask);
	}

	constexpr StageLighting() : 
		diffuse(0), ambient(0), emission(0), specular(0), 
		lights{{}, {}, {}, {}} 
	{}

	constexpr StageLighting(
		GXRgb d, GXRgb a, GXRgb e,
		DirLight l0=DirLight({},0,false), DirLight l1=DirLight({},0,false), 
		DirLight l2=DirLight({},0,false), DirLight l3=DirLight({},0,false)
	) : 
		diffuse(d), ambient(a), emission(e), specular(0),
		lights{l0, l1, l2, l3} 
	{}

	constexpr StageLighting(
		GXRgb d, GXRgb a, GXRgb e, GXRgb s,
		DirLight l0=DirLight({},0,false), DirLight l1=DirLight({},0,false), 
		DirLight l2=DirLight({},0,false), DirLight l3=DirLight({},0,false)
	) : 
		diffuse(d), ambient(a), emission(e), specular(s),
		lights{l0, l1, l2, l3} 
	{}
};

constexpr StageLighting LightingProfiles[] = {

	// 0: Normal
	{
		GX_RGB(31,31,31), GX_RGB(6,6,6), GX_RGB(10,9,8),
		{{0,-22.5deg,-22.5deg},GX_RGB(31,31,31)}
	}, 

	// 1: World 8 Outside
	{ 
		GX_RGB(31,31,31), GX_RGB(6,6,6), GX_RGB(10,9,8),
		{{-4.21875deg,14.0625deg,-22.5deg},GX_RGB(27,31,31)}
	}, 

	// 2: Castle/Ghost house
	{
		GX_RGB(31,31,31), GX_RGB(6,6,6), GX_RGB(10,9,8),
		{{0,22.5deg,-22.5deg}, GX_RGB(31,31,31)}
	},

	// 3: Jungle/Outside water
	{ 
		GX_RGB(31,31,31), GX_RGB(6,6,6), GX_RGB(10,9,8),
		{{-22.5deg,-132.1875deg,-67.5deg}, GX_RGB(31,31,31)}
	},
	
	// 4: Sunset (unused)
	{ 
		GX_RGB(31,31,31), GX_RGB(15,6,6), GX_RGB(19,16,0),
		{{22.5deg,157.5deg,-90deg}, GX_RGB(31,31,31)}
	},
	
	// 5: Underwater
	{
		GX_RGB(31,31,31), GX_RGB(6,6,6), GX_RGB(10,9,8),
		{{-8.4375deg,-14.0625deg,-22.5deg}, GX_RGB(31,31,31)}
	},

	// ---------------- YOUR NEW LIGHTING ---------------- //
	
	// REMEMBER: * 22.5deg = 0x1000, values cannot be >= 0x8000 or < -0x8000 (-180deg to 179.99deg);
	//			 * 31 is max value for RGB

	// 6: example - purple gradient
	{
		// Global material colours
		GX_RGB(31,31,31),			// diffuse (apart from player, affects very few models)
		GX_RGB(10,0,10),			// ambient
		GX_RGB(0,0,25),				// emission
		GX_RGB(0,0,0),				// specular (set to 0 implicitly, and will not affect models in the base game (?) unless forced to

		// Lights
		{
			{22.5deg,112.5deg,-22.5deg}, 	// light 0 direction
			GX_RGB(31,0,31),				// light 0 colour
			true							// light 0 on (set to true implicitly)
		},								
											// ... add up to three more lights
	},

	// 7: example - heat lamp
	{		
		GX_RGB(31,31,31),		
		GX_RGB(20,6,5),		
		GX_RGB(20,5,5),
		{{-135deg,11.25deg,-22.5deg}, 
		GX_RGB(31,28,27)}
	},

	// 8: example - silhouette
	{		
		GX_RGB(0,0,0),	
		GX_RGB(0,0,0),		
		GX_RGB(0,0,0),
		{{0,0,0}, 
		GX_RGB(0,0,0)}		
	},

	// 9
	{GX_RGB(31,31,31), GX_RGB(0,0,0), GX_RGB(0,0,0), GX_RGB(31,31,31), {{0deg,-84.38deg,-118.13deg}, GX_RGB(31,12,0)},{{-90deg,-84.38deg,5.62deg}, GX_RGB(20,0,27)},{{0deg,151.87deg,-61.88deg}, GX_RGB(4,0,31)},{{78.74deg,-33.75deg,67.49deg}, GX_RGB(0,22,6)}},
	
	// 10: Default worldmap lighting
	{
		GX_RGB(31,31,31),GX_RGB(6,6,6),GX_RGB(6,6,14),
		{{-0xae0,-1.0fxs,-0xdc0},GX_RGB(31,31,31)},
	},

	// 11: Grayscale
	{
		// Global material colours
		GX_RGB(16,16,16),
		GX_RGB(8,8,8),
		GX_RGB(4,4,4),
		GX_RGB(0,0,0),

		// Lights
		{
			{22.5deg,112.5deg,-22.5deg},
			GX_RGB(16,16,16),
			true
		},
	},
	

	// --------------------------------------------------- //

};

constexpr u32 NumLightingProfiles = NTR_ARRAY_SIZE(LightingProfiles);

extern u32 currentProfileID;

void setLight(const DirLight& light, GXLightId lightID);
void setMatLighting(const StageLighting& lighting);
void setLightingFromProfile(u32 profileID);
void setMatLightMask(NNSG3dResMdl* pMdl, GXLightMask lightMask);

NTR_INLINE void setGlbLightMask(GXLightMask lightMask) {
	NNS_G3dGlb.prmPolygonAttr = (NNS_G3dGlb.prmPolygonAttr & ~0xf) | lightMask;
}

}