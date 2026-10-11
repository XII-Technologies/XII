/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <ModelImporter/ModelImporterDLL.h>

class xiiStringBuilder;

/// Imported convex authoring geometry, expressed in the referenced joint's local coordinates.
/// Kept separate from native runtime skeleton resources and independent of Editor document types.
struct XII_MODELIMPORTER_DLL xiiModelSkeletonCollider
{
  xiiUInt16                  m_uiJointIndex = xiiMath::MaxValue<xiiUInt16>();
  xiiString                  m_sIdentifier;
  xiiDynamicArray<xiiVec3>   m_Vertices;
  xiiDynamicArray<xiiUInt32> m_TriangleIndices;
};

struct XII_MODELIMPORTER_DLL xiiModelSkeletonImport
{
  xiiSkeletonResourceDescriptor             m_Skeleton;
  xiiDynamicArray<xiiModelSkeletonCollider> m_Colliders;

  /// Reads an absolute source filename using the existing Assimp dependency, including external glTF buffers.
  /// Preserves the complete named scene hierarchy in parent-before-child order and signed local TRS poses.
  /// UCX_<joint>[_suffix] mesh instances become joint-local convex authoring geometry.
  /// Duplicate / empty joint names, shear, non-finite or singular poses and invalid geometry fail explicitly.
  /// Conversion is iterative; format parsing and its limits belong to Assimp. Failure clears out_import.
  static xiiResult ReadFile(xiiStringView sSourceFile, xiiModelSkeletonImport& out_import, xiiStringBuilder* out_pError = nullptr);
};
