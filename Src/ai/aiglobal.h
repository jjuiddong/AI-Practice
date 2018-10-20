//
// 2018-10-15, jjuiddong
// Global Variable & Function
//
#pragma once


namespace ai
{

	class cAiGlobal
	{
	public:
		cAiGlobal();
		virtual ~cAiGlobal();

		graphic::cNode* IsCollisionUnit(const graphic::cNode *node
			, const cBoundingSphere &srcBSphere
			, OUT cBoundingSphere &out);

		bool IsCollisionWall(const cBoundingSphere &bsphere
			, OUT cBoundingPlane &out);

		bool IsCollision(const graphic::cNode *srcNode
			, const graphic::cBoundingSphere &srcBSphere, OUT sCollisionResult &out);

		int IsCollisionByRay(const Ray &ray
			, const graphic::cNode *srcNode
			, const float radius
			, OUT sCollisionResult &out);


	public:
		enum { MAX_PLAYER = 10 };
		vector<cZealot*> m_zealots;
		vector<cZealot*> m_select; // reference
		ai::cNavigationMesh m_navi;
	};

}

extern ai::cAiGlobal g_ai;
