/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <SharedPluginAssets/Common/MeshImportTransform.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMeshImportTransform, 1)
  XII_ENUM_CONSTANTS(xiiMeshImportTransform::Blender_YUp, xiiMeshImportTransform::Blender_ZUp, xiiMeshImportTransform::Custom)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiBasisAxis::Enum xiiMeshImportTransform::GetRightDir(Enum transform, xiiBasisAxis::Enum dir)
{
  return transform == Custom ? dir : xiiBasisAxis::NegativeX;
}

xiiBasisAxis::Enum xiiMeshImportTransform::GetUpDir(Enum transform, xiiBasisAxis::Enum dir)
{
  if (transform == Blender_YUp)
    return xiiBasisAxis::PositiveY;
  if (transform == Blender_ZUp)
    return xiiBasisAxis::PositiveZ;
  return dir;
}

bool xiiMeshImportTransform::GetFlipForward(Enum transform, bool bFlip)
{
  return transform == Custom && bFlip;
}
