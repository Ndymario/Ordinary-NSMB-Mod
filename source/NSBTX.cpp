#include "NSBTX.hpp"

#include "nsmb/game.hpp"

void NSBTX::setup(void* file, const Vec2& size, const Vec2& center, int texID, int palID)
{
	this->size = size;
	this->center = center;

	tex.load(file);
	tex.getTextureParams(texInfo.textureParams, texID);
	tex.getPaletteParams(texInfo.paletteParams, palID);
}

void NSBTX::setTexture(int texID)
{
	tex.getTextureParams(texInfo.textureParams, texID);
}

void NSBTX::setPalette(int palID)
{
	tex.getPaletteParams(texInfo.paletteParams, palID);
}

void NSBTX::render(Vec3& position, Vec3& scale)
{
	NNS_G3dGeFlushBuffer();
	G3_LoadMtx43(&Game::viewMatrix);

	u32 cx = this->center.x;
	u32 cy = this->center.y;
	u32 cfx = cx << FX32_SHIFT;
	u32 cfy = cy << FX32_SHIFT;

	G3_MtxMode(GX_MTXMODE_POSITION);
	G3_Translate(position.x + cfx, position.y + cfy, position.z);
	G3_Scale(scale.x * FX32_ONE, scale.y * FX32_ONE, FX32_ONE * FX32_ONE);

	G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
	G3_PolygonAttr(GX_LIGHTMASK_0, GX_POLYGONMODE_MODULATE, GX_CULL_FRONT, 0, 31, GX_POLYGON_ATTR_MISC_XLU_DEPTH_UPDATE | GX_POLYGON_ATTR_MISC_FAR_CLIPPING);
	G3_Direct1(G3OP_TEXIMAGE_PARAM, texInfo.textureParams);
	G3_Direct1(G3OP_TEXPLTT_BASE, texInfo.paletteParams);
	G3_Color(GX_RGB(31, 31, 31));

	G3_Begin(GX_BEGIN_QUADS);

	u32 x = this->size.x;
	u32 y = this->size.y;
	fx32 mx = x << FX32_SHIFT;
	fx32 my = y << FX32_SHIFT;

	u16 xP = (x - cx);
	u16 xN = -cx;
	u16 yP = (y - cy);
	u16 yN = -cy;

	// Top-left
	G3_TexCoord(0, 0);
	G3_Vtx(xN, yP, 0);
	// Top-right
	G3_TexCoord(mx, 0);
	G3_Vtx(xP, yP, 0);
	//Bottom-right
	G3_TexCoord(mx, my);
	G3_Vtx(xP, yN, 0);
	//Bottom-left
	G3_TexCoord(0, my);
	G3_Vtx(xN, yN, 0);

	G3_End();
}
