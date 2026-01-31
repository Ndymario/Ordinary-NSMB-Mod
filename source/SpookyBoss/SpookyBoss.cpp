#include "nsmb.hpp"
#include "SpookyBoss.hpp"
#include "SpookyController.hpp"
#include "BlockProjectile.hpp"
#include "util/collisionviewer.hpp"
#include <nsmb/game/stage/layout/data/entrance.hpp>

ncp_over(0x020c560c, 0) const ObjectInfo objectInfo = SpookyBoss::objectInfo; //Stage Object ID 44 (use this in the editor)
ncp_over(0x02039a34) static constexpr const ActorProfile* profile = &SpookyBoss::profile; //objectID 46

static void copyChunk(u32 source, u32 destination);
static inline void refreshChunks();

SpookyBoss* SpookyBoss::instance = nullptr;
bool SpookyBoss::fightOnSubScreen = false;

bool SpookyBoss::loadResources() {
    FS::Cache::loadFile(bossModelID, false);
    FS::Cache::loadFile(idleAnimationID, false);
    return true;
}

s32 SpookyBoss::onCreate(){
    instance = this;
    SpookyController* ctrl = SpookyController::getInstance();
    
    // Disable the spooky controller
    ctrl->switchState(&SpookyController::waitSpawnChaserState);
    ctrl->doTicks = false;
    ctrl->finalBoss = true;

    // Clear the screen flip flag
    fightOnSubScreen = false;

    // Remove spooky mode if needed
    if(ctrl->usingSpookyPalette){
        ctrl->switchState(&SpookyController::transitionState);
    }

    loadResources();

    void* bossModelFile = FS::Cache::getFile(bossModelID);
    void* idleAnmFile = FS::Cache::getFile(idleAnimationID);

    model.create(bossModelFile, idleAnmFile, 0, 0, 0);

    fogFlag = false;
	alpha = 0xff;
	renderOffset = {0, 0};
	rotationTranslation = {180, 0};

    model.init(1, FrameCtrl::Looping, 1fx, 0);

    initialPosition = position;

    // Init active collider for hit detection
    activeCollider.init(this, bossActiveColliderInfo, 0);
    activeCollider.link();

    switchState(&SpookyBoss::idleState);

	return true;
}

s32 SpookyBoss::onDestroy(){
	activeCollider.unlink();
    if (instance == this) {
        instance = nullptr;
    }
	return true;
}

bool SpookyBoss::updateMain(){
    // Render collision debugging, mirroring BlockProjectile implementation
    CollisionViewer::renderActiveCollider(activeCollider, CollisionViewer::Flags::ActiveCollider);
    // Enforce the screen swap each frame so it takes effect immediately in-stage.
    {
        volatile u16* powcnt = rcast<volatile u16*>(0x04000304);
        if (fightOnSubScreen) {
            *powcnt &= 0x7FFF;
        } else {
            *powcnt |= 0x8000;
        }
    }
    updateAnimation();
    if (invulnTimer > 0) invulnTimer--;
    updateFunc(this);
	return true;
}

void SpookyBoss::idleState(){
    if (updateStep == Func::Init) {
        updateStep = 1;
        return;
    }

    if (updateStep == 1){
        Vec3 playerLocation = player->position;
        if (position.x - playerLocation.x <= 16fx * fightActivationRange){
            switchState(&SpookyBoss::introAnimation);
        }
    }

    if (updateStep == Func::Exit) {
        return;
    }
}

void SpookyBoss::introAnimation(){
    if (updateStep == Func::Init) {
        player->beginCutscene(true);
        updateStep = 1;
        return;
    }

    if(updateStep == 1){
        model.init(1, FrameCtrl::Type::Standard, 1fx, 0);

        if(model.frameController.finished()){
            SpookyController* instance = SpookyController::getInstance();
            instance->switchState(&SpookyController::transitionState);
            copyChunk(9, 1);
            refreshChunks();
            updateStep++;
        }
    }

    if (updateStep == 2){
        Log() << "continue animation";

        model.init(1, FrameCtrl::Type::Standard, 1fx, 0);

        if(model.frameController.finished()){
            switchState(&SpookyBoss::mimicState);
        }
    }

    if (updateStep == Func::Exit) {
        position.y += 16fx * 5;
        position.x -= 16fx * 7;
        player->endCutscene();
        player->position.y -= 16fx;
        Vec2 bumpVelocity = Vec2(-4fx, 0);
        player->doBossBump(bumpVelocity);
        SND::playBGM(21, false);
        return;
    }
}

void SpookyBoss::mimicState(){
    if (updateStep == Func::Init) {
        zone = StageZone::get(0, &zoneRect);
        model.init(0, FrameCtrl::Type::Standard, 1fx, 0);
        mirrorPosition = position;
        updateStep = 1;
        bossTimer = getAttackIntervalForHits(phaseOneHits);
        moveTimer = 0;
        throwPauseTimer = 0;
        anchorY = initialPosition.y;
        return;
    }

    if (updateStep == Func::Exit) {
        return;
    }

    // We always want Mario/Luigi to look at the boss
    Game::setPlayerLookAtPosition(Vec3(position.x, position.y + 32fx, 0));
    Game::setPlayerLookingAtTarget(true);

    if (player->position.x < zoneRect.x || player->position.x > zoneRect.x + zoneRect.width || player->position.y > zoneRect.y || player->position.y < zoneRect.y - zoneRect.height){
            SpookyController* instance = SpookyController::getInstance();
            instance->staticDuration = 10;
            player->position = Vec3(initialPosition.x - 4fx, initialPosition.y, player->position.z);
            player->doPlayerBump(Vec2(-2fx, 0), true);
    }
    

    // Movement: hover with limited Y variance and gentle X offsets around Mario,
    // while staying inside the zone.
    // Periodic block throw trigger remains controlled by bossTimer/updateStep.
    if (updateStep == 1 || updateStep == 3){
        if (bossTimer <= 0){
            bossTimer = getAttackIntervalForHits(phaseOneHits);
            throwPauseTimer = throwWindupFrames;
            updateStep = 2;
        }
    }

    if (updateStep == 2){
        // Move to the top-center of the arena while winding up
        if (zone) {
            fx32 topCenterX = zoneRect.x + (zoneRect.width / 2);
            fx32 topCenterY = zoneRect.y - 8fx;
            Vec3 topCenter = Vec3(topCenterX, topCenterY, position.z);
            position.lerp(topCenter, 0.08fx);
        }
        if (throwPauseTimer > 0) {
            throwPauseTimer--;
        } else {
            performAttackPattern();
            updateStep = 3;
        }
    }

    // Perform hovering movement during steps 1 and 3
    if (updateStep == 1 || updateStep == 3) {
        // Boundaries (keep a small margin inside the zone)
        fx32 minX = zoneRect.x + 8fx;
        fx32 maxX = zoneRect.x + zoneRect.width - 8fx;
        fx32 maxY = zoneRect.y - 8fx; // top
        fx32 minY = zoneRect.y - zoneRect.height + 8fx; // bottom

        // Pick a new target at random intervals
        if (moveTimer == 0) {
            // Rare teleport to reposition without being too chaotic
            teleportMode = scast<bool>((Net::getRandom() % 100) < 2);

            // X: hover around Mario with left/right offsets
            s32 offset = 32 + scast<s32>(Net::getRandom() % 49); // [32, 80]
            if ((Net::getRandom() % 2) == 0) offset = -offset;
            fx32 targetX = player->position.x + scast<fx32>(offset);

            // Y: stay near initial spawn height with small variance
            s32 yVariance = scast<s32>(Net::getRandom() % 41) - 20; // [-20, 20]
            fx32 targetY = anchorY + scast<fx32>(yVariance);

            if (targetX < minX) targetX = minX;
            if (targetX > maxX) targetX = maxX;
            if (targetY > maxY) targetY = maxY;
            if (targetY < minY) targetY = minY;

            moveTarget = Vec3(targetX, targetY, position.z);

            // Duration for this interval: slower pacing for smoother motion
            moveTimer = 48 + scast<u16>(Net::getRandom() % 97);

            if (teleportMode) {
                position.x = moveTarget.x;
                position.y = moveTarget.y;
            }
        } else {
            // Lerp towards target if not teleport interval
            if (!teleportMode) {
                position.lerp(moveTarget, 0.05fx);
            }
            moveTimer--;
        }

        // Clamp inside zone just in case
        if (position.x < minX) position.x = minX;
        if (position.x > maxX) position.x = maxX;
        if (position.y > maxY) position.y = maxY;
        if (position.y < minY) position.y = minY;
    }

    // Decrement the timer
    bossTimer--;

    // No extra respawn; cadence is controlled by bossTimer
}

// Spawn a block projectile from Luigi's position (targeting Mario)
BlockProjectile* SpookyBoss::spawnBossBlock(u8 pattern){
    return spawnBossBlock(pattern, false, -1);
}

// Spawn a block projectile from Luigi's position with optional variants/direction
BlockProjectile* SpookyBoss::spawnBossBlock(u8 pattern, bool spiked, s8 dirIndex){
    return spawnBossBlock(pattern, spiked, dirIndex, Vec2(0, 0));
}

BlockProjectile* SpookyBoss::spawnBossBlock(u8 pattern, bool spiked, s8 dirIndex, const Vec2& offset, fx32 speedScale){
    Vec3 spawnPos = position;
    spawnPos.y += 16fx; // lower to avoid ceiling collisions
    spawnPos.x += offset.x;
    spawnPos.y += offset.y;

    s32 settings = 0;
    settings |= (scast<s32>(pattern) & 0xFF) << BlockProjectile::SettingsPatternShift;
    if (spiked) {
        settings |= BlockProjectile::SettingsSpiked;
    }
    if (phaseOneHits > 0) {
        settings |= BlockProjectile::SettingsSlowThrow;
    }
    if (dirIndex >= 0) {
        settings |= BlockProjectile::SettingsUseDirection;
        settings |= (scast<s32>(dirIndex) & 0x0F) << BlockProjectile::SettingsDirShift;
    }

#ifdef NTR_DEBUG
    Log::print(
        "SpookyBoss spawnBossBlock: hits=%u pattern=%u spiked=%u dir=%d settings=0x%08X",
        scast<u32>(phaseOneHits),
        scast<u32>(pattern),
        spiked ? 1 : 0,
        scast<s32>(dirIndex),
        scast<u32>(settings)
    );
#endif

    BlockProjectile* proj = scast<BlockProjectile*>(Actor::spawnActor(280, settings, &spawnPos));
    if (proj) {
        proj->speedScale = speedScale;
    }
    return proj;
}

void SpookyBoss::performAttackPattern(){
    const u8 hits = phaseOneHits;
    const u8 pattern = (hits == 0) ? 0 : 1; // slower throws once he's been hit
    const Vec2 leftOffset(-16fx, 0);
    const Vec2 rightOffset(16fx, 0);

    if (hits == 0) {
        // Either one normal block or one spiked block
        if ((Net::getRandom() & 1) == 0) {
            spawnBossBlock(pattern, false, -1);
        } else {
            spawnBossBlock(pattern, true, -1);
        }
        return;
    }

    if (hits == 1) {
        // Pattern A: normal + spiked, diagonally down left/right (random which side)
        // Pattern B: two spiked, diagonally down left/right
        bool patternA = (Net::getRandom() & 1) == 0;
        if (patternA) {
            bool normalLeft = (Net::getRandom() & 1) == 0;
            if (normalLeft) {
                spawnBossBlock(pattern, false, 0, leftOffset); // down-left normal
                spawnBossBlock(pattern, true, 1, rightOffset, 5.0fx);  // down-right spiked
            } else {
                spawnBossBlock(pattern, true, 0, leftOffset, 5.0fx);  // down-left spiked
                spawnBossBlock(pattern, false, 1, rightOffset); // down-right normal
            }
        } else {
            spawnBossBlock(pattern, true, 0, leftOffset, 5.0fx); // down-left spiked
            spawnBossBlock(pattern, true, 1, rightOffset, 5.0fx); // down-right spiked
        }
        return;
    }

    // hits >= 2
    bool patternB = (Net::getRandom() & 1) == 0;
    // Base: two spiked diagonally down
    spawnBossBlock(pattern, true, 0, leftOffset, 8.0fx); // down-left spiked
    spawnBossBlock(pattern, true, 1, rightOffset, 8.0fx); // down-right spiked

    if (patternB) {
        // Add one normal block in a random direction not equal to the two diagonals
        static const u8 dirs[] = {2, 3, 4, 5, 6, 7};
        u8 dir = dirs[Net::getRandom() % (sizeof(dirs) / sizeof(dirs[0]))];
        spawnBossBlock(pattern, false, scast<s8>(dir), Vec2(0, -8fx));
    }
}

// Spawn a block projectile from the arena edge to simulate a background throw
void SpookyBoss::spawnBackgroundBlock(u8 pattern){
    // Quick telegraph using static overlay
    SpookyController* ctrl = SpookyController::getInstance();
    ctrl->staticDuration = 8;

    // Ensure zone is cached
    if (!zone){
        zone = StageZone::get(0, &zoneRect);
    }

    // Compute random spawn position ON the inset edges of the arena
    fx32 leftX   = zoneRect.x + 8fx;
    fx32 rightX  = zoneRect.x + zoneRect.width - 8fx;
    fx32 topY    = zoneRect.y - 8fx; // inside top
    fx32 botY    = zoneRect.y - zoneRect.height + 8fx; // inside bottom

    fx32 rangeX = rightX - leftX;
    fx32 rangeY = topY - botY;

    Vec3 spawnPos = position;
    u32 edge = scast<u32>(Net::getRandom() % 4);
    switch (edge) {
        case 0: // top edge
            spawnPos.y = topY;
            spawnPos.x = leftX + scast<fx32>(Net::getRandom() % scast<u32>(rangeX));
            break;
        case 1: // bottom edge
            spawnPos.y = botY;
            spawnPos.x = leftX + scast<fx32>(Net::getRandom() % scast<u32>(rangeX));
            break;
        case 2: // left edge
            spawnPos.x = leftX;
            spawnPos.y = botY + scast<fx32>(Net::getRandom() % scast<u32>(rangeY));
            break;
        default: // right edge
            spawnPos.x = rightX;
            spawnPos.y = botY + scast<fx32>(Net::getRandom() % scast<u32>(rangeY));
            break;
    }

    // settings: bit0 = fromBackground, bits8..15 = pattern
    s32 settings = 0;
    settings |= BlockProjectile::SettingsFromBackground; // fromBackground flag
    settings |= (scast<s32>(pattern) & 0xFF) << 8;

    Actor::spawnActor(280, settings, &spawnPos);
}

// Active collider callback: handle projectile hits on the boss
void SpookyBoss::activeCallback(ActiveCollider& self, ActiveCollider& other){
    // Only handle collisions with other entities
    if (!other.checkCollidedGroup(AcGroup::Entity)) {
        return;
    }
    SpookyBoss& boss = scast<SpookyBoss&>(*self.owner);
    if (boss.invulnTimer > 0) {
        return;
    }
    if (!other.owner || !(other.owner->settings & BlockProjectile::SettingsReflected)) {
        return;
    }

    // Register a hit in Phase 1
    boss.phaseOneHits++;
    boss.invulnTimer = 60; // brief invulnerability
    SND::playSFXUnique(380);
    // Flip the screen
    SpookyBoss::fightOnSubScreen = !SpookyBoss::fightOnSubScreen;

    // Reset attack cadence based on new hit count
    boss.bossTimer = getAttackIntervalForHits(boss.phaseOneHits);

    // Destroy the colliding entity (projectile)
    if (other.owner != nullptr) {
        other.owner->Base::destroy();
    }

    // No extra respawn here; attack cadence handles the next wave

    // TODO: transition to next phase when hits threshold reached
    if (boss.phaseOneHits >= requiredHits) {
        Log() << "Phase 1 complete";
        // Placeholder: will switch to Phase 2 state when implemented
    }
}

void SpookyBoss::switchState(void (SpookyBoss::*updateFunc)()) {
	auto updateFuncRaw = ptmf_cast(updateFunc);

	if (this->updateFunc != updateFuncRaw) {
		if (this->updateFunc) {
			this->updateStep = Func::Exit;
			this->updateFunc(this);
		}

		this->updateFunc = updateFuncRaw;

		this->updateStep = Func::Init;
		this->updateFunc(this);
	}
}

// Utility functions
static void copyChunk(u32 srcChunk, u32 dstChunk){
#ifdef NTR_DEBUG 
    u16 chunkCount = rcast<u16*>(Stage::stageLayout)[0x232];
    if (srcChunk >= chunkCount || dstChunk >= chunkCount) {
        Log::print("ERROR: Chunk index out of bounds. (src: %u, dst: %u, count: %u)", srcChunk, dstChunk, scast<u32>(chunkCount));
        OS_Terminate();
        return;
    }
#endif
    void** chunks = rcast<void**>(0x020CAFE0);
    MI_CpuCopyFast(chunks[srcChunk], chunks[dstChunk], 0x200);
}

static inline void refreshChunks(){
    rcast<u16*>(Stage::stageLayout)[0x46E/2] = rcast<u16*>(Stage::stageLayout)[0x46C/2];
}

u16 SpookyBoss::getAttackIntervalForHits(u8 hits){
    if (hits == 0) return attackIntervalHit0;
    if (hits == 1) return attackIntervalHit1;
    return attackIntervalHit2;
}
