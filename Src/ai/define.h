//
// 2018-10-14, jjuiddong
//
//
#pragma once


namespace ai
{

	struct sCollisionResult {
		int type; // 0: no collision, 1:bsphere, 2:bplane, 3:bsphere+bplane
		cBoundingSphere bsphere;
		cBoundingPlane bplane;
		graphic::cNode *node;
		float distance;
	};

}
