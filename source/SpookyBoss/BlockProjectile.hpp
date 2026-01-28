#pragma once

#include "nsmb.hpp"
#include "nsmb_nitro.hpp"
#include "NSBTX.hpp"

class SpookyController;

class BlockProjectile: public StageEntity {
private:
    // State machine functions
    void spawn();
    void bouncing();
    void returnToBoss();
    void despawn();
    void idle();

    // Collsion handlers
    static void hitFromTop(StageActor& self, StageActor& other);
    static void hitFromBottom(StageActor& self, StageActor& other);
    static void hitFromSide(StageActor& self, StageActor& other);

    // Active collider callback
    static void activeCallback(ActiveCollider& self, ActiveCollider& other);
    
    const LineSensorH bottomSensor = {
        0fx, // Start position from the origin (left)
        16fx, // End position from the origin (right)
        0fx, // Y Offset (negative = down)
    };

    const LineSensorH topSensor = {
        0fx, // Start position from the origin (left)
        16fx, // End position from the origin (right)
        0fx, // Y Offset (negative = down)
    };
    
    const LineSensorV sideSensor = {
        0fx, // Start position from the origin (top)
        16fx, // End position from the origin (bottom)
        0fx, // X offset (negative = left)
    };

public:
    static bool loadResources();
    virtual s32 onCreate() override;
    void prepareResources();

    virtual bool updateMain() override;
    virtual s32 onRender() override;

    virtual s32 onDestroy() override;

    void (*updateFunc)(BlockProjectile*);
	s8 updateStep;

    void switchState(void (BlockProjectile::*updateFunc)());

    static constexpr u16 objectID = 280; //objectID

    static constexpr ObjectInfo objectInfo = {
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        EntityProperties::None,
        SpawnSettings::None
    };

    static constexpr ColliderInfo BlockProjectile_colliderInfo =
    {
        -10fx, //left
        9fx, //top
        7fx, //right
        -10fx, //bottom
        {
            BlockProjectile::hitFromTop, //callbackTop
            BlockProjectile::hitFromBottom, //callbackBottom
            BlockProjectile::hitFromSide, //callbackSide
        }
    };

    static constexpr AcConfig activeColliderInfo = {
        { 0fx, 0fx, 10fx, 10fx }, // rect: xOffset, yOffset, halfWidth, halfHeight
        AcGroup::Entity,
        AcAttack::None,
        AC_GROUP_MASK(AcGroup::Player, AcGroup::PlayerSpecial, AcGroup::Entity, AcGroup::Fireball),
        AC_ATTACK_MASK_INC(AcAttack::None),
        0,
        BlockProjectile::activeCallback
    };

    Collider collider;

    // Settings bit masks (shared with SpookyBoss hit detection)
    static constexpr s32 SettingsFromBackground = 0x1;
    static constexpr s32 SettingsReflected = 0x2;
    static constexpr s32 SettingsSpiked = 0x4;
    static constexpr s32 SettingsUseDirection = 0x8;
    static constexpr s32 SettingsSlowThrow = 0x10; // force slower initial speed
    static constexpr s32 SettingsPatternShift = 8;
    static constexpr s32 SettingsDirShift = 16;

    static constexpr u16 updatePriority = objectID;
    static constexpr u16 renderPriority = objectID;
    static constexpr ActorProfile profile = {&constructObject<BlockProjectile>, updatePriority, renderPriority, loadResources};

    // Variable stuff
    u8 blockPalette = 0;
    OAM::Flags oamFlags = OAM::Flags::None;
    OAM::Settings oamSettings = OAM::Settings::None;
    u8 oamPriority = 3; // dynamic OAM priority (3 background -> 0 foreground)
    bool flipX = false;
    bool flipY = false;
    u16 rot = 0;
    Vec2 oamScale;
    u32 frameCounter = 0;
    Vec3 toPlayer;
    Vec3 lastPos;
    u8 stillFrames = 0;          // counts frames with negligible movement
    static constexpr u8 stuckLimit = 10;     // fail after 10 frames
    static constexpr fx32 minMoveThreshold = 0.02fx; // min movement per frame considered "significant"
    u8 stuckGrace = 20;           // skip stuck check for first N frames of bouncing

    // Speed scaling (applied to initial throw speed)
    fx32 speedScale = 1.0fx;
    
    // Background entry parameters
    bool fromBackground = false; // set from settings bit 0
    u8 throwPattern = 0;         // optional pattern (settings >> 8)
    u16 enterFrames = 0;         // counts frames during entry
    u16 enterDuration = 12;      // how long to scale-in before switching to bouncing

    // Reflection/return behavior
    bool reflected = false;
    u16 returnTimer = 0;
    static constexpr u16 returnDuration = 90;
    static constexpr fx32 returnSpeed = 1.6fx;

    // Spiked variant (hazard) behavior
    bool spikedVariant = false;
    u8 bounceCount = 0;
    static constexpr u8 spikeBounceLimit = 5;
    u8 normalBounceCount = 0;
    static constexpr u8 normalBounceLimit = 3;
    bool despawning = false;
    u8 despawnTimer = 0;
    static constexpr u8 despawnFrames = 10;

    // Optional fixed direction override (for scripted patterns)
    bool useFixedDirection = false;
    u8 fixedDirection = 0;

    // Cached zone info to avoid per-frame allocations
    Rectangle<fx32> zoneRect = Rectangle<fx32>(0, 0, 0, 0);
    StageZone* zone = nullptr;
};
