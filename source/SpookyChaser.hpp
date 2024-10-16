#pragma once

#include "nsmb.hpp"
#include "nsmb_nitro.hpp"

class SpookyController;

class Chaser: public StageEntity {
private:
	SpookyController* ctrl;

    Model headModel;
    const static u32 headID = 1878 - 131;
    Vec3 headScale;
    fx16 headPitch = 0;
    fx16 headYaw = 0;

    ModelAnm bodyModel;
    const static u32 bodyID = 1881 - 131;
    const static u32 bodyAnimationID = 1892 - 131;
    Vec3 bodyScale;

    Player* closestPlayer;

	void moveTowardsPlayer();

public:
    static bool loadResources();
    virtual s32 onCreate() override;

    virtual bool updateMain() override;
    virtual s32 onRender() override;

    virtual s32 onDestroy() override;

    static constexpr u16 objectID = 92; //objectID

    static constexpr ObjectInfo objectInfo = {
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        CollisionSwitch::None,
    };

    static constexpr u16 updatePriority = objectID;
    static constexpr u16 renderPriority = objectID;
    static constexpr ActorProfile profile = {&constructObject<Chaser>, updatePriority, renderPriority, loadResources};
};
