//
// Wall Collision Test
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


public:
	cCamera3D m_camera;
	cGrid m_ground;

	cCube m_cube1;
	cCube m_cube2;
	cSphere m_sphere;
	cSphere m_sphere2; // Collision Position
	bool m_isCollisionPosition;

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
	m_windowName = L"AI Wall Collision Test";
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
	const float WINSIZE_X = m_windowRect.right - m_windowRect.left;
	const float WINSIZE_Y = m_windowRect.bottom - m_windowRect.top;
	m_camera.SetCamera(Vector3(-3, 10, -10), Vector3(0, 0, 0), Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.0f, 10000.f);
	m_camera.SetViewPort(WINSIZE_X, WINSIZE_Y);

	m_ground.Create(m_renderer, 10, 10, 1, 1, eVertexType::POSITION | eVertexType::NORMAL);
	m_ground.m_mtrl.Init(Vector4(0.2f, 0.2f, 0.2f, 1), Vector4(0.2f, 0.2f, 0.2f, 1), Vector4(0.2f, 0.2f, 0.2f, 0.2f));

	{
		cBoundingBox bbox;
		bbox.SetBoundingBox(Vector3(-2, 0.3f, 1), Vector3(0.2f, 0.2f, 3.f), Quaternion());
		m_cube1.Create(m_renderer, bbox, eVertexType::POSITION | eVertexType::NORMAL);
	}

	{
		cBoundingBox bbox;
		bbox.SetBoundingBox(Vector3(1, 0.3f, -2), Vector3(3.f, 0.2f, .2f), Quaternion());
		m_cube2.Create(m_renderer, bbox, eVertexType::POSITION | eVertexType::NORMAL);
	}

	m_sphere.Create(m_renderer, 0.5f, 10, 10);
	m_sphere.m_transform.pos = Vector3(-1.f, 0, -1.f);
	m_sphere.m_boundingSphere.SetRadius(0.5f);

	m_sphere2.Create(m_renderer, 0.1f, 10, 10);

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL);
	const Vector3 lightPos(-400, 600, -300);
	const Vector3 lightLookat(0, 0, 0);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection((lightLookat - lightPos).Normal());

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	GetMainCamera().Update(deltaSeconds);
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

		static float t = 0;
		t += deltaSeconds;
		m_cube1.m_transform.rot.SetRotationY(t);
		m_cube2.m_transform.rot.SetRotationY(t * 1.5f);

		// Sphere Move to Center
		const Vector3 mov = -m_sphere.m_transform.pos.Normal() * 3 * deltaSeconds;
		m_sphere.m_transform.pos += mov;

		cBoundingBox bbox1 = m_cube1.m_boundingBox;
		bbox1 *= m_cube1.GetWorldMatrix();

		cBoundingBox bbox2 = m_cube2.m_boundingBox;
		bbox2 *= m_cube2.GetWorldMatrix();

		Vector3 collisionPos;
		Plane collisionPlane;
		m_isCollisionPosition = false;

		const float reflectLen = 0.01f;
		cBoundingSphere bsphere = m_sphere.m_boundingSphere;
		bsphere.SetPos(m_sphere.m_transform.pos);
		if (bbox1.Collision2D(bsphere, &collisionPos, &collisionPlane))
		{
			m_isCollisionPosition = true;
			m_sphere2.m_transform.pos = collisionPos;
			m_sphere.m_transform.pos = collisionPos + collisionPlane.N * (m_sphere.GetRadius()+ reflectLen);
		}

		if (bbox2.Collision2D(bsphere, &collisionPos, &collisionPlane))
		{
			m_isCollisionPosition = true;
			m_sphere2.m_transform.pos = collisionPos;
			m_sphere.m_transform.pos = collisionPos + collisionPlane.N * (m_sphere.GetRadius() + reflectLen);
		}

		if (m_isCollisionPosition)
		{
			m_sphere.m_mtrl.m_diffuse = Vector4(1, 0, 0, 1);
		}
		else
		{
			m_sphere.m_mtrl.m_diffuse = Vector4(1, 1, 1, 1);
		}

		CommonStates state(m_renderer.GetDevice());
		m_renderer.GetDevContext()->RSSetState(state.Wireframe());
		m_ground.Render(m_renderer);
		m_ground.RenderLine(m_renderer);
		m_cube1.Render(m_renderer);
		m_cube2.Render(m_renderer);
		m_sphere.Render(m_renderer);

		if (m_isCollisionPosition)
			m_sphere2.Render(m_renderer);

		m_renderer.RenderAxis();

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

		int fwKeys = GET_KEYSTATE_WPARAM(wParam);
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		//dbg::Print("%d %d", fwKeys, zDelta);

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

			m_camera.Yaw2(x * 0.005f);
			m_camera.Pitch2(y * 0.005f);

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

