//
// Navigation Mesh Move
//
#pragma once


class cViewer : public framework::cGameMain
{
public:
	cViewer();
	virtual ~cViewer();

	virtual bool OnInit() override;
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnLostDevice() override;
	virtual void OnShutdown() override;
	virtual void OnMessageProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void ChangeWindowSize();


protected:
	void UpdateLookAt();
	void MakeLineList(graphic::cRenderer &renderer
		, const ai::cNavigationMesh &navi
		, OUT graphic::cDbgLineList &out);


public:
	graphic::cCamera3D m_camera;
	graphic::cTerrain m_terrain;
	vector<graphic::cNode*> m_walls;
	graphic::cCascadedShadowMap m_ccsm;
	ai::cNavigationMesh m_navi;
	graphic::cDbgAxis m_dbgAxis;
	graphic::cDbgLineList m_pathLineList;
	graphic::cDbgLineList m_nodeLineList;
	graphic::cTextManager m_nodeTextMgr;

	enum { MAX_PLAYER = 1 };
	vector<cZealot*> m_zealots;
	cGroup m_group;
	bool m_isWireframe;

	vector<graphic::cBoundingPlane> m_wallPlanes;

	sf::Vector2i m_curPos;
	Plane m_groundPlane1, m_groundPlane2;
	float m_moveLen;
	bool m_LButtonDown;
	bool m_RButtonDown;
	bool m_MButtonDown;
};
