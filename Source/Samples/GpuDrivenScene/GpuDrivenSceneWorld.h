/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Color.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/RayTracingScene.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Scene/SceneDatabaseManager.h>
#include <GraphicsCore/Scene/SceneSpatialHierarchy.h>
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

  [[nodiscard]] xiiSceneDatabase&             GetScene() { return m_SceneContext.GetDatabase(); }
  [[nodiscard]] const xiiSceneDatabase&       GetScene() const { return m_SceneContext.GetDatabase(); }
  [[nodiscard]] xiiSceneDatabaseContextHandle GetSceneHandle() const { return m_SceneContext.GetHandle(); }
  [[nodiscard]] xiiSceneSpatialHierarchy&     GetSpatialHierarchy() { return m_SpatialHierarchy; }
  [[nodiscard]] const xiiGpuDrivenSceneLight& GetSunLight() const { return m_SunLight; }
  [[nodiscard]] xiiBoundingBox                GetAnimatedShadowBounds() const;
  [[nodiscard]] xiiUInt32                     GetMaterialFrameBase(xiiUInt64 uiFrameIndex) const;

private:
  struct GeometryAsset
  {
    xiiGeometryHandle                            m_hGeometry;
    xiiRayTracingGeometryHandle                  m_hRayTracingGeometry;
    xiiDynamicArray<xiiMeshBufferResourceHandle> m_Lods;
  };

  xiiResult CreateMaterials();
  xiiResult CreateGeometry();
  xiiResult CreateSceneObjects();

  xiiGpuDrivenSceneConfiguration                     m_Configuration;
  xiiGpuDrivenSceneLight                             m_SunLight;
  xiiSceneDatabaseContext                            m_SceneContext;
  xiiSceneSpatialHierarchy                           m_SpatialHierarchy;
  xiiDynamicArray<GeometryAsset>                     m_GeometryAssets;
  xiiDynamicArray<xiiMaterialGpuHandle>              m_Materials;
  xiiDynamicArray<xiiSharedPtr<xiiMaterialSchema>>   m_MaterialSchemas;
  xiiDynamicArray<xiiSharedPtr<xiiMaterialInstance>> m_MaterialInstances;
  xiiSceneObjectHandle                               m_hAssemblyRoot;
  xiiDynamicArray<xiiSceneObjectHandle>              m_Objects;
  xiiDynamicArray<xiiRayTracingInstanceHandle>       m_RayTracingInstances;
  xiiDynamicArray<xiiVec3>                           m_BasePositions;
  xiiBoundingBox                                     m_AnimatedShadowInvalidationBounds = xiiBoundingBox::MakeInvalid();
  float                                              m_fAnimationTime                   = 0.0f;
};
