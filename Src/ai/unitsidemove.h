//
// 2018-10-20, jjuiddong
// Unit Side Move
//	대기상태에서 다른 유닛이 이동 중에 충돌이 났다면
//	, 대기상태인 유닛이 길을 비켜준다.
//
#pragma once


namespace ai
{

	template<class T>
	class cUnitSideMove : public cAction<T>
		, public common::cMemoryPool<cUnitSideMove<T>>
	{
	public:
		cUnitSideMove(T *agent, T *oppAgent, const Vector3 &oppDir
			, const Vector3 &oppDest);

		virtual bool StartAction() override;
		virtual bool ActionExecute(const float deltaSeconds) override;
		virtual bool MessageProccess(const sMsg &msg) override;


	public:
		using cAction<T>::m_agent; // base template class member access
		T *m_oppAgent; // opponent agent
		Vector3 m_oppDir; // opponent direction
		Vector3 m_oppDest; // opponent destination
		Vector3 m_dest;
		Vector3 m_dir;
	};


	template<class T>
	cUnitSideMove<T>::cUnitSideMove(T *agent, T *oppAgent, const Vector3 &oppDir
		, const Vector3 &oppDest)
		: cAction<T>(agent, "side move", "", eActionType::MOVE)
		, m_oppAgent(oppAgent)
		, m_oppDir(oppDir)
		, m_oppDest(oppDest)
	{
	}


	//-----------------------------------------------------------------------
	// StartAction
	//-----------------------------------------------------------------------
	template<class T>
	bool cUnitSideMove<T>::StartAction()
	{
		const Vector3 curPos = m_agent->m_transform.pos;
		Vector3 dir = (curPos - m_oppAgent->m_transform.pos).Normal();
		dir.y = 0.f;
		dir.Normalize();

		m_dest = curPos + dir * m_agent->m_boundingSphere.GetRadius()*2.f;
		m_dir = dir;

		vector<Vector3> path;
		path.push_back(m_dest);
		this->PushAction(new cUnitMove2<T>(m_agent, path));

		return true;

		//const cBoundingSphere srcBSphere = m_agent->m_boundingSphere * m_agent->m_transform;
		//const Vector3 fromOppDir = (pos - m_oppAgent->m_transform.pos).Normal();


		//           dir
		//       7    0    1
		//         \     /
		//       6  - + -  2
		//         /     \
		//       5    4    3
		//
		// Direction 방향을 12시로해서, 시계방향으로
		// 총 8방향을, 0 ~ 7번으로 번호가 매겨진다.

		//Vector3 dirs[8];
		//const Vector3 forward = m_oppDir;
		//dirs[0] = forward;
		//dirs[2] = Vector3(0, 1, 0).CrossProduct(forward).Normal();
		//dirs[1] = (dirs[2] + dirs[0]).Normal();
		//dirs[4] = -dirs[0];
		//dirs[3] = (dirs[2] + dirs[4]).Normal();
		//dirs[6] = -dirs[2];
		//dirs[5] = (dirs[6] + dirs[4]).Normal();
		//dirs[7] = (dirs[6] + dirs[0]).Normal();

		//// oppDir 방향으로 길을 비킨다.
		//// 0,1,7
		//const bool check[8] = {
		//	true, true, false, false, false, false, false, true
		//};

		//// 각 방향으로 이동할시, 거리 스케일
		//const float sqrt2 = sqrt(2.f);
		//const float lenscale[8] = {
		//	2.f, sqrt2, 0.f, 0.f, 0.f, 0.f, 0.f, sqrt2
		//};

		//const Vector3 pos = m_agent->m_transform.pos;
		//const cBoundingSphere srcBSphere = m_agent->m_boundingSphere * m_agent->m_transform;
		//const Vector3 fromOppDir = (pos - m_oppAgent->m_transform.pos).Normal();

		//int dirIdx = -1; // 이동할 방향
		//int collisionType = 0; // 8방향에서 선택된 방향의 충돌 타입 (unit or wall)
		//graphic::cNode *collisionNode = NULL;
		//cBoundingPlane collisionBPlane;
		//cBoundingSphere collisionBSphere;
		//float minLen = FLT_MAX;
		//for (int i = 0; i < 8; ++i)
		//{
		//	if (!check[i])
		//		continue;

		//	const Ray ray(pos + Vector3(0, srcBSphere.GetPos().y, 0), dirs[i]);
		//	float len = FLT_MAX;
		//	sCollisionResult colResult;
		//	const int colltype = g_ai.IsCollisionByRay(ray, m_agent
		//		, srcBSphere.GetRadius(), colResult);
		//	const bool isBlock = (colResult.distance < (srcBSphere.GetRadius() * 1.3f));
		//	if (isBlock)
		//		continue;

		//	const float dot = dirs[i].DotProduct(fromOppDir);
		//	if (dot < minLen)
		//	{
		//		dirIdx = i;
		//		minLen = dot;
		//	}
		//}

		//if (dirIdx < 0)
		//	return false;
		//dirs[dirIdx] * 

		//return true;
	}


	//-----------------------------------------------------------------------
	// Action Execute
	//-----------------------------------------------------------------------
	template<class T>
	bool cUnitSideMove<T>::ActionExecute(const float deltaSeconds)
	{
		// nothing ~
		return false;
	}


	//-----------------------------------------------------------------------
	// MessageProcess
	//-----------------------------------------------------------------------
	template<class T>
	bool cUnitSideMove<T>::MessageProccess(const sMsg &msg)
	{
		return true;
	}

}
