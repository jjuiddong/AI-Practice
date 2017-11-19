//
// AStar Move
//
#include "stdafx.h"
#include "astarmove.h"
#include "move2.h"

using namespace graphic;

framework::cDbgMonitor g_dbgMonitor;
ai::cPathFinder g_pathFinder;
vector<Vector3> g_route;

INIT_FRAMEWORK(cViewer);


cViewer::cViewer()
	: m_groundPlane1(Vector3(0, 1, 0), 0)
	, m_camera("main camera")
	, m_editType(0)
{
	m_windowName = L"Astar Move";
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
	m_camera.SetCamera(Vector3(-10, 30, -10), Vector3(10, 0, 10), Vector3(0, 1, 0));
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
	const Vector3 lightPos(-400, 600, -300);
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
		Vector3(2,0,2)
		, Vector3(0,0,0)
		, Vector3(4,0,0)
		, Vector3(4,0,4)
		, Vector3(0,0,4)
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

	for (auto &p : m_zealots)
		m_group.m_ai->AddActor(p->m_ai);

	for (auto &node : m_terrain.m_children)
	{
		if (cRect3D *p = dynamic_cast<cRect3D*>(node))
		{
			// Update BoundingBox for Collision
			const Vector3 vw = p->m_pos[1] - p->m_pos[0];
			const Vector3 vh = p->m_pos[2] - p->m_pos[1];
			const float w = vw.Length() / 2.f;
			const float h = vh.Length() / 2.f;
			const Vector3 center = (p->m_pos[0] + p->m_pos[2]) / 2.f;

			Transform tfm;
			tfm.pos = center;
			tfm.scale = Vector3(w, 1, h);
			p->m_boundingBox.SetBoundingBox(tfm);
			m_areas.push_back(p);

			// 2차원 rect로 변환한 후, pathfinder에 추가한다.
			// pathfinder는 area를 바탕으로 길을 탐색한다.
			sRectf rect;
			rect.left = center.x - w;
			rect.top = center.z - h;
			rect.right = center.x + w;
			rect.bottom = center.z + h;
			g_pathFinder.AddArea(rect);
		}
	}

	m_walls.reserve(128);
	vector<cNode*> stack;
	stack.reserve(64);
	for (auto &node : m_terrain.m_children)
		stack.push_back(node);
	while (!stack.empty())
	{
		cNode *node = stack.back(); stack.pop_back();
		if (cCube *p = dynamic_cast<cCube*>(node))
			m_walls.push_back(p);

		for (auto &p : node->m_children)
			stack.push_back(p);
	}

	g_dbgMonitor.Create(m_renderer, "DebugMonitorShmem::jjuiddong");

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	cAutoCam cam(&m_camera);
	GetMainCamera().Update(deltaSeconds);

	for (auto &p : m_zealots)
		p->Update(m_renderer, deltaSeconds);

	g_dbgMonitor.UpdateApp(m_renderer);
}


void cViewer::OnRender(const float deltaSeconds)
{
	cAutoCam cam(&m_camera);

	m_gui.NewFrame();

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

		cColor colors[] = {
			cColor::RED
			, cColor::GREEN
			, cColor::BLUE
			, cColor::YELLOW
			, cColor::BLACK
		};

		//for (u_int i=0; i < m_zealots.size(); ++i)
		//{
		//	cZealot *p = m_zealots[i];
		//	for (auto &vtx : p->m_route)
		//	{
		//		const cBoundingBox bbox(vtx + Vector3(i*0.05f,0,0)
		//			, Vector3(1, 10, 1) * 0.1f
		//			, Quaternion());
		//		m_renderer.m_dbgBox.SetColor(colors[i]);
		//		m_renderer.m_dbgBox.SetBox(bbox);
		//		m_renderer.m_dbgBox.Render(m_renderer);
		//	}
		//}

		// Render Zealot Moving Path
		for (auto &vtx : g_route)
		{
			const cBoundingBox bbox(vtx + Vector3(0, 0, 0)
				, Vector3(1, 10, 1) * 0.1f
				, Quaternion());
			m_renderer.m_dbgBox.SetColor(colors[0]);
			m_renderer.m_dbgBox.SetBox(bbox);
			m_renderer.m_dbgBox.Render(m_renderer);
		}

		// Render Zealot Moving Target
		for (u_int i = 0; i < m_zealots.size(); ++i)
		{
			m_renderer.m_dbgSphere.m_color = colors[i];
			m_renderer.m_dbgSphere.SetRadius(.3f);
			m_renderer.m_dbgSphere.SetPos(m_zealots[i]->m_nextPos + Vector3(i*0.05f, 0, 0));
			m_renderer.m_dbgSphere.Render(m_renderer);

			Vector3 dir = m_zealots[i]->m_dir + m_zealots[i]->m_transform.pos + Vector3(0,0.5f,0);
			m_renderer.m_dbgArrow.SetDirection(m_zealots[i]->m_transform.pos+ Vector3(0, 0.5f, 0), dir, 0.1f);
			m_renderer.m_dbgArrow.m_color = colors[i];
			m_renderer.m_dbgArrow.Render(m_renderer);
		}

		// Collision
		for (auto &wall : m_walls)
			wall->m_color = cColor::WHITE;

		for (auto &zealot : m_zealots)
		{
			if (!zealot->m_isLoaded)
				continue;

			cBoundingBox bbox1 = zealot->m_boundingBox;
			bbox1 *= zealot->GetWorldMatrix();

			for (auto &wall : m_walls)
			{
				cBoundingBox bbox2 = wall->m_boundingBox;
				bbox2 *= wall->GetWorldMatrix();
				if (bbox1.Collision(bbox2))
					wall->m_color = cColor::RED;
			}
		}

		// Render Zealot
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
		case VK_TAB:
		{
			if (m_slowFactor < 1.f)
				m_slowFactor = 1.f;
			else
				m_slowFactor = 0.2f;
		}
		break;

		case VK_RETURN: break;

		case VK_SPACE:
		{
			if (m_state == RUN)
				Pause();
			else
				Resume();
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

		if ((!GetAsyncKeyState(VK_LCONTROL)) && (m_state == RUN))
			m_group.m_ai->Move(p1);
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

			if (GetAsyncKeyState(VK_LCONTROL) || (PAUSE == m_state))
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

		m_curPos = pos;
	}
	break;
	}
}

