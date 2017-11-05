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
			, m_collisionInterval(-1.f)
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


		virtual bool StartAction() override {
			const Vector3 curPos = m_agent->aiGetTransform().pos;
			const float distance = curPos.Distance(m_dest);
			if (distance < 0.1f)
				return false;

			m_agent->aiSetAnimation("Walk");
			return true;
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
			Vector3 pos = curPos + m_dir * m_speed * deltaSeconds;

			// �浹 üũ
			{
				graphic::cBoundingSphere bsphere = m_agent->m_ptr->m_boundingSphere * m_agent->aiGetTransform();
				graphic::cBoundingSphere out;
				if (m_agent->aiCollision(bsphere, out))
				{
					Vector3 toPlayer = (pos - out.GetPos()).Normal();
					Vector3 toDest = (m_dest - out.GetPos()).Normal();

					// �浹 ��ü�� �־����� ��Ȳ�� ���, ������ ��ȸ���� �ʴ´�.
					if (toPlayer.DotProduct(toDest) >= 0)
					{
						// Nothing~
					}
					else
					{
						// �浹 ��ü�� ��������� ��Ȳ�� ���
						// �浹 ��ü�� ���� ���� ���� ��������, ������ ư��.
						Vector3 newDir = Vector3(0, 1, 0).CrossProduct(toPlayer).Normal();
						if (newDir.DotProduct(m_dir) < 0)
							newDir = -newDir;

						Quaternion q;
						q.SetRotationArc(Vector3(0, 0, -1), newDir);
						m_fromDir = m_agent->aiGetTransform().rot;
						m_toDir = q;

						m_rotateTime = 0;
						m_collisionInterval = 0.3f;
						m_dir = newDir;
						pos = out.GetPos() + toPlayer * (out.GetRadius() + bsphere.GetRadius());
						//dbg::Log("collision \n");
					}
				}
				else
				{ // No Collision
					m_collisionInterval -= deltaSeconds;
					if (m_collisionInterval < 0)
					{
						Vector3 newDir = m_dest - curPos;
						newDir.y = 0;
						newDir.Normalize();

						const float dot = newDir.DotProduct(m_dir);
						if (dot < 0.99f)
						{
							Quaternion q;
							q.SetRotationArc(Vector3(0, 0, -1), newDir);
							m_fromDir = m_agent->aiGetTransform().rot;
							m_toDir = q;
							m_rotateTime = 0;
							//dbg::Log("dot = %f\n", dot);
						}

						m_dir = newDir;
					}
				}
			}

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
			//if (m_oldDistance < distance)
			//{
			//	m_agent->aiSetAnimation("Stand");
			//	return false;
			//}

			m_oldDistance = distance;
			return true;
		}


	public:
		Vector3 m_dest;
		float m_speed; // 3.f
		float m_rotateInterval; // ȸ�� ���� �ð�, 0.3��
		float m_collisionInterval; // �浹�� �Ͼ ��, �������� ��ȸ�ϱ��� ���ð�, 0.3f
		float m_distance;
		float m_oldDistance;

		Vector3 m_dir; // �̵� ����
		Quaternion m_fromDir; // �̵� �� ����
		Quaternion m_toDir; // �̵� �� ����
		float m_rotateTime; // ȸ�� ���� �ð�.
	};

}
