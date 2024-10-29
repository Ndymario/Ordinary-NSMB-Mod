#pragma once

#include "nsmb.hpp"
#include "nsmb_nitro.hpp"
#include "NSBTX.hpp"

class SpookyController;

class Chaser: public StageEntity {
private:
	SpookyController* ctrl;

	void moveTowardsPlayer();

    fx32 playerBuffer = 40fx;

public:
    static bool loadResources();
    virtual bool onPrepareResources() override;
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
    
    Player* closestPlayer;
    NSBTX spookyNsbtx;
    u8 texID = 0;
    bool resetMusic;
};
