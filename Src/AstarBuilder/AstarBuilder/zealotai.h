//
// 2017-10-30, jjuiddong
//
//
#pragma once

#include "zealot.h"


class cZealotAI : public ai::cActor<cZealot>
{
public:
	cZealotAI(ai::iActorInterface<cZealot> *agent);
	virtual ~cZealotAI();

};
