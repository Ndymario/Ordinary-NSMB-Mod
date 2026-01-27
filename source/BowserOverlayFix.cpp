#include "nsmb.hpp"

// Redirect Bowser resource loads to reduce main RAM pressure in W1 Bowser.
// Verified in Ghidra:
//   FS::Cache::loadFile        @ 0x02009C64
//   FS::Cache::loadFileToOverlay @ 0x02009C14

asm(R"(
.type FS_Cache_loadFile_SUPER, %function
FS_Cache_loadFile_SUPER:
	stmdb sp!,{lr}
	b 0x02009C68

.type FS_Cache_loadFileToOverlay_SUPER, %function
FS_Cache_loadFileToOverlay_SUPER:
	stmdb sp!,{lr}
	b 0x02009C18
)");

extern "C" {
	void* FS_Cache_loadFile_SUPER(u32 extFileID, bool compressed);
	void* FS_Cache_loadFileToOverlay_SUPER(u32 extFileID, bool compressed);
}

// extFileID = fileID - 131
constexpr u32 kBowserAnimExt = 0x576;     // file ID 1529: enemy/koopa_new.nsbca
constexpr u32 kBoneKoopaMdlExt = 0x4D9;   // file ID 1372: enemy/bonekoopa.nsbmd
constexpr u32 kBoneKoopaAnmExt = 0x4DA;   // file ID 1373: enemy/bonekoopa2.nsbca

ncp_jump(0x02009C64)
void* FS_Cache_loadFile_OVERRIDE(u32 extFileID, bool compressed)
{
	if (extFileID == kBowserAnimExt) {
		// Move Bowser animation to overlay heap to save main RAM.
		return FS_Cache_loadFileToOverlay_SUPER(extFileID, compressed);
	}

	return FS_Cache_loadFile_SUPER(extFileID, compressed);
}

ncp_jump(0x02009C14)
void* FS_Cache_loadFileToOverlay_OVERRIDE(u32 extFileID, bool compressed)
{
	if (extFileID == kBoneKoopaMdlExt || extFileID == kBoneKoopaAnmExt) {
		// Keep Dry Bones assets in main RAM to preserve overlay space.
		return FS_Cache_loadFile_SUPER(extFileID, compressed);
	}

	return FS_Cache_loadFileToOverlay_SUPER(extFileID, compressed);
}
