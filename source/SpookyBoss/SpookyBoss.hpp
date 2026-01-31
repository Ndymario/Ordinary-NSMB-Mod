#pragma once

#include "nsmb.hpp"

class BlockProjectile;

class SpookyBoss: public StageEntity3DAnm {
private:
    // Model stuff
    const static u32 bossModelID = 2089 - 131;
    const static u32 idleAnimationID = 2093 - 131;
    Vec3 bossScale;

    u8 fightActivationRange = 8;
    Vec3 initialPosition;

    Player* player = Game::getLocalPlayer();

    s32 bossTimer;

    // State Machineeeeee
        // Pre-fight
    void idleState();
    void introAnimation();
        // Phase 1
    void mimicState();
    void mimicDamage();
    void mimicDefeated();
    BlockProjectile* spawnBossBlock(u8 pattern = 0);
    BlockProjectile* spawnBossBlock(u8 pattern, bool spiked, s8 dirIndex);
    BlockProjectile* spawnBossBlock(u8 pattern, bool spiked, s8 dirIndex, const Vec2& offset, fx32 speedScale = 1.0fx);
    void performAttackPattern();
    Vec3 mirrorPosition;
    Rectangle<fx32> zoneRect = Rectangle<fx32>(0, 0, 0, 0);
    StageZone* zone;

    // Active collider for detecting projectile hits
    static void activeCallback(ActiveCollider& self, ActiveCollider& other);
    // Tune the boss hitbox to cover Luigi's body (feet -> head)
    // Offsets are relative to the boss origin; positive y raises the box.
    static constexpr AcConfig bossActiveColliderInfo = {
        { 0fx, 24fx, 16fx, 28fx },          // xOff, yOff, halfW, halfH
        AcGroup::Entity,                    // boss is an entity
        AcAttack::None,                     // boss isn't attacking with AC
        AC_GROUP_MASK(AcGroup::Entity),     // detect entities (projectiles)
        AC_ATTACK_MASK_INC(AcAttack::None), // respond to 'None' attacks too
        0,
        SpookyBoss::activeCallback
    };

    // Phase 1 hit tracking
    u8 phaseOneHits = 0;
    u16 invulnTimer = 0;
    // After-hit block respawn timer (frames); 0 = idle
    u16 respawnTimer = 0;
    // Required hits to finish phase one
    static constexpr u8 requiredHits = 3;

    // Flip the fight view to the sub screen when true
    static bool fightOnSubScreen;
    void applyFightScreenFlag();

    // Hover/movement behavior during mimic phase
    fx32 hoverOffset = 32fx;       // how far above the player to hover
    u16 moveTimer = 0;             // countdown to pick a new target
    Vec3 moveTarget = Vec3(0,0,0); // current target when lerping
    bool teleportMode = false;     // whether the current interval is a teleport
    bool wonderMode = false;       // wander around arena before next RNG pick
    u16  wonderTimer = 0;          // total duration of current wander
    u16  wanderStepTimer = 0;      // time until next wander micro-target
    fx32 anchorY = 0fx;            // preferred Y anchor for mimic phase

    // Throw windup
    u16 throwPauseTimer = 0;
    static constexpr u16 throwWindupFrames = 24;

    // Attack cadence (frames at 60fps)
    static constexpr u16 attackIntervalHit0 = 600; // 10s
    static constexpr u16 attackIntervalHit1 = 480; // 8s
    static constexpr u16 attackIntervalHit2 = 360; // 6s

    static u16 getAttackIntervalForHits(u8 hits);
public:
    void prepareResources();

    // Helper: boss spawns a block from the background edges
    void spawnBackgroundBlock(u8 pattern = 0);

    // Active boss instance for projectile targeting
    static SpookyBoss* instance;

    virtual s32 onCreate() override;

    static bool loadResources();

    virtual bool updateMain() override;

    virtual s32 onDestroy() override;

    static constexpr u16 objectID = 46;

    static constexpr ObjectInfo objectInfo = {
        0, 16,
        0, 0,
        0, 0,
        0, 0,
        EntityProperties::None,
        SpawnSettings::None
    };

    static constexpr u16 updatePriority = objectID;
    static constexpr u16 renderPriority = objectID;
    static constexpr ActorProfile profile = {&constructObject<SpookyBoss>, updatePriority, renderPriority, loadResources};

    void (*updateFunc)(SpookyBoss*);
	s8 updateStep;

    void switchState(void (SpookyBoss::*updateFunc)());
};
