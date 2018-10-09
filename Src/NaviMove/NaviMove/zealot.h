//
// 2017-10-30, jjuiddong
//
// 2017-11-18
//	- route
//
// 2018-10-09
//	- dirs
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


protected:
	virtual void InitModel(graphic::cRenderer &renderer) override;


public:
	bool m_isLoaded;
	bool m_isSelect;
	cZealotBrain *m_brain;
	graphic::cCube *m_collisionWall;

	// for debugging
	vector<Vector3> m_route;

	bool m_isCollisionTurn; // 충돌후 선회
	struct sCandidateDir {
		bool use;
		float len;
		Vector3 dir;
	};
	sCandidateDir m_dirs[8];
	Vector3 m_nextPos;
	Vector3 m_dir;
};
