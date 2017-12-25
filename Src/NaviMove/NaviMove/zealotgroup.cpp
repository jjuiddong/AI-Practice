
#include "stdafx.h"
#include "zealotgroup.h"
#include "move2.h"
#include "groupmove.h"

using namespace graphic;

//----------------------------------------------------------------
cGroup::cGroup() 
	: ai::iActorInterface<cGroup>(this)
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

	ai::cNavigationMesh &navi = ((cViewer*)g_application)->m_navi;
	vector<Vector3> path;
	vector<int> nodePath;
	if (!navi.Find(center, dest, path, nodePath))
	{
		assert(0);
		return;
	}

	for (auto &p : m_children.m_Seq)
	{
		cActor<cZealot> *actor = dynamic_cast<cActor<cZealot>*>(p);
		const Vector3 offset = actor->m_agent->aiGetTransform().pos - center;
		actor->SetAction(new ai::cGroupMove<cZealot>(actor->m_agent, path, offset));
	}

	m_agent->m_ptr->m_nodePath = nodePath;

	// path line
	graphic::cDbgLineList &lineList = ((cViewer*)g_application)->m_pathLineList;
	graphic::cRenderer &renderer = ((cViewer*)g_application)->m_renderer;
	lineList.ClearLines();
	for (int i = 0; i < (int)path.size() - 1; ++i)
	{
		const Vector3 p0(path[i].x, 0.5f, path[i].z);
		const Vector3 p1(path[i + 1].x, 0.5f, path[i + 1].z);
		lineList.AddLine(renderer, p0, p1, false);
	}
	lineList.UpdateBuffer(renderer);

	// find wall collision plane
	vector<cBoundingPlane> &wallPlanes = ((cViewer*)g_application)->m_wallPlanes;
	wallPlanes.clear();

	for (auto idx : nodePath)
	{
		const ai::cNavigationMesh::sNaviNode &node = navi.m_naviNodes[idx];
		const int idxs[] = { node.idx1, node.idx2, node.idx3 };
		for (int i = 0; i < 3; ++i)
		{
			if (node.adjacent[i] >= 0)
				continue;

			// no adjacent edge, we need collision plane
			const Vector3 offsetY(0, 10, 0);
			const Vector3 p1 = navi.m_vertices[idxs[i]];
			const Vector3 p2 = navi.m_vertices[idxs[(i + 1) % 3]];
			const Vector3 p3 = p1 + offsetY;
			const Vector3 p4 = p2 + offsetY;

			cBoundingPlane bplane(p3, p4, p2, p1);
			wallPlanes.push_back(bplane);
		}
	}
}
