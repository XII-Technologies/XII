/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Foundation/Reflection/Reflection.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Append stream ABI. Element zero stores the raw request count in VirtualKeyLow.
struct XII_GRAPHICSCORE_DLL xiiGpuVirtualShadowFeedback
{
  XII_DECLARE_POD_TYPE();
  xiiUInt32 m_uiVirtualKeyLow  = 0;
  xiiUInt32 m_uiVirtualKeyHigh = 0;
  xiiUInt32 m_uiPriority       = 0;
  xiiUInt32 m_uiFlags          = 0; ///< bit zero requests pinned residency.
};
static_assert(sizeof(xiiGpuVirtualShadowFeedback) == 16 && alignof(xiiGpuVirtualShadowFeedback) <= 8);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVirtualShadowFeedback);

struct xiiVirtualShadowFeedbackStats
{
  xiiUInt32 m_uiRawRequests = 0, m_uiSubmittedRequests = 0, m_uiOverflowRequests = 0;
  xiiUInt32 m_uiDroppedReadbacks = 0, m_uiStaleReadbacks = 0;
  xiiUInt64 m_uiSourceFrame = 0;
};
