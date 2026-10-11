/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

/// Asset import coordinate presets. Reflected values retain existing mesh and skeleton document settings.
struct XII_SHAREDPLUGINASSETS_DLL xiiMeshImportTransform
{
  using StorageType = xiiInt8;
  enum Enum
  {
    Blender_YUp,
    Blender_ZUp,
    Custom  = 127,
    Default = Blender_YUp
  };

  static xiiBasisAxis::Enum GetRightDir(Enum transform, xiiBasisAxis::Enum dir);
  static xiiBasisAxis::Enum GetUpDir(Enum transform, xiiBasisAxis::Enum dir);
  static bool               GetFlipForward(Enum transform, bool bFlip);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_SHAREDPLUGINASSETS_DLL, xiiMeshImportTransform);
