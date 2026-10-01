/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>

/// Describes one independently culled shadow view rasterized into a depth atlas region.
struct XII_GRAPHICSCORE_DLL xiiGpuShadowRasterDescription
{
  xiiMat4    m_ViewProjectionMatrix      = xiiMat4::MakeIdentity();
  xiiVec4U32 m_Viewport                  = xiiVec4U32::MakeZero(); ///< x, y, width, height in atlas texels.
  xiiUInt32  m_uiVertexStride            = 0U;
  xiiUInt32  m_uiMeshDispatchGroupCountX = 1U;
  xiiUInt32  m_uiMeshDispatchGroupCountY = 1U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuShadowRasterDescription);

class xiiGpuShadowRasterManagerState;

/// Subsystem-owned mesh-shader shadow raster service.
///
/// The manager has no per-view singleton state. Callers provide versioned visibility outputs and
/// an atlas handle for every pass; the subsystem owns only shader lifetime and pipeline creation.
class XII_GRAPHICSCORE_DLL xiiGpuShadowRasterManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGpuShadowRasterManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, GpuShadowRasterManager);

public:
  xiiGpuShadowRasterManager() = delete;

  [[nodiscard]] static bool IsSupported();

  /// Adds one graph-managed depth-only mesh dispatch. The returned handle is the new atlas version.
  [[nodiscard]] static xiiRenderGraphTextureHandle AddPass(xiiRenderGraph& graph, xiiStringView sName,
                                                           xiiRenderGraphTextureHandle hDepthAtlas, const xiiGpuVisibilityOutputs& visibility,
                                                           const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiGpuShadowRasterDescription& description);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<xiiGpuShadowRasterManagerState> s_pState;
};
