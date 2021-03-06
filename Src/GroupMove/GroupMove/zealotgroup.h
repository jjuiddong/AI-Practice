//
// 2017-10-30, jjuiddong
//
//
#pragma once

#include "zealot.h"


class cZealotGroupBrain;
class cGroup
{
public:
	cGroup();
	virtual ~cGroup();


public:
	cZealotGroupBrain *m_ai;
};


class cZealotGroupBrain : public ai::cBrain<cGroup>
{
public:
	cZealotGroupBrain(cGroup *agent);
	virtual ~cZealotGroupBrain();

	void Move(const Vector3 &dest);
};
