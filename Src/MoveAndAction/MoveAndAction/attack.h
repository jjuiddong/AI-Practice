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
		cAttack(T *agent, const Vector3 &target)
			: cAction<T>(agent, "attack", "Attack", eActionType::ATTACK)
		{
			m_target = target;

			const Vector3 curPos = agent->m_transform.pos;
			Vector3 dir = target - curPos;
			Quaternion q;
			q.SetRotationArc(Vector3(0, 0, -1), dir);
			agent->m_transform.rot = q;
		}

		virtual bool StartAction() override {
			m_agent->m_aniIncT = 0.001f;
			m_agent->SetAnimation("Attack");
			return true;
		}

		virtual bool ActionExecute(const float deltaSeconds) override
		{
			// 애니메이션이 끝나면, 원래 상태로 복귀
			if (m_agent->m_aniIncT < 0.001f)
			{
				m_agent->SetAnimation("Stand");
				return false;
			}

			return true;
		}


	public:
		Vector3 m_target;
	};

}
