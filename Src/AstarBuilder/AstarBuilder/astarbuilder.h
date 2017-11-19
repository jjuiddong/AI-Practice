#pragma once


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
	cImGui m_gui;
	int m_editType;
	graphic::cTerrain m_terrain;
	graphic::cDbgLineList m_lineList;
	vector<graphic::cRect3D*> m_areas; // reference

	enum { MAX_PLAYER = 1 };
	vector<cZealot*> m_zealots;

	sf::Vector2i m_curPos;
	Plane m_groundPlane1, m_groundPlane2;
	float m_moveLen;
	bool m_LButtonDown;
	bool m_RButtonDown;
	bool m_MButtonDown;
};

