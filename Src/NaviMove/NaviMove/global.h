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

	int IsCollisionByRay(const Ray &ray
		, OUT graphic::cBoundingSphere *outSphere = NULL
		, OUT graphic::cBoundingPlane *outPlane = NULL
		, OUT float *outDistance = NULL);


public:
	cViewer *m_main;
};

extern cGlobal g_global;
