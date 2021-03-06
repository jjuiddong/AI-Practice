
#include "stdafx.h"
#include "zealot.h"
#include "zealotai.h"
//#include "groupmove.h"

using namespace graphic;


cZealot::cZealot()
	: m_ai(NULL)
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

	m_ai = new cZealotBrain(this);	
	RETV2(!m_ai->Init(), false);

	return true;
}


void cZealot::InitModel(cRenderer &renderer)
{
	__super::InitModel(renderer);

	m_boundingSphere.SetRadius(m_boundingSphere.GetRadius()*0.5f);
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


bool cZealot::Collision(const cBoundingSphere &srcBSphere
	, OUT cBoundingSphere &collisionSphrere)
{
	
	//for (auto &zealot : ((cViewer*)g_application)->m_zealots)
	//{
	//	if (zealot == this)
	//		continue;

	//	cBoundingSphere bsphere = zealot->m_boundingSphere * zealot->m_transform;
	//	if (bsphere.Intersects(srcBSphere))
	//	{
	//		collisionSphrere = zealot->m_boundingSphere * zealot->m_transform;
	//		collisionSphrere.SetPos(zealot->m_transform.pos); // 모델 위치로 리턴한다. (SphereBox 중점은 모델위치와 약간 다르다)
	//		return true;
	//	}
	//}

	return false;
}
