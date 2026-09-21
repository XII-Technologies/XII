/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Algorithm/HashingUtils.h>

class xiiWorld;
class xiiViewHandle;

/// Used in xiiDebugRenderer to determine where debug geometry should be rendered.
class XII_GRAPHICSCORE_DLL xiiDebugRendererContext
{
public:
  xiiDebugRendererContext() = default;

  /// If this constructor is used, the geometry is rendered in all views for that scene.
  xiiDebugRendererContext(const xiiWorld* pWorld);

  /// If this constructor is used, the geometry is only rendered in this view.
  xiiDebugRendererContext(const xiiViewHandle& hView);

  XII_ALWAYS_INLINE bool operator==(const xiiDebugRendererContext& other) const { return m_uiId == other.m_uiId; }

private:
  friend struct xiiHashHelper<xiiDebugRendererContext>;

  xiiUInt32 m_uiId = xiiInvalidIndex;
};

template <>
struct xiiHashHelper<xiiDebugRendererContext>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiDebugRendererContext value) { return xiiHashHelper<xiiUInt32>::Hash(value.m_uiId); }

  XII_ALWAYS_INLINE static bool Equal(xiiDebugRendererContext a, xiiDebugRendererContext b) { return a == b; }
};
