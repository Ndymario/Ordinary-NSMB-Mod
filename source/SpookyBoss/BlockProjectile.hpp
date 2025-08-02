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
        -10fx, //left
        9fx, //top
        7fx, //right
        -10fx, //bottom
        AcGroup::Entity,
        AcAttack::None,
        AC_GROUP_MASK(AcGroup::Player, AcGroup::PlayerSpecial, AcGroup::Entity, AcGroup::Fireball),
        AC_ATTACK_MASK_EXC(AcAttack::None),
        0,
        BlockProjectile::activeCallback
    };

    Collider collider;

    static constexpr u16 updatePriority = objectID;
    static constexpr u16 renderPriority = objectID;
    static constexpr ActorProfile profile = {&constructObject<BlockProjectile>, updatePriority, renderPriority, loadResources};

    // Variable stuff
    u8 blockPalette = 0;
    OAM::Flags oamFlags = OAM::Flags::None;
    OAM::Settings oamSettings = OAM::Settings::None;
    bool flipX = false;
    bool flipY = false;
    s16 rot = 32767;
    Vec2 oamScale;
    u32 frameCounter = 0;
    Vec3 toPlayer;
};