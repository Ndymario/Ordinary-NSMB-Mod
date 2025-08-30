#include "nsmb.hpp"
#include "BlockProjectile.hpp"
#include "util/collisionviewer.hpp"
#include "SpookyController.hpp"

namespace Math {
    constexpr fx32 pi = 3.14159265358979323846fx;
}

ncp_over(0x020c6688, 0) const ObjectInfo objectInfo = BlockProjectile::objectInfo; //Stage Actor ID 255
ncp_over(0x02039ddc) static constexpr const ActorProfile* profile = &BlockProjectile::profile; //objectID 280

static GXOamAttr** BlockProjectileOAM = rcast<GXOamAttr**>(0x0212f180);

bool BlockProjectile::loadResources(){
    return true;
}

void BlockProjectile::prepareResources(){
    return;
}

void BlockProjectile::activeCallback(ActiveCollider& self, ActiveCollider& other){
    Log() << "Active callback";

}

s32 BlockProjectile::onCreate(){
    prepareResources();
	this->collider.init(this, BlockProjectile_colliderInfo, 0, 0, &this->scale);
	this->collider.link();
    collisionMgr.init(this, &bottomSensor, &topSensor, &sideSensor); 
    activeCollider.init(this, activeColliderInfo, 0);
    activeCollider.link();
    switchState(&BlockProjectile::spawn);
    scale.x = 0fx;
    scale.y = 0fx;
	return true;
}

s32 BlockProjectile::onDestroy(){
	return true;
}

s32 BlockProjectile::onRender(){
    oamScale = Vec2(scale.x, scale.y);

    oamFlags = OAM::Flags::None;

    // Set OAM flags based on velocity direction
    oamFlags = OAM::Flags::None;
    if (velocity.y < 0fx) {
        if (velocity.x > 0fx) oamFlags |= OAM::Flags::FlipX;
    }
    
    Log() << "Rot: " << rot;
    
    SpookyController* instance = SpookyController::getInstance();

    // Don't draw OAM over the screen static
    if(!instance->isRenderingStatic || instance->staticDuration <= 0){
        OAM::drawSprite(BlockProjectileOAM[blockPalette % 4], position.x, position.y, oamFlags, 0, 3, &oamScale, 0, nullptr, oamSettings);
    }
    
    // Only update palette every 8 frames
    if (++frameCounter >= 8) {
        blockPalette++;
        frameCounter = 0;
    }
    return true;
}

bool BlockProjectile::updateMain(){
    CollisionViewer::renderCollider(collider, CollisionViewer::Flags::Collider);
    CollisionViewer::renderActiveCollider(activeCollider, CollisionViewer::Flags::ActiveCollider);
	updateFunc(this);
	return true;
}

void BlockProjectile::spawn(){
	// When becoming idle, initalize the timer stuff back to "rest"
	if (updateStep == Func::Init) {
        updateStep = 1;
        return;
    }

    if (updateStep == Func::Exit) {
        return;
    }

    if (updateStep == 1){
        scale.lerp(VecFx32(1.0fx, 1.0fx), 0.5fx);
        if (scale.x >= 1.0fx){
            Player* player = Game::getLocalPlayer();
            Vec3 toPlayer = player->position - position;
            toPlayer.y += 32fx;
            toPlayer = toPlayer.normalize();
            velocity.x = toPlayer.x * 0.001fx;
            velocity.y = toPlayer.y * 0.001fx;
            switchState(&BlockProjectile::bouncing);
        }
    }
}

void BlockProjectile::bouncing(){
    // Initialize zone information once and reuse
    if (zone == nullptr) {
        zone = StageZone::get(0, &zoneRect);
    }

    if (updateStep == Func::Init) {
        scale = Vec3(1.0fx, 1.0fx, 1.0fx);
        updateStep = 1;
        return;
    }

    // Update velocity and apply movement
    if (updateStep == 1){
        Player* player = Game::getLocalPlayer();
        toPlayer = player->position - position;
        toPlayer.y += 32fx;
        toPlayer = toPlayer.normalize();
        velocity.x = toPlayer.x * 0.001fx;
        velocity.y = toPlayer.y * 0.001fx;

        if (position.x < zoneRect.x && velocity.x < 0){
            velocity.x = -velocity.x;
            flipX = !flipX;
        }

        if (position.x > zoneRect.x + zoneRect.width && velocity.x > 0){
            velocity.x = -velocity.x;
            flipX = !flipX;
        }

        if (position.y > zoneRect.y && velocity.y > 0){
            velocity.y = -velocity.y;
            flipY = !flipY;
        }
        
        if (position.y < zoneRect.y - zoneRect.height && velocity.y < 0){
            velocity.y = -velocity.y;
            flipY = !flipY;
        }

        applyMovement();
        collider.updatePosition();
    }
}

void BlockProjectile::switchState(void (BlockProjectile::*updateFunc)()) {
	auto updateFuncRaw = ptmf_cast(updateFunc);

	if (this->updateFunc != updateFuncRaw) {
		if (this->updateFunc) {
			this->updateStep = Func::Exit;
			this->updateFunc(this);
		}

		this->updateFunc = updateFuncRaw;

		this->updateStep = Func::Init;
		this->updateFunc(this);
	}
}

void BlockProjectile::hitFromTop(StageActor& _self, StageActor& other)
{
	BlockProjectile& self = scast<BlockProjectile&>(_self);

	//If not hit by player
	if (other.actorType != ActorType::Player)
		return;

	Player& player = scast<Player&>(other);

	if (player.actionFlag.groundpounding && (player.animID != 0x15))
	{
		Log() << "Hit from top (ground pound)";
	}
}

void BlockProjectile::hitFromBottom(StageActor& _self, StageActor& other)
{
	BlockProjectile& self = scast<BlockProjectile&>(_self);

	//If not hit by player or if being hit already
	if (other.actorType != ActorType::Player)
		return;

	Log() << "Hit from bottom";
}

void BlockProjectile::hitFromSide(StageActor& _self, StageActor& other)
{
	BlockProjectile& self = scast<BlockProjectile&>(_self);

	if (bool(other.collisionMgr.sideSensor->flags & CollisionMgr::SensorFlags::ActivateQuestionBlocks))
	{
		Log() << "Hit from side";
	}
}
