//
// 2017-10-20, jjuiddong
// �̵� �ൿ.
//
#pragma once


namespace ai
{

	template<class T>
	class cMove2 : public cAction<T>
		, public common::cMemoryPool< cMove2<T> >
	{
	public:
		cMove2(ai::iActorInterface<T> *agent, const Vector3 &dest, const float speed = 3.f)
			: cAction<T>(agent, "move", "zealot_walk.ani", ACTION_TYPE::MOVE)
			, m_rotateTime(0)
			, m_speed(speed)
			, m_rotateInterval(0.3f)
			, m_distance(0)
		{
			m_dest = dest;
			m_rotateTime = 0;

			const Vector3 curPos = agent->aiGetTransform().pos;
			m_distance = curPos.LengthRoughly(m_dest);
			m_oldDistance = curPos.LengthRoughly(m_dest);
			m_dir = m_dest - curPos;
			m_dir.y = 0;
			m_dir.Normalize();

			Quaternion q;
			q.SetRotationArc(Vector3(0, 0, -1), m_dir);

			m_fromDir = agent->aiGetTransform().rot;
			m_toDir = q;
			m_rotateTime = 0;

			m_agent->aiGetTransform().rot = q;
		}


		virtual void StartAction() override {
			m_agent->aiSetAnimation("Walk");
		}


		virtual bool ActionExecute(const float deltaSeconds) override
		{
			const Vector3 curPos = m_agent->aiGetTransform().pos;

			// ȸ�� ���� ���
			if (m_rotateTime < m_rotateInterval)
			{
				m_rotateTime += deltaSeconds;

				const float alpha = min(1, m_rotateTime / m_rotateInterval);
				const Quaternion q = m_fromDir.Interpolate(m_toDir, alpha);
				m_agent->aiGetTransform().rot = q;
			}

			// ĳ���� �̵�.
			const Vector3 pos = curPos + m_dir * m_speed * deltaSeconds;
			m_agent->aiGetTransform().pos = pos;

			// �������� �����ٸ� ����.
			// �������� ���� ��, ����Ǵ� ������ ���� �� �ִ�.
			const float distance = pos.LengthRoughly(m_dest);
			if (pos.LengthRoughly(m_dest) < 0.01f)
			{
				m_agent->aiSetAnimation("Stand");
				return false; // ������ ����. �׼�����.
			}

			// ������ ���� �ָ� �Դٸ�, �����.
			if (m_oldDistance < distance)
			{
				m_agent->aiSetAnimation("Stand");
				return false;
			}

			m_oldDistance = distance;
			return true;
		}


	public:
		Vector3 m_dest;
		float m_speed; // 3.f
		float m_rotateInterval; // ȸ�� ���� �ð�, 0.3��
		float m_distance;
		float m_oldDistance;

		Vector3 m_dir; // �̵� ����
		Quaternion m_fromDir; // �̵� �� ����
		Quaternion m_toDir; // �̵� �� ����
		float m_rotateTime; // ȸ�� ���� �ð�.
	};

}
