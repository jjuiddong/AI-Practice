
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
	vector<Vector3> path;
	vector<int> nodePath;
	if (!navi.Find(center, dest, path, nodePath))
	{
		assert(0);
		return;
	}

	for (auto &p : m_children.m_Seq)
	{
		cBrain<cZealot> *actor = dynamic_cast<cBrain<cZealot>*>(p);
		const Vector3 offset = actor->m_agent->m_transform.pos - center;
		actor->SetAction(new ai::cGroupMove<cZealot>(actor->m_agent, path, offset));
	}

	m_agent->m_nodePath = nodePath;

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


	// 첫 번째, 마지막 노드의 인접노드에 붙은 Wall을 추가한다.
	// 하나의 노드는 삼각형이기 때문에, 시작과 마지막에 Wall이 빠질 수도 있다.
	set<int> wallNodes;
	if (!nodePath.empty())
	{
		const ai::cNavigationMesh::sNaviNode &node0 = navi.m_naviNodes[nodePath[0]];
		wallNodes.insert(node0.adjacent[0]);
		wallNodes.insert(node0.adjacent[1]);
		wallNodes.insert(node0.adjacent[2]);

		if (nodePath.size() > 1)
		{
			const ai::cNavigationMesh::sNaviNode &node1 = navi.m_naviNodes[nodePath.back()];
			wallNodes.insert(node1.adjacent[0]);
			wallNodes.insert(node1.adjacent[1]);
			wallNodes.insert(node1.adjacent[2]);
		}

		for (auto idx : nodePath)
			wallNodes.insert(idx);
	}

	for (auto idx : wallNodes)
	{
		if (idx < 0)
			continue;

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
