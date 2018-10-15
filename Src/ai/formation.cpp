
#include "stdafx.h"
#include "formation.h"

using namespace ai;


cFormation::cFormation()
{
}

cFormation::~cFormation()
{
	Clear();
}


bool cFormation::Create(const Vector3 &dest, const vector<cZealot*> &units)
{
	RETV(units.empty(), false);
	RETV(units.size() != 5, false);

	Clear();

	Vector3 center;
	for (auto &unit : units)
		center += unit->m_transform.pos;
	center /= units.size();
	center.y = 0.f;

	m_dir = (dest - center).Normal();
	m_dir.y = 0.f;
	m_dir.Normalize();
	assert(!m_dir.IsEmpty());

	// 
	//       /\  Direction
	//       ||
	// +----+----+----+
	// | O  |    | O  |
	// |    |    |    |
	// +----+----+----+
	// | O  |  O |  O |
	// |    |    |    |
	// +----+----+----+
	//
	// 5 unit
	// row,col = {0,0}, {0,2}, {1,0}, {1,1}, {1,2}
	// center row,col = {0,1}
	m_units[MakeKey(0, 0)] = { 0, 0 };
	m_units[MakeKey(0, 2)] = { 0, 2 };
	m_units[MakeKey(1, 0)] = { 1, 0 };
	m_units[MakeKey(1, 1)] = { 1, 1 };
	m_units[MakeKey(1, 2)] = { 1, 2 };

	const float gap = 1.f;
	vector<Vector3> fpos; // formation pos
	const Vector3 right = Vector3(0, 1, 0).CrossProduct(m_dir).Normal();
	const Vector3 back = -m_dir;
	center -= right * (gap * 1.5f);
	center += m_dir * gap;

	int cnt = 0;
	for (auto &kv : m_units)
	{
		const int row = kv.second.row;
		const int col = kv.second.col;
		const Vector3 pos = (right * (col * gap)) + (back * (row * gap));
		kv.second.pos = pos;
		kv.second.unit = units[cnt++];
	}

	m_pos = center;
	m_rows = 2;
	m_cols = 3;

	return true;
}


void cFormation::Clear()
{
	m_units.clear();
}


int cFormation::MakeKey(const int row, const int col)
{
	return row * 1000 + col;
}
