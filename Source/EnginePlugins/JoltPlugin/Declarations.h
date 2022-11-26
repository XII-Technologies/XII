#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Enum.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct XII_JOLTPLUGIN_DLL xiiJoltSteppingMode
{
  typedef xiiUInt32 StorageType;

  enum Enum
  {
    Variable,
    Fixed,
    SemiFixed,

    Default = SemiFixed
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiJoltSteppingMode);

//////////////////////////////////////////////////////////////////////////

struct xiiOnJoltContact
{
  typedef xiiUInt32 StorageType;

  enum Enum
  {
    None = 0,
    //SendReportMsg = XII_BIT(0),
    ImpactReactions = XII_BIT(1),
    SlideReactions  = XII_BIT(2),
    RollXReactions  = XII_BIT(3),
    RollYReactions  = XII_BIT(4),
    RollZReactions  = XII_BIT(5),

    AllRollReactions      = RollXReactions | RollYReactions | RollZReactions,
    SlideAndRollReactions = AllRollReactions | SlideReactions,
    AllReactions          = ImpactReactions | AllRollReactions | SlideReactions,

    Default = None
  };

  struct Bits
  {
    StorageType SendReportMsg : 1;
    StorageType ImpactReactions : 1;
    StorageType SlideReactions : 1;
    StorageType RollXReactions : 1;
    StorageType RollYReactions : 1;
    StorageType RollZReactions : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiOnJoltContact);
XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiOnJoltContact);

//////////////////////////////////////////////////////////////////////////

struct xiiJoltSettings
{
  xiiVec3 m_vObjectGravity    = xiiVec3(0, 0, -9.81f);
  xiiVec3 m_vCharacterGravity = xiiVec3(0, 0, -12.0f);

  xiiEnum<xiiJoltSteppingMode> m_SteppingMode    = xiiJoltSteppingMode::SemiFixed;
  float                        m_fFixedFrameRate = 60.0f;
  xiiUInt32                    m_uiMaxSubSteps   = 4;

  xiiUInt32 m_uiMaxBodies = 1000 * 10;
};
