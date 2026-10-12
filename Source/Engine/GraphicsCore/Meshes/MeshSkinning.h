/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Types/ArrayPtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Shared CPU skinning operations for spatial bounds and consumers of native mesh geometry.
/// Palettes contain mesh-space skinning matrices, including the inverse bind transform.
/// These operations allocate no storage and leave their output unchanged on failure.
namespace xiiMeshSkinning
{
  /// Accepts finite affine matrices, including non-uniform scale, reflection and shear.
  /// Empty palettes represent bind pose. O(palette size).
  [[nodiscard]] XII_GRAPHICSCORE_DLL xiiResult ValidatePalette(xiiArrayPtr<const xiiMat4> palette);

  /// Evaluates a position with native ushort4 indices and normalized byte4 weights.
  /// Nonzero weights are renormalized by their sum; an empty palette or all-zero weights
  /// preserves the bind position. Only nonzero influences require valid palette indices.
  /// Referenced matrices and the resulting position must be finite and affine. O(1).
  [[nodiscard]] XII_GRAPHICSCORE_DLL xiiResult TransformPosition(const xiiVec3& position, const xiiVec4U16& indices,
                                                                 const xiiColorLinearUB& weights, xiiArrayPtr<const xiiMat4> palette, xiiVec3& out_position);

  /// Conservatively encloses every normalized positive blend of points in bindBounds.
  /// Includes bind pose for vertices without influences. Uses the union of transformed
  /// boxes, so shear and singular affine transforms remain conservative. No CPU vertex
  /// retention or GPU readback is required. O(palette size); potentially loose for large rigs.
  [[nodiscard]] XII_GRAPHICSCORE_DLL xiiResult ComputeBounds(const xiiBoundingBoxSphere& bindBounds,
                                                             xiiArrayPtr<const xiiMat4> palette, xiiBoundingBoxSphere& out_bounds);
} // namespace xiiMeshSkinning
