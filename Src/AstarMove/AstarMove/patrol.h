//
// 2017-11-05, jjuiddong
// ¼øÂû Çàµ¿.
//
#pragma once

#include "move2.h"


namespace ai
{

	template<class T>
	class cPatrol : public cAction<T>
		, public common::cMemoryPool< cPatrol<T> >
	{
	public:
		cPatrol(ai::iActorInterface<T> *agent, const Vector3 &dest, const float speed = 3.f)
			: cAction<T>(agent, "patrol", "", eActionType::PATROL)
		{
			m_patrolPos = 0;
			m_dest1 = dest;
			m_dest2 = m_agent->aiGetTransform().pos;
		}


		virtual bool StartAction() override {
			PushAction(new cMove2<T>(m_agent, m_dest1));
			return true;
		}


		virtual bool ActionExecute(const float deltaSeconds) override
		{
			PushAction(new cMove2<T>(m_agent, (m_patrolPos==0)? m_dest2 : m_dest1));
			m_patrolPos = (m_patrolPos == 0) ? 1 : 0;
			return true;
		}


	public:
		int m_patrolPos; // 0:m_dest1, 1:m_dest2
		Vector3 m_dest1;
		Vector3 m_dest2;
	};

}
