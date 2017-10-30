//
// Move And Action
//

#include "stdafx.h"
#include "zealot.h"
#include "zealotai.h"
#include "move2.h"

using namespace graphic;

class cViewer : public framework::cGameMain
{
public:
	cViewer();
	virtual ~cViewer();

	virtual bool OnInit() override;
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnShutdown() override;
	virtual void OnMessageProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void ChangeWindowSize();


public:
	cCamera m_camera;
	cGrid m_ground;
	cZealot m_zealot;
	Vector3 m_dest;

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
{
	m_windowName = L"Move And Action";
	//const RECT r = { 0, 0, 1024, 768 };
	const RECT r = { 0, 0, 1280, 1024 };
	m_windowRect = r;
	m_moveLen = 0;
	m_LButtonDown = false;
	m_RButtonDown = false;
	m_MButtonDown = false;
}

cViewer::~cViewer()
{
	graphic::ReleaseRenderer();
}


bool cViewer::OnInit()
{
	DragAcceptFiles(m_hWnd, TRUE);

	const float WINSIZE_X = float(m_windowRect.right - m_windowRect.left);
	const float WINSIZE_Y = float(m_windowRect.bottom - m_windowRect.top);
	m_camera.SetCamera(Vector3(-3, 3, -5), Vector3(0, 0, 0), Vector3(0, 1, 0));
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

	m_zealot.Create(m_renderer);

	ai::cMove2<cZealot> *action = new ai::cMove2<cZealot>(&m_zealot, Vector3(10,0,10));
	m_zealot.m_ai->PushAction(action);

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	cAutoCam cam(&m_camera);
	GetMainCamera().Update(deltaSeconds);
	m_zealot.Update(m_renderer, deltaSeconds);
}


void cViewer::OnRender(const float deltaSeconds)
{
	cAutoCam cam(&m_camera);

	// Render
	if (m_renderer.ClearScene())
	{
		m_renderer.BeginScene();

		GetMainCamera().Bind(m_renderer);
		GetMainLight().Bind(m_renderer);

		m_ground.Render(m_renderer);
		m_zealot.m_techniqueName = "Unlit";
		m_zealot.Render(m_renderer);
		Transform tfm;
		tfm.pos = m_dest;
		tfm.scale = Vector3(1, 1, 1)*0.1f;
		m_renderer.m_dbgBox.SetBox(tfm);
		m_renderer.m_dbgBox.Render(m_renderer);
		m_renderer.RenderAxis();

		m_renderer.EndScene();
		m_renderer.Present();
	}
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
		Vector3 orig, dir;
		GetMainCamera().GetRay(pos.x, pos.y, orig, dir);
		Vector3 lookAt = m_groundPlane1.Pick(orig, dir);
		len = (orig - lookAt).Length();

		//const float len = graphic::GetMainCamera().GetDistance();
		float zoomLen = (len > 100) ? 50 : (len / 4.f);
		if (fwKeys & 0x4)
			zoomLen = zoomLen / 10.f;

		Vector3 eyePos = GetMainCamera().m_eyePos + dir * ((zDelta <= 0) ? -zoomLen : zoomLen);
		if (eyePos.y > 1)
			GetMainCamera().Zoom(dir, (zDelta < 0) ? -zoomLen : zoomLen);

		//graphic::GetMainCamera().Zoom((zDelta<0) ? -zoomLen : zoomLen);
	}
	break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_TAB: break;
		case VK_RETURN: break;
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

		Vector3 orig, dir;
		graphic::GetMainCamera().GetRay(pos.x, pos.y, orig, dir);
		Vector3 p1 = m_groundPlane1.Pick(orig, dir);
		m_moveLen = common::clamp(1, 100, (p1 - orig).Length());
		graphic::GetMainCamera().MoveCancel();

		ai::cMove2<cZealot> *action = new ai::cMove2<cZealot>(&m_zealot, p1);
		m_zealot.m_ai->SetAction(action);
		m_dest = p1;
	}
	break;

	case WM_LBUTTONUP:
		ReleaseCapture();
		m_LButtonDown = false;
		break;

	case WM_RBUTTONDOWN:
	{
		SetCapture(m_hWnd);
		m_RButtonDown = true;
		m_curPos.x = LOWORD(lParam);
		m_curPos.y = HIWORD(lParam);

		Ray ray(m_curPos.x, m_curPos.y, 1024, 768,
			GetMainCamera().GetProjectionMatrix(),
			GetMainCamera().GetViewMatrix());
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

		Vector3 orig, dir;
		graphic::GetMainCamera().GetRay(pos.x, pos.y, orig, dir);
		Vector3 p1 = m_groundPlane1.Pick(orig, dir);

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

			graphic::GetMainCamera().Yaw2(x * 0.005f, Vector3(0, 1, 0));
			graphic::GetMainCamera().Pitch2(y * 0.005f, Vector3(0, 1, 0));

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

