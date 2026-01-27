#include "nsmb.hpp"
#include "BlockProjectile.hpp"
#include "util/collisionviewer.hpp"
#include "SpookyController.hpp"
#include "SpookyBoss.hpp"

namespace Math {
    constexpr fx32 pi = 3.14159265358979323846fx;
}

ncp_over(0x020c6688, 0) const ObjectInfo objectInfo = BlockProjectile::objectInfo; //Stage Actor ID 255
ncp_over(0x02039ddc) static constexpr const ActorProfile* profile = &BlockProjectile::profile; //objectID 280

static GXOamAttr** BlockProjectileOAM = rcast<GXOamAttr**>(0x0212f180);
// Rotating Spiked Block OAM index within the animated tiles table
static constexpr u32 RotatingSpikeOamIndex = 9; // matches ov54 table entry for spiked block

static constexpr u8 spikePaletteOffset = 0;
static constexpr fx32 spikeHurtOffset = 12fx;
static constexpr fx32 spikeHurtRadius = 6fx;

// OAM::drawSprite expects rotation as an unsigned 0..0xFFFF value in practice.
// Call it via a u16 signature to avoid sign-extension for angles > 0x7FFF.
static inline bool drawSpriteU16(const GXOamAttr* oamAttrs, fx32 x, fx32 y, OAM::Flags flags,
                                 u8 palette, u8 affineSet, const Vec2* scale, u16 rot,
                                 const s16 rotCenter[2], OAM::Settings settings) {
    using Fn = bool (*)(const GXOamAttr*, fx32, fx32, OAM::Flags, u8, u8, const Vec2*, u16, const s16*, OAM::Settings);
    return rcast<Fn>(0x0200D578)(oamAttrs, x, y, flags, palette, affineSet, scale, rot, rotCenter, settings);
}


static inline bool getSpikeOam(GXOamAttr** out) {
    u32 idx = RotatingSpikeOamIndex;
    GXOamAttr* oam = BlockProjectileOAM[idx];
    if (!oam) return false;
    u32 addr = rcast<u32>(oam);
    if (addr < 0x02000000 || addr > 0x02FFFFFF) return false;
    *out = oam;
    return true;
}

static inline void updateSpikeHurtbox(BlockProjectile& self) {
    // Rotate hurtbox in the same (clockwise) direction as the sprite
    fx32 sinv = scast<fx32>(Math::sin(scast<int>(self.rot)));
    fx32 cosv = scast<fx32>(Math::cos(scast<int>(self.rot)));
    self.activeCollider.config.rect.x = Math::mul(spikeHurtOffset, sinv);
    self.activeCollider.config.rect.y = Math::mul(spikeHurtOffset, cosv);
    self.activeCollider.shape = AcShape::Round;
    self.activeCollider.config.rect.halfWidth = spikeHurtRadius;
    self.activeCollider.config.rect.halfHeight = spikeHurtRadius;
    self.activeCollider.sharedData = self.rot;
}

static inline void setVelocityToward(BlockProjectile& self, const Vec3& target, fx32 speed) {
    Vec3 toTarget = target - self.position;

    fx32 ax = Math::abs(toTarget.x);
    fx32 ay = Math::abs(toTarget.y);
    fx32 maxc = (ax > ay) ? ax : ay;

    fx32 dirX = 0fx;
    fx32 dirY = 0fx;
    if (maxc > 0fx) {
        dirX = Math::div(toTarget.x, maxc); // [-1..1] in fx32
        dirY = Math::div(toTarget.y, maxc); // [-1..1] in fx32
    } else {
        dirX = 0fx;
        dirY = -1fx;
    }

    self.velocity.x = Math::mul(dirX, speed);
    self.velocity.y = Math::mul(dirY, speed);
}

static inline s16 facingAngleFromVelocity(fx32 vx, fx32 vy) {
    fx32 ax = Math::abs(vx);
    fx32 ay = Math::abs(vy);

    if (ax > (ay * 2)) {
        return (vx >= 0fx) ? 0x0000 : 0x8000;
    }
    if (ay > (ax * 2)) {
        return (vy >= 0fx) ? scast<s16>(0xC000) : 0x4000;
    }

    if (vx >= 0fx && vy < 0fx) return 0x2000;  // up-right
    if (vx < 0fx && vy < 0fx)  return 0x6000;  // up-left
    if (vx < 0fx && vy >= 0fx) return scast<s16>(0xA000); // down-left
    return scast<s16>(0xE000); // down-right
}

static inline void applyHomingTowardPlayer(BlockProjectile& self, fx32 speed, fx32 rngScale) {
    Player* player = Game::getLocalPlayer();
    Vec3 toPlayer = player->position - self.position;

    fx32 ax = Math::abs(toPlayer.x);
    fx32 ay = Math::abs(toPlayer.y);
    fx32 maxc = (ax > ay) ? ax : ay;

    fx32 dirX = 0fx;
    fx32 dirY = 0fx;
    if (maxc > 0fx) {
        dirX = Math::div(toPlayer.x, maxc);
        dirY = Math::div(toPlayer.y, maxc);
    } else {
        dirX = 0fx;
        dirY = -1fx;
    }

    fx32 perpX = -dirY;
    fx32 perpY =  dirX;
    s32 r = scast<s32>(Net::getRandom() % 2001) - 1000; // [-1000, 1000]
    fx32 k = scast<fx32>(r) * rngScale * 0.001fx;

    fx32 newX = dirX + perpX * k;
    fx32 newY = dirY + perpY * k;

    fx32 nax = Math::abs(newX);
    fx32 nay = Math::abs(newY);
    fx32 nmax = (nax > nay) ? nax : nay;
    if (nmax > 0fx) {
        newX = Math::div(newX, nmax);
        newY = Math::div(newY, nmax);
    }

    self.velocity.x = Math::mul(newX, speed);
    self.velocity.y = Math::mul(newY, speed);
}

bool BlockProjectile::loadResources(){
    return true;
}

void BlockProjectile::prepareResources(){
    return;
}

void BlockProjectile::activeCallback(ActiveCollider& self, ActiveCollider& other){
    if (!self.owner || !other.owner) return;
    BlockProjectile& proj = scast<BlockProjectile&>(*self.owner);
    if (!proj.spikedVariant) return;
    if (other.owner->actorType != ActorType::Player) return;
    Player& player = scast<Player&>(*other.owner);
    player.damage(proj, 0, 0, PlayerDamageType::Hit);
}

s32 BlockProjectile::onCreate(){
    prepareResources();
	this->collider.init(this, BlockProjectile_colliderInfo, 0, 0, &this->scale);
	this->collider.link();
    collisionMgr.init(this, &bottomSensor, &topSensor, &sideSensor); 
    activeCollider.init(this, activeColliderInfo, 0);
    activeCollider.link();

    // Decode settings for background entry and pattern
    fromBackground = (settings & SettingsFromBackground) != 0;
    reflected      = (settings & SettingsReflected) != 0;
    spikedVariant  = (settings & SettingsSpiked) != 0;
    useFixedDirection = (settings & SettingsUseDirection) != 0;
    manualRot      = (settings & SettingsManualRot) != 0;
    fixedDirection    = scast<u8>((settings >> SettingsDirShift) & 0x0F);
    throwPattern   = scast<u8>((settings >> SettingsPatternShift) & 0xFF);

    // Default scale/priority
    if (fromBackground) {
        scale.x = 0.2fx;
        scale.y = 0.2fx;
        oamPriority = 3;
        enterFrames = 0;
    } else {
        scale.x = 0fx;
        scale.y = 0fx;
        oamPriority = 0;
    }
    if (spikedVariant) {
        reflected = false;
        settings &= ~SettingsReflected;
        oamPriority = 2;
        activeCollider.shape = AcShape::Round;
        activeCollider.config.rect.halfWidth = spikeHurtRadius;
        activeCollider.config.rect.halfHeight = spikeHurtRadius;
    }

    switchState(&BlockProjectile::spawn);

    // Initialize movement tracking
    lastPos = position;
    stillFrames = 0;
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
    if (!spikedVariant) {
        if (velocity.y < 0fx) {
            if (velocity.x > 0fx) oamFlags |= OAM::Flags::FlipX;
        }
    }
    
    OAM::Flags prioFlag = OAM::Flags::None;
    if (oamPriority == 1) prioFlag = OAM::Flags::Prio1;
    else if (oamPriority == 2) prioFlag = OAM::Flags::Prio2;
    else if (oamPriority >= 3) prioFlag = OAM::Flags::Prio3;
    
    SpookyController* instance = SpookyController::getInstance();

    // Don't draw OAM over the screen static
    if(!instance->isRenderingStatic || instance->staticDuration <= 0){
        if (spikedVariant) {
            GXOamAttr* spikeOam = nullptr;
            if (getSpikeOam(&spikeOam)) {
                Vec2 spikeScale = despawning ? Vec2(scale.x, scale.y) : Vec2(1.0fx, 1.0fx);
                drawSpriteU16(spikeOam, position.x, position.y, oamFlags | prioFlag, spikePaletteOffset, 0, &spikeScale, rot, nullptr, oamSettings);
            } else {
                OAM::drawSprite(BlockProjectileOAM[blockPalette % 4], position.x, position.y, oamFlags | prioFlag, 0, 0, &oamScale, 0, nullptr, oamSettings);
            }
        } else {
            OAM::drawSprite(BlockProjectileOAM[blockPalette % 4], position.x, position.y, oamFlags | prioFlag, 0, 0, &oamScale, 0, nullptr, oamSettings);
        }
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
    if (spikedVariant) {
        if (!manualRot) {
            // Use full-circle 0..0xFFFF rotation so scale stays correct in all quadrants
            u16 angle = scast<u16>(Math::atan2(velocity.y, velocity.x));
            rot = scast<u16>(0x4000 - angle); // sprite faces up by default
        }
        updateSpikeHurtbox(*this);
    }
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
        // Scale in; if from background, bring OAM priority forward during entry
        scale.lerp(VecFx32(1.0fx, 1.0fx), 0.5fx);
        if (fromBackground){
            enterFrames++;
            if (enterFrames > enterDuration / 2 && oamPriority > 1) {
                oamPriority = 1;
            }
            if (scale.x >= 0.95fx) {
                oamPriority = 0;
            }
        }

        if (scale.x >= 1.0fx){
            // Per-axis 0..1 direction toward player (or fixed scripted direction)
            fx32 dirX = 0fx;
            fx32 dirY = 0fx;
            if (useFixedDirection) {
                switch (fixedDirection) {
                    case 0: dirX = -1fx; dirY = -1fx; break; // down-left
                    case 1: dirX =  1fx; dirY = -1fx; break; // down-right
                    case 2: dirX = -1fx; dirY =  1fx; break; // up-left
                    case 3: dirX =  1fx; dirY =  1fx; break; // up-right
                    case 4: dirX = -1fx; dirY =  0fx; break; // left
                    case 5: dirX =  1fx; dirY =  0fx; break; // right
                    case 6: dirX =  0fx; dirY = -1fx; break; // down
                    case 7: dirX =  0fx; dirY =  1fx; break; // up
                    default: dirX =  1fx; dirY = -1fx; break;
                }
            } else {
                Player* player = Game::getLocalPlayer();
                Vec3 toPlayer = player->position - position;
                toPlayer.y += 32fx;
                fx32 ax = Math::abs(toPlayer.x);
                fx32 ay = Math::abs(toPlayer.y);
                fx32 maxc = (ax > ay) ? ax : ay;
                if (maxc > 0fx) {
                    dirX = Math::div(toPlayer.x, maxc); // [-1..1] in fx32
                    dirY = Math::div(toPlayer.y, maxc); // [-1..1] in fx32
                } else {
                    // Fallback tiny nudge to avoid a dead zero vector
                    dirX = (Net::getRandom() & 1) ? 1fx : -1fx;
                    dirY = 0fx;
                }
            }

            // Apply a random deviation for background throws; boss throws aim directly at Mario
            // newDir = dir + k * perp, where angle(newDir, dir) = atan(k)
            if (!useFixedDirection) {
                fx32 perpX = -dirY;
                fx32 perpY =  dirX;
                fx32 k = 0fx;
                if (!spikedVariant && fromBackground) {
                    s32 r = scast<s32>(Net::getRandom() % 2001) - 1000; // [-1000,1000]
                    k = scast<fx32>(r) * 0.001fx;                      // [-1.0, 1.0]
                }
                fx32 newX = dirX + perpX * k;
                fx32 newY = dirY + perpY * k;
                // Re-apply per-axis 0..1 normalization to preserve method semantics
                fx32 nax = Math::abs(newX);
                fx32 nay = Math::abs(newY);
                fx32 nmax = (nax > nay) ? nax : nay;
                if (nmax > 0fx) {
                    dirX = Math::div(newX, nmax);
                    dirY = Math::div(newY, nmax);
                }
            }

            // Start with a faster initial speed per-axis
            fx32 speed = 1.0fx;
            if (throwPattern == 1) speed = 0.25fx;
            if (throwPattern == 2) speed = 1.15fx;
            if (throwPattern == 3) speed = 1.25fx;
            if (settings & SettingsSlowThrow) speed = 0.25fx;
            if (manualRot) speed = 0fx;

#ifdef NTR_DEBUG
            Log::print(
                "BlockProjectile spawn: settings=0x%08X patt=%u slow=%u spiked=%u fromBg=%u fixed=%u dirIdx=%u speedRaw=0x%08X speed=%d",
                scast<u32>(settings),
                scast<u32>(throwPattern),
                (settings & SettingsSlowThrow) ? 1 : 0,
                spikedVariant ? 1 : 0,
                fromBackground ? 1 : 0,
                useFixedDirection ? 1 : 0,
                scast<u32>(fixedDirection),
                scast<u32>(speed),
                scast<s32>(speed >> 12)
            );
#endif

            velocity.x = Math::mul(dirX, speed);
            velocity.y = Math::mul(dirY, speed);

#ifdef NTR_DEBUG
            Log::print(
                "BlockProjectile velocity: vxRaw=0x%08X vyRaw=0x%08X vx=%d vy=%d",
                scast<u32>(velocity.x),
                scast<u32>(velocity.y),
                scast<s32>(velocity.x >> 12),
                scast<s32>(velocity.y >> 12)
            );
#endif
            switchState(&BlockProjectile::bouncing);
        }
    }
}

void BlockProjectile::bouncing(){
    if (updateStep == Func::Init) {
        scale = Vec3(1.0fx, 1.0fx, 1.0fx);
        // Reset failsafe trackers and provide a short grace period
        lastPos = position;
        stillFrames = 0;
        stuckGrace = 20;
        bounceCount = 0;
        normalBounceCount = 0;
        despawning = false;
        // Cache zone once
        if (!zone) zone = StageZone::get(0, &zoneRect);
        updateStep = 1;
        return;
    }

    // Update velocity and apply movement, then bounce off zone bounds
    if (updateStep == 1){
        // Precompute zone bounds with a small inset
        fx32 minX = zoneRect.x + 8fx;
        fx32 maxX = zoneRect.x + zoneRect.width - 8fx;
        fx32 topY = zoneRect.y - 8fx;
        fx32 botY = zoneRect.y - zoneRect.height + 8fx;

        bool bounced = false;

        applyMovement();

        if (position.x < minX) {
            position.x = minX;
            velocity.x = -velocity.x;
            bounced = true;
            if (!spikedVariant) {
                // Add slight random jitter to break straight paths
                fx32 jx = scast<fx32>(scast<s32>(Net::getRandom() % 5) - 2) * 0.05fx; // [-0.1, 0.1]
                fx32 jy = scast<fx32>(scast<s32>(Net::getRandom() % 5) - 2) * 0.05fx;
                velocity.x += jx;
                velocity.y += jy;
            }
            flipX = !flipX;
            if (!spikedVariant) {
                // Ensure minimum per-axis speed
                if (Math::abs(velocity.x) < 0.05fx) velocity.x = (velocity.x >= 0fx ? 0.05fx : -0.05fx);
                if (Math::abs(velocity.y) < 0.05fx) velocity.y = (velocity.y >= 0fx ? 0.05fx : -0.05fx);
            }
        } else if (position.x > maxX) {
            position.x = maxX;
            velocity.x = -velocity.x;
            bounced = true;
            if (!spikedVariant) {
                fx32 jx = scast<fx32>(scast<s32>(Net::getRandom() % 5) - 2) * 0.05fx;
                fx32 jy = scast<fx32>(scast<s32>(Net::getRandom() % 5) - 2) * 0.05fx;
                velocity.x += jx;
                velocity.y += jy;
            }
            flipX = !flipX;
            if (!spikedVariant) {
                if (Math::abs(velocity.x) < 0.05fx) velocity.x = (velocity.x >= 0fx ? 0.05fx : -0.05fx);
                if (Math::abs(velocity.y) < 0.05fx) velocity.y = (velocity.y >= 0fx ? 0.05fx : -0.05fx);
            }
        }

        if (position.y > topY) {
            position.y = topY;
            velocity.y = -velocity.y;
            bounced = true;
            if (!spikedVariant) {
                fx32 jx = scast<fx32>(scast<s32>(Net::getRandom() % 5) - 2) * 0.05fx;
                fx32 jy = scast<fx32>(scast<s32>(Net::getRandom() % 5) - 2) * 0.05fx;
                velocity.x += jx;
                velocity.y += jy;
            }
            flipY = !flipY;
            if (!spikedVariant) {
                if (Math::abs(velocity.x) < 0.05fx) velocity.x = (velocity.x >= 0fx ? 0.05fx : -0.05fx);
                if (Math::abs(velocity.y) < 0.05fx) velocity.y = (velocity.y >= 0fx ? 0.05fx : -0.05fx);
            }
        } else if (position.y < botY) {
            position.y = botY;
            velocity.y = -velocity.y;
            bounced = true;
            if (!spikedVariant) {
                fx32 jx = scast<fx32>(scast<s32>(Net::getRandom() % 5) - 2) * 0.05fx;
                fx32 jy = scast<fx32>(scast<s32>(Net::getRandom() % 5) - 2) * 0.05fx;
                velocity.x += jx;
                velocity.y += jy;
            }
            flipY = !flipY;
            if (!spikedVariant) {
                if (Math::abs(velocity.x) < 0.05fx) velocity.x = (velocity.x >= 0fx ? 0.05fx : -0.05fx);
                if (Math::abs(velocity.y) < 0.05fx) velocity.y = (velocity.y >= 0fx ? 0.05fx : -0.05fx);
            }
        }

        if (bounced) {
            if (spikedVariant) {
                bounceCount++;
                if (bounceCount >= spikeBounceLimit) {
                    despawning = true;
                    switchState(&BlockProjectile::despawn);
                    return;
                }
                fx32 speed = Math::abs(velocity.x);
                fx32 absY = Math::abs(velocity.y);
                if (absY > speed) speed = absY;
                if (speed < 0.05fx) speed = 0.05fx;
                applyHomingTowardPlayer(*this, speed, 0.35fx);
            } else {
                normalBounceCount++;
                if (normalBounceCount >= normalBounceLimit) {
                    despawning = true;
                    switchState(&BlockProjectile::despawn);
                    return;
                }
            }
        }

        collider.updatePosition();

        // Debug/manual rotation blocks should remain stationary without respawning
        if (manualRot) {
            stillFrames = 0;
            lastPos = position;
            return;
        }

        // Failsafe: if movement is negligible for multiple frames, respawn a fresh block
        if (stuckGrace > 0) {
            stuckGrace--;
            lastPos = position;
        } else {
            fx32 dx = Math::abs(position.x - lastPos.x);
            fx32 dy = Math::abs(position.y - lastPos.y);
            if (dx < minMoveThreshold && dy < minMoveThreshold) {
                if (++stillFrames >= stuckLimit) {
                    // Spawn a replacement inside the arena near the player
                    if (!zone) zone = StageZone::get(0, &zoneRect);
                    // Random replacement spawn on the inset edges of the zone
                    fx32 leftX   = zoneRect.x + 8fx;
                    fx32 rightX  = zoneRect.x + zoneRect.width - 8fx;
                    fx32 topY2   = zoneRect.y - 8fx;
                    fx32 botY2   = zoneRect.y - zoneRect.height + 8fx;

                    Vec3 spawnPos = position;
                    fx32 rangeX = rightX - leftX;
                    fx32 rangeY = topY2 - botY2;
                    u32 edge = scast<u32>(Net::getRandom() % 4);
                    switch (edge) {
                        case 0: // top edge
                            spawnPos.y = topY2;
                            spawnPos.x = leftX + scast<fx32>(Net::getRandom() % scast<u32>(rangeX));
                            break;
                        case 1: // bottom edge
                            spawnPos.y = botY2;
                            spawnPos.x = leftX + scast<fx32>(Net::getRandom() % scast<u32>(rangeX));
                            break;
                        case 2: // left edge
                            spawnPos.x = leftX;
                            spawnPos.y = botY2 + scast<fx32>(Net::getRandom() % scast<u32>(rangeY));
                            break;
                        default: // right edge
                            spawnPos.x = rightX;
                            spawnPos.y = botY2 + scast<fx32>(Net::getRandom() % scast<u32>(rangeY));
                            break;
                    }

                    s32 settings = 0x1; // fromBackground
                    settings |= scast<s32>((Net::getRandom() % 4) & 0xFF) << 8; // random pattern
                    Actor::spawnActor(280, settings, &spawnPos);

                    Base::destroy();
                    return;
                }
            } else {
                stillFrames = 0;
                lastPos = position;
            }
        }
    }
}

void BlockProjectile::despawn(){
    if (updateStep == Func::Init) {
        despawnTimer = despawnFrames;
        collider.unlink();
        activeCollider.unlink();
        updateStep = 1;
        return;
    }

    if (updateStep == 1){
        scale.lerp(VecFx32(0.0fx, 0.0fx), 0.3fx);
        if (despawnTimer > 0) despawnTimer--;
        if (despawnTimer == 0 || scale.x <= 0.05fx) {
            Base::destroy();
        }
    }
}

void BlockProjectile::returnToBoss(){
    if (updateStep == Func::Init) {
        // Ensure we're fully visible and moving fast enough
        scale = Vec3(1.0fx, 1.0fx, 1.0fx);
        oamPriority = 0;
        returnTimer = returnDuration;
        updateStep = 1;
        return;
    }

    if (updateStep == 1){
        // If boss is gone, go back to normal bouncing
        SpookyBoss* boss = SpookyBoss::instance;
        if (!boss) {
            reflected = false;
            settings &= ~SettingsReflected;
            switchState(&BlockProjectile::bouncing);
            return;
        }

        Vec3 target = boss->position;
        target.y += 24fx;
        setVelocityToward(*this, target, returnSpeed);
        applyMovement();

        // Keep inside arena bounds
        if (!zone) zone = StageZone::get(0, &zoneRect);
        fx32 minX = zoneRect.x + 8fx;
        fx32 maxX = zoneRect.x + zoneRect.width - 8fx;
        fx32 topY = zoneRect.y - 8fx;
        fx32 botY = zoneRect.y - zoneRect.height + 8fx;
        if (position.x < minX) position.x = minX;
        if (position.x > maxX) position.x = maxX;
        if (position.y > topY) position.y = topY;
        if (position.y < botY) position.y = botY;

        collider.updatePosition();

        if (returnTimer > 0) {
            returnTimer--;
        } else {
            // Failed to reach boss in time; allow it to be re-bumped
            reflected = false;
            settings &= ~SettingsReflected;
            switchState(&BlockProjectile::bouncing);
        }
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

    if (self.spikedVariant) {
        return;
    }
}

void BlockProjectile::hitFromBottom(StageActor& _self, StageActor& other)
{
	BlockProjectile& self = scast<BlockProjectile&>(_self);

	//If not hit by player or if being hit already
	if (other.actorType != ActorType::Player)
		return;

    if (self.spikedVariant) {
        return;
    }

    if (!self.reflected) {
        self.reflected = true;
        self.settings |= SettingsReflected;
        self.switchState(&BlockProjectile::returnToBoss);
        Log() << "Hit from bottom (reflected)";
    }
}

void BlockProjectile::hitFromSide(StageActor& _self, StageActor& other)
{
	BlockProjectile& self = scast<BlockProjectile&>(_self);

	if (bool(other.collisionMgr.sideSensor->flags & CollisionMgr::SensorFlags::ActivateQuestionBlocks))
	{
		Log() << "Hit from side";
	}
}
