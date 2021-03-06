//
// 2018-10-15, jjuiddong
// Zealot Group Move
//
#pragma once


namespace ai
{

	template<class T>
	class cGroupMove : public cAction<T>
		, public common::cMemoryPool<cGroupMove<T>>
	{
	public:
		cGroupMove(T *agent, const Vector3 &dest) 
			: cAction<T>(agent, "group move")
			, m_dest(dest)
		{
			int a = 0;
		}

		virtual bool StartAction() override;
		virtual bool ActionExecute(const float deltaSeconds) override;


	public:
		using cAction<T>::m_agent;
		Vector3 m_dest;
	};


	template<class T>
	bool cGroupMove<T>::StartAction()
	{
		//Vector3 center;
		//for (auto &p : m_agent->m_brain->m_children.m_Seq)
		//{
		//	cBrain<cZealot> *brain = dynamic_cast<cBrain<cZealot>*>(p);
		//	center += brain->m_agent->m_transform.pos;
		//}
		//center /= m_agent->m_brain->m_children.size();

		ai::cNavigationMesh &navi = g_ai.m_navi;

		vector<Vector3> path;
		for (auto &p : m_agent->m_brain->m_children.m_Seq)
		{
			cBrain<cZealot> *brain = dynamic_cast<cBrain<cZealot>*>(p);
			if (!brain->m_agent->m_isSelect)
				continue;

			path.clear();
			if (navi.Find(brain->m_agent->m_transform.pos, m_dest, path))
			{
				brain->SetAction(new ai::cUnitAction<cZealot>(brain->m_agent));
				brain->PushAction(new ai::cUnitMove2<cZealot>(brain->m_agent, path, Vector3(0, 0, 0)));
			}
		}

		return false; // action finish
	}


	template<class T>
	bool cGroupMove<T>::ActionExecute(const float deltaSeconds)
	{
		return false; // action finish
	}

}
