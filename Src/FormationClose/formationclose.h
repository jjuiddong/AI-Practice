//
// 2018-10-14, jjuiddong
// Formation Close Move
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
	cImGui m_gui;
	graphic::cCamera3D m_camera;
	graphic::cTerrain m_terrain;
	vector<graphic::cNode*> m_walls;
	graphic::cCascadedShadowMap m_ccsm;
	graphic::cDbgAxis m_dbgAxis;
	graphic::cDbgLineList m_nodeLineList;
	graphic::cTextManager m_nodeTextMgr;
	graphic::cRect2D m_rect2D;
	graphic::cDbgFrustum m_dbgFrustum;
	graphic::cGridLine m_grid;

	cGroup m_group;
	bool m_isWireframe;

	bool m_isDragRect;
	sf::Vector2i m_clickPt;
	sf::Vector2i m_curPos;
	Plane m_groundPlane1, m_groundPlane2;
	float m_moveLen;
	bool m_LButtonDown;
	bool m_RButtonDown;
	bool m_MButtonDown;
};
