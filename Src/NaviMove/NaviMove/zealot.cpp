
#include "stdafx.h"
#include "zealot.h"
#include "zealotai.h"

using namespace graphic;


cZealot::cZealot()
	: ai::iActorInterface<cZealot>(this)
	, m_ai(NULL)
	, m_isLoaded(false)
	, m_collisionWall(NULL)
{
}

cZealot::~cZealot()
{
	SAFE_DELETE(m_ai);
}


bool cZealot::Create(graphic::cRenderer &renderer)
{
	RETV2(!__super::Create(renderer, common::GenerateId(), "zealot.fbx"), false);
	SetAnimation("Stand");

	m_ai = new cZealotAI(this);	
	RETV2(!m_ai->Init(), false);

	return true;
}


void cZealot::InitModel(cRenderer &renderer)
{
	__super::InitModel(renderer);

	m_isLoaded = true;
	*(Vector3*)&m_boundingBox.m_bbox.Extents *= 0.5f;
	m_boundingSphere.SetRadius(m_boundingSphere.GetRadius()*0.5f);
}


void cZealot::aiSetAnimation(const Str64 &animationName)
{
	SetAnimation(animationName);
}


bool cZealot::Render(cRenderer &renderer
	, const XMMATRIX &parentTm //= XMIdentity
	, const int flags //= 1
)
{
	__super::Render(renderer, parentTm, flags);
	return true;
}


bool cZealot::Update(cRenderer &renderer, const float deltaSeconds)
{
	__super::Update(renderer, deltaSeconds);
	m_ai->Update(deltaSeconds);
	return true;
}


cNode* cZealot::aiCollision(const cBoundingSphere &srcBSphere
	, OUT cBoundingSphere &collisionSphrere)
{
	vector<cZealot*> &zealots = ((cViewer*)g_application)->m_zealots;

	for (auto &zealot : zealots)
	{
		if (zealot == this)
			continue;

		cBoundingSphere bsphere = zealot->m_boundingSphere * zealot->m_transform;
		if (bsphere.Intersects(srcBSphere))
		{
			collisionSphrere = zealot->m_boundingSphere * zealot->m_transform;
			// �� ��ġ�� �����Ѵ�. (SphereBox ������ ����ġ�� �ణ �ٸ���)
			collisionSphrere.SetPos(zealot->m_transform.pos);
			return zealot;
		}
	}

	return NULL;
}


bool cZealot::aiCollisionWall(OUT graphic::cBoundingPlane &out)
{
	vector<cBoundingPlane> &wallPlanes = ((cViewer*)g_application)->m_wallPlanes;

	cBoundingSphere bsphere = m_boundingSphere * m_transform;

	int mostNearIdx = -1;
	float mostNearLen = FLT_MAX;
	for (u_int i=0; i < wallPlanes.size(); ++i)
	{
		auto &bplane = wallPlanes[i];
		
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
