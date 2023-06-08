#pragma once

#include <Foundation/Math/Vec3.h>

#include <Core/World/Declarations.h>
#include <Foundation/Types/RefCounted.h>

template <typename Type>
struct xiiCoordinateSystemTemplate
{
  XII_DECLARE_POD_TYPE();

  xiiVec3Template<Type> m_vForwardDir;
  xiiVec3Template<Type> m_vRightDir;
  xiiVec3Template<Type> m_vUpDir;
};

using xiiCoordinateSystem       = xiiCoordinateSystemTemplate<xiiReal>;
using xiiCoordinateSystemDouble = xiiCoordinateSystemTemplate<double>;
using xiiCoordinateSystemFloat  = xiiCoordinateSystemTemplate<float>;

template <typename Type>
class xiiCoordinateSystemProviderTemplate : public xiiRefCounted
{
public:
  xiiCoordinateSystemProviderTemplate(const xiiWorld* pOwnerWorld) :
    m_pOwnerWorld(pOwnerWorld)
  {
  }

  virtual ~xiiCoordinateSystemProviderTemplate() = default;

  virtual void GetCoordinateSystem(const xiiVec3Template<Type>& vGlobalPosition, xiiCoordinateSystemTemplate<Type>& out_coordinateSystem) const = 0;

protected:
  friend class xiiWorld;

  const xiiWorld* m_pOwnerWorld;
};

/// \brief Helper class to convert between two xiiCoordinateSystem spaces.
///
/// All functions will do an identity transform until SetConversion is called to set up
/// the conversion. Afterwards the convert functions can be used to convert between
/// the two systems in both directions.
/// Currently, only uniformly scaled orthogonal coordinate systems are supported.
/// They can however be right handed or left handed.
template <typename Type>
class xiiCoordinateSystemConversionTemplate
{
public:
  /// \brief Creates a new conversion that until set up, does identity conversions.
  xiiCoordinateSystemConversionTemplate(); // [tested]

  /// \brief Set up the source and target coordinate systems.
  void SetConversion(const xiiCoordinateSystemTemplate<Type>& source, const xiiCoordinateSystemTemplate<Type>& target); // [tested]

  /// \brief Returns the equivalent point in the target coordinate system.
  xiiVec3Template<Type> ConvertSourcePosition(const xiiVec3Template<Type>& vPos) const; // [tested]

  /// \brief Returns the equivalent rotation in the target coordinate system.
  xiiQuatTemplate<Type> ConvertSourceRotation(const xiiQuatTemplate<Type>& qOrientation) const; // [tested]

  /// \brief Returns the equivalent length in the target coordinate system.
  Type ConvertSourceLength(Type fLength) const; // [tested]

  /// \brief Returns the equivalent point in the source coordinate system.
  xiiVec3Template<Type> ConvertTargetPosition(const xiiVec3Template<Type>& vPos) const; // [tested]

  /// \brief Returns the equivalent rotation in the source coordinate system.
  xiiQuatTemplate<Type> ConvertTargetRotation(const xiiQuatTemplate<Type>& qOrientation) const; // [tested]

  /// \brief Returns the equivalent length in the source coordinate system.
  Type ConvertTargetLength(Type fLength) const; // [tested]

private:
  xiiMat3Template<Type> m_mSourceToTarget;
  xiiMat3Template<Type> m_mTargetToSource;

  Type m_fWindingSwap         = static_cast<Type>(1);
  Type m_fSourceToTargetScale = static_cast<Type>(1);
  Type m_fTargetToSourceScale = static_cast<Type>(1);
};

using xiiCoordinateSystemConversion        = xiiCoordinateSystemConversionTemplate<xiiReal>;
using xiiCoordinateSusstemConversionFloat  = xiiCoordinateSystemConversionTemplate<float>;
using xiiCoordinateSusstemConversionDouble = xiiCoordinateSystemConversionTemplate<double>;


#include <Core/World/Implementation/CoordinateSystem_inl.h>
