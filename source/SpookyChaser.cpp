/*
The "chaser" is the sp00ky Mario that the player can see chasing them when sp00ky mode is activated.
*/

#include "SpookyChaser.hpp"
#include "SpookyController.hpp"

ncp_over(0x020C619C, 0) const ObjectInfo objectInfo = Chaser::objectInfo; //Stage Actor ID 192
ncp_over(0x02039AEC) static constexpr const ActorProfile* profile = &Chaser::profile; //objectID 92

bool Chaser::loadResources() {
    FS::Cache::loadFile(headID, true); // Luigi head
    FS::Cache::loadFile(bodyID, true); // Luigi body
    FS::Cache::loadFile(bodyAnimationID, true); // Luigi body animations
    return true;
}

// Code that runs the first time the Actor loads in
s32 Chaser::onCreate() {
	loadResources();

	ctrl = SpookyController::getInstance();

    // Model stuff
    void* headFile = FS::Cache::getFile(headID);
    void* bodyFile = FS::Cache::getFile(bodyID);
    void* bodyAnmFile = FS::Cache::getFile(bodyAnimationID);

    headModel.create(headFile, 0, 0);
    headScale = Vec3(1.0fx, 1.0fx, 1.0fx);

    bodyModel.create(bodyFile, bodyAnmFile, 0, 0, 0);
    bodyModel.init(1, FrameCtrl::Looping, 1.0fx, 0);
    bodyScale = Vec3(2.0fx, 2.0fx, 2.0fx);

    viewOffset = Vec2(50, 50);
    activeSize = Vec2(500.0, 1000.0);

    scale = Vec3(2.0fx, 2.0fx, 2.0fx);

	moveTowardsPlayer();

    return 1;
}

// Code that runs every frame
bool Chaser::updateMain() {
    bodyModel.update();

	moveTowardsPlayer();

    ctrl->deathTimer -= 1;

    if (ctrl->deathTimer <= 0) {
        closestPlayer->damage(*this, 0, 0, PlayerDamageType::Death);
		headModel.disableRendering();
		bodyModel.disableRendering();
		ctrl->deathTimer = 0;
    }

    return 1;
}

void Chaser::moveTowardsPlayer() {
    fx32 newXPos;

	closestPlayer = getClosestPlayer(nullptr, nullptr);

    if (ctrl->deathTimer >= ctrl->suspenseTime) {
        newXPos = closestPlayer->position.x - ctrl->deathTimer * 1.0fx;
    } else {
        newXPos = closestPlayer->position.x - ctrl->deathTimer * 0.25fx;
    }

    position = Vec3(newXPos, closestPlayer->position.y, closestPlayer->position.z);
}

s32 Chaser::onRender() {
    // Render the body at the correct location with the correct rotation
    MTX::identity(bodyModel.matrix);
    MTX::translate(bodyModel.matrix, position);
    MTX::rotate(bodyModel.matrix, rotation);
    Game::modelMatrix = bodyModel.matrix;
    bodyModel.render(&bodyScale);

    // Put the head on node 15 (the head node)
        // This would apply the pitch to the head model if I could get the math right...
    // MTX::identity(headModel.matrix);
    // MTX::rotateX(headModel.matrix, headPitch);
    bodyModel.getNodeMatrix(15, &headModel.matrix);
    MTX::rotateY(headModel.matrix, headYaw);
    MTX::rotateZ(headModel.matrix, headPitch);
    headModel.render(&headScale);
    return 1;
}

// Code runs when this Actor is being destroyed
s32 Chaser::onDestroy() {
    SpookyController* controller = SpookyController::getInstance();
	controller->chaser = nullptr;
    return 1;
}
