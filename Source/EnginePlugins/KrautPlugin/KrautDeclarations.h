#pragma once

#include <KrautPlugin/KrautPluginDLL.h>

struct xiiResourceEvent;

class xiiKrautLodInfo;
class xiiKrautRenderData;

class xiiKrautTreeComponent;
class xiiKrautTreeComponentManager;

enum class xiiKrautLodType : xiiUInt8
{
  None              = 0xFF,
  Mesh              = 0,
  StaticImpostor    = 1,
  BillboardImpostor = 2,
};

enum class xiiKrautMaterialType : xiiUInt8
{
  None   = 0xFF,
  Branch = 0,
  Frond  = 1,
  Leaf   = 2,
};

enum class xiiKrautBranchType : xiiUInt8
{
  None          = 0xFF,
  Trunk1        = 0,
  Trunk2        = 1,
  Trunk3        = 2,
  MainBranches1 = 3,
  MainBranches2 = 4,
  MainBranches3 = 5,
  SubBranches1  = 6,
  SubBranches2  = 7,
  SubBranches3  = 8,
  Twigs1        = 9,
  Twigs2        = 10,
  Twigs3        = 11,
};
