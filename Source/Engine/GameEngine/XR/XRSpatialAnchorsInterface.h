#pragma once

#include <GameEngine/GameEngineDLL.h>

typedef xiiGenericId<32, 16> xiiXRSpatialAnchorID;

/// \brief XR spatial anchors interface.
///
/// Aquire interface via xiiSingletonRegistry::GetSingletonInstance<xiiXRSpatialAnchorsInterface>().
class xiiXRSpatialAnchorsInterface
{
public:
  /// \brief Creates a spatial anchor at the given world space position.
  /// Returns an invalid handle if anchors can't be created right now. Retry next frame.
  virtual xiiXRSpatialAnchorID CreateAnchor(const xiiTransform& globalTransform) = 0;

  /// \brief Destroys a previously created anchor.
  virtual xiiResult DestroyAnchor(xiiXRSpatialAnchorID id) = 0;

  /// \brief Tries to resolve the anchor position. Can fail of the anchor is invalid or tracking is
  /// currently lost.
  virtual xiiResult TryGetAnchorTransform(xiiXRSpatialAnchorID id, xiiTransform& out_globalTransform) = 0;
};
