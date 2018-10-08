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
		, const graphic::cBoundingSphere &srcBSphere
		, OUT graphic::cBoundingSphere &out);

	bool IsCollisionWall( const graphic::cBoundingSphere &bsphere
		, OUT graphic::cBoundingPlane &out);


	struct sCollisionResult {
		int type; // 0: no collision, 1:bsphere, 2:bplane, 3:bsphere+bplane
		graphic::cBoundingSphere bsphere;
		graphic::cBoundingPlane bplane;
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
