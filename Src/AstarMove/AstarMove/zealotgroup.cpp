
#include "stdafx.h"
#include "zealotgroup.h"
#include "move2.h"
#include "groupmove.h"


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

	vector<Vector3> path;
	if (!g_pathFinder.Find(center, dest, path))
	{
		assert(0);
		return;
	}

	g_route = path;

	for (auto &p : m_children.m_Seq)
	{
		cBrain<cZealot> *actor = dynamic_cast<cBrain<cZealot>*>(p);
		const Vector3 offset = actor->m_agent->m_transform.pos - center;
		actor->SetAction(new ai::cGroupMove<cZealot>(actor->m_agent, path, offset));
	}

}
