//
// 2017-10-31, jjuiddong
// 이동 행동.
//
#pragma once


namespace ai
{

	template<class T>
	class cAttack : public cAction<T>
		, public common::cMemoryPool< cMove2<T> >
	{
	public:
		cAttack(ai::iActorInterface<T> *agent, const Vector3 &target)
			: cAction<T>(agent, "attack", "Attack", eActionType::ATTACK)
		{
			m_target = target;

			const Vector3 curPos = agent->aiGetTransform().pos;
			Vector3 dir = target - curPos;
			Quaternion q;
			q.SetRotationArc(Vector3(0, 0, -1), dir);
			agent->aiGetTransform().rot = q;
		}

		virtual void StartAction() override {
			m_agent->m_ptr->m_aniIncT = 0.001f;
			m_agent->aiSetAnimation("Attack");
		}

		virtual bool ActionExecute(const float deltaSeconds) override
		{
			// 애니메이션이 끝나면, 원래 상태로 복귀
			if (m_agent->m_ptr->m_aniIncT < 0.001f)
			{
				m_agent->aiSetAnimation("Stand");
				return false;
			}

			return true;
		}


	public:
		Vector3 m_target;
	};

}
