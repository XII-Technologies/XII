#pragma once

#include <Core/World/WorldModule.h>

struct XII_CORE_DLL xiiWindStrength
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Calm,
    LightBreeze,
    GentleBreeze,
    ModerateBreeze,
    StrongBreeze,
    Storm,
    WeakShockwave,
    MediumShockwave,
    StrongShockwave,
    ExtremeShockwave,

    Default = LightBreeze
  };

  static float GetInMetersPerSecond(xiiWindStrength::Enum strength);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiWindStrength);

class XII_CORE_DLL xiiWindWorldModuleInterface : public xiiWorldModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiWindWorldModuleInterface, xiiWorldModule);

protected:
  xiiWindWorldModuleInterface(xiiWorld* pWorld);

public:
  virtual xiiVec3 GetWindAt(const xiiVec3& vPosition) const = 0;

  /// \brief Computes a 'fluttering' wind motion orthogonal to an object direction.
  ///
  /// This is used to apply sideways or upwards wind forces on an object, such that it flutters in the wind,
  /// even when the wind is constant.
  ///
  /// \param vWind The sampled (and potentially boosted or clamped) wind value.
  /// \param vObjectDir The main direction of the object. For example the (average) direction of a tree branch, or the direction of a rope or cable. The flutter value will be orthogonal to the object direction and the wind direction. So when when blows sideways onto a branch, the branch would flutter upwards and downwards. For a rope hanging downwards, wind blowing against it would make it flutter sideways.
  /// \param fFlutterSpeed How fast the object shall flutter (frequency).
  /// \param uiFlutterRandomOffset A random number that adds an offset to the flutter, such that multiple objects next to each other will flutter out of phase.
  xiiVec3 ComputeWindFlutter(const xiiVec3& vWind, const xiiVec3& vObjectDir, float fFlutterSpeed, xiiUInt32 uiFlutterRandomOffset) const;
};
