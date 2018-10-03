
#include "stdafx.h"
#include "global.h"
#include "zealot.h"

cGlobal g_global; // global variable

using namespace graphic;


cGlobal::cGlobal()
	: m_main(NULL)
{
}

cGlobal::~cGlobal()
{
}


cNode* cGlobal::IsCollisionUnit(
	const cNode *node
	, const cBoundingSphere &srcBSphere
	, OUT cBoundingSphere &out)
{
	const vector<cZealot*> &zealots = m_main->m_zealots;

	for (auto &zealot : zealots)
	{
		if (zealot == node)
			continue;

		cBoundingSphere bsphere = zealot->m_boundingSphere * zealot->m_transform;
		if (bsphere.Intersects(srcBSphere))
		{
			out = zealot->m_boundingSphere * zealot->m_transform;
			// 모델 위치로 리턴한다. (SphereBox 중점은 모델위치와 약간 다르다)
			out.SetPos(zealot->m_transform.pos);
			return zealot;
		}
	}

	return NULL;
}


bool cGlobal::IsCollisionWall(
	const cBoundingSphere &bsphere
	, OUT cBoundingPlane &out)
{
	const vector<cBoundingPlane> &wallPlanes = m_main->m_wallPlanes;

	int mostNearIdx = -1;
	float mostNearLen = FLT_MAX;
	for (u_int i = 0; i < wallPlanes.size(); ++i)
	{
		const auto &bplane = wallPlanes[i];
		float distance = 0;
		if (bplane.Collision(bsphere, NULL, &distance))
		{
			if (mostNearLen > distance)
			{
				mostNearIdx = i;
				mostNearLen = distance;
			}
		}
	}

	if (mostNearIdx >= 0)
		out = wallPlanes[mostNearIdx];

	return (mostNearIdx >= 0);
}


// IsCollisionByRay
// return 0: no collision
//		  1: collision unit
//		  2: collision plane
int cGlobal::IsCollisionByRay(const Ray &ray
	, OUT graphic::cBoundingSphere *outSphere //= NULL
	, OUT graphic::cBoundingPlane *outPlane //= NULL
	, OUT float *outDistance //= NULL
)
{
	const vector<cBoundingPlane> &wallPlanes = m_main->m_wallPlanes;

	int mostNearIdx = -1;
	float mostNearLen = FLT_MAX;
	for (u_int i = 0; i < wallPlanes.size(); ++i)
	{
		const auto &bplane = wallPlanes[i];
		float distance = 0;
		if (bplane.Pick(ray, &distance))
		{
			if (mostNearLen > distance)
			{
				mostNearIdx = i;
				mostNearLen = distance;
			}
		}
	}

	if (mostNearIdx >= 0)
	{
		if (outPlane)
			*outPlane = wallPlanes[mostNearIdx];
		if (outDistance)
			*outDistance = mostNearLen;
		return 2;
	}

	return 0;
}
