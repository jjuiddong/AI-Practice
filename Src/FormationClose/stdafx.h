#pragma once


#include "../../../Common/Common/common.h"
using namespace common;
#include "../../../Common/Graphic11/graphic11.h"
#include "../../../Common/Framework11/framework11.h"
#include "../../../Common/AI/ai.h"

extern framework::cGameMain* g_application;

#include "../ai/define.h"
#include "../ai/zealot.h"
#include "../ai/zealotgroup.h"
#include "../ai/zealotbrain.h"
#include "../ai/aiglobal.h"
#include "../ai/groupmove.h"
#include "formationclose.h"


extern ai::cPathFinder g_pathFinder;
extern vector<Vector3> g_route;
