//
// 2018-10-15, jjuiddong
// Group Formation Move
//
#pragma once

#include "formation.h"

namespace ai
{

	template<class T>
	class cFormationMove : public cAction<T>
		, public common::cMemoryPool<cFormationMove<T>>
	{
	public:
		cFormationMove(T *agent, const Vector3 &dest);

		virtual bool StartAction() override;
		virtual bool ActionExecute(const float deltaSeconds) override;


	public:
		using cAction<T>::m_agent; // base template class member access
		Vector3 m_dest;
		cFormation m_formation;
		float m_incT;
		bool m_isClose;
	};


	template<class T>
	cFormationMove<T>::cFormationMove(T *agent, const Vector3 &dest)
		: cAction<T>(agent, "formation move", "", eActionType::GROUP_MOVE)
		, m_dest(dest)
		, m_incT(0)
		, m_isClose(false)
	{
	}


	template<class T>
	bool cFormationMove<T>::StartAction()
	{
		// build formation
		m_formation.Create(m_dest, g_ai.m_select);

		// move action
		ai::cNavigationMesh &navi = g_ai.m_navi;
		vector<Vector3> path;
		vector<int> nodePath;
		for (auto &kv : m_formation.m_units)
		{
			cZealot *zealot = kv.second.unit;
			const Vector3 &dest = m_formation.m_pos + kv.second.pos;

			path.clear();
			nodePath.clear();
			if (navi.Find(zealot->m_transform.pos, dest, path, nodePath))
				zealot->m_brain->SetAction(new ai::cUnitMove<cZealot>(zealot, path, Vector3(0, 0, 0)));
		}

		return true;
	}


	template<class T>
	bool cFormationMove<T>::ActionExecute(const float deltaSeconds)
	{
		RETV(m_isClose, false);

		m_incT += deltaSeconds;
		if (m_incT < 0.01f)
			return true;
		const float dt = m_incT;
		m_incT = 0;

		// formation이 어느정도 잡혔다면, 이동한다.
		int cnt = 0;
		for (auto &kv : m_formation.m_units)
		{
			cZealot *zealot = kv.second.unit;
			if (!zealot->m_brain->IsAction(eActionType::GROUP_MOVE))
			{
				++cnt;
			}
		}

		if (m_formation.m_units.size() != cnt)
			return true;

		m_isClose = true;
		this->PushAction(new ai::cGroupMove<cGroup>(m_agent, m_dest));

		return true;
	}

}
