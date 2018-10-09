
#include "stdafx.h"
#include "zealotgroup.h"


//----------------------------------------------------------------
cGroup::cGroup()
{
	m_ai = new cZealotGroupBrain(this);
}

cGroup::~cGroup() 
{
	SAFE_DELETE(m_ai);
}


//----------------------------------------------------------------
cZealotGroupBrain::cZealotGroupBrain(cGroup *agent)
	: ai::cBrain<cGroup>(agent)
{
}

cZealotGroupBrain::~cZealotGroupBrain()
{
}


void cZealotGroupBrain::Move(const Vector3 &dest)
{
	Vector3 center;
	for (auto &p : m_children.m_Seq)
	{
		cBrain<cZealot> *actor = dynamic_cast<cBrain<cZealot>*>(p);
		center += actor->m_agent->m_transform.pos;
	}
	center /= m_children.size();

	for (auto &p : m_children.m_Seq)
	{
		cBrain<cZealot> *actor = dynamic_cast<cBrain<cZealot>*>(p);
		const Vector3 offset = actor->m_agent->m_transform.pos - center;
		actor->SetAction(new ai::cMove2<cZealot>(actor->m_agent, dest+offset*0.9f));
	}
}
