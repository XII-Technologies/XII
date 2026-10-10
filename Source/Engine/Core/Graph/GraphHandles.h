/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Types/Id.h>

/// Internal identity shared by the two strongly typed graph handles. Not a persistent asset identifier.
struct xiiGraphElementId
{
  XII_DECLARE_POD_TYPE();

  using LocalId = xiiGenericId<32, 31>;
  LocalId   m_LocalId;
  xiiUInt64 m_uiGraphId = 0;

  bool operator==(const xiiGraphElementId& other) const = default;
  bool operator<(const xiiGraphElementId& other) const
  {
    return m_uiGraphId < other.m_uiGraphId || (m_uiGraphId == other.m_uiGraphId && m_LocalId < other.m_LocalId);
  }
  void Invalidate()
  {
    m_LocalId.Invalidate();
    m_uiGraphId = 0;
  }
  bool IsInvalidated() const { return m_uiGraphId == 0 || m_LocalId.IsInvalidated(); }
};

struct xiiGraphVertexHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGraphVertexHandle, xiiGraphElementId);
};

struct xiiGraphEdgeHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGraphEdgeHandle, xiiGraphElementId);
};

template <>
struct xiiHashHelper<xiiGraphVertexHandle>
{
  static xiiUInt32 Hash(xiiGraphVertexHandle value)
  {
    const auto      id     = value.GetInternalID();
    const xiiUInt64 data[] = {id.m_LocalId.m_Data, id.m_uiGraphId};
    return xiiHashingUtils::xxHash32(data, sizeof(data));
  }
  static bool Equal(xiiGraphVertexHandle a, xiiGraphVertexHandle b) { return a == b; }
};

template <>
struct xiiHashHelper<xiiGraphEdgeHandle>
{
  static xiiUInt32 Hash(xiiGraphEdgeHandle value)
  {
    const auto      id     = value.GetInternalID();
    const xiiUInt64 data[] = {id.m_LocalId.m_Data, id.m_uiGraphId};
    return xiiHashingUtils::xxHash32(data, sizeof(data));
  }
  static bool Equal(xiiGraphEdgeHandle a, xiiGraphEdgeHandle b) { return a == b; }
};
