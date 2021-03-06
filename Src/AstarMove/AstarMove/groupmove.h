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
			, const float speed = 3.f)
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
		virtual bool StartAction() override {
			m_idx = 0;
			NextMove(0);
			m_agent->SetAnimation("Walk");
			return true;
		}

		// Action Execute
		virtual bool ActionExecute(const float deltaSeconds) override
		{
			if ((int)m_path.size() <= m_idx)
			{
				m_agent->SetAnimation("Stand");
				return false;
			}

			const Vector3 curPos = m_agent->m_transform.pos;
			const Vector3 dest = m_dest;

			// 목적지에 가깝다면, 다음 노드로 이동
			// 프레임이 낮을 때, 통과되는 문제가 있을 수 있다.
			const float distance = curPos.LengthRoughly(dest);
			if (curPos.LengthRoughly(dest) < 0.01f)
			{
				++m_idx;
				NextMove(m_idx);
				return true; // 목적지 도착. 액션종료.
			}

			// 회전 보간 계산
			if (m_rotateTime < m_rotateInterval)
			{
				m_rotateTime += deltaSeconds;

				const float alpha = min(1, m_rotateTime / m_rotateInterval);
				const Quaternion q = m_fromDir.Interpolate(m_toDir, alpha);
				m_agent->m_transform.rot = q;
			}

			// 캐릭터 이동.
			Vector3 pos = curPos + m_dir * m_speed * deltaSeconds;

			// 충돌 체크
			graphic::cBoundingSphere bsphere = m_agent->m_boundingSphere * m_agent->m_transform;
			graphic::cBoundingSphere out;
			graphic::cNode *collisionNode = m_agent->m_collisionWall;
			if (collisionNode)
			{
				out = collisionNode->m_boundingSphere * collisionNode->GetWorldTransform();
				// 모델 위치로 리턴한다. (SphereBox 중점은 모델위치와 약간 다르다)
				Vector3 pos = collisionNode->GetWorldTransform().pos;
				pos.y = curPos.y;
				out.SetPos(pos);
			}
			else
			{
				collisionNode = m_agent->Collision(bsphere, out);
			}

			//if (graphic::cNode *collisionNode = m_agent->Collision(bsphere, out))
			if (collisionNode)
			{
				bool isMoveCollision = false;
				bool isWallCollision = false;
				Vector3 opponentDir;
				if (cZealot *zealot = dynamic_cast<cZealot*>(collisionNode))
				{
					if (zealot->m_ai->IsCurrentAction(eActionType::MOVE))
					{
						if (cGroupMove *movAction = dynamic_cast<cGroupMove*>(zealot->m_ai->GetAction()))
						{
							isMoveCollision = true;
							opponentDir = movAction->m_dir;
						}
					}
				}
				else
				{
					isWallCollision = true;
				}

				Vector3 toMe = (pos - out.GetPos()).Normal();
				Vector3 toDest = (m_dest - out.GetPos()).Normal();

				bool isWait = false;
				if (isMoveCollision)
				{
					// 다른 이동 객체와 충돌 했다면, 
					// 상대방의 이동 방향과 같다면, 선회하지 않고, 잠깐 기다린다.
					// 만약, 상대방이 자신보다 앞에 있고, 같은 방향으로 이동 중이라면
					if ((toMe.DotProduct(opponentDir) < 0.f) // 자신을 등지고, 앞으로 전진한다면,
						&& (m_dir.DotProduct(opponentDir) > 0.f)) // 같은 방향으로 가고 있다면
					{
						// 잠깐 대기했다가 전진한다.
						m_collisionInterval = 0.3f;
						isWait = true;
						pos = curPos; // 현재 위치로 유지
					}
				}

				m_isWaiting = isWait;

				// 잠깐 대기할 경우 이거나,
				// 충돌 객체와 멀어지는 상황일 경우, 방향을 선회하지 않는다.
				if (!isWallCollision && (isWait || (toMe.DotProduct(toDest) >= 0)))
				{
					// Nothing~
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
					pos = out.GetPos() + toMe * (out.GetRadius() + bsphere.GetRadius());
					dbg::Log("collision \n");
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

		// NextMove
		void NextMove(const int idx)
		{
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
		}


	public:
		Vector3 m_offset;
		Vector3 m_dest;
		float m_speed; // 3.f
		float m_rotateInterval; // 회전 보간 시간, 0.3초
		float m_collisionInterval; // 충돌이 일어난 후, 목적지로 선회하기전 대기시간, 0.3f
		float m_distance;
		float m_oldDistance;

		Vector3 m_dir; // 이동 방향
		Quaternion m_fromDir; // 이동 전 방향
		Quaternion m_toDir; // 이동 후 방향
		float m_rotateTime; // 회전 중인 시간.
		bool m_isWaiting;

		int m_idx;
		vector<Vector3> m_path;
	};

}
