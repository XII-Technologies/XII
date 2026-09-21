/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/WorldModule.h>
#include <Foundation/SimdMath/SimdVec4f.h>

/// Defines the strength / speed of wind. Inspired by the Beaufort Scale.
///
/// See https://en.wikipedia.org/wiki/Beaufort_scale
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

  /// Maps the wind strength name to a meters per second speed value as defined by the Beaufort Scale.
  ///
  /// The value only defines how fast wind moves, how much it affects an object, like bending it, depends
  /// on additional factors like stiffness and is thus object specific.
  static float GetInMetersPerSecond(xiiWindStrength::Enum strength);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiWindStrength);

class XII_CORE_DLL xiiWindWorldModuleInterface : public xiiWorldModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiWindWorldModuleInterface, xiiWorldModule);

protected:
  xiiWindWorldModuleInterface(xiiWorld* pWorld);

public:
  virtual xiiVec3      GetWindAt(const xiiVec3& vPosition) const = 0;
  virtual xiiSimdVec4f GetWindAtSimd(const xiiSimdVec4f& vPosition) const;

  /// Computes a 'fluttering' wind motion orthogonal to an object direction.
  ///
  /// This is used to apply sideways or upwards wind forces on an object, such that it flutters in the wind,
  /// even when the wind is constant.
  ///
  /// \param vWind The sampled (and potentially boosted or clamped) wind value.
  /// \param vObjectDir The main direction of the object. For example the (average) direction of a tree branch, or the direction of a rope or cable. The flutter value will be orthogonal to the object direction and the wind direction. So when wind blows sideways onto a branch, the branch would flutter upwards and downwards. For a rope hanging downwards, wind blowing against it would make it flutter sideways.
  /// \param fFlutterSpeed How fast the object shall flutter (frequency).
  /// \param uiFlutterRandomOffset A random number that adds an offset to the flutter, such that multiple objects next to each other will flutter out of phase.
  xiiVec3 ComputeWindFlutter(const xiiVec3& vWind, const xiiVec3& vObjectDir, float fFlutterSpeed, xiiUInt32 uiFlutterRandomOffset) const;
};
