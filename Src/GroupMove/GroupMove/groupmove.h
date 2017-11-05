#pragma once

#include "zealotgroup.h"

class cZealot;

class cViewer : public framework::cGameMain
{
public:
	cViewer();
	virtual ~cViewer();

	virtual bool OnInit() override;
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnMessageProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void ChangeWindowSize();


public:
	graphic::cCamera3D m_camera;
	graphic::cGrid m_ground;

	enum { MAX_PLAYER = 9 };
	vector<cZealot*> m_zealots;
	cGroup m_group;
	Vector3 m_dest;

	sf::Vector2i m_curPos;
	Plane m_groundPlane1, m_groundPlane2;
	float m_moveLen;
	bool m_LButtonDown;
	bool m_RButtonDown;
	bool m_MButtonDown;
};

