//
// Wall Move
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
	std::pair<bool, float> RecursiveCheck_Collision(const Vector3 &curPos, const Vector3 &movDir
		, const Vector3 &dest, const float totalMovLen, const int cnt);

	std::pair<bool, float> RecursiveCheck_Move(const Vector3 &curPos, const Vector3 &movDir
		, const Vector3 &dest, const float totalMovLen, const int cnt);

	std::pair<bool, float> RecursiveCheck_Ray(const Vector3 &curPos, const Vector3 &nextPos
		, const Vector3 &dest, const float totalMovLen, const int cnt);

	bool IsPassThrough(const Vector3 &curPos, const Vector3 &dir);
	std::pair<cNode*,float> GetNearestWall(const Vector3 &curPos, const Vector3 &nextPos);
	


public:
	cCamera3D m_camera;
	cCube m_cube3;
	cSphere m_sphere;
	cSphere m_sphere2; // Collision Position
	cDbgAxis m_dbgAxis;
	cTerrain m_terrain;
	bool m_isCollisionPosition;
	vector<cNode*> m_walls;
	cCascadedShadowMap m_ccsm;

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
	m_windowName = L"AI Wall Move";
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
	loader.Read(m_renderer, "../media2/wall2.trn");

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

	{
		cBoundingBox bbox;
		bbox.SetBoundingBox(Vector3(0, 0, 0), Vector3(.2f, 1.f, .2f), Quaternion());
		m_cube3.Create(m_renderer, bbox, eVertexType::POSITION | eVertexType::NORMAL);
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

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	GetMainCamera().Update(deltaSeconds);
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

		// Sphere Move to Center
		Vector3 dest;
		if (m_curIdx >= 0)
		{
			dest = m_path[m_curIdx];
			const Vector3 curPos = m_sphere.m_transform.pos;
			if ((curPos.LengthRoughly(dest) < 0.1f)
				|| (curPos.LengthRoughly(m_dest) < 0.1f))
			{
				--m_curIdx;
				if (m_curIdx < 0)
				{
					m_movState = eMoveState::WAIT;
				}
				else
				{
					m_movState = eMoveState::MOVE;
				}
			}

			if (eMoveState::CMOVE == m_movState)
			{
				//목적지 방향으로 방해물이 없다면, 목적지로 전진한다.
				const Vector3 viewDir = (m_path[m_curIdx - 1] - curPos).Normal();
				const Vector3 rightV = Vector3(0, 1, 0).CrossProduct(viewDir).Normal();
				const Vector3 leftV = -rightV;
				const Vector3 curPosL = curPos + Vector3(0,0.5f,0) + leftV * m_sphere.GetRadius();
				const Vector3 curPosR = curPos + Vector3(0, 0.5f, 0) + rightV * m_sphere.GetRadius();

				const Ray ray1(curPosL, viewDir);
				const Ray ray2(curPosR, viewDir);

				// 가장 가까운 방해물을 검색한다.
				float nearLen1 = FLT_MAX;
				float nearLen2 = FLT_MAX;
				for (u_int i = 0; i < m_walls.size(); ++i)
				{
					cBoundingBox bbox = m_walls[i]->m_boundingBox;
					bbox *= m_walls[i]->GetWorldMatrix();
					//auto &bbox = bboxes[i];
					float dist = FLT_MAX;
					if (bbox.Pick(ray1, &dist))
						if (nearLen1 > dist)
							nearLen1 = dist;
					if (bbox.Pick(ray2, &dist))
						if (nearLen2 > dist)
							nearLen2 = dist;
				}

				const float limitR = m_sphere.GetRadius() * 2.5f;
				if ((nearLen1 > limitR) && (nearLen2 > limitR))
				{
					--m_curIdx;
					m_movState = eMoveState::MOVE;
				}
			}

			if ((eMoveState::CMOVE == m_movState) || (eMoveState::MOVE == m_movState))
			{
				dest = m_path[m_curIdx];
				Vector3 v = (dest - m_sphere.m_transform.pos);
				v.y = 0;
				v.Normalize();
				const Vector3 mov = v * 3.f * deltaSeconds;
				m_dir = v;
				m_sphere.m_transform.pos += mov;
			}
		}


		Vector3 collisionPos;
		Plane collisionPlane;
		m_isCollisionPosition = false;

		const float reflectLen = 0.1f;
		cBoundingSphere bsphere = m_sphere.m_boundingSphere;
		bsphere.SetPos(m_sphere.m_transform.pos);

		for (auto &wall : m_walls)
		{
			cBoundingBox bbox = wall->m_boundingBox;
			bbox *= wall->GetWorldMatrix();

			Vector3 collisionVertex1, collisionVertex2;
			if (bbox.Collision2D(bsphere, &collisionPos, &collisionPlane
				, &collisionVertex1, &collisionVertex2))
			{
				m_isCollisionPosition = true;
				m_sphere2.m_transform.pos = collisionPos;
				const Vector3 cpos = collisionPos + collisionPlane.N * (m_sphere.GetRadius() + reflectLen);
				//const Matrix44 ref = collisionPlane.GetReflectMatrix();
				//const Vector3 reflectDir = m_dir.MultiplyNormal(ref);

				// 꼭지점보다 구의 지름만큼 더 가서 선회한다.
				Vector3 v1 = (collisionVertex1 - collisionPos);
				const float len1 = v1.Length();
				v1.Normalize();
				Vector3 v2 = (collisionVertex2 - collisionPos);
				const float len2 = v2.Length();
				v2.Normalize();

				const Vector3 nv1 = (v1.IsEmpty()) ? -v2 : v1;
				const Vector3 nv2 = (v2.IsEmpty()) ? -v1 : v2;
				const Vector3 nextPos1 = cpos + (collisionPlane.N*reflectLen) + nv1 * (len1 + m_sphere.GetRadius() * 1.5f);
				const Vector3 nextPos2 = cpos + (collisionPlane.N*reflectLen) + nv2 * (len2 + m_sphere.GetRadius() * 1.5f);

				const std::pair<bool, float> r1 = RecursiveCheck_Ray(cpos, nextPos1, m_dest, 0.f, 10);
				const std::pair<bool, float> r2 = RecursiveCheck_Ray(cpos, nextPos2, m_dest, 0.f, 10);
				
				Vector3 newPos;
				if (r1.first && r2.first)
				{
					newPos = (r1.second < r2.second) ? nextPos1 : nextPos2;
				}
				else if (r1.first || r2.first)
				{
					newPos = r1.first ? nextPos1 : nextPos2;
				}
				else
				{
					newPos = (r1.second < r2.second) ? nextPos1 : nextPos2;
				}

				// 충돌이 일어난 위치로 튕겨나간다.
				m_sphere.m_transform.pos = cpos;


				m_path.clear();
				m_path.push_back(m_dest);
				m_path.push_back(newPos);
				m_curIdx = 1;
				m_movState = eMoveState::CMOVE;
				break;
			}
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
		m_renderer.GetDevContext()->RSSetState(m_isWireframe? state.Wireframe() : state.CullCounterClockwise());
		m_terrain.SetTechnique("ShadowMap");
		m_terrain.RenderCascadedShadowMap(m_renderer, m_ccsm);
		
		m_cube3.m_color = cColor::RED;
		m_cube3.m_transform.pos = m_dest;
		m_cube3.Render(m_renderer);
		m_cube3.m_color = cColor::BLUE;
		m_cube3.m_transform.pos = dest;
		m_cube3.Render(m_renderer);

		m_sphere.Render(m_renderer);
		m_sphere2.Render(m_renderer);
		
		m_dbgAxis.Render(m_renderer);
		m_renderer.RenderFPS();

		m_renderer.EndScene();
		m_renderer.Present();
	}
}


// curPos에서 dest까지 도착하는데 필요한 가장 짧은 거리를, 재귀적으로 검색해서, 리턴한다.
// 못찾으면, false, 이동거리+목적지까지거리 를 리턴한다.
// 찾으면, true, 이동거리 를 리턴한다.
// cnt 만큼 반복한다.
std::pair<bool, float> cViewer::RecursiveCheck_Ray(const Vector3 &curPos, const Vector3 &nextPos
	, const Vector3 &dest, const float totalMovLen, const int cnt)
{
	if (curPos.LengthRoughly(dest) < 0.1f)
		return{ true, totalMovLen };

	if (cnt <= 0)
		return{ false, totalMovLen + curPos.Distance(dest) };

	auto result = GetNearestWall(curPos, nextPos);
	float nearDist = result.second;
	cNode *nearWall = result.first;

	const Ray ray(curPos, (nextPos - curPos).Normal());

	// 벽과 충돌 했을 때.
	if (nearWall)
	{
		const float nextMoveLen = curPos.Distance(nextPos);
		if (nextMoveLen < nearDist)
		{
			// 벽과 충돌 없이 nextPos에 도착했을 때.
			// 다음 목적지를 검사한다.
			return RecursiveCheck_Ray(nextPos, dest, dest, totalMovLen + curPos.Distance(nextPos), cnt - 1);
		}
		else
		{
			const float reflectLen = 0.01f;
			if (nearDist <= (m_sphere.GetRadius() + (reflectLen+0.05f)))
			{
				// 벽이 바로 앞에 있을 경우에는, 경로에서 제외 시킨다.
				//dbg::Log("end of path\n");
				return{ false, FLT_MAX };
			}

			// nextPos로 가는 도중에 벽과 부딪쳤을 때.
			// 부딪친 위치에서 목적지로 선회 한다.
			cBoundingSphere bsphere = m_sphere.m_boundingSphere;

			const Vector3 pos = curPos + ray.dir * (nearDist - (m_sphere.GetRadius()/10.f));
			bsphere.SetPos(pos);

			cBoundingBox bbox = nearWall->m_boundingBox;
			bbox *= nearWall->GetWorldMatrix();
			
			Vector3 collisionPos, collisionVertex1, collisionVertex2;
			Plane collisionPlane;
			if (bbox.Collision2D(bsphere, &collisionPos, &collisionPlane
				, &collisionVertex1, &collisionVertex2))
			{
				// 벽과 충돌 했다면,
				// 벽의 면을 따라 이동한다. (왼쪽 or 오른쪽)
				// 목적지에 가까운 경로의 거리를 리턴한다.
				const Vector3 cpos = collisionPos + collisionPlane.N * (m_sphere.GetRadius() + reflectLen);
				const Vector3 leftV = Vector3(0, 1, 0).CrossProduct(collisionPlane.N).Normal();
				const Vector3 rightV = -leftV;

				// 꼭지점보다 구의 지름만큼 더 가서 선회한다.
				Vector3 v1 = (collisionVertex1 - collisionPos);
				const float len1 = v1.Length();
				v1.Normalize();
				Vector3 v2 = (collisionVertex2 - collisionPos);
				const float len2 = v2.Length();
				v2.Normalize();

				const Vector3 nv1 = (v1.IsEmpty()) ? -v2 : v1;
				const Vector3 nv2 = (v2.IsEmpty()) ? -v1 : v2;
				const Vector3 nextPos1 = cpos + nv1 * (len1 + m_sphere.GetRadius() * 2.f);
				const Vector3 nextPos2 = cpos + nv2 * (len2 + m_sphere.GetRadius() * 2.f);

				std::pair<bool, float> r1 = RecursiveCheck_Ray(cpos, nextPos1, dest
					, totalMovLen + nearDist, cnt - 1);
				std::pair<bool, float> r2 = RecursiveCheck_Ray(cpos, nextPos2, dest
					, totalMovLen + nearDist, cnt - 1);

				if (r1.first && r2.first)
				{
					return{ true, min(r1.second, r2.second) };
				}
				else if (r1.first || r2.first)
				{
					return{ true, r1.first ? r1.second : r2.second };
				}
				else
				{
					return{ false, min(r1.second, r2.second) };
				}
			}
		}
	}
	else
	{
		// 벽과 충돌없이 nextPos에 도착했을 때.
		// 다음 목적지를 검사한다.
		return RecursiveCheck_Ray(nextPos, dest, dest, totalMovLen + curPos.Distance(nextPos), cnt-1);
	}

	return{false, FLT_MAX};
}


// cnt가 0보다 클때까지 재귀호출한다.
// 목적지까지의 이동 거리를 리턴한다. (작은 값이 더 나은 경로다.)
std::pair<bool, float> cViewer::RecursiveCheck_Collision(const Vector3 &curPos, const Vector3 &movDir
	, const Vector3 &dest, const float totalMovLen, const int cnt)
{
	if (cnt <= 0)
		return{ false, totalMovLen + curPos.Distance(dest) };

	// movDir 방향으로 움직여가면서, dest 방향으로 나아갈수 있는지 확인한다.
	// dest 방향으로 갈수 있다면, 재귀적으로 따라간다.
	float movLen = 1.f;
	while (movLen < 10.f)
	{
		const Vector3 pos = curPos + movDir*movLen;
		Vector3 dir = dest - pos;
		dir.y = 0;
		dir.Normalize();

		if (dest.LengthRoughly(pos) < 0.1f)
			return{ true, totalMovLen + movLen }; // 목적지 도착

		if (IsPassThrough(pos, dir))
		{
			const auto result = RecursiveCheck_Move(pos, dir, dest, totalMovLen+movLen, cnt - 1);
			return result;
		}

		movLen += 1.f;
	}

	return{ false, FLT_MAX };
}


// 충돌 체크를 하면서 movDir 방향으로 전진한다.
// 목적지까지의 이동 거리를 리턴한다. (작은 값이 더 나은 경로다.)
std::pair<bool, float> cViewer::RecursiveCheck_Move(const Vector3 &curPos, const Vector3 &movDir
	, const Vector3 &dest, const float totalMovLen, const int cnt)
{
	const float reflectLen = 0.1f;
	cBoundingSphere bsphere = m_sphere.m_boundingSphere;

	// movDir 방향으로 전진하면서, 벽과 충돌이 있는지 확인한다.
	// 목적지에 도착할 때까지 전진.
	float distance = FLT_MAX;
	float movLen = 0.f;

	while (movLen < 100.f)
	{
		const Vector3 pos = curPos + movDir * movLen;
		bsphere.SetPos(pos);

		if (dest.LengthRoughly(pos) < 0.1f)
			return{ true, totalMovLen + movLen }; // 목적지 도착

		for (auto &wall : m_walls)
		{
			cBoundingBox bbox = wall->m_boundingBox;
			bbox *= wall->GetWorldMatrix();

			Vector3 collisionPos;
			Plane collisionPlane;
			if (bbox.Collision2D(bsphere, &collisionPos, &collisionPlane))
			{
				// 벽과 충돌 했다면,
				// 벽의 면을 따라 이동한다. (왼쪽 or 오른쪽)
				// 목적지에 가까운 경로의 거리를 리턴한다.
				const Vector3 cpos = collisionPos + collisionPlane.N * (m_sphere.GetRadius() + reflectLen);
				const Matrix44 ref = collisionPlane.GetReflectMatrix();
				const Vector3 reflectDir = movDir.MultiplyNormal(ref);

				const Vector3 leftV = Vector3(0, 1, 0).CrossProduct(collisionPlane.N).Normal();
				const Vector3 rightV = -leftV;
				const Vector3 movDir = (reflectDir.DotProduct(rightV) >= 0) ? rightV : leftV;

				const std::pair<bool, float> r1 = RecursiveCheck_Collision(cpos, leftV, dest, totalMovLen + movLen, cnt);
				const std::pair<bool, float> r2 = RecursiveCheck_Collision(cpos, rightV, dest, totalMovLen + movLen, cnt);

				if (r1.first && r2.first)
				{
					return{ true, min(r1.second, r2.second) };
				}
				else if (r1.first || r2.first)
				{
					return{ true, r1.first ? r1.second : r2.second };
				}
				else
				{
					return{ false, min(r1.second, r2.second) };
				}				
			}
		}

		movLen += 1.f;
	}

	// 목적지에 도착 못 했다면, 이 경로는 무시되어야 한다.
	return{ false, FLT_MAX };
}


// 가장 가까운 벽과 그 거리를 리턴한다.
// 없다면 NULL, FLT_MAX 를 리턴한다.
std::pair<cNode*, float> cViewer::GetNearestWall(const Vector3 &curPos, const Vector3 &nextPos)
{
	//목적지 방향으로 방해물이 있는지 검사하고, 있다면 방해물과 거리를 리턴한다.
	const Vector3 dir = (nextPos - curPos).Normal();
	const Vector3 rightV = Vector3(0, 1, 0).CrossProduct(dir).Normal();
	const Vector3 leftV = -rightV;
	const Vector3 curPosL = curPos + Vector3(0, 0.5f, 0) + leftV * m_sphere.GetRadius();
	const Vector3 curPosR = curPos + Vector3(0, 0.5f, 0) + rightV * m_sphere.GetRadius();

	const Ray ray1(curPosL, dir);
	const Ray ray2(curPosR, dir);
	const Ray ray3(curPos, dir);

	// 가장 가까운 방해물을 검색한다.
	cNode *nearWall = NULL;
	float nearLen = FLT_MAX;
	for (u_int i = 0; i < m_walls.size(); ++i)
	{
		cBoundingBox bbox = m_walls[i]->m_boundingBox;
		bbox *= m_walls[i]->GetWorldMatrix();

		float dist1 = FLT_MAX, dist2 = FLT_MAX, dist3 = FLT_MAX;
		bbox.Pick(ray1, &dist1);
		bbox.Pick(ray2, &dist2);
		bbox.Pick(ray3, &dist3);

		if (nearLen > min(dist1, min(dist2, dist3)))
		{
			nearLen = min(dist1, min(dist2, dist3));
			nearWall = m_walls[i];
		}
	}

	return{ nearWall, nearLen };
}


bool cViewer::IsPassThrough(const Vector3 &curPos, const Vector3 &dir)
{
	//목적지 방향으로 방해물이 없다면, 목적지로 전진한다.
	const Vector3 rightV = Vector3(0, 1, 0).CrossProduct(dir).Normal();
	const Vector3 leftV = -rightV;
	const Vector3 curPosL = curPos + Vector3(0, 0.5f, 0) + leftV * m_sphere.GetRadius();
	const Vector3 curPosR = curPos + Vector3(0, 0.5f, 0) + rightV * m_sphere.GetRadius();

	const Ray ray1(curPosL, dir);
	const Ray ray2(curPosR, dir);

	// 가장 가까운 방해물을 검색한다.
	float nearLen1 = FLT_MAX;
	float nearLen2 = FLT_MAX;
	for (u_int i = 0; i < m_walls.size(); ++i)
	{
		cBoundingBox bbox = m_walls[i]->m_boundingBox;
		bbox *= m_walls[i]->GetWorldMatrix();

		float dist = FLT_MAX;
		if (bbox.Pick(ray1, &dist))
			if (nearLen1 > dist)
				nearLen1 = dist;
		if (bbox.Pick(ray2, &dist))
			if (nearLen2 > dist)
				nearLen2 = dist;
	}

	const float limitR = m_sphere.GetRadius() * 2.5f;
	if ((nearLen1 > limitR) && (nearLen2 > limitR))
	{
		return true;
	}

	return false;
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
			m_path.push_back(m_dest);
			m_movState = eMoveState::MOVE;
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
