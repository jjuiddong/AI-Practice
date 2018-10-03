#include "stdafx.h"
#include "navimove.h"

using namespace graphic;

INIT_FRAMEWORK(cViewer);

cViewer::cViewer()
	: m_groundPlane1(Vector3(0, 1, 0), 0)
	, m_camera("main camera")
{
	m_windowName = L"Navigation Mesh Move";
	//const RECT r = { 0, 0, 1024, 768 };
	const RECT r = { 0, 0, 1280, 1024 };
	m_windowRect = r;
	m_moveLen = 0;
	m_LButtonDown = false;
	m_RButtonDown = false;
	m_MButtonDown = false;
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
	m_camera.SetCamera(Vector3(17, 30, -15), Vector3(2.5f, 0, -1.5f), Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.0f, 10000.f);
	m_camera.SetViewPort(WINSIZE_X, WINSIZE_Y);

	g_global.m_main = this;

	cTerrainLoader loader(&m_terrain);
	loader.Read(m_renderer, "../media2/wall2.trn");

	// get all wall
	for (auto &tile : m_terrain.m_tiles)
	{
		tile->m_isRenderLine = false;
		for (auto &p : tile->m_children)
			if (p->m_name == "cube")
				m_walls.push_back(p);
	}

	for (auto &wall : m_walls)
		if (cCube *cube = dynamic_cast<cCube*>(wall))
			cube->m_color = cColor(0.8f, 0.8f, 0.8f, 1.f);

	{
		cBoundingBox bbox;
		bbox.SetBoundingBox(Vector3(0, 0, 0), Vector3(1, 1, 1), Quaternion());
		m_dbgAxis.Create(m_renderer);
		m_dbgAxis.SetAxis(bbox, false);
	}

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL);
	const Vector3 lightPos(-400, 800, -300);
	const Vector3 lightLookat(0, 0, 0);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection((lightLookat - lightPos).Normal());

	m_ccsm.Create(m_renderer);

	if (!m_navi.ReadFromPathFile("../media2/wallnavi3.txt"))
	{
		::MessageBoxA(m_hWnd, "Error Read Navigation Mesh file", "Error", MB_OK);
		return false;
	}

	m_pathLineList.Create(m_renderer, 128, cColor::BLUE);

	for (int i = 0; i < MAX_PLAYER; ++i)
	{
		cZealot *zealot = new cZealot();
		zealot->Create(m_renderer);
		zealot->m_name.Format("Zealot%d", i);
		//zealot->m_transform.pos = Vector3(i * 2, 0, 0);
		m_terrain.AddModel(zealot);
		m_zealots.push_back(zealot);
	}

	for (auto &p : m_zealots)
		m_group.m_brain->AddActor(p->m_brain);

	m_nodeLineList.Create(m_renderer, 128);
	MakeLineList(m_renderer, m_navi, m_nodeLineList);

	m_nodeTextMgr.Create(1024, 512);

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	GetMainCamera().Update(deltaSeconds);
	m_terrain.Update(m_renderer, deltaSeconds);
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

		m_pathLineList.Render(m_renderer);

		m_nodeTextMgr.NewFrame();
		if (1)
		{
			m_renderer.m_dbgBox.SetColor(cColor::WHITE);

			for (u_int i=0; i < m_navi.m_vertices.size(); ++i)
			{
				auto &vtx = m_navi.m_vertices[i];

				// Render Vertex Position
				{
					Transform tfm;
					tfm.pos = vtx;
					tfm.scale = Vector3(1, 1, 1)*0.2f;
					m_renderer.m_dbgBox.SetBox(tfm);
					m_renderer.m_dbgBox.Render(m_renderer);
				}

				// Render Vertex Number
				{
					Transform tfm;
					tfm.pos = vtx;
					tfm.scale = Vector3(1, 1, 1)*0.3f;

					WStr32 str;
					str.Format(L"%d", i);
					m_renderer.m_textMgr.AddTextRender(m_renderer, i, str.c_str()
						, cColor::YELLOW, cColor::BLACK, BILLBOARD_TYPE::ALL_AXIS, tfm, true);
				}
			}

			// Render Node Number
			for (u_int i=0; i < m_navi.m_naviNodes.size(); ++i)
			{
				auto &node = m_navi.m_naviNodes[i];
				const Vector3 center = node.center;

				Transform tfm;
				tfm.pos = center;
				tfm.pos.y += 0.3f;
				tfm.scale = Vector3(1, 1, 1)*0.2f;
				WStrId strId;
				strId.Format(L"%d", i);

				// if node path, red color
				auto it = std::find(m_group.m_nodePath.begin(), m_group.m_nodePath.end(), i);
				const cColor color = (m_group.m_nodePath.end() == it) ? cColor(0.f, 1.f, 0.f) : cColor(1.f, 0.f, 0.f);

				m_nodeTextMgr.AddTextRender(m_renderer, i, strId.c_str()
					, color
					, cColor(0.f, 0.f, 0.f)
					, BILLBOARD_TYPE::ALL_AXIS
					, tfm, true);
			}

			// Render Node Line
			{
				Transform tfm;
				tfm.pos = Vector3(0, 0.2f, 0);
				m_nodeLineList.Render(m_renderer, tfm.GetMatrixXM());
			}
		}
		m_nodeTextMgr.Render(m_renderer);



		m_dbgAxis.Render(m_renderer);
		m_renderer.RenderFPS();

		m_renderer.EndScene();
		m_renderer.Present();
	}
}


// Node 
void cViewer::MakeLineList(graphic::cRenderer &renderer
	, const ai::cNavigationMesh &navi
	, OUT graphic::cDbgLineList &out)
{
	set<int> vertices; // key: vertex index1*size() - vertex index2

	for (auto &node : navi.m_naviNodes)
	{
		const int indices[] = { node.idx1, node.idx2, node.idx3 };
		for (int i = 0; i < 3; ++i)
		{
			const int vtx1 = indices[i];
			const int vtx2 = indices[(i + 1) % 3];
			const int key1 = vtx1 * navi.m_vertices.size() + vtx2;
			const int key2 = vtx2 * navi.m_vertices.size() + vtx1;
			auto it1 = vertices.find(key1);
			auto it2 = vertices.find(key2);
			if ((vertices.end() != it1) || (vertices.end() != it2))
				continue;

			vertices.insert(key1);
			vertices.insert(key2);

			out.AddLine(renderer, navi.m_vertices[vtx1], navi.m_vertices[vtx2], false);
		}
	}

	out.UpdateBuffer(renderer);
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


void cViewer::UpdateLookAt()
{
	GetMainCamera().MoveCancel();

	const float centerX = GetMainCamera().m_width / 2;
	const float centerY = GetMainCamera().m_height / 2;
	const Ray ray = GetMainCamera().GetRay((int)centerX, (int)centerY);
	const float distance = m_groundPlane1.Collision(ray.dir);
	if (distance < -0.2f)
	{
		GetMainCamera().m_lookAt = m_groundPlane1.Pick(ray.orig, ray.dir);
	}
	else
	{ // horizontal viewing
		const Vector3 lookAt = GetMainCamera().m_eyePos + GetMainCamera().GetDirection() * 50.f;
		GetMainCamera().m_lookAt = lookAt;
	}

	GetMainCamera().UpdateViewMatrix();
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
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		const Ray ray = graphic::GetMainCamera().GetRay(m_curPos.x, m_curPos.y);
		const Vector3 lookPos = m_groundPlane1.Pick(ray.orig, ray.dir);
		const float len = GetMainCamera().GetEyePos().Distance(lookPos);
		const float zoomLen = len / 6.f;
		if ((zDelta > 0) && GetMainCamera().GetEyePos().y < 1.f)
			break;

		graphic::GetMainCamera().Zoom(ray.dir, (zDelta<0) ? -zoomLen : zoomLen);
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
		{
			if (RUN == m_state)
				Pause();
			else
				Resume();
		}
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
		UpdateLookAt();

		SetCapture(m_hWnd);
		m_RButtonDown = true;
		m_curPos.x = pos.x;
		m_curPos.y = pos.y;

		if ((!GetAsyncKeyState(VK_LCONTROL)) && (m_state != PAUSE))
		{
			const Ray ray = GetMainCamera().GetRay(pos.x, pos.y);
			Plane ground(Vector3(0, 1, 0), 0);
			Vector3 dest = ground.Pick(ray.orig, ray.dir);
			dest.y = 0.f;

			if ((!GetAsyncKeyState(VK_LCONTROL)) && (m_state == RUN))
				m_group.m_brain->Move(dest);
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

		if (m_LButtonDown)
		{
			const int x = pos.x - m_curPos.x;
			const int y = pos.y - m_curPos.y;

			if ((abs(x) > 1000) || (abs(y) > 1000))
				break;

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
			if (GetAsyncKeyState(VK_LCONTROL) || (PAUSE == m_state))
			{
				m_camera.Yaw2(x * 0.005f);
				m_camera.Pitch2(y * 0.005f);
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
