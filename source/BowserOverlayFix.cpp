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
constexpr u32 kBowserModelExt = 0x577;    // file ID 1530: enemy/koopa_new.nsbmd
constexpr u32 kBoneKoopaMdlExt = 0x4D9;   // file ID 1372: enemy/bonekoopa.nsbmd
constexpr u32 kBoneKoopaAnmExt = 0x4DA;   // file ID 1373: enemy/bonekoopa2.nsbca

static bool sDryBowserCleanupDone = false;

static inline u32 align16(u32 size)
{
	return (size + 15) & 0xFFFFFFF0;
}

ncp_jump(0x02009C64)
void* FS_Cache_loadFile_OVERRIDE(u32 extFileID, bool compressed)
{
	if (extFileID == kBowserAnimExt) {
		// Move Bowser animation to overlay heap to save main RAM.
		return FS_Cache_loadFileToOverlay_SUPER(extFileID, compressed);
	}

	if (extFileID == kBoneKoopaMdlExt || extFileID == kBoneKoopaAnmExt) {
		// Dry Bowser assets: use overlay only when there is enough space.
		u32 required = align16(FS::getFileSize(extFileID));
		u32 freeOverlay = FS::Cache::overlayFileSize;
		u32 bowserAnimSize = align16(FS::getFileSize(kBowserAnimExt));
		bool canFit = (freeOverlay + bowserAnimSize) >= required;

		if (canFit) {
			// Unload Bowser resources to free RAM/overlay headroom.
			if (!sDryBowserCleanupDone) {
				FS::Cache::unloadFile(kBowserAnimExt);
				FS::Cache::unloadFile(kBowserModelExt);
				sDryBowserCleanupDone = true;
			}

			return FS_Cache_loadFileToOverlay_SUPER(extFileID, compressed);
		}

		// Fallback to RAM to avoid overlay OOM in W1 Bowser.
		return FS_Cache_loadFile_SUPER(extFileID, compressed);
	}

	return FS_Cache_loadFile_SUPER(extFileID, compressed);
}

ncp_jump(0x02009C14)
void* FS_Cache_loadFileToOverlay_OVERRIDE(u32 extFileID, bool compressed)
{
	if (extFileID == kBoneKoopaMdlExt || extFileID == kBoneKoopaAnmExt) {
		u32 required = align16(FS::getFileSize(extFileID));
		if (FS::Cache::overlayFileSize < required) {
			// Not enough overlay space; fall back to RAM.
			return FS_Cache_loadFile_SUPER(extFileID, compressed);
		}
	}

	return FS_Cache_loadFileToOverlay_SUPER(extFileID, compressed);
}
