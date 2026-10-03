/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialGpuStorage.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>

/// Describes one GPU-scene mesh-shader depth rasterization.
struct XII_GRAPHICSCORE_DLL xiiGpuSceneDepthRasterDescription
{
  xiiMat4    m_ViewProjectionMatrix      = xiiMat4::MakeIdentity();
  xiiUInt32  m_uiWidth                   = 0U;
  xiiUInt32  m_uiHeight                  = 0U;
  xiiUInt32  m_uiVertexStride            = 0U;
  xiiUInt32  m_uiTexCoordOffset          = 0U;
  xiiUInt32  m_uiMeshDispatchGroupCountX = 1U;
  xiiUInt32  m_uiMeshDispatchGroupCountY = 1U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuSceneDepthRasterDescription);

/// Describes canonical material G-buffer generation from a compact GPU visibility stream.
struct XII_GRAPHICSCORE_DLL xiiGpuSceneGBufferRasterDescription
{
  xiiMat4   m_ViewProjectionMatrix      = xiiMat4::MakeIdentity();
  xiiUInt32 m_uiWidth                   = 0U;
  xiiUInt32 m_uiHeight                  = 0U;
  xiiUInt32 m_uiVertexStride            = 0U;
  xiiUInt32 m_uiNormalOffset            = 0U;
  xiiUInt32 m_uiTangentOffset           = 0U;
  xiiUInt32 m_uiTexCoordOffset          = 0U;
  xiiUInt32 m_uiMeshDispatchGroupCountX = 1U;
  xiiUInt32 m_uiMeshDispatchGroupCountY = 1U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuSceneGBufferRasterDescription);

/// Versions produced by one GPU-scene G-buffer raster pass.
struct XII_GRAPHICSCORE_DLL xiiGpuSceneGBufferOutputs
{
  xiiRenderGraphTextureHandle m_hAlbedo;
  xiiRenderGraphTextureHandle m_hNormal;
  xiiRenderGraphTextureHandle m_hMaterial;
  xiiRenderGraphTextureHandle m_hEmissive;
  xiiRenderGraphTextureHandle m_hNormalRoughness;

  [[nodiscard]] bool IsValid() const { return m_hAlbedo.IsValid() && m_hNormal.IsValid() && m_hMaterial.IsValid() && m_hEmissive.IsValid() && m_hNormalRoughness.IsValid(); }
};

class xiiGpuSceneRasterManagerState;

/// Subsystem-owned raster backend for GPU-scene visibility streams.
///
/// The service owns shader lifetime only. Scene, geometry, visibility and target resources remain
/// versioned render-graph inputs, which lets async visibility transition directly into graphics
/// without a view-owned manager or manual barriers.
class XII_GRAPHICSCORE_DLL xiiGpuSceneRasterManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGpuSceneRasterManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, GpuSceneRasterManager);

public:
  xiiGpuSceneRasterManager() = delete;

  [[nodiscard]] static bool IsSubsystemInitialized();
  [[nodiscard]] static bool IsInitialized();
  [[nodiscard]] static bool IsSupported();

  /// Adds a reversed-Z depth prepass and returns the produced depth version. Invalid input or a
  /// device without mesh shaders returns an invalid handle so callers can select an indexed fallback.
  [[nodiscard]] static xiiRenderGraphTextureHandle AddDepthPrepass(xiiRenderGraph& graph, xiiStringView sName,
                                                                   const xiiGpuVisibilityOutputs& visibility,
                                                                   const xiiGeometryResidencyManager::UploadHandles& geometry,
                                                                   const xiiMaterialGpuStorage::UploadHandles& materials,
                                                                   const xiiGpuSceneDepthRasterDescription& description);

  /// Rasterizes canonical surface materials into the deferred targets while depth testing against
  /// the scene prepass. The compact normal/roughness target is emitted as a fifth MRT so AO and
  /// lighting preparation do not need a second geometry submission.
  [[nodiscard]] static xiiGpuSceneGBufferOutputs AddGBufferPass(xiiRenderGraph& graph, xiiStringView sName,
                                                                xiiRenderGraphTextureHandle hSceneDepth,
                                                                const xiiGpuVisibilityOutputs& visibility,
                                                                const xiiGeometryResidencyManager::UploadHandles& geometry,
                                                                const xiiMaterialGpuStorage::UploadHandles& materials,
                                                                const xiiGpuSceneGBufferRasterDescription& description);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<xiiGpuSceneRasterManagerState> s_pState;
};
