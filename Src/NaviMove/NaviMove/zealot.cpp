
#include "stdafx.h"
#include "zealot.h"
#include "zealotbrain.h"

using namespace graphic;


cZealot::cZealot()
	: m_brain(NULL)
	, m_isLoaded(false)
	, m_isSelect(false)
	, m_collisionWall(NULL)
	, m_isCollisionTurn(false)
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
		// Render NextPos
		{
			Transform tfm = m_transform;
			tfm.pos = m_nextPos;
			tfm.scale = Vector3(1, 1, 1) * 0.2f;
			renderer.m_dbgBox.SetBox(tfm);
			renderer.m_dbgBox.SetColor(cColor::YELLOW);
			renderer.m_dbgBox.Render(renderer);
		}

		// Render Path
		renderer.m_dbgLine.SetColor(cColor::BLUE);
		for (int i = 0; i < (int)m_route.size() - 1; ++i)
		{
			renderer.m_dbgLine.SetLine(m_route[i], m_route[i + 1], 0.005f);
			renderer.m_dbgLine.Render(renderer);
		}

		// Render Directios
		if (m_isCollisionTurn)
		{
			for (auto &info : m_dirs)
			{
				renderer.m_dbgLine.SetColor(info.use? cColor::RED : cColor::WHITE);
				renderer.m_dbgLine.SetLine(m_transform.pos
					, m_transform.pos + info.dir * ((info.len >= FLT_MAX)? 10.f : info.len), 0.005f);
				renderer.m_dbgLine.Render(renderer);
			}
		}

		Transform tfm = m_transform;
		tfm.scale = Vector3(1, 1, 1) * 0.1f;
		renderer.m_textMgr.AddTextRender(renderer, m_id, m_name.wstr().c_str()
			, m_isSelect? cColor(0.8f,0.8f,0) : cColor::WHITE
			, cColor::BLACK
			, BILLBOARD_TYPE::ALL_AXIS
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
