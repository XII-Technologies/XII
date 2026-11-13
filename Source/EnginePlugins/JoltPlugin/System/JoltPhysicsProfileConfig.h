/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/Math/Vec3.h>

/// Defaults copied when a world creates its Jolt module. A live world owns its native settings.
class XII_JOLTPLUGIN_DLL xiiJoltPhysicsProfileConfig final : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltPhysicsProfileConfig, xiiProfileConfigData);

public:
  inline static constexpr double    s_fDefaultFixedStep              = 1.0 / 60.0;
  inline static constexpr xiiUInt32 s_uiDefaultMaxCatchUpSteps       = 4;
  inline static constexpr xiiUInt32 s_uiDefaultMaxBodies             = 4096;
  inline static constexpr xiiUInt32 s_uiDefaultMaxBodyPairs          = 16384;
  inline static constexpr xiiUInt32 s_uiDefaultMaxContactConstraints = 8192;

  xiiVec3   m_vGravity                = xiiVec3(0.0f, 0.0f, -9.81f);
  double    m_fFixedStep              = s_fDefaultFixedStep;
  xiiUInt32 m_uiMaxCatchUpSteps       = s_uiDefaultMaxCatchUpSteps;
  xiiUInt32 m_uiMaxBodies             = s_uiDefaultMaxBodies;
  xiiUInt32 m_uiMaxBodyPairs          = s_uiDefaultMaxBodyPairs;
  xiiUInt32 m_uiMaxContactConstraints = s_uiDefaultMaxContactConstraints;

  xiiResult Validate() const override;
  void      SaveRuntimeData(xiiChunkStreamWriter& ref_stream) const override;
  void      LoadRuntimeData(xiiChunkStreamReader& ref_stream) override;
};
