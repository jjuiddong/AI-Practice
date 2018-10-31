//
// 2018-10-15, jjuiddong
// formation
//
//
//         /\  Direction
//         ||
// +----+----+----+----+
// |Row0|    |    |    |
// |Col0|Col1|Col2|Col3|
// +----+----+----+----+
// |Row1|    |    |    |
// |    |    |    |    |
// +----+----+----+----+
// |Row2|    |    |    |
// |    |    |    |    |
// +----+----+----+----+
// |Row3|    |    |    |
// |    |    |    |    |
// +----+----+----+----+
#pragma once


class cZealot;

namespace ai
{

	class cFormation
	{
	public:
		cFormation();
		virtual ~cFormation();

		bool Create(const Vector3 &dest, const vector<cZealot*> &units);
		void Clear();


	protected:
		inline int MakeKey(const int row, const int col);
		//void Sort(const Vector3 &dest, const vector<cZealot*> &units);


	public:
		Vector3 m_pos; // formation center position
		Vector3 m_dir;
		int m_rows; // row count
		int m_cols; // column count

		struct sInfo {
			int row;
			int col;
			Vector3 pos;  // relation pos to m_pos
			cZealot *unit;
		};
		map<int, sInfo> m_units; //key = col + row*1000
		//vector<cZealot*> m_sortUnits; // sort unit reference, most near dest position unit
	};

}
