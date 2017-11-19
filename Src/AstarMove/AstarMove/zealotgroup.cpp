
#include "stdafx.h"
#include "zealotgroup.h"
#include "move2.h"
#include "groupmove.h"


//----------------------------------------------------------------
cGroup::cGroup() :
	ai::iActorInterface<cGroup>(this)
{
	m_ai = new cZealotGroupAI(this);
}

cGroup::~cGroup() 
{
	SAFE_DELETE(m_ai);
}


//----------------------------------------------------------------
cZealotGroupAI::cZealotGroupAI(ai::iActorInterface<cGroup> *agent)
	: ai::cActor<cGroup>(agent)
{
}

cZealotGroupAI::~cZealotGroupAI()
{
}


void cZealotGroupAI::Move(const Vector3 &dest)
{
	Vector3 center;
	for (auto &p : m_children.m_Seq)
	{
		cActor<cZealot> *actor = dynamic_cast<cActor<cZealot>*>(p);
		center += actor->m_agent->aiGetTransform().pos;
	}
	center /= m_children.size();

	vector<Vector3> path;
	if (!g_pathFinder.Find(center, dest, path))
	{
		assert(0);
		return;
	}

	g_route = path;

	for (auto &p : m_children.m_Seq)
	{
		cActor<cZealot> *actor = dynamic_cast<cActor<cZealot>*>(p);
		const Vector3 offset = actor->m_agent->aiGetTransform().pos - center;
		actor->SetAction(new ai::cGroupMove<cZealot>(actor->m_agent, path, offset));
	}

}
