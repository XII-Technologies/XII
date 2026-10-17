/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Geometry/GeometryResidency.h>

/// GPU request table entry. Key is the complete geometry ID plus one; zero is an empty entry.
/// One initialized frame table merges a geometry's requests toward the finest LOD and strongest priority.
/// Entry zero stores raw, unique and overflow counts in Key, DesiredLod and Priority respectively.
struct xiiGpuGeometryLodRequest
{
  XII_DECLARE_POD_TYPE();
  xiiUInt32         m_uiKey         = 0;
  xiiUInt32         m_uiDesiredLod  = xiiInvalidIndex;
  xiiUInt32         m_uiPriority    = 0;
  xiiUInt32         m_uiPurposeMask = 0;
  xiiGeometryHandle GetGeometry() const { return m_uiKey ? xiiGeometryHandle(xiiGeometryId(m_uiKey - 1)) : xiiGeometryHandle(); }
};
static_assert(sizeof(xiiGeometryId) == 4 && xiiGeometryId::MAX_INSTANCES == (1U << 24), "GPU request keys require the existing 24-bit geometry slot and 8-bit generation ABI.");
static_assert(sizeof(xiiGpuGeometryLodRequest) == 16 && offsetof(xiiGpuGeometryLodRequest, m_uiPriority) == 8);

struct xiiGeometryLodFeedbackStats
{
  xiiUInt32 m_uiRawRequests = 0, m_uiUniqueRequests = 0, m_uiOverflowRequests = 0;
  xiiUInt32 m_uiAcceptedRequests = 0, m_uiStaleRequests = 0, m_uiDroppedReadbacks = 0;
  xiiUInt64 m_uiSourceFrame = 0;
};
