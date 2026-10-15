/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/World.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Color.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>

/// Reflected sample configuration doubles as an editor/tooling example for the new systems.
struct xiiGpuDrivenSceneConfiguration
{
  xiiUInt32 m_uiGridWidth          = 24U;
  xiiUInt32 m_uiGridHeight         = 16U;
  float     m_fObjectSpacing       = 2.4f;
  xiiUInt32 m_uiMaxVisibleMeshlets = 262144U;
  xiiUInt32 m_uiFramesInFlight     = 3U;
  bool      m_bAsyncCompute        = true;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiGpuDrivenSceneConfiguration);

/// A simple light authored as scene data and consumed by the GPU-driven draw pass.
struct xiiGpuDrivenSceneLight
{
  xiiVec3  m_vDirection = xiiVec3(-0.4f, 0.6f, -0.7f);
  xiiColor m_Color      = xiiColor(1.0f, 0.92f, 0.78f);
  float    m_fIntensity = 1.35f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiGpuDrivenSceneLight);

/// Owns authoring/runtime scene state used by the sample. The class intentionally exercises the
/// same stable handles, streaming, bindless tables and frame-sliced material storage used by a
/// production renderer instead of uploading sample-only draw data.
class xiiGpuDrivenSceneWorld
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGpuDrivenSceneWorld);

public:
  xiiGpuDrivenSceneWorld() = default;

  /// Publishes capacity policies before high-level startup so manager subsystems allocate once.
  [[nodiscard]] static xiiResult ConfigureSubsystems(const xiiGpuDrivenSceneConfiguration& configuration);
  xiiResult                      Initialize(xiiGALDevice* pDevice, const xiiGpuDrivenSceneConfiguration& configuration);
  void                           Shutdown(xiiUInt64 uiLastSubmittedFrame);
  void                           Update(xiiUInt64 uiFrameIndex, xiiTime deltaTime);

  const xiiGpuInstanceData& GetGpuInstances() const
  {
    XII_LOCK(m_World->GetReadMarker());
    return *static_cast<const xiiWorld*>(m_World.Borrow())->GetModule<xiiRenderWorldModule>()->GetGpuInstanceData();
  }
  xiiRayTracingResources::BuildHandles AddRayTracingBuildPass(xiiRenderGraph& graph, xiiUInt64 frame) const
  {
    XII_LOCK(m_World->GetReadMarker());
    return static_cast<const xiiWorld*>(m_World.Borrow())->GetModule<xiiRenderWorldModule>()->AddRayTracingBuildPass(graph, frame);
  }
  [[nodiscard]] const xiiGpuDrivenSceneLight& GetSunLight() const { return m_SunLight; }
  [[nodiscard]] xiiBoundingBox                GetAnimatedShadowBounds() const;

private:
  struct GeometryAsset
  {
    xiiGeometryHandle     m_hGeometry;
    xiiMeshResourceHandle m_hMesh;
  };

  xiiResult CreateMaterials();
  xiiResult CreateGeometry();
  xiiResult CreateSceneObjects();

  xiiGpuDrivenSceneConfiguration             m_Configuration;
  xiiGpuDrivenSceneLight                     m_SunLight;
  xiiUniquePtr<xiiWorld>                     m_World;
  xiiDynamicArray<GeometryAsset>             m_GeometryAssets;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiGameObjectHandle                        m_hAssemblyRoot;
  xiiDynamicArray<xiiGameObjectHandle>       m_Objects;
  xiiDynamicArray<xiiVec3>                   m_BasePositions;
  xiiBoundingBox                             m_AnimatedShadowInvalidationBounds = xiiBoundingBox::MakeInvalid();
  float                                      m_fAnimationTime                   = 0.0f;
};
