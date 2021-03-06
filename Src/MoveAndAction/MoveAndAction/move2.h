//
// 2017-10-20, jjuiddong
// 이동 행동.
//
#pragma once


namespace ai
{

	template<class T>
	class cMove2 : public cAction<T>
		, public common::cMemoryPool< cMove2<T> >
	{
	public:
		cMove2(T *agent, const Vector3 &dest, const float speed = 3.f)
			: cAction<T>(agent, "move", "zealot_walk.ani", eActionType::MOVE)
			, m_rotateTime(0)
			, m_speed(speed)
			, m_rotateInterval(0.3f)
			, m_distance(0)
		{
			m_dest = dest;
			m_rotateTime = 0;

			const Vector3 curPos = agent->m_transform.pos;
			m_distance = curPos.LengthRoughly(m_dest);
			m_oldDistance = curPos.LengthRoughly(m_dest);
			m_dir = m_dest - curPos;
			m_dir.y = 0;
			m_dir.Normalize();

			Quaternion q;
			q.SetRotationArc(Vector3(0, 0, -1), m_dir);

			m_fromDir = agent->m_transform.rot;
			m_toDir = q;
			m_rotateTime = 0;

			m_agent->m_transform.rot = q;
		}


		virtual bool StartAction() override {
			m_agent->SetAnimation("Walk");
			return true;
		}


		virtual bool ActionExecute(const float deltaSeconds) override
		{
			const Vector3 curPos = m_agent->m_transform.pos;

			// 회전 보간 계산
			if (m_rotateTime < m_rotateInterval)
			{
				m_rotateTime += deltaSeconds;

				const float alpha = min(1, m_rotateTime / m_rotateInterval);
				const Quaternion q = m_fromDir.Interpolate(m_toDir, alpha);
				m_agent->m_transform.rot = q;
			}

			// 캐릭터 이동.
			const Vector3 pos = curPos + m_dir * m_speed * deltaSeconds;
			m_agent->m_transform.pos = pos;

			// 목적지에 가깝다면 종료.
			// 프레임이 낮을 때, 통과되는 문제가 있을 수 있다.
			const float distance = pos.LengthRoughly(m_dest);
			if (pos.LengthRoughly(m_dest) < 0.01f)
			{
				m_agent->SetAnimation("Stand");
				return false; // 목적지 도착. 액션종료.
			}

			// 도착점 보다 멀리 왔다면, 멈춘다.
			if (m_oldDistance < distance)
			{
				m_agent->SetAnimation("Stand");
				return false;
			}

			m_oldDistance = distance;
			return true;
		}


	public:
		Vector3 m_dest;
		float m_speed; // 3.f
		float m_rotateInterval; // 회전 보간 시간, 0.3초
		float m_distance;
		float m_oldDistance;

		Vector3 m_dir; // 이동 방향
		Quaternion m_fromDir; // 이동 전 방향
		Quaternion m_toDir; // 이동 후 방향
		float m_rotateTime; // 회전 중인 시간.
	};

}
