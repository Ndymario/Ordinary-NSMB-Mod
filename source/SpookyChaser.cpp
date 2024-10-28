/*
The "chaser" is the sp00ky Luigi that the player can see chasing them when sp00ky mode is activated.
*/

#include "SpookyChaser.hpp"
#include "SpookyController.hpp"

ncp_over(0x020C619C, 0) const ObjectInfo objectInfo = Chaser::objectInfo; //Stage Actor ID 192
ncp_over(0x02039AEC) static constexpr const ActorProfile* profile = &Chaser::profile; //objectID 92

bool Chaser::onPrepareResources(){
    void* nsbtxFile = FS::Cache::loadFile(2090 - 131, false);
	spookyNsbtx.setup(nsbtxFile, Vec2(64, 64), Vec2(0, 0), 0, 0);
    return 1;
}

bool Chaser::loadResources() {
    return true;
}

// Code that runs the first time the Actor loads in
s32 Chaser::onCreate() {
	ctrl = SpookyController::getInstance();

    onPrepareResources();
	loadResources();

    viewOffset = Vec2(50, 50);
    activeSize = Vec2(500.0, 1000.0);

    return 1;
}

// Code that runs every frame
bool Chaser::updateMain() {
    spookyNsbtx.setTexture(texID);
    spookyNsbtx.setPalette(texID);
    if(ctrl->deathTimer % 10 == 0){
        texID = (texID + 1) % 6;
    }
	moveTowardsPlayer();

    ctrl->deathTimer -= 1;

    if (ctrl->deathTimer <= 0) {
        closestPlayer->damage(*this, 0, 0, PlayerDamageType::Death);
		ctrl->deathTimer = 0;
    }

    return 1;
}

void Chaser::moveTowardsPlayer() {
	closestPlayer = getClosestPlayer(nullptr, nullptr);

    if (ctrl->deathTimer >= ctrl->suspenseTime) {
        position.x = closestPlayer->position.x - ctrl->deathTimer * 1.0fx;
    } else {
        position.x = closestPlayer->position.x - playerBuffer - ctrl->deathTimer * 0.25fx;
    }

    position.y = closestPlayer->position.y - 16fx;
    position.z = closestPlayer->position.z;
}

s32 Chaser::onRender() {
    Vec3 scale(1fx);
    spookyNsbtx.render(position, scale);
    return 1;
}

// Code runs when this Actor is being destroyed
s32 Chaser::onDestroy() {
    SpookyController* controller = SpookyController::getInstance();
	controller->chaser = nullptr;
    return 1;
}
