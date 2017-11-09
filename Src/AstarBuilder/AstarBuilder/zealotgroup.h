//
// 2017-10-30, jjuiddong
//
//
#pragma once

#include "zealot.h"


class cZealotGroupAI;
class cGroup : public ai::iActorInterface<cGroup>
{
public:
	cGroup();
	virtual ~cGroup();


public:
	cZealotGroupAI *m_ai;
};


class cZealotGroupAI : public ai::cActor<cGroup>
{
public:
	cZealotGroupAI(ai::iActorInterface<cGroup> *agent);
	virtual ~cZealotGroupAI();

	void Move(const Vector3 &dest);
};
