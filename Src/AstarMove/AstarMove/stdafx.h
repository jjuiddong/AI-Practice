#pragma once


#include "../../../../Common/Common/common.h"
using namespace common;
#include "../../../../Common/Graphic11/graphic11.h"
#include "../../../../Common/Framework11/framework11.h"

extern framework::cGameMain* g_application;

#include "zealotgroup.h"
#include "zealot.h"
#include "zealotai.h"
#include "astarmove.h"


extern ai::cPathFinder g_pathFinder;
extern vector<Vector3> g_route;
