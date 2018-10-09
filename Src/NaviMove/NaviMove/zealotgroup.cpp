
#include "stdafx.h"
#include "zealotgroup.h"
#include "move2.h"
#include "groupmove.h"

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
	Vector3 center;
	for (auto &p : m_children.m_Seq)
	{
		cBrain<cZealot> *actor = dynamic_cast<cBrain<cZealot>*>(p);
		center += actor->m_agent->m_transform.pos;
	}
	center /= m_children.size();

	ai::cNavigationMesh &navi = ((cViewer*)g_application)->m_navi;

	//vector<Vector3> path;
	//vector<int> nodePath;
	//if (!navi.Find(center, dest, path, nodePath))
	//{
	//	assert(0);
	//	return;
	//}

	//for (auto &p : m_children.m_Seq)
	//{
	//	cBrain<cZealot> *actor = dynamic_cast<cBrain<cZealot>*>(p);
	//	const Vector3 offset = actor->m_agent->m_transform.pos - center;
	//	actor->SetAction(new ai::cGroupMove<cZealot>(actor->m_agent, path, offset));
	//}

	vector<Vector3> path;
	vector<int> nodePath;
	for (auto &p : m_children.m_Seq)
	{
		cBrain<cZealot> *actor = dynamic_cast<cBrain<cZealot>*>(p);
		if (!actor->m_agent->m_isSelect)
			continue;

		path.clear();
		nodePath.clear();
		if (navi.Find(actor->m_agent->m_transform.pos, dest, path, nodePath))
			actor->SetAction(new ai::cGroupMove<cZealot>(actor->m_agent, path, Vector3(0,0,0)));
	}

	m_agent->m_nodePath = nodePath;

	// path line
	//graphic::cDbgLineList &lineList = ((cViewer*)g_application)->m_pathLineList;
	//graphic::cRenderer &renderer = ((cViewer*)g_application)->m_renderer;
	//lineList.ClearLines();
	//for (int i = 0; i < (int)path.size() - 1; ++i)
	//{
	//	const Vector3 p0(path[i].x, 0.5f, path[i].z);
	//	const Vector3 p1(path[i + 1].x, 0.5f, path[i + 1].z);
	//	lineList.AddLine(renderer, p0, p1, false);
	//}
	//lineList.UpdateBuffer(renderer);

	// find wall collision plane
	//vector<cBoundingPlane> &wallPlanes = ((cViewer*)g_application)->m_wallPlanes;
	//wallPlanes.clear();

	// 지나가는 경로에 벽이 있으면, 충돌체크를 위해서 미리 저장해둔다.
	set<int> wallNodes;
	for (auto i : nodePath)
	{
		const ai::cNavigationMesh::sNaviNode &node = navi.m_naviNodes[i];
		const int idxs[] = { node.idx1, node.idx2, node.idx3 };
		for (int k=0; k < 3; ++k)
			navi.GetNodesFromVertexIdx(idxs[k], wallNodes);
	}
	//navi.GetWallPlane(wallNodes, wallPlanes);
}
