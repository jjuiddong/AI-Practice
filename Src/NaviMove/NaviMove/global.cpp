
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
	int cnt = 0;
	cBoundingPlane nearBPlane[2]; // most near 2 bplane
	Vector3 collisionPos[2];

	float mostNearLen1 = FLT_MAX;
	float mostNearLen2 = FLT_MAX;
	for (u_int i = 0; i < wallPlanes.size(); ++i)
	{
		const auto &bplane = wallPlanes[i];
		float distance = FLT_MAX;
		Vector3 pos;
		if (bplane.Collision(bsphere, &pos, &distance))
		{
			if (mostNearLen1 > distance)
			{
				if (cnt >= 1)
				{
					cnt = 2;
					mostNearLen2 = mostNearLen1;
					nearBPlane[1] = nearBPlane[0];
					collisionPos[1] = collisionPos[0];
				}
				else
				{
					cnt = 1;
				}

				mostNearLen1 = distance;
				nearBPlane[0] = bplane;
				collisionPos[0] = pos;
			}
			else if (mostNearLen2 > distance)
			{
				cnt = 2;
				mostNearLen2 = distance;
				nearBPlane[1] = bplane;
				collisionPos[1] = pos;
			}
		}
	}

	// 두 개의 bplane과 충돌한다면, 겹치는 두 충돌 위치가 가깝다면, 
	// 마주하는 두 bplane에 충돌한 것으로 가정한다.
	if (cnt == 2)
	{
		if (collisionPos[0].Distance(collisionPos[1]) < 0.1f)
		{
			Vector3 center = (collisionPos[0] + collisionPos[1]) / 2.f;
			center.y = 0;
			const Vector3 norm = (nearBPlane[0].Normal() + nearBPlane[1].Normal()).Normal();

			// Normal = Vector3(0,0,-1)
			Vector3 vtx[4] = {
				Vector3(-1,2,0)
				, Vector3(1,2,0)
				, Vector3(1,0,0)
				, Vector3(-1,0,0)
			};
			
			Quaternion q;
			q.SetRotationArc(Vector3(0, 0, -1), norm);
			for (int i = 0; i < 4; ++i)
				vtx[i] = center + (vtx[i] * q);

			out.SetVertex(vtx[0], vtx[1], vtx[2], vtx[3]);
			return true;
		}
		
		out = nearBPlane[0];
		return true;
	}
	else if (cnt == 1)
	{
		out = nearBPlane[0];
		return true;
	}

	//if (mostNearIdx >= 0)
	//	out = wallPlanes[mostNearIdx];
	//return (mostNearIdx >= 0);
	return false;
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
