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
			, const graphic::cBoundingSphere &srcBSphere
			, const graphic::cBoundingSphere &collSphere
			, const graphic::cNode *collisionNode
		);

		Vector3 WallCollisionAction(const Vector3 &curPos, const Vector3 &nextPos
			, const graphic::cBoundingSphere &srcBSphere
			, const graphic::cBoundingPlane &bplane
			, const float deltaSeconds
		);

		bool CollisionAction(const Vector3 &curPos, const Vector3 &nextPos
			, const graphic::cBoundingSphere &srcBSphere
			, const cGlobal::sCollisionResult &result
			, const float deltaSeconds
			, OUT Vector3 &out
		);

		bool NextMove(const int idx);


	public:
		float m_incT;
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
		int m_waitCount; // �ʹ� �������� ����ϴ� ���� �����ϱ� ���� ����.
		int m_idx;
		vector<Vector3> m_path;
	};


	template<class T>
	cGroupMove<T>::cGroupMove(T *agent, const vector<Vector3> &path
		, const Vector3 offset
		, const float speed // =3.f
	)
		: cAction<T>(agent, "move", "zealot_walk.ani", eActionType::GROUP_MOVE)
		, m_incT(0)
		, m_offset(offset)
		, m_rotateTime(0)
		, m_speed(speed)
		, m_rotateInterval(0.3f)
		, m_collisionInterval(-1.f)
		, m_distance(0)
		, m_idx(0)
		, m_isWaiting(false)
		, m_waitCount(0)
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
		m_incT += deltaSeconds;
		if (m_incT < 0.01f)
			return true;
		const float dt = m_incT;
		m_incT = 0;
		
		// ��� ��, ����
		if ((int)m_path.size() <= m_idx)
		{
			m_agent->SetAnimation("Stand");
			return false;
		}

		const Vector3 curPos = m_agent->m_transform.pos;

		// �������� �����ٸ�, ���� ���� �̵�
		// �������� ���� ��, ����Ǵ� ������ ���� �� �ִ�.
		// Y�� �� ����
		const float distance = Vector3(curPos.x,0, curPos.z).LengthRoughly(
			Vector3(m_dest.x, 0, m_dest.z));
		//if (distance < 0.1f)
		if (distance <= m_agent->m_boundingSphere.GetRadius()*0.5f)
		{
			++m_idx;
			NextMove(m_idx);
			return true; // ������ ����. �׼�����.
		}

		// rotation interpolation
		if (m_rotateTime < m_rotateInterval)
		{
			m_rotateTime += dt;

			const float alpha = min(1, m_rotateTime / m_rotateInterval);
			const Quaternion q = m_fromDir.Interpolate(m_toDir, alpha);
			m_agent->m_transform.rot = q;
		}

		// move next pos
		Vector3 nextPos = curPos + m_dir * m_speed * dt;

		// Collision Test
		cGlobal::sCollisionResult colResult;
		graphic::cBoundingSphere bsphere = m_agent->m_boundingSphere * m_agent->m_transform;
		if (g_global.IsCollision(m_agent, bsphere, colResult))
		{
			if (!CollisionAction(curPos, nextPos, bsphere, colResult, dt, nextPos))
			{
				// ������ ����.
				m_agent->SetAnimation("Stand");
				return false;
			}
		}
		else
		{ // No Collision
			m_collisionInterval -= dt;
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

		m_agent->m_transform.pos = nextPos;
		m_agent->m_dir = m_dir;
		m_agent->m_nextPos = m_dest;

		m_oldDistance = distance;
		return true;
	}


	//---------------------------------------------------------------------------------------
	// UnitCollision
	// ���ְ� �浹���� ���
	// bsphere : Agent ������ bounding sphere
	// collSphere : �浹�� ������ bounding sphere
	// collisionNode : �浹�� ����
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

		Vector3 toMe = (curPos - collSphere.GetPos()).Normal();
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
			m_isWaiting = isWait;
			newPos = curPos; // ���� ��ġ�� ����
		}
		//if (!toMe.IsEmpty() && (isWait || (toMe.DotProduct(toDest) >= 0)))
		else if (!toMe.IsEmpty() && (toMe.DotProduct(m_dir) >= 0.f))
		{
			// �浹������, �ռ� ������ �ִ� ���, ��� ������ �����Ѵ�.
			m_isWaiting = false;
			newPos = nextPos;
			m_collisionInterval = 0.f;
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
			newPos.y = nextPos.y;
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
			collisionPos = collisionPos + (bplane.Normal() * bsphere.GetRadius());
			collisionPos.y = curPos.y;
		}

		Quaternion q;
		q.SetRotationArc(Vector3(0, 0, -1), newDir);
		m_fromDir = m_agent->m_transform.rot;
		m_toDir = q;

		m_rotateTime = 0;
		m_collisionInterval = 0.3f;
		m_dir = newDir;
		Vector3 newPos = collisionPos + newDir * m_speed * deltaSeconds;

		m_dest -= m_offset;
		m_offset = Vector3(0, 0, 0);

		return newPos;
	}


	//---------------------------------------------------------------------------------------
	// CollisionAction
	// return value = true: �浹 �׼� ó��
	//				  false: �̵� ����
	//---------------------------------------------------------------------------------------
	template<class T>
	bool cGroupMove<T>::CollisionAction(const Vector3 &curPos, const Vector3 &nextPos
		, const graphic::cBoundingSphere &srcBSphere
		, const cGlobal::sCollisionResult &result
		, const float deltaSeconds
		, OUT Vector3 &out
	)
	{
		Vector3 pos = curPos; // ���� ��ġ

		// �浹�� ��ü�� ���� �ٸ��� ó���Ѵ�.
		Vector3 forward;
		if (1 == result.type) // collision unit sphere
		{
			forward = (curPos - result.bsphere.GetPos()).Normal();
		}
		else if (2 == result.type) // collision wall plane
		{
			// ���� �浹�ϸ�, ��ġ�� �����Ѵ�.
			Vector3 collisionPos;
			result.bplane.Collision(srcBSphere, &collisionPos);
			pos = collisionPos + (result.bplane.Normal() * srcBSphere.GetRadius());
			pos.y = curPos.y;

			forward = result.bplane.Normal();
		}
		else if (3 == result.type) // collision unit and wall
		{
			// �浹�� ������ ��ġ�� ������ ��, ���ְ� �浹�� ������ ó���Ѵ�.
			Vector3 collisionPos;
			result.bplane.Collision(srcBSphere, &collisionPos);
			Vector3 pos = collisionPos + (result.bplane.Normal() * srcBSphere.GetRadius());
			pos.y = curPos.y;

			forward = (pos - result.bsphere.GetPos()).Normal();
		}

		forward.y = 0;
		forward.Normalize();

		// ���ְ� �浹 ��, �տ� �ִ� ������ ������ ���� ��� ����� ������ �Ǵ��Ѵ�.
		m_isWaiting = false;
		if ((1 == result.type) || (3 == result.type))
		{
			Vector3 opponentDir;
			if (const cZealot *zealot = dynamic_cast<const cZealot*>(result.node))
				if (zealot->m_brain->IsCurrentAction(eActionType::GROUP_MOVE))
					if (cGroupMove *movAction = dynamic_cast<cGroupMove*>(zealot->m_brain->GetAction()))
						opponentDir = (movAction->m_dest - movAction->m_agent->m_transform.pos).Normal();

			Vector3 toMe = (curPos - result.bsphere.GetPos()).Normal();
			Vector3 toDest = (m_dest - curPos).Normal();

			// �ٸ� �̵� ��ü�� �浹 �ߴٸ�, 
			// ������ �̵� ����� ���ٸ�, ��ȸ���� �ʰ�, ��� ��ٸ���.
			// ����, ������ �ڽź��� �տ� �ְ�, ���� �������� �̵� ���̶��
			if ((toMe.DotProduct(opponentDir) < -0.3f) // �ڽ��� ������, ������ �����Ѵٸ�,
				&& (toDest.DotProduct(opponentDir) > 0.3f)) // ���� �������� ���� �ִٸ�
			{
				// ��� ����ߴٰ� �����Ѵ�.
				m_collisionInterval = 0.3f;
				m_isWaiting = true;
				++m_waitCount; // �ʹ� �������� ����ϴ� ���� �����Ѵ�.

				if (m_waitCount < 10)
				{
					out = curPos; // ���� ��ġ�� ����, ����
					return true;
				}
			}
		}

		// ��� Ƚ�� �ʱ�ȭ
		m_waitCount = 0;

		// ��� ����ϴ� ���°� �ƴ϶��, m_dest�� ���� ���� ���� ���� ã�´�.
		// ��ü�� �浹�ϸ� 8�������� �ֺ��� �˻��� ��,
		// �̵� ��ο� ���� ����� �������� ��ȸ�Ѵ�.
		Vector3 dirs[8];
		dirs[0] = forward;
		dirs[2] = Vector3(0, 1, 0).CrossProduct(forward).Normal();
		dirs[1] = (dirs[2] + dirs[0]).Normal();
		dirs[4] = -dirs[0];
		dirs[3] = (dirs[2] + dirs[4]).Normal();
		dirs[6] = -dirs[2];
		dirs[5] = (dirs[6] + dirs[4]).Normal();
		dirs[7] = (dirs[6] + dirs[0]).Normal();

		int dirIdx = -1; // �̵��� ����
		int collisionType = 0; // 8���⿡�� ���õ� ������ �浹 Ÿ�� (unit or wall)
		graphic::cNode *collisionNode = NULL;
		graphic::cBoundingPlane collisionBPlane;
		graphic::cBoundingSphere collisionBSphere;
		float minLen = FLT_MAX;
		for (int i = 0; i < 8; ++i)
		{
			// 8 �������� �浹 üũ�Ѵ�.
			const Ray ray(pos + Vector3(0, srcBSphere.GetPos().y, 0), dirs[i]);
			float len = FLT_MAX;
			//graphic::cBoundingPlane bplane;
			//graphic::cBoundingSphere bsphere;
			cGlobal::sCollisionResult colResult;
			const int cResult = g_global.IsCollisionByRay(ray, m_agent, colResult);
			if (colResult.distance < (srcBSphere.GetRadius() * 1.3f))
				continue; // ��ֹ��� ������, �ش� ������ ����

			// �̵� ��ο� ���� ����� ������ �����Ѵ�.
			const Vector3 movPos = pos + dirs[i] * srcBSphere.GetRadius();
			const float dist = movPos.Distance(m_dest);
			if (dist < minLen)
			{
				dirIdx = i;
				minLen = dist;
				collisionType = cResult;
				collisionNode = colResult.node;
				if (1 == cResult)
					collisionBSphere = colResult.bsphere;
				else if (2 == cResult)
					collisionBPlane = colResult.bplane;
			}
		}

		if (dirIdx < 0)
		{
			//assert(0);
			out = pos;
			return true;
		}

		// �������� ���ֿ� ���� �����ְ�, ������ ������ ���¶��, �̵��� �����.
		if (1 == collisionType)
		{
			if (const cZealot *zealot = dynamic_cast<const cZealot*>(collisionNode))
				if (!zealot->m_brain->IsCurrentAction(eActionType::GROUP_MOVE))
					return false;
		}

		const Vector3 newDir = dirs[dirIdx];

		Quaternion q;
		q.SetRotationArc(Vector3(0, 0, -1), newDir);
		m_fromDir = m_agent->m_transform.rot;
		m_toDir = q;

		m_rotateTime = 0;
		m_collisionInterval = 0.3f;
		m_dir = newDir;
		m_dest -= m_offset;
		m_offset = Vector3(0, 0, 0);
		out = pos + newDir * m_speed * deltaSeconds;
		return true;
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
