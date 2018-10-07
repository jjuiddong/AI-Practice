
#include "stdafx.h"
#include "zealot.h"
#include "zealotbrain.h"

using namespace graphic;


cZealot::cZealot()
	: m_brain(NULL)
	, m_isLoaded(false)
	, m_collisionWall(NULL)
{
}

cZealot::~cZealot()
{
	SAFE_DELETE(m_brain);
}


bool cZealot::Create(graphic::cRenderer &renderer)
{
	RETV2(!__super::Create(renderer, common::GenerateId(), "zealot.fbx"), false);
	SetAnimation("Stand");

	m_brain = new cZealotBrain(this);
	RETV2(!m_brain->Init(), false);

	return true;
}


void cZealot::InitModel(cRenderer &renderer)
{
	__super::InitModel(renderer);

	m_isLoaded = true;
	*(Vector3*)&m_boundingBox.m_bbox.Extents *= 0.5f;
	m_boundingSphere.SetRadius(m_boundingSphere.GetRadius()*0.5f);
}


bool cZealot::Render(cRenderer &renderer
	, const XMMATRIX &parentTm //= XMIdentity
	, const int flags //= 1
)
{
	__super::Render(renderer, parentTm, flags);

	if (1)
	{
		Transform tfm = m_transform;
		tfm.scale = Vector3(1, 1, 1) * 0.2f;
		renderer.m_textMgr.AddTextRender(renderer, m_id, m_name.wstr().c_str()
			, cColor::WHITE, cColor::BLACK, BILLBOARD_TYPE::ALL_AXIS
			, tfm, true);
	}

	return true;
}


bool cZealot::Update(cRenderer &renderer, const float deltaSeconds)
{
	__super::Update(renderer, deltaSeconds);
	m_brain->Update(deltaSeconds);
	return true;
}
