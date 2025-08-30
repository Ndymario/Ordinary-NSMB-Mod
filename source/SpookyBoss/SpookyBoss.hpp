#pragma once

#include "nsmb.hpp"

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
    Vec3 mirrorPosition;
    Rectangle<fx32> zoneRect = Rectangle<fx32>(0, 0, 0, 0);
    StageZone* zone;
public:
    void prepareResources();

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
