//
// 2017-10-30, jjuiddong
//
// 2017-11-18
//	- route
//
#pragma once


class cZealotAI;
class cZealot : public graphic::cModel
			, public ai::iActorInterface<cZealot>
{
public:
	cZealot();
	virtual ~cZealot();

	bool Create(graphic::cRenderer &renderer);
	virtual bool Render(graphic::cRenderer &renderer, const XMMATRIX &parentTm = graphic::XMIdentity, const int flags = 1) override;
	virtual bool Update(graphic::cRenderer &renderer, const float deltaSeconds) override;

	// iActorInterface Override
	virtual Transform& aiGetTransform() override;
	virtual void aiSetAnimation(const Str64 &animationName) override;
	virtual graphic::cNode* aiCollision(const graphic::cBoundingSphere &srcBSphere
		, OUT graphic::cBoundingSphere &collisionSphrere) override;
	virtual bool aiCollisionWall(OUT graphic::cBoundingPlane &out) override;


protected:
	virtual void InitModel(graphic::cRenderer &renderer) override;


public:
	cZealotAI *m_ai;
	bool m_isLoaded;
	graphic::cCube *m_collisionWall;

	// for debugging
	vector<Vector3> m_route;
	Vector3 m_nextPos;
	Vector3 m_dir;
};


inline Transform& cZealot::aiGetTransform() { return m_transform; }
