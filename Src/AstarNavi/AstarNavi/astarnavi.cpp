//
// Astar + Navigation Mesh
//

#include "../../../../Common/Common/common.h"
using namespace common;
#include "../../../../Common/Graphic11/graphic11.h"
#include "../../../../Common/Framework11/framework11.h"


using namespace graphic;

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
	void NextMove(const int idx);


public:
	cCamera3D m_camera;
	cSphere m_sphere;
	cSphere m_sphere2; // Collision Position
	cDbgAxis m_dbgAxis;
	cTerrain m_terrain;
	bool m_isCollisionPosition;
	vector<cNode*> m_walls;
	cCascadedShadowMap m_ccsm;
	ai::cNavigationMesh m_navi;
	cDbgLineList m_lineList;
	
	// move state
	int m_curIdx;
	Vector3 m_dest;
	Vector3 m_dir;
	vector<Vector3> m_path;
	struct eMoveState {
		enum Enum {
			WAIT
			, MOVE
			, CMOVE // collision move
		};
	};
	eMoveState::Enum m_movState;
	//

	bool m_isWireframe;

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
	, m_isCollisionPosition(false)
{
	m_windowName = L"AI AStar + Navigation Mesh";
	//const RECT r = { 0, 0, 1024, 768 };
	const RECT r = { 0, 0, 1280, 1024 };
	m_windowRect = r;
	m_moveLen = 0;
	m_LButtonDown = false;
	m_RButtonDown = false;
	m_MButtonDown = false;

	m_curIdx = 0;
	m_path.push_back(Vector3(0, 0, 0));
	m_movState = eMoveState::MOVE;
	m_isWireframe = false;
}

cViewer::~cViewer()
{
	graphic::ReleaseRenderer();
}


bool cViewer::OnInit()
{
	const float WINSIZE_X = m_windowRect.right - m_windowRect.left;
	const float WINSIZE_Y = m_windowRect.bottom - m_windowRect.top;
	m_camera.SetCamera(Vector3(10, 25, -10), Vector3(10, 0, 10), Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.0f, 10000.f);
	m_camera.SetViewPort(WINSIZE_X, WINSIZE_Y);

	cTerrainLoader loader(&m_terrain);
	loader.Read(m_renderer, "../media2/wall.trn");

	// get all wall
	for (auto &tile : m_terrain.m_tiles)
		for (auto &p : tile->m_children)
			if (p->m_name == "cube")
				m_walls.push_back(p);

	for (auto &wall : m_walls)
		if (cCube *cube = dynamic_cast<cCube*>(wall))
			cube->m_color = cColor(0.8f, 0.8f, 0.8f, 1.f);

	{
		cBoundingBox bbox;
		bbox.SetBoundingBox(Vector3(0, 0, 0), Vector3(1, 1, 1), Quaternion());
		m_dbgAxis.Create(m_renderer);
		m_dbgAxis.SetAxis(bbox, false);
	}

	m_sphere.Create(m_renderer, 0.5f, 10, 10);
	m_sphere.m_transform.pos = Vector3(1.f, 0.5f, 1.f);
	m_sphere.m_boundingSphere.SetRadius(0.5f);
	m_movState = eMoveState::WAIT;

	m_sphere2.Create(m_renderer, 0.1f, 10, 10);

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL);
	const Vector3 lightPos(-400, 800, -300);
	const Vector3 lightLookat(0, 0, 0);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection((lightLookat - lightPos).Normal());

	m_ccsm.Create(m_renderer);

	if (!m_navi.ReadFromPathFile("../media2/wallnavi.txt"))
	{
		::MessageBoxA(m_hWnd, "Error Read Navigation Mesh file", "Error", MB_OK);
		return false;
	}

	m_lineList.Create(m_renderer, 128);

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	GetMainCamera().Update(deltaSeconds);

	if (eMoveState::MOVE == m_movState)
	{
		Vector3 diff = m_sphere.m_transform.pos - m_path[m_curIdx];
		diff.y = 0;

		if (diff.Length() < 0.01f) // arrive
		{
			NextMove(m_curIdx + 1);
		}
		else
		{
			m_sphere.m_transform.pos += m_dir * deltaSeconds * 2.f;
		}
	}
}


void cViewer::NextMove(const int idx)
{
	if ((int)m_path.size() <= idx)
	{
		m_movState = eMoveState::WAIT;
		return;
	}

	Vector3 nextPos = m_path[idx];
	Vector3 diff = nextPos - m_sphere.m_transform.pos;
	diff.y = 0;
	if (diff.Length() < 0.01f) // arrive
	{
		NextMove(idx + 1);
		return;
	}

	m_movState = eMoveState::MOVE;
	m_curIdx = idx;

	m_dir = nextPos - m_sphere.m_transform.pos;
	m_dir.y = 0;
	m_dir.Normalize();
}


void cViewer::OnRender(const float deltaSeconds)
{
	cAutoCam cam(&m_camera);

	GetMainCamera().Bind(m_renderer);
	GetMainLight().Bind(m_renderer);
	m_terrain.BuildCascadedShadowMap(m_renderer, m_ccsm);

	// Render
	if (m_renderer.ClearScene())
	{
		m_renderer.BeginScene();

		GetMainCamera().Bind(m_renderer);
		GetMainLight().Bind(m_renderer);

		CommonStates state(m_renderer.GetDevice());
		m_renderer.GetDevContext()->RSSetState(m_isWireframe ? state.Wireframe() : state.CullCounterClockwise());
		m_terrain.SetTechnique("ShadowMap");
		m_terrain.RenderCascadedShadowMap(m_renderer, m_ccsm);

		m_sphere.Render(m_renderer);
		m_sphere2.Render(m_renderer);
		m_lineList.Render(m_renderer);

		m_dbgAxis.Render(m_renderer);
		m_renderer.RenderFPS();

		m_renderer.EndScene();
		m_renderer.Present();
	}
}


void cViewer::OnLostDevice()
{
	m_renderer.ResetDevice(0, 0, true);
	m_camera.SetViewPort(m_renderer.m_viewPort.GetWidth(), m_renderer.m_viewPort.GetHeight());
}


void cViewer::OnShutdown()
{
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

	case WM_MOUSEWHEEL:
	{
		cAutoCam cam(&m_camera);
		int fwKeys = GET_KEYSTATE_WPARAM(wParam);
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		const float len = graphic::GetMainCamera().GetDistance();
		float zoomLen = (len > 100) ? 50 : (len / 4.f);
		if (fwKeys & 0x4)
			zoomLen = zoomLen / 10.f;
		graphic::GetMainCamera().Zoom((zDelta<0) ? -zoomLen : zoomLen);
	}
	break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_TAB:
		{
			if (m_slowFactor < 1.f)
				m_slowFactor = 1.f;
			else
				m_slowFactor = 0.2f;
		}
		break;

		case VK_SPACE:
			if (RUN == m_state)
				Pause();
			else
				Resume();
			break;

		case VK_RETURN:
			m_isWireframe = !m_isWireframe;
			break;
		}
		break;

	case WM_LBUTTONDOWN:
	{
		cAutoCam cam(&m_camera);
		POINT pos = { (short)LOWORD(lParam), (short)HIWORD(lParam) };

		SetCapture(m_hWnd);
		m_LButtonDown = true;
		m_curPos.x = LOWORD(lParam);
		m_curPos.y = HIWORD(lParam);

		const Ray ray = GetMainCamera().GetRay(pos.x, pos.y);
		Vector3 p1 = m_groundPlane1.Pick(ray.orig, ray.dir);
		m_moveLen = common::clamp(1, 100, (p1 - ray.orig).Length());
		graphic::GetMainCamera().MoveCancel();
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

		if ((!GetAsyncKeyState(VK_LCONTROL)) && (m_state != PAUSE))
		{
			const Ray ray = GetMainCamera().GetRay(pos.x, pos.y);
			Plane ground(Vector3(0, 1, 0), 0);
			m_dest = ground.Pick(ray.orig, ray.dir);
			m_dest.y = 1.f;
			m_path.clear();
			m_curIdx = 0;
			vector<Vector3> out1;
			m_navi.Find(m_sphere.m_transform.pos, m_dest, out1, m_path);
			NextMove(0);

			m_lineList.ClearLines();
			for (int i=0; i < (int)m_path.size()-1; ++i)
				m_lineList.AddLine(m_renderer, m_path[i], m_path[i + 1], false);
			m_lineList.UpdateBuffer(m_renderer);
		}
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

		if (wParam & 0x10) // middle button down
		{
		}

		if (m_LButtonDown)
		{
			const int x = pos.x - m_curPos.x;
			const int y = pos.y - m_curPos.y;

			if ((abs(x) > 1000) || (abs(y) > 1000))
				break;

			m_curPos = pos;

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
			m_curPos = pos;

			if (GetAsyncKeyState(VK_LCONTROL) || (PAUSE == m_state))
			{
				m_camera.Yaw2(x * 0.005f);
				m_camera.Pitch2(y * 0.005f);
			}
		}
		else if (m_MButtonDown)
		{
			const sf::Vector2i point = { pos.x - m_curPos.x, pos.y - m_curPos.y };
			m_curPos = pos;

			const float len = graphic::GetMainCamera().GetDistance();
			graphic::GetMainCamera().MoveRight(-point.x * len * 0.001f);
			graphic::GetMainCamera().MoveUp(point.y * len * 0.001f);
		}
		else
		{
		}
	}
	break;
	}
}
