
#include "stdafx.h"
#include "zealotgroup.h"
#include "groupmove.h"
#include "formation.h"
#include "formationmove.h"

using namespace graphic;


//----------------------------------------------------------------
cGroup::cGroup() 
{
	m_brain = new cZealotGroupBrain(this);
}

cGroup::~cGroup() 
{
	SAFE_DELETE(m_brain);
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
	ClearAction();
	PushAction(new ai::cGroupMove<cGroup>(m_agent, dest));

	//Vector3 center;
	//for (auto &p : m_children.m_Seq)
	//{
	//	cBrain<cZealot> *actor = dynamic_cast<cBrain<cZealot>*>(p);
	//	center += actor->m_agent->m_transform.pos;
	//}
	//center /= m_children.size();

	//ai::cNavigationMesh &navi = g_ai.m_navi;

	//vector<Vector3> path;
	//vector<int> nodePath;
	//for (auto &p : m_children.m_Seq)
	//{
	//	cBrain<cZealot> *actor = dynamic_cast<cBrain<cZealot>*>(p);
	//	if (!actor->m_agent->m_isSelect)
	//		continue;

	//	path.clear();
	//	nodePath.clear();
	//	if (navi.Find(actor->m_agent->m_transform.pos, dest, path, nodePath))
	//		actor->SetAction(new ai::cUnitMove<cZealot>(actor->m_agent, path, Vector3(0,0,0)));
	//}

	//m_agent->m_nodePath = nodePath;
}


void cZealotGroupBrain::FormationMove(const Vector3 &dest)
{
	ClearAction();
	PushAction(new ai::cFormationMove<cGroup>(m_agent, dest));
}
