#pragma once

#include "nsmb.hpp"
#include "nsmb_nitro.hpp"
#include "NSBTX.hpp"
#include "SpookyResources.hpp"

class SpookyController;

class Chaser: public StageEntity {
private:
    enum class RenderMode : u8 {
        Model3D,
        NsbtxFallback
    };

	SpookyController* ctrl;

    ModelAnm* model = nullptr;
    Vec3 modelScale = Vec3(1fx);
    RenderMode renderMode = RenderMode::NsbtxFallback;

	void moveTowardsPlayer();
    bool isVisible() const;
    bool canUseModelResources(bool& modelWasCached, bool& animationWasCached) const;
    bool prepareModelResources();
    void prepareNsbtxFallback();
    void unloadFailedModelResources(bool modelWasCached, bool animationWasCached);

    fx32 playerBuffer = 40fx;
    static constexpr u32 modelMemoryReserve = 96 * 1024;

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
        EntityProperties::None,
        SpawnSettings::None
    };

    static constexpr u16 updatePriority = objectID;
    static constexpr u16 renderPriority = objectID;
    static constexpr ActorProfile profile = {&constructObject<Chaser>, updatePriority, renderPriority, loadResources};
    
    Player* targetPlayer;
    NSBTX spookyNsbtx;
    u8 texID = 0;
    bool resetMusic;

    s32 currentTarget = settings & 0xFF;

    // Reload palette to restore chaser colors after greyscale effect
    void reloadPalette() { spookyNsbtx.reloadPalette(); }
};
