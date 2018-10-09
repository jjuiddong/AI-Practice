//
// Global Variable & Function
//
//
#pragma once


class cViewer;
class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();


	graphic::cNode* IsCollisionUnit(const graphic::cNode *node
		, const cBoundingSphere &srcBSphere
		, OUT cBoundingSphere &out);

	bool IsCollisionWall( const cBoundingSphere &bsphere
		, OUT cBoundingPlane &out);


	struct sCollisionResult {
		int type; // 0: no collision, 1:bsphere, 2:bplane, 3:bsphere+bplane
		cBoundingSphere bsphere;
		cBoundingPlane bplane;
		graphic::cNode *node;
		float distance;
	};
	bool IsCollision(const graphic::cNode *srcNode
		, const graphic::cBoundingSphere &srcBSphere, OUT sCollisionResult &out);

	int IsCollisionByRay(const Ray &ray
		, const graphic::cNode *srcNode
		, const float radius
		, OUT sCollisionResult &out);


public:
	cViewer *m_main;
};

extern cGlobal g_global;
