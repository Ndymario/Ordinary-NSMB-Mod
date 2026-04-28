/*
The "chaser" is the sp00ky Luigi that the player can see chasing them when sp00ky mode is activated.
*/

#include "SpookyChaser.hpp"
#include "SpookyController.hpp"
#include "nsmb/core/system/memory.hpp"

ncp_over(0x020C619C, 0) const ObjectInfo objectInfo = Chaser::objectInfo; //Stage Actor ID 192
ncp_over(0x02039AEC) static constexpr const ActorProfile* profile = &Chaser::profile; //objectID 92

#ifdef NTR_DEBUG
extern "C" void MemoryDebug_setOptionalChaserResourceScope(bool enabled);
#endif

static u32 align16(u32 size)
{
    return (size + 15) & 0xFFFFFFF0;
}

static void setOptionalChaserResourceScope(bool enabled)
{
#ifdef NTR_DEBUG
    MemoryDebug_setOptionalChaserResourceScope(enabled);
#else
    (void)enabled;
#endif
}

bool Chaser::onPrepareResources(){
    return 1;
}

bool Chaser::loadResources() {
    return true;
}

// Code that runs the first time the Actor loads in
s32 Chaser::onCreate() {
	ctrl = SpookyController::getInstance();

	loadResources();

    if (!prepareModelResources()) {
        prepareNsbtxFallback();

        // Reload palette to restore chaser colors (palette was made greyscale before chaser spawned)
        reloadPalette();
    }

    viewOffset = Vec2(50, 50);
    activeSize = Vec2(1000.0, 3000.0);

    resetMusic = true;

    return 1;
}

// Code that runs every frame
bool Chaser::updateMain() {
    bool timerPaused = ctrl->shouldPauseTimer();

    if (renderMode == RenderMode::NsbtxFallback) {
        spookyNsbtx.setTexture(texID);
        spookyNsbtx.setPalette(texID);
        if(!timerPaused && ctrl->deathTimer % 5 == 0){
            texID = (texID + 1) % 6;
        }
    } else if (isVisible() && !timerPaused) {
        model->update();
    }

	moveTowardsPlayer();

    if (!timerPaused) {
        ctrl->deathTimer -= 1;
    }

    if (ctrl->deathTimer <= 0) {
        targetPlayer->damage(*this, 0, 0, PlayerDamageType::Death);
        if(Game::vsMode){
            ctrl->onBlockHit();
        }
    }

    return 1;
}

bool Chaser::isVisible() const {
    return ctrl->deathTimer < ctrl->suspenseTime;
}

void Chaser::moveTowardsPlayer() {
    targetPlayer = Game::getPlayer(currentTarget);

    if (ctrl->deathTimer >= ctrl->suspenseTime) {
        position.x = targetPlayer->position.x - ctrl->deathTimer * 1.0fx;
    } else {
        position.x = targetPlayer->position.x - playerBuffer - ctrl->deathTimer * 0.25fx;
    }

    position.y = targetPlayer->position.y - 16fx;
    position.z = targetPlayer->position.z;

    wrapPosition(position);
    
    if(resetMusic){
        if(!Game::vsMode){
            SND::pauseBGM(true);
        }
        SND::playSFXUnique(380);
        resetMusic = false;
    }
}

s32 Chaser::onRender() {
    if(!isVisible()){
        return 1;
    }

    if (renderMode == RenderMode::Model3D) {
        MTX::identity(model->matrix);
        MTX::translate(model->matrix, position);
        MTX::rotateY(model->matrix, 180);
        Game::modelMatrix = model->matrix;
        model->render(&modelScale);
    } else {
        Vec3 scale(1fx);
        spookyNsbtx.render(position, scale);
    }

    return 1;
}

// Code runs when this Actor is being destroyed
s32 Chaser::onDestroy() {
    if (model != nullptr) {
        delete model;
        model = nullptr;
    }

    SpookyController* controller = SpookyController::getInstance();
	controller->chaser = nullptr;
    return 1;
}

bool Chaser::canUseModelResources(bool& modelWasCached, bool& animationWasCached) const {
    modelWasCached = FS::Cache::getFile(SpookyResources::bossChaserModelID) != nullptr;
    animationWasCached = FS::Cache::getFile(SpookyResources::bossChaserIdleAnimationID) != nullptr;

    u32 required = modelMemoryReserve;
    if (!modelWasCached) {
        required += align16(FS::getFileSize(SpookyResources::bossChaserModelID));
    }
    if (!animationWasCached) {
        required += align16(FS::getFileSize(SpookyResources::bossChaserIdleAnimationID));
    }

    Heap* heap = Memory::currentHeapPtr;
    return heap != nullptr && heap->vMaxAllocatableSize(16) >= required;
}

bool Chaser::prepareModelResources() {
    bool modelWasCached = false;
    bool animationWasCached = false;
    if (!canUseModelResources(modelWasCached, animationWasCached)) {
        return false;
    }

    setOptionalChaserResourceScope(true);

    void* modelFile = FS::Cache::getFile(SpookyResources::bossChaserModelID);
    if (modelFile == nullptr) {
        modelFile = FS::Cache::loadFile(SpookyResources::bossChaserModelID, false);
    }

    void* animationFile = FS::Cache::getFile(SpookyResources::bossChaserIdleAnimationID);
    if (animationFile == nullptr) {
        animationFile = FS::Cache::loadFile(SpookyResources::bossChaserIdleAnimationID, false);
    }

    if (modelFile == nullptr || animationFile == nullptr) {
        unloadFailedModelResources(modelWasCached, animationWasCached);
        setOptionalChaserResourceScope(false);
        return false;
    }

    model = new ModelAnm();
    if (model == nullptr) {
        unloadFailedModelResources(modelWasCached, animationWasCached);
        setOptionalChaserResourceScope(false);
        return false;
    }

    if (!model->create(modelFile, animationFile, 0, 0, 0)) {
        delete model;
        model = nullptr;
        unloadFailedModelResources(modelWasCached, animationWasCached);
        setOptionalChaserResourceScope(false);
        return false;
    }

    model->init(1, FrameCtrl::Looping, 1fx, 0);
    renderMode = RenderMode::Model3D;
    setOptionalChaserResourceScope(false);
    return true;
}

void Chaser::prepareNsbtxFallback() {
    void* nsbtxFile;
    if(!Game::getPlayerCharacter(currentTarget)){
        nsbtxFile = FS::Cache::loadFile(SpookyResources::chaserLuigiNsbtxID, false);
    } else {
        nsbtxFile = FS::Cache::loadFile(SpookyResources::chaserMarioNsbtxID, false);
    }

    texID = 0;
    spookyNsbtx.setup(nsbtxFile, Vec2(64, 64), Vec2(0, 0), 0, 0);
    renderMode = RenderMode::NsbtxFallback;
}

void Chaser::unloadFailedModelResources(bool modelWasCached, bool animationWasCached) {
    if (!animationWasCached) {
        FS::Cache::unloadFile(SpookyResources::bossChaserIdleAnimationID);
    }
    if (!modelWasCached) {
        FS::Cache::unloadFile(SpookyResources::bossChaserModelID);
    }
}
