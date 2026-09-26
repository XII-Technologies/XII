/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Scene/SceneDatabase.h>
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

  xiiResult Initialize(xiiGALDevice* pDevice, const xiiGpuDrivenSceneConfiguration& configuration);
  void      Shutdown(xiiUInt64 uiLastSubmittedFrame);
  void      Update(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, xiiTime deltaTime);

  [[nodiscard]] xiiSceneDatabase&             GetScene() { return m_Scene; }
  [[nodiscard]] const xiiSceneDatabase&       GetScene() const { return m_Scene; }
  [[nodiscard]] xiiSceneSpatialHierarchy&     GetSpatialHierarchy() { return m_SpatialHierarchy; }
  [[nodiscard]] xiiGeometryResidencyManager&  GetGeometryResidency() { return *xiiGeometryResidencyManager::GetSingleton(); }
  [[nodiscard]] xiiGALBindlessResourceTable&  GetBindlessResources() { return *xiiGALBindlessResourceTable::GetSingleton(); }
  [[nodiscard]] const xiiGpuDrivenSceneLight& GetSunLight() const { return m_SunLight; }
  [[nodiscard]] xiiUInt32                     GetMaterialFrameBase(xiiUInt64 uiFrameIndex) const;

private:
  struct GeometryAsset
  {
    xiiGeometryHandle                             m_hGeometry;
    xiiDynamicArray<xiiMeshBufferResourceHandle>  m_Lods;
    xiiDynamicArray<xiiGALBindlessResourceHandle> m_BindlessBuffers;
  };

  xiiResult CreateMaterials();
  xiiResult CreateGeometry();
  xiiResult CreateSceneObjects();
  xiiResult RegisterGeometryBuffers(GeometryAsset& asset);

  xiiGpuDrivenSceneConfiguration                     m_Configuration;
  xiiGpuDrivenSceneLight                             m_SunLight;
  xiiSceneDatabase                                   m_Scene;
  xiiSceneSpatialHierarchy                           m_SpatialHierarchy;
  xiiDynamicArray<GeometryAsset>                     m_GeometryAssets;
  xiiDynamicArray<xiiMaterialGpuHandle>              m_Materials;
  xiiDynamicArray<xiiSharedPtr<xiiMaterialSchema>>   m_MaterialSchemas;
  xiiDynamicArray<xiiSharedPtr<xiiMaterialInstance>> m_MaterialInstances;
  xiiSceneObjectHandle                               m_hAssemblyRoot;
  xiiDynamicArray<xiiSceneObjectHandle>              m_Objects;
  xiiDynamicArray<xiiVec3>                           m_BasePositions;
  float                                              m_fAnimationTime = 0.0f;
};
