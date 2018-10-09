//
// 2017-10-30, jjuiddong
//
// 2017-11-18
//	- route
//
#pragma once


class cZealotBrain;
class cZealot : public graphic::cModel
{
public:
	cZealot();
	virtual ~cZealot();

	bool Create(graphic::cRenderer &renderer);
	virtual bool Render(graphic::cRenderer &renderer, const XMMATRIX &parentTm = graphic::XMIdentity, const int flags = 1) override;
	virtual bool Update(graphic::cRenderer &renderer, const float deltaSeconds) override;

	graphic::cNode* Collision(const graphic::cBoundingSphere &srcBSphere
		, OUT graphic::cBoundingSphere &collisionSphrere);


protected:
	virtual void InitModel(graphic::cRenderer &renderer) override;


public:
	cZealotBrain *m_ai;
	bool m_isLoaded;
	graphic::cCube *m_collisionWall;

	// for debugging
	vector<Vector3> m_route;
	Vector3 m_nextPos;
	Vector3 m_dir;
};
