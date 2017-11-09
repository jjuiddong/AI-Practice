//
// AStar Builder
//
#include "stdafx.h"
#include "move2.h"

using namespace graphic;

struct sSharedData
{
	// output
	float fps;
	double dtVal;

	// input
	bool isDbgRender;
	int dbgRenderStyle;

	char dummy[256];
};
sSharedData *g_sharedData = NULL;
cShmmem g_dbgShmem;
cMutex g_mutex("DebugMonitorMutex::jjuiddong");
ai::cPathFinder g_pathFinder;


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
	cCamera3D m_camera;
	cGrid m_ground;
	cImGui m_gui;
	int m_editType;
	cTerrain m_terrain;
	cDbgLineList m_lineList;

	enum { MAX_PLAYER = 1 };
	vector<cZealot*> m_zealots;

	sf::Vector2i m_curPos;
	Plane m_groundPlane1, m_groundPlane2;
	float m_moveLen;
	bool m_LButtonDown;
	bool m_RButtonDown;
	bool m_MButtonDown;
};

INIT_FRAMEWORK(cViewer);


cViewer::cViewer()
	: m_groundPlane1(Vector3(0, 1, 0), 0)
	, m_camera("main camera")
	, m_editType(0)
{
	m_windowName = L"Astar Builder";
	const RECT r = { 0, 0, 1024, 768 };
	//const RECT r = { 0, 0, 1280, 1024 };
	m_windowRect = r;
	m_moveLen = 0;
	m_LButtonDown = false;
	m_RButtonDown = false;
	m_MButtonDown = false;
}

cViewer::~cViewer()
{
	m_gui.Shutdown();
	graphic::ReleaseRenderer();
}


bool cViewer::OnInit()
{
	DragAcceptFiles(m_hWnd, TRUE);

	dbg::RemoveLog();

	const float WINSIZE_X = float(m_windowRect.right - m_windowRect.left);
	const float WINSIZE_Y = float(m_windowRect.bottom - m_windowRect.top);
	m_camera.SetCamera(Vector3(-10, 12, -10), Vector3(0, 0, 0), Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.0f, 10000.f);
	m_camera.SetViewPort(WINSIZE_X, WINSIZE_Y);
	m_camera.m_isMovingLimitation = true;
	m_camera.m_boundingHSphere.SetBoundingHalfSphere(Vector3(0, 0, 0), 500);

	m_ground.Create(m_renderer, 100, 100, 1, eVertexType::POSITION);
	m_ground.m_mtrl.InitGray();
	m_ground.m_primitiveType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL,
		Vector4(0.2f, 0.2f, 0.2f, 1), Vector4(0.9f, 0.9f, 0.9f, 1),
		Vector4(0.2f, 0.2f, 0.2f, 1));
	const Vector3 lightPos(-300, 300, -300);
	const Vector3 lightLookat(0, 0, 0);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection((lightLookat - lightPos).Normal());

	cBoundingBox bbox2(Vector3(0, 0, 0), Vector3(1, 1, 1), Quaternion());
	m_renderer.m_dbgAxis.SetAxis(bbox2, false);

	m_gui.Init(m_hWnd, m_renderer.GetDevice(), m_renderer.GetDevContext(), NULL);

	//m_terrain.Create()
	cTerrainLoader loader(&m_terrain);
	loader.Read(m_renderer, "../media2/astar.trn");
	g_pathFinder.Read("../media2/astar.txt");
	m_lineList.Create(m_renderer, 1024);
	for (auto &vtx : g_pathFinder.m_vertices)
	{
		for (int i = 0; i < ai::sVertex::MAX_EDGE; ++i)
		{
			if (vtx.edge[i] < 0)
				break;
			m_lineList.AddLine(m_renderer, vtx.pos + Vector3(0, 0.1f, 0)
				, g_pathFinder.m_vertices[vtx.edge[i]].pos + Vector3(0, 0.1f, 0));
		}
	}

	Vector3 posArray[] = {
		Vector3(5,0,5)
		, Vector3(0,0,0)
		, Vector3(10,0,0)
		, Vector3(10,0,10)
		, Vector3(0,0,10)
		, Vector3(1,0,1)
		, Vector3(11,0,0)
		, Vector3(11,0,11)
		, Vector3(0,0,11)
	};
	for (int i = 0; i < MAX_PLAYER; ++i)
	{
		cZealot *zealot = new cZealot();
		zealot->Create(m_renderer);
		zealot->m_name.Format("Zealot%d", i);
		zealot->m_transform.pos = posArray[i];
		m_zealots.push_back(zealot);
	}


	if (!g_dbgShmem.Init("DebugMonitorShmem::jjuiddong"))
		assert(0);
	g_sharedData = (sSharedData*)g_dbgShmem.m_memPtr;

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	cAutoCam cam(&m_camera);
	GetMainCamera().Update(deltaSeconds);

	for (auto &p : m_zealots)
		p->Update(m_renderer, deltaSeconds);

	// DbgMonitor
	g_mutex.Lock();
	m_renderer.m_isDbgRender = g_sharedData->isDbgRender;
	m_renderer.m_dbgRenderStyle = g_sharedData->dbgRenderStyle;
	g_mutex.Unlock();
}


void cViewer::OnRender(const float deltaSeconds)
{
	cAutoCam cam(&m_camera);

	m_gui.NewFrame();

	//static bool isOpen = true;
	//ImGui::Begin("Test", &isOpen, ImVec2(200,400));
	//ImGui::RadioButton("View", &m_editType, 0); ImGui::SameLine();
	//ImGui::RadioButton("Vertex", &m_editType, 1); ImGui::SameLine();
	//ImGui::RadioButton("Edge", &m_editType, 2);
	//ImGui::End();


	// Render
	if (m_renderer.ClearScene())
	{
		m_renderer.BeginScene();

		GetMainCamera().Bind(m_renderer);
		GetMainLight().Bind(m_renderer);
		m_terrain.Render(m_renderer);

		// Render Path
		m_renderer.m_dbgBox.m_color = cColor::WHITE;
		for (auto &vtx : g_pathFinder.m_vertices)
		{
			const cBoundingBox bbox(vtx.pos + Vector3(0, 0, 0)
				, Vector3(1, 1, 1) * 0.2f
				, Quaternion());
			m_renderer.m_dbgBox.SetBox(bbox);
			m_renderer.m_dbgBox.Render(m_renderer);
		}
		m_lineList.Render(m_renderer);

		for (auto &p : m_zealots)
		{
			p->SetTechnique("Unlit");
			p->Render(m_renderer);
		}

		m_gui.Render();

		m_renderer.RenderAxis();
		m_renderer.RenderFPS();

		m_renderer.EndScene();
		m_renderer.Present();
	}
}


void cViewer::ChangeWindowSize()
{
	if (m_renderer.CheckResetDevice())
	{
		m_renderer.ResetDevice();
		m_camera.SetViewPort(m_renderer.m_viewPort.GetWidth(), m_renderer.m_viewPort.GetHeight());
	}
}


void cViewer::OnMessageProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool maximizeWnd = false;
	m_gui.WndProcHandler(m_hWnd, message, wParam, lParam);
	if (ImGui::IsAnyItemHovered())
		return;

	switch (message)
	{
	case WM_EXITSIZEMOVE:
		ChangeWindowSize();
		break;

	case WM_SIZE:
		if (SIZE_MAXIMIZED == wParam)
		{
			maximizeWnd = true;
			ChangeWindowSize();
		}
		else if (maximizeWnd && (SIZE_RESTORED == wParam))
		{
			maximizeWnd = false;
			ChangeWindowSize();
		}
		break;

	case WM_DROPFILES:
	{
		const HDROP hdrop = (HDROP)wParam;
		char filePath[MAX_PATH];
		const UINT size = DragQueryFileA(hdrop, 0, filePath, MAX_PATH);
		if (size == 0)
			return;// handle error...
	}
	break;

	case WM_MOUSEWHEEL:
	{
		cAutoCam cam(&m_camera);

		POINT pos = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
		ScreenToClient(m_hWnd, &pos);
		int fwKeys = GET_KEYSTATE_WPARAM(wParam);
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

		float len = 0;
		const Ray ray = GetMainCamera().GetRay(pos.x, pos.y);
		Vector3 lookAt = m_groundPlane1.Pick(ray.orig, ray.dir);
		len = (ray.orig - lookAt).Length();

		float zoomLen = (len > 100) ? 50 : (len / 4.f);
		if (fwKeys & 0x4)
			zoomLen = zoomLen / 10.f;

		Vector3 eyePos = GetMainCamera().m_eyePos + ray.dir * ((zDelta <= 0) ? -zoomLen : zoomLen);
		if (eyePos.y > 1)
			GetMainCamera().Zoom(ray.dir, (zDelta < 0) ? -zoomLen : zoomLen);

		//graphic::GetMainCamera().Zoom((zDelta<0) ? -zoomLen : zoomLen);
	}
	break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_TAB: break;
		case VK_RETURN: break;

		case VK_SPACE:
		{
			cAutoCam cam(&m_camera);

			const Ray ray = graphic::GetMainCamera().GetRay(m_curPos.x, m_curPos.y);
			Vector3 p1 = m_groundPlane1.Pick(ray.orig, ray.dir);
		}
		break;
		}
		break;

	case WM_LBUTTONDOWN:
	{
		cAutoCam cam(&m_camera);
		POINT pos = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
		GetMainCamera().MoveCancel();

		SetCapture(m_hWnd);
		m_LButtonDown = true;
		m_curPos.x = pos.x;
		m_curPos.y = pos.y;

		const Ray ray = graphic::GetMainCamera().GetRay(pos.x, pos.y);
		Vector3 p1 = m_groundPlane1.Pick(ray.orig, ray.dir);
		m_moveLen = common::clamp(1, 100, (p1 - ray.orig).Length());
	}
	break;

	case WM_LBUTTONUP:
		ReleaseCapture();
		m_LButtonDown = false;
		break;

	case WM_RBUTTONDOWN:
	{
		cAutoCam cam(&m_camera);
		POINT pos = { (short)LOWORD(lParam), (short)HIWORD(lParam) };

		SetCapture(m_hWnd);
		m_RButtonDown = true;
		m_curPos.x = pos.x;
		m_curPos.y = pos.y;

		const Ray ray = graphic::GetMainCamera().GetRay(pos.x, pos.y);
		Vector3 p1 = m_groundPlane1.Pick(ray.orig, ray.dir);
		m_moveLen = common::clamp(1, 100, (p1 - ray.orig).Length());
		graphic::GetMainCamera().MoveCancel();

		ai::cMove2<cZealot> *action = new ai::cMove2<cZealot>(m_zealots[0], p1);
		m_zealots[0]->m_ai->SetAction(action);
	}
	break;

	case WM_RBUTTONUP:
		m_RButtonDown = false;
		ReleaseCapture();
		break;

	case WM_MBUTTONDOWN:
		SetCapture(m_hWnd);
		m_MButtonDown = true;
		m_curPos.x = LOWORD(lParam);
		m_curPos.y = HIWORD(lParam);
		break;

	case WM_MBUTTONUP:
		ReleaseCapture();
		m_MButtonDown = false;
		break;

	case WM_MOUSEMOVE:
	{
		cAutoCam cam(&m_camera);

		sf::Vector2i pos = { (int)LOWORD(lParam), (int)HIWORD(lParam) };

		const Ray ray = graphic::GetMainCamera().GetRay(pos.x, pos.y);
		Vector3 p1 = m_groundPlane1.Pick(ray.orig, ray.dir);

		if (wParam & 0x10) // middle button down
		{
		}

		if (m_LButtonDown)
		{
			const int x = pos.x - m_curPos.x;
			const int y = pos.y - m_curPos.y;

			if ((abs(x) > 1000) || (abs(y) > 1000))
			{
				break;
			}

			Vector3 dir = graphic::GetMainCamera().GetDirection();
			Vector3 right = graphic::GetMainCamera().GetRight();
			dir.y = 0;
			dir.Normalize();
			right.y = 0;
			right.Normalize();

			graphic::GetMainCamera().MoveRight(-x * m_moveLen * 0.001f);
			graphic::GetMainCamera().MoveFrontHorizontal(y * m_moveLen * 0.001f);
		}
		else if (m_RButtonDown)
		{
			const int x = pos.x - m_curPos.x;
			const int y = pos.y - m_curPos.y;

			//if (GetAsyncKeyState(VK_LCONTROL)) 
			{
				m_camera.Yaw2(x * 0.005f, Vector3(0, 1, 0));
				m_camera.Pitch2(y * 0.005f, Vector3(0, 1, 0));
			}

		}
		else if (m_MButtonDown)
		{
			const sf::Vector2i point = { pos.x - m_curPos.x, pos.y - m_curPos.y };

			const float len = graphic::GetMainCamera().GetDistance();
			graphic::GetMainCamera().MoveRight(-point.x * len * 0.001f);
			graphic::GetMainCamera().MoveUp(point.y * len * 0.001f);
		}
		else
		{
		}

		m_curPos = pos;
	}
	break;
	}
}

