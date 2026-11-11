/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Reflection/Reflection.h>

/// Device-wide streaming policy. Values never belong to a view or pipeline asset.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALStreamingMemorySettings
{
  bool      m_bUseNativeBudget                                     = true;
  xiiUInt32 m_uiBudgetMiB                                          = 4096U;
  xiiUInt32 m_uiHeadroomMiB                                        = 256U;
  xiiUInt32 m_uiPressureHighPercent                                = 95U;
  xiiUInt32 m_uiPressureLowPercent                                 = 85U;
  bool      operator==(const xiiGALStreamingMemorySettings&) const = default;
  xiiResult Validate() const;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALStreamingMemorySettings);
