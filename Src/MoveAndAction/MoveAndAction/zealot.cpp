
#include "stdafx.h"
#include "zealot.h"
#include "zealotai.h"

using namespace graphic;


cZealot::cZealot()
	: ai::iActorInterface<cZealot>(this)
	, m_ai(NULL)
{
}

cZealot::~cZealot()
{
	SAFE_DELETE(m_ai);
}


bool cZealot::Create(graphic::cRenderer &renderer)
{
	RETV2(!__super::Create(renderer, common::GenerateId(), "zealot2.fbx"), false);

	m_ai = new cZealotAI(this);	
	RETV2(!m_ai->Init(), false);

	return true;
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
