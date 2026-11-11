/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>

/// Device-wide streaming policy. Values never belong to a view or pipeline asset.
struct XII_GRAPHICSCORE_DLL xiiResourceStreamingSettings
{
  xiiInt32  m_iTextureUploadMiB                                   = 64;
  float     m_fTextureMipBias                                     = 0.0f;
  xiiInt32  m_iTextureMinimumMip                                  = 0;
  xiiInt32  m_iTextureCacheMiB                                    = 512;
  xiiInt32  m_iGeometryCacheMiB                                   = 256;
  xiiUInt32 m_uiPressureTrimInterval                              = 8U;
  xiiUInt32 m_uiPressureMinimumIdleFrames                         = 120U;
  xiiUInt32 m_uiPressureMaxEvictions                              = 4U;
  bool      operator==(const xiiResourceStreamingSettings&) const = default;
  xiiResult Validate() const;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiResourceStreamingSettings);
