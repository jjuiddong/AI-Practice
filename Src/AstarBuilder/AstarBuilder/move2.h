//
// 2017-10-20, jjuiddong
// �̵� �ൿ.
//
#pragma once


namespace ai
{

	template<class T>
	class cMove2 : public cAction<T>
		, public common::cMemoryPool<cMove2<T>>
	{
	public:
		cMove2(T *agent, const Vector3 &dest, const float speed = 3.f)
			: cAction<T>(agent, "move", "zealot_walk.ani", eActionType::MOVE)
			, m_rotateTime(0)
			, m_speed(speed)
			, m_rotateInterval(0.3f)
			, m_collisionInterval(-1.f)
			, m_distance(0)
			, m_idx(0)
		{
			m_dest = dest;
			m_rotateTime = 0;
		}


		virtual bool StartAction() override {
			const Vector3 curPos = m_agent->m_transform.pos;
			if (!g_pathFinder.Find(curPos, m_dest, m_path))
				return false;

			g_route = m_path;

			m_idx = 0;
			NextMove(0);
			m_agent->SetAnimation("Walk");
			return true;
		}


		virtual bool ActionExecute(const float deltaSeconds) override
		{
			if ((int)m_path.size() <= m_idx)
			{
				m_agent->SetAnimation("Stand");
				return false;
			}

			const Vector3 curPos = m_agent->m_transform.pos;
			const Vector3 dest = m_path[m_idx];

			// �������� �����ٸ�, ���� ���� �̵�
			// �������� ���� ��, ����Ǵ� ������ ���� �� �ִ�.
			const float distance = curPos.LengthRoughly(dest);
			if (curPos.LengthRoughly(dest) < 0.01f)
			{
				++m_idx;
				NextMove(m_idx);
				return true; // ������ ����. �׼�����.
			}

			// ȸ�� ���� ���
			if (m_rotateTime < m_rotateInterval)
			{
				m_rotateTime += deltaSeconds;

				const float alpha = min(1, m_rotateTime / m_rotateInterval);
				const Quaternion q = m_fromDir.Interpolate(m_toDir, alpha);
				m_agent->m_transform.rot = q;
			}

			const Vector3 pos = curPos + m_dir * m_speed * deltaSeconds;
			m_agent->m_transform.pos = pos;


			//// ĳ���� �̵�.
			//Vector3 pos = curPos + m_dir * m_speed * deltaSeconds;

			//// �浹 üũ
			//{
			//	graphic::cBoundingSphere bsphere = m_agent->m_boundingSphere * m_agent->m_transform;
			//	graphic::cBoundingSphere out;
			//	if (m_agent->Collision(bsphere, out))
			//	{
			//		Vector3 toPlayer = (pos - out.GetPos()).Normal();
			//		Vector3 toDest = (m_dest - out.GetPos()).Normal();

			//		// �浹 ��ü�� �־����� ��Ȳ�� ���, ������ ��ȸ���� �ʴ´�.
			//		if (toPlayer.DotProduct(toDest) >= 0)
			//		{
			//			// Nothing~
			//		}
			//		else
			//		{
			//			// �浹 ��ü�� ��������� ��Ȳ�� ���
			//			// �浹 ��ü�� ���� ���� ���� ��������, ������ ư��.
			//			Vector3 newDir = Vector3(0, 1, 0).CrossProduct(toPlayer).Normal();
			//			if (newDir.DotProduct(m_dir) < 0)
			//				newDir = -newDir;

			//			Quaternion q;
			//			q.SetRotationArc(Vector3(0, 0, -1), newDir);
			//			m_fromDir = m_agent->m_transform.rot;
			//			m_toDir = q;

			//			m_rotateTime = 0;
			//			m_collisionInterval = 0.3f;
			//			m_dir = newDir;
			//			pos = out.GetPos() + toPlayer * (out.GetRadius() + bsphere.GetRadius());
			//			//dbg::Log("collision \n");
			//		}
			//	}
			//	else
			//	{ // No Collision
			//		m_collisionInterval -= deltaSeconds;
			//		if (m_collisionInterval < 0)
			//		{
			//			Vector3 newDir = m_dest - curPos;
			//			newDir.y = 0;
			//			newDir.Normalize();

			//			const float dot = newDir.DotProduct(m_dir);
			//			if (dot < 0.99f)
			//			{
			//				Quaternion q;
			//				q.SetRotationArc(Vector3(0, 0, -1), newDir);
			//				m_fromDir = m_agent->m_transform.rot;
			//				m_toDir = q;
			//				m_rotateTime = 0;
			//				//dbg::Log("dot = %f\n", dot);
			//			}

			//			m_dir = newDir;
			//		}
			//	}
			//}

			//m_agent->m_transform.pos = pos;

			//// �������� �����ٸ� ����.
			//// �������� ���� ��, ����Ǵ� ������ ���� �� �ִ�.
			//const float distance = pos.LengthRoughly(m_dest);
			//if (pos.LengthRoughly(m_dest) < 0.01f)
			//{
			//	m_agent->SetAnimation("Stand");
			//	return false; // ������ ����. �׼�����.
			//}

			//// ������ ���� �ָ� �Դٸ�, �����.
			////if (m_oldDistance < distance)
			////{
			////	m_agent->SetAnimation("Stand");
			////	return false;
			////}

			//m_oldDistance = distance;
			return true;
		}

		void NextMove(const int idx)
		{
			const Vector3 curPos = m_agent->m_transform.pos;
			const Vector3 dest = m_path[idx];

			m_dir = dest - curPos;
			m_dir.y = 0;
			m_dir.Normalize();

			Quaternion q;
			q.SetRotationArc(Vector3(0, 0, -1), m_dir);

			m_fromDir = m_agent->m_transform.rot;
			m_toDir = q;
			m_rotateTime = 0;
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

		int m_idx;
		vector<Vector3> m_path;
	};

}
