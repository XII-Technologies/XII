/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Vec3.h>

#include <Core/World/Declarations.h>
#include <Foundation/Types/RefCounted.h>

struct XII_CORE_DLL xiiCoordinateSystem
{
  XII_DECLARE_POD_TYPE();

  xiiVec3 m_vForwardDir;
  xiiVec3 m_vRightDir;
  xiiVec3 m_vUpDir;
};

class XII_CORE_DLL xiiCoordinateSystemProvider : public xiiRefCounted
{
public:
  xiiCoordinateSystemProvider(const xiiWorld* pOwnerWorld) :
    m_pOwnerWorld(pOwnerWorld)
  {
  }

  virtual ~xiiCoordinateSystemProvider() = default;

  virtual void GetCoordinateSystem(const xiiVec3& vGlobalPosition, xiiCoordinateSystem& out_coordinateSystem) const = 0;

protected:
  friend class xiiWorld;

  const xiiWorld* m_pOwnerWorld;
};

/// Helper class to convert between two xiiCoordinateSystem spaces.
///
/// All functions will do an identity transform until SetConversion is called to set up
/// the conversion. Afterwards the convert functions can be used to convert between
/// the two systems in both directions.
/// Currently, only uniformly scaled orthogonal coordinate systems are supported.
/// They can however be right handed or left handed.
class XII_CORE_DLL xiiCoordinateSystemConversion
{
public:
  /// Creates a new conversion that until set up, does identity conversions.
  xiiCoordinateSystemConversion(); // [tested]

  /// Set up the source and target coordinate systems.
  void SetConversion(const xiiCoordinateSystem& source, const xiiCoordinateSystem& target); // [tested]
  /// Returns the equivalent point in the target coordinate system.
  xiiVec3 ConvertSourcePosition(const xiiVec3& vPos) const; // [tested]
  /// Returns the equivalent rotation in the target coordinate system.
  xiiQuat ConvertSourceRotation(const xiiQuat& qOrientation) const; // [tested]
  /// Returns the equivalent length in the target coordinate system.
  float ConvertSourceLength(float fLength) const; // [tested]

  /// Returns the equivalent point in the source coordinate system.
  xiiVec3 ConvertTargetPosition(const xiiVec3& vPos) const; // [tested]
  /// Returns the equivalent rotation in the source coordinate system.
  xiiQuat ConvertTargetRotation(const xiiQuat& qOrientation) const; // [tested]
  /// Returns the equivalent length in the source coordinate system.
  float ConvertTargetLength(float fLength) const; // [tested]

private:
  xiiMat3 m_mSourceToTarget;
  xiiMat3 m_mTargetToSource;
  float   m_fWindingSwap         = 1.0f;
  float   m_fSourceToTargetScale = 1.0f;
  float   m_fTargetToSourceScale = 1.0f;
};
