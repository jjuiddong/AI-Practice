
#include "stdafx.h"
#include "aiglobal.h"
#include "zealot.h"

ai::cAiGlobal g_ai; // global variable

using namespace graphic;
using namespace ai;


cAiGlobal::cAiGlobal()
{
}

cAiGlobal::~cAiGlobal()
{
}


// node : 충돌 체크할 유닛 Node
// srcBSphere : 충돌 체크할 agent bounding Sphere
// out : 충돌 된 유닛으 Bounding Sphere를 리턴한다.
cNode* cAiGlobal::IsCollisionUnit(
	const cNode *node
	, const cBoundingSphere &srcBSphere
	, OUT cBoundingSphere &out)
{
	const vector<cZealot*> &zealots = m_zealots;

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


bool cAiGlobal::IsCollisionWall(
	const cBoundingSphere &bsphere
	, OUT cBoundingPlane &out)
{
	int cnt = 0;
	cBoundingPlane nearBPlane[2]; // most near 2 bplane
	Vector3 collisionPos[2];

	float mostNearLen1 = FLT_MAX;
	float mostNearLen2 = FLT_MAX;

	set<int> nodeIndices;
	m_navi.GetNodesFromPosition(bsphere.GetPos(), nodeIndices);
	for (auto &nodeIdx : nodeIndices)
	{
		auto it = m_navi.m_wallMap.find(nodeIdx);
		if (m_navi.m_wallMap.end() == it)
			continue;

		const auto &wallIndices = it->second;
		for (u_int i = 0; i < wallIndices.size(); ++i)
		{
			const auto &bplane = m_navi.m_walls[wallIndices[i]].bplane;
			float distance = FLT_MAX;
			Vector3 pos;
			if (bplane.Collision(bsphere, &pos, &distance))
			{
				// for collision debugging
				m_navi.m_walls[wallIndices[i]].collision = true;

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

	return false;
}


// wall, unit 충돌체크 후, 결과정보를 리턴한다.
// return type : 충돌 시 true
bool cAiGlobal::IsCollision(const cNode *srcNode
	, const cBoundingSphere &srcBSphere
	, OUT sCollisionResult &out)
{
	cBoundingSphere bsphere;
	cNode *colNode = IsCollisionUnit(srcNode, srcBSphere, bsphere);
	const bool col1 = (colNode) ? true : false;

	cBoundingPlane bplane;
	const bool col2 = IsCollisionWall(srcBSphere, bplane);

	if (!col1 && !col2)
		return false;

	if (col1 && !col2)
	{
		out.type = 1;
		out.bsphere = bsphere;
		out.node = colNode;
	}
	else if (col2 && !col1)
	{
		out.type = 2;
		out.bplane = bplane;
	}
	else // col1 && col2
	{
		// 벽과 유닛에 동시에 충돌
		out.type = 3;
		out.bplane = bplane;
		out.bsphere = bsphere;
		out.node = colNode;
	}

	return true;
}


// IsCollisionByRay
// return 0: no collision
//		  1: collision unit
//		  2: collision plane
int cAiGlobal::IsCollisionByRay(const Ray &ray
	, const cNode *srcNode
	, const float radius
	, OUT sCollisionResult &out
)
{
	// Check Unit
	int mostNearIdx1 = -1;
	float mostNearLen1 = FLT_MAX;
	const vector<cZealot*> &zealots = m_zealots;
	for (u_int i=0; i < zealots.size(); ++i)
	{
		auto &zealot = zealots[i];
		if (zealot == srcNode)
			continue;

		cBoundingSphere bsphere = zealot->m_boundingSphere * zealot->m_transform;
		float distance = FLT_MAX;
		if (bsphere.Pick(ray, &distance))
		{
			if (mostNearLen1 > distance)
			{
				mostNearIdx1 = i;
				mostNearLen1 = distance;
			}
		}
	}

	// Check Wall
	set<int> nodeIndices;
	m_navi.GetNodesFromPosition(ray.orig, nodeIndices);

	int mostNearIdx2 = -1;
	float mostNearLen2 = FLT_MAX;
	for (auto &nodeIdx : nodeIndices)
	{
		auto it = m_navi.m_wallMap.find(nodeIdx);
		if (m_navi.m_wallMap.end() == it)
			continue;

		const auto &wallIndices = it->second;
		for (u_int i=0; i < wallIndices.size(); ++i)
		{
			const auto &bplane = m_navi.m_walls[wallIndices[i]].bplane;// wallPlanes[i];
			float distance = FLT_MAX;
			if (bplane.Intersect(ray, radius, &distance))
			{
				if (mostNearLen2 > distance)
				{
					mostNearIdx2 = i;
					mostNearLen2 = distance;
				}
			}
		}
	}

	int type = 0;
	if ((mostNearIdx1 >= 0) && (mostNearIdx2 < 0))
		type = 1;
	else if ((mostNearIdx2 >= 0) && (mostNearIdx1 < 0))
		type = 2;
	else if (((mostNearIdx2 >= 0) && (mostNearIdx1 >= 0)) && (mostNearLen1 < mostNearLen2))
		type = 1;
	else if (((mostNearIdx2 >= 0) && (mostNearIdx1 >= 0)) && (mostNearLen1 > mostNearLen2))
		type = 2;

	if (1 == type)
	{
		auto &zealot = zealots[mostNearIdx1];
		out.type = 1;
		out.bsphere = zealot->m_boundingSphere * zealot->m_transform;
		// 모델 위치로 리턴한다. (SphereBox 중점은 모델위치와 약간 다르다)
		out.bsphere.SetPos(zealot->m_transform.pos);
		out.node = zealot;
		out.distance = mostNearLen1;
	}
	else if (2 == type)
	{
		out.type = 2;
		out.bplane = m_navi.m_walls[mostNearIdx2].bplane;
		out.distance = mostNearLen2;
	}
	else
	{
		out.type = 0;
		out.distance = FLT_MAX;
	}

	return type;
}
