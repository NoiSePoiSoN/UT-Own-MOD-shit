/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#include "RTdef.h"
#if RT_COMPILE
#include "ActorBase.h"

#include <PxMat44.h>
#include "PxRigidBodyExt.h"

#include "SimSceneBase.h"
#include "CompoundBase.h"

namespace physx
{
namespace fracture
{
namespace base
{

Actor::Actor(SimScene* scene):
	mScene(scene),
	mMinConvexSize(scene->mMinConvexSize),
	mDepthLimit(100),
	mDestroyIfAtDepthLimit(false)
{

}

Actor::~Actor()
{
	PxScene* pxScene = mScene->getScene();
	if(pxScene != NULL)
	{
		pxScene->lockWrite();
	}
	clear();
	if(pxScene != NULL)
	{
		pxScene->unlockWrite();
	}
	mScene->removeActor(this);
}

void Actor::clear()
{
	for (physx::PxU32 i = 0; i < mCompounds.size(); i++) {
		PX_DELETE(mCompounds[i]);
	}
	mCompounds.clear();
}

void Actor::addCompound(Compound *c)
{
	mCompounds.pushBack(c);
	PxRigidDynamic *a = c->getPxActor();
    if (a) {
//		a->setContactReportFlags(Px_NOTIFY_ON_TOUCH_FORCE_THRESHOLD | Px_NOTIFY_ON_START_TOUCH_FORCE_THRESHOLD);
		a->setContactReportThreshold(mScene->mFractureForceThreshold);
    }
	c->mActor = this;
	++(mScene->mSceneVersion);
}

void Actor::removeCompound(Compound *c)
{
	physx::PxU32 num = 0;
	for (physx::PxU32 i = 0; i < mCompounds.size(); i++) {
		if (mCompounds[i] != c) {
			mCompounds[num] = mCompounds[i];
			num++;
		}
	}
	if (mScene->mPickActor == c->getPxActor())
		mScene->mPickActor = NULL;

	c->clear();
	//delCompoundList.push_back(c);
	//delete c;
	mScene->delCompoundList.pushBack(c);
	mCompounds.resize(num);
	++mScene->mSceneVersion;
}

void Actor::preSim(float dt)
{
	physx::PxU32 num = 0;
	for (physx::PxU32 i = 0; i < (physx::PxU32)mCompounds.size(); i++) {
		mCompounds[i]->step(dt);
		if (mCompounds[i]->getLifeFrames() == 0) {
			mCompounds[i]->clear();
			//delCompoundList.push_back(mCompounds[i]);
			//delete mCompounds[i];
			mScene->delCompoundList.pushBack(mCompounds[i]);
		}
		else {
			mCompounds[num] = mCompounds[i];
			num++;
		}
	}
	mCompounds.resize(num);
}

void Actor::postSim(float /*dt*/)
{
}

bool Actor::rayCast(const PxVec3 &orig, const PxVec3 &dir, float &dist, int &compoundNr, int &convexNr, PxVec3 &normal) const
{
	dist = PX_MAX_F32;
	compoundNr = -1;
	convexNr = -1;

	for (physx::PxU32 i = 0; i < mCompounds.size(); i++) {
		float d;
		int cNr;
		PxVec3 n;
		if (mCompounds[i]->rayCast(orig, dir, d, cNr, n)) {
			if (d < dist) {
				dist = d;
				compoundNr = (int)i;
				convexNr = cNr;
				normal = n;
			}
		}
	}
	return compoundNr >= 0;
}

bool Actor::patternFracture(const PxVec3 &orig, const PxVec3 &dir, const PxMat33 patternTransform, float impactRadius, float radialImpulse, float directionalImpulse)
{
	float dist;
	float objectSize = 0.0f;
	int actorNr;
	int compoundNr;
	int convexNr;
	PxVec3 normal; 

	// do global rayCast.
	if (!mScene->rayCast(orig, dir, dist, actorNr, compoundNr, convexNr, normal))
		return false;
	if (mScene->mActors[(physx::PxU32)actorNr] != this)
		return false;

	mScene->debugPoints.clear();
	shdfnd::Array<Compound*> compounds;

	mScene->profileBegin("patternFracture");
	bool OK = mCompounds[(physx::PxU32)compoundNr]->patternFracture(orig + dir * dist, mMinConvexSize, 
		compounds, patternTransform, mScene->debugPoints, impactRadius, radialImpulse, normal * directionalImpulse);
	mScene->profileEnd("patternFracture");
	if (!OK)
		return false;

	if (compounds.empty())
		return false;

	if (mCompounds[(physx::PxU32)compoundNr]->getPxActor() == mScene->mPickActor)
		mScene->mPickActor = NULL;

	PxBounds3 bounds;
	mCompounds[(physx::PxU32)compoundNr]->getWorldBounds(bounds);
	objectSize = bounds.getDimensions().magnitude();

	//delCompoundList.push_back( mCompounds[compoundNr]);
	mScene->delCompoundList.pushBack( mCompounds[(physx::PxU32)compoundNr]);
	mCompounds[(physx::PxU32)compoundNr]->clear();
	//delete mCompounds[compoundNr];
	
	mCompounds[(physx::PxU32)compoundNr] = compounds[0];

	for (physx::PxU32 i = 1; i < compounds.size(); i++)
		mCompounds.pushBack(compounds[i]);

	++mScene->mSceneVersion;

	
	// playShatterSound(objectSize);
	//if (fluidSim)
	//fluidSim->mistPS->seedDustParticles((PxVec3*)&debugPoints[0], debugPoints.size(), seedDustRadius, seedDustNumPerSite, dustMinLife, dustMaxLife, 0.0f, 1.0f);

	physx::PxU32 numConvexes = 0;
	for (physx::PxU32 i = 0; i < mCompounds.size(); i++)
		numConvexes += mCompounds[i]->getConvexes().size();

	//printf("\n------- pattern fracture------\n");
	//printf("i compounds, %i convexes after fracture\n", mCompounds.size(), numConvexes); 

	return true;
}

bool Actor::patternFracture(const PxVec3 &hitLocation, const PxVec3 &normal, const int &compoundNr, const PxMat33 patternTransform,  float impactRadius, float radialImpulse, float directionalImpulse)
{
	float objectSize = 0.0f;

	mScene->debugPoints.clear();
	shdfnd::Array<Compound*> compounds;

	mScene->profileBegin("patternFracture");
	bool OK = mCompounds[(physx::PxU32)compoundNr]->patternFracture(hitLocation, mMinConvexSize, 
		compounds, patternTransform, mScene->debugPoints, impactRadius, radialImpulse, normal * directionalImpulse);
	mScene->profileEnd("patternFracture");
	if (!OK)
		return false;

	if (compounds.empty())
		return false;

	if (mCompounds[(physx::PxU32)compoundNr]->getPxActor() == mScene->mPickActor)
		mScene->mPickActor = NULL;

	PxBounds3 bounds;
	mCompounds[(physx::PxU32)compoundNr]->getWorldBounds(bounds);
	objectSize = bounds.getDimensions().magnitude();

	//delCompoundList.push_back( mCompounds[compoundNr]);
	mScene->delCompoundList.pushBack( mCompounds[(physx::PxU32)compoundNr]);
	mCompounds[(physx::PxU32)compoundNr]->clear();
	//delete mCompounds[compoundNr];
	
	mCompounds[(physx::PxU32)compoundNr] = compounds[0];

	for (physx::PxU32 i = 1; i < compounds.size(); i++)
		mCompounds.pushBack(compounds[i]);

	++mScene->mSceneVersion;

	
	// playShatterSound(objectSize);
	//if (fluidSim)
	//fluidSim->mistPS->seedDustParticles((PxVec3*)&debugPoints[0], debugPoints.size(), seedDustRadius, seedDustNumPerSite, dustMinLife, dustMaxLife, 0.0f, 1.0f);

	physx::PxU32 numConvexes = 0;
	for (physx::PxU32 i = 0; i < mCompounds.size(); i++)
		numConvexes += mCompounds[i]->getConvexes().size();

	//printf("\n------- pattern fracture------\n");
	//printf("i compounds, %i convexes after fracture\n", mCompounds.size(), numConvexes); 

	return true;
}

bool Actor::findCompound(const Compound* c, int& compoundNr)
{
	for(physx::PxU32 i = 0; i < mCompounds.size(); i++)
	{
		if(mCompounds[i] == c)
		{
			compoundNr = (physx::PxI32)i;
			return true;
		}
	}
	return false;
}

}
}
}
#endif