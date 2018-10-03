//
// 2017-11-19, jjuiddong
// �׷� �̵�
//
#pragma once


namespace ai
{

	template<class T>
	class cGroupMove : public cAction<T>
		, public common::cMemoryPool<cGroupMove<T>>
	{
	public:
		cGroupMove(T *agent, const vector<Vector3> &path
			, const Vector3 offset
			, const float speed = 3.f);

		virtual bool StartAction() override;
		virtual bool ActionExecute(const float deltaSeconds) override;

		Vector3 UnitCollisionAction(const Vector3 &curPos, const Vector3 &nextPos
			, const graphic::cBoundingSphere &bsphere
			, const graphic::cBoundingSphere &collSphere
			, const graphic::cNode *collisionNode
		);

		Vector3 WallCollisionAction(const Vector3 &curPos, const Vector3 &nextPos
			, const graphic::cBoundingSphere &bsphere
			, const graphic::cBoundingPlane &bplane
			, const float deltaSeconds
		);

		bool NextMove(const int idx);


	public:
		Vector3 m_offset;
		Vector3 m_dest;
		Vector3 m_prvPos;
		float m_speed; // default=3.f
		float m_rotateInterval; // ȸ�� ���� �ð�, 0.3��
		float m_collisionInterval; // �浹�� �Ͼ ��, �������� ��ȸ�ϱ��� ���ð�, 0.3f
		float m_distance;
		float m_oldDistance;
		Vector3 m_dir; // �̵� ����
		Quaternion m_fromDir; // �̵� �� ����
		Quaternion m_toDir; // �̵� �� ����
		float m_rotateTime; // ȸ�� ���� �ð�.
		bool m_isWaiting;
		int m_idx;
		vector<Vector3> m_path;
	};


	template<class T>
	cGroupMove<T>::cGroupMove(T *agent, const vector<Vector3> &path
		, const Vector3 offset
		, const float speed // =3.f
	)
		: cAction<T>(agent, "move", "zealot_walk.ani", eActionType::GROUP_MOVE)
		, m_offset(offset)
		, m_rotateTime(0)
		, m_speed(speed)
		, m_rotateInterval(0.3f)
		, m_collisionInterval(-1.f)
		, m_distance(0)
		, m_idx(0)
		, m_isWaiting(false)
	{
		m_path = path;
		m_rotateTime = 0;
	}


	// StartAction
	template<class T>
	bool cGroupMove<T>::StartAction()
	{
		m_idx = 0;
		NextMove(0);
		m_agent->SetAnimation("Walk");
		return true;
	}


	//-----------------------------------------------------------------------
	// Action Execute
	//-----------------------------------------------------------------------
	template<class T>
	bool cGroupMove<T>::ActionExecute(const float deltaSeconds)
	{
		if ((int)m_path.size() <= m_idx)
		{
			m_agent->SetAnimation("Stand");
			return false;
		}

		const Vector3 curPos = m_agent->m_transform.pos;
		const Vector3 dest = m_dest;

		// �������� �����ٸ�, ���� ���� �̵�
		// �������� ���� ��, ����Ǵ� ������ ���� �� �ִ�.
		const float distance = curPos.LengthRoughly(dest);
		//if (curPos.LengthRoughly(dest) < 0.01f)
		if (curPos.LengthRoughly(dest) <= m_agent->m_boundingSphere.GetRadius())
		{
			++m_idx;
			NextMove(m_idx);
			return true; // ������ ����. �׼�����.
		}

		// rotation interpolation
		if (m_rotateTime < m_rotateInterval)
		{
			m_rotateTime += deltaSeconds;

			const float alpha = min(1, m_rotateTime / m_rotateInterval);
			const Quaternion q = m_fromDir.Interpolate(m_toDir, alpha);
			m_agent->m_transform.rot = q;
		}

		// character move
		Vector3 pos = curPos + m_dir * m_speed * deltaSeconds;

		// Collision Test
		graphic::cBoundingSphere bsphere = m_agent->m_boundingSphere * m_agent->m_transform;
		graphic::cBoundingSphere collSphere;
		graphic::cNode *collisionNode = m_agent->m_collisionWall;

		graphic::cBoundingPlane bplane;
		const bool isWallCollision = g_global.IsCollisionWall(bsphere, bplane);
		if (!isWallCollision)
			collisionNode = g_global.IsCollisionUnit(m_agent, bsphere, collSphere);

		const bool isMoveCollision = collisionNode ? true : false;
		if (isMoveCollision)
		{
			pos = UnitCollisionAction(curPos, pos, bsphere, collSphere, collisionNode);
		}
		else if (isWallCollision)
		{
			pos = WallCollisionAction(curPos, pos, bsphere, bplane, deltaSeconds);
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
					m_fromDir = m_agent->m_transform.rot;
					m_toDir = q;
					m_rotateTime = 0;
					//dbg::Log("dot = %f\n", dot);
				}

				m_dir = newDir;
			}
		}

		m_agent->m_transform.pos = pos;
		m_agent->m_dir = m_dir;
		m_agent->m_nextPos = m_dest;

		m_oldDistance = distance;
		return true;
	}


	//---------------------------------------------------------------------------------------
	// UnitCollision
	// ���ְ� �浹���� ���
	//---------------------------------------------------------------------------------------
	template<class T>
	Vector3 cGroupMove<T>::UnitCollisionAction(
		const Vector3 &curPos
		, const Vector3 &nextPos
		, const graphic::cBoundingSphere &bsphere
		, const graphic::cBoundingSphere &collSphere
		, const graphic::cNode *collisionNode
	)
	{
		Vector3 newPos = nextPos;

		Vector3 opponentDir;
		if (const cZealot *zealot = dynamic_cast<const cZealot*>(collisionNode))
			if (zealot->m_brain->IsCurrentAction(eActionType::GROUP_MOVE))
				if (cGroupMove *movAction = dynamic_cast<cGroupMove*>(zealot->m_brain->GetAction()))
					opponentDir = movAction->m_dir;

		Vector3 toMe = (nextPos - collSphere.GetPos()).Normal();
		Vector3 toDest = (m_dest - collSphere.GetPos()).Normal();

		bool isWait = false;

		// �ٸ� �̵� ��ü�� �浹 �ߴٸ�, 
		// ������ �̵� ����� ���ٸ�, ��ȸ���� �ʰ�, ��� ��ٸ���.
		// ����, ������ �ڽź��� �տ� �ְ�, ���� �������� �̵� ���̶��
		if ((toMe.DotProduct(opponentDir) < 0.f) // �ڽ��� ������, ������ �����Ѵٸ�,
			&& (m_dir.DotProduct(opponentDir) > 0.f)) // ���� �������� ���� �ִٸ�
		{
			// ��� ����ߴٰ� �����Ѵ�.
			m_collisionInterval = 0.3f;
			isWait = true;
			newPos = curPos; // ���� ��ġ�� ����
		}

		m_isWaiting = isWait;

		// ��� ����� ��� �̰ų�,
		// �浹 ��ü�� �־����� ��Ȳ�� ���, ������ ��ȸ���� �ʴ´�.
		if (!toMe.IsEmpty() && (isWait || (toMe.DotProduct(toDest) >= 0)))
		{
			// Nothing~
		}
		else
		{
			// �浹 ��ü�� ��������� ��Ȳ�� ���
			// �浹 ��ü�� ���� ���� ���� ��������, ������ ư��.
			Vector3 newDir = Vector3(0, 1, 0).CrossProduct(toMe).Normal();
			if (newDir.DotProduct(m_dir) < 0)
				newDir = -newDir;

			Quaternion q;
			q.SetRotationArc(Vector3(0, 0, -1), newDir);
			m_fromDir = m_agent->m_transform.rot;
			m_toDir = q;

			m_rotateTime = 0;
			m_collisionInterval = 0.3f;
			m_dir = newDir;
			newPos = collSphere.GetPos() + toMe * (collSphere.GetRadius() + bsphere.GetRadius());
			//dbg::Log("collision \n");
		}

		return newPos;
	}


	//---------------------------------------------------------------------------------------
	// WallCollision
	// ���� �浹���� ���
	//---------------------------------------------------------------------------------------
	template<class T>
	Vector3 cGroupMove<T>::WallCollisionAction(const Vector3 &curPos, const Vector3 &nextPos
		, const graphic::cBoundingSphere &bsphere
		, const graphic::cBoundingPlane &bplane
		, const float deltaSeconds
	)
	{
		// ���� �浹�ϸ� 8�������� �ֺ��� �˻��� ��,
		// �̵� ��ο� ���� ����� �������� ������ ��ȯ�Ѵ�.
		Vector3 dirs[8];
		dirs[0] = bplane.Normal();
		dirs[2] = Vector3(0, 1, 0).CrossProduct(bplane.Normal()).Normal();
		dirs[1] = (dirs[2] + dirs[0]).Normal();
		dirs[4] = -dirs[0];
		dirs[3] = (dirs[2] + dirs[4]).Normal();
		dirs[6] = -dirs[2];
		dirs[5] = (dirs[6] + dirs[4]).Normal();
		dirs[7] = (dirs[6] + dirs[0]).Normal();

		int dirIdx = - 1;
		float minLen = FLT_MAX;
		for (int i = 0; i < 8; ++i)
		{
			const Ray ray(curPos + Vector3(0,1,0), dirs[i]);
			float len = FLT_MAX;
			g_global.IsCollisionByRay(ray, NULL, NULL, &len);
			if (len < bsphere.GetRadius())
				continue;

			const Vector3 pos = curPos + dirs[i] * bsphere.GetRadius();
			const float dist = pos.Distance(m_dest);
			if (dist < minLen)
			{
				dirIdx = i;
				minLen = dist;
			}
		}

		if (dirIdx < 0)
		{
			//assert(0);
			return nextPos;
		}

		// �浹 ��ü�� ��������� ��Ȳ�� ���
		// �浹 ��ü�� ���� ���� ���� ��������, ������ ư��.
		Vector3 newDir = dirs[dirIdx];
		Vector3 collisionPos = curPos;
		bplane.Collision(bsphere, &collisionPos);	
		collisionPos.y = curPos.y;
		if (collisionPos != curPos)
		{
			//collisionPos = collisionPos - (m_dir * bsphere.GetRadius() * 1.1f);
			collisionPos = collisionPos + (bplane.Normal() * bsphere.GetRadius() * 0.1f);
		}

		Quaternion q;
		q.SetRotationArc(Vector3(0, 0, -1), newDir);
		m_fromDir = m_agent->m_transform.rot;
		m_toDir = q;

		m_rotateTime = 0;
		m_collisionInterval = 0.3f;
		m_dir = newDir;
		Vector3 newPos = collisionPos + newDir * m_speed * deltaSeconds;

		static float sLen = 0;
		if (sLen < newPos.Distance(curPos))
		{
			sLen = newPos.Distance(curPos);
		}

		//newPos = collSphere.GetPos() + toMe * (collSphere.GetRadius() + bsphere.GetRadius());

		//graphic::cBoundingPlane bplane;
		//const bool isWallCollision = m_agent->aiCollisionWall(bplane);
		//if (!isWallCollision)
		//	collisionNode = m_agent->aiCollision(bsphere, collSphere);

		return newPos;
	}


	//---------------------------------------------------------------------------------------
	// NextMove
	//---------------------------------------------------------------------------------------
	template<class T>
	bool cGroupMove<T>::NextMove(const int idx)
	{
		if ((int)m_path.size() <= idx)
			return false;

		const Vector3 curPos = m_agent->m_transform.pos;
		const Vector3 dest = m_path[idx] + m_offset;

		m_dir = dest - curPos;
		m_dir.y = 0;
		m_dir.Normalize();

		Quaternion q;
		q.SetRotationArc(Vector3(0, 0, -1), m_dir);

		m_fromDir = m_agent->m_transform.rot;
		m_toDir = q;
		m_rotateTime = 0;
		m_dest = dest;
		m_prvPos = curPos;
		return true;
	}

}
