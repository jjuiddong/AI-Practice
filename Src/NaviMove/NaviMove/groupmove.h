//
// 2017-11-19, jjuiddong
// 그룹 이동
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
		float m_rotateInterval; // 회전 보간 시간, 0.3초
		float m_collisionInterval; // 충돌이 일어난 후, 목적지로 선회하기전 대기시간, 0.3f
		float m_distance;
		float m_oldDistance;
		Vector3 m_dir; // 이동 방향
		Quaternion m_fromDir; // 이동 전 방향
		Quaternion m_toDir; // 이동 후 방향
		float m_rotateTime; // 회전 중인 시간.
		bool m_isWaiting;
		int m_waitCount; // 너무 오랫동안 대기하는 것을 방지하기 위한 변수.
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
		
		// 경로 끝, 종료
		if ((int)m_path.size() <= m_idx)
		{
			m_agent->SetAnimation("Stand");
			return false;
		}

		const Vector3 curPos = m_agent->m_transform.pos;

		// 목적지에 가깝다면, 다음 노드로 이동
		// 프레임이 낮을 때, 통과되는 문제가 있을 수 있다.
		// Y축 값 무시
		const float distance = Vector3(curPos.x,0, curPos.z).LengthRoughly(
			Vector3(m_dest.x, 0, m_dest.z));
		//if (distance < 0.1f)
		if (distance <= m_agent->m_boundingSphere.GetRadius()*0.5f)
		{
			++m_idx;
			NextMove(m_idx);
			return true; // 목적지 도착. 액션종료.
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
				// 목적지 도착.
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
	// 유닛과 충돌했을 경우
	// bsphere : Agent 유닛의 bounding sphere
	// collSphere : 충돌된 유닛의 bounding sphere
	// collisionNode : 충돌된 유닛
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

		// 다른 이동 객체와 충돌 했다면, 
		// 상대방의 이동 방향과 같다면, 선회하지 않고, 잠깐 기다린다.
		// 만약, 상대방이 자신보다 앞에 있고, 같은 방향으로 이동 중이라면
		if ((toMe.DotProduct(opponentDir) < 0.f) // 자신을 등지고, 앞으로 전진한다면,
			&& (m_dir.DotProduct(opponentDir) > 0.f)) // 같은 방향으로 가고 있다면
		{
			// 잠깐 대기했다가 전진한다.
			m_collisionInterval = 0.3f;
			isWait = true;
			m_isWaiting = isWait;
			newPos = curPos; // 현재 위치로 유지
		}
		//if (!toMe.IsEmpty() && (isWait || (toMe.DotProduct(toDest) >= 0)))
		else if (!toMe.IsEmpty() && (toMe.DotProduct(m_dir) >= 0.f))
		{
			// 충돌했지만, 앞서 나가고 있는 경우, 계속 앞으로 진행한다.
			m_isWaiting = false;
			newPos = nextPos;
			m_collisionInterval = 0.f;
		}
		else
		{
			// 충돌 객체와 가까워지는 상황일 경우
			// 충돌 객체의 원의 접선 벡터 방향으로, 방향을 튼다.
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
	// 벽과 충돌했을 경우
	//---------------------------------------------------------------------------------------
	template<class T>
	Vector3 cGroupMove<T>::WallCollisionAction(const Vector3 &curPos, const Vector3 &nextPos
		, const graphic::cBoundingSphere &bsphere
		, const graphic::cBoundingPlane &bplane
		, const float deltaSeconds
	)
	{
		// 벽과 충돌하면 8방향으로 주변을 검색한 후,
		// 이동 경로에 가장 가까운 방향으로 방향을 전환한다.
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

		// 충돌 객체와 가까워지는 상황일 경우
		// 충돌 객체의 원의 접선 벡터 방향으로, 방향을 튼다.
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
	// return value = true: 충돌 액션 처리
	//				  false: 이동 종료
	//---------------------------------------------------------------------------------------
	template<class T>
	bool cGroupMove<T>::CollisionAction(const Vector3 &curPos, const Vector3 &nextPos
		, const graphic::cBoundingSphere &srcBSphere
		, const cGlobal::sCollisionResult &result
		, const float deltaSeconds
		, OUT Vector3 &out
	)
	{
		Vector3 pos = curPos; // 현재 위치

		// 충돌한 객체에 따라 다르게 처리한다.
		Vector3 forward;
		if (1 == result.type) // collision unit sphere
		{
			forward = (curPos - result.bsphere.GetPos()).Normal();
		}
		else if (2 == result.type) // collision wall plane
		{
			// 벽과 충돌하면, 위치를 보정한다.
			Vector3 collisionPos;
			result.bplane.Collision(srcBSphere, &collisionPos);
			pos = collisionPos + (result.bplane.Normal() * srcBSphere.GetRadius());
			pos.y = curPos.y;

			forward = result.bplane.Normal();
		}
		else if (3 == result.type) // collision unit and wall
		{
			// 충돌한 벽에서 위치를 보정한 후, 유닛과 충돌한 것으로 처리한다.
			Vector3 collisionPos;
			result.bplane.Collision(srcBSphere, &collisionPos);
			Vector3 pos = collisionPos + (result.bplane.Normal() * srcBSphere.GetRadius());
			pos.y = curPos.y;

			forward = (pos - result.bsphere.GetPos()).Normal();
		}

		forward.y = 0;
		forward.Normalize();

		// 유닛과 충돌 시, 앞에 있는 유닛이 지나갈 동안 잠깐 대기할 것인지 판단한다.
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

			// 다른 이동 객체와 충돌 했다면, 
			// 상대방의 이동 방향과 같다면, 선회하지 않고, 잠깐 기다린다.
			// 만약, 상대방이 자신보다 앞에 있고, 같은 방향으로 이동 중이라면
			if ((toMe.DotProduct(opponentDir) < -0.3f) // 자신을 등지고, 앞으로 전진한다면,
				&& (toDest.DotProduct(opponentDir) > 0.3f)) // 같은 방향으로 가고 있다면
			{
				// 잠깐 대기했다가 전진한다.
				m_collisionInterval = 0.3f;
				m_isWaiting = true;
				++m_waitCount; // 너무 오랫동안 대기하는 것을 방지한다.

				if (m_waitCount < 10)
				{
					out = curPos; // 현재 위치로 유지, 종료
					return true;
				}
			}
		}

		// 대기 횟수 초기화
		m_waitCount = 0;

		// 잠깐 대기하는 상태가 아니라면, m_dest에 가는 가장 빠른 길을 찾는다.
		// 물체와 충돌하면 8방향으로 주변을 검색한 후,
		// 이동 경로에 가장 가까운 방향으로 선회한다.
		Vector3 dirs[8];
		dirs[0] = forward;
		dirs[2] = Vector3(0, 1, 0).CrossProduct(forward).Normal();
		dirs[1] = (dirs[2] + dirs[0]).Normal();
		dirs[4] = -dirs[0];
		dirs[3] = (dirs[2] + dirs[4]).Normal();
		dirs[6] = -dirs[2];
		dirs[5] = (dirs[6] + dirs[4]).Normal();
		dirs[7] = (dirs[6] + dirs[0]).Normal();

		int dirIdx = -1; // 이동할 방향
		int collisionType = 0; // 8방향에서 선택된 방향의 충돌 타입 (unit or wall)
		graphic::cNode *collisionNode = NULL;
		graphic::cBoundingPlane collisionBPlane;
		graphic::cBoundingSphere collisionBSphere;
		float minLen = FLT_MAX;
		for (int i = 0; i < 8; ++i)
		{
			// 8 방향으로 충돌 체크한다.
			const Ray ray(pos + Vector3(0, srcBSphere.GetPos().y, 0), dirs[i]);
			float len = FLT_MAX;
			//graphic::cBoundingPlane bplane;
			//graphic::cBoundingSphere bsphere;
			cGlobal::sCollisionResult colResult;
			const int cResult = g_global.IsCollisionByRay(ray, m_agent, colResult);
			if (colResult.distance < (srcBSphere.GetRadius() * 1.3f))
				continue; // 장애물이 있으면, 해당 방향은 무시

			// 이동 경로와 가장 가까운 방향을 선택한다.
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

		// 목적지가 유닛에 의해 막혀있고, 유닛이 정지한 상태라면, 이동을 멈춘다.
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
