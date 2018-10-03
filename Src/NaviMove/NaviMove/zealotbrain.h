//
// 2017-10-30, jjuiddong
//
#pragma once

#include "zealot.h"


class cZealotBrain : public ai::cBrain<cZealot>
{
public:
	cZealotBrain(cZealot *agent);
	virtual ~cZealotBrain();

};
