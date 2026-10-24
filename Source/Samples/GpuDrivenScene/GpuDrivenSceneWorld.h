/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/World.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Color.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
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
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiGpuDrivenSceneConfiguration);

/// Shader constants derived from the world's directional light component.
struct xiiGpuDrivenSceneLight
{
  xiiVec3  m_vDirection = xiiVec3(-0.4f, 0.6f, -0.7f);
  xiiColor m_Color      = xiiColor(1.0f, 0.92f, 0.78f);
  float    m_fIntensity = 1.35f;
};

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
  void                           Shutdown();
  void                           Update(xiiTime deltaTime);

  xiiView* GetRenderView() const;

private:
  struct GeometryAsset
  {
    xiiMeshResourceHandle m_hMesh;
  };

  xiiResult CreateMaterials();
  xiiResult CreateGeometry();
  xiiResult CreateSceneObjects();

  xiiGpuDrivenSceneConfiguration             m_Configuration;
  xiiComponentHandle                         m_hSunLight;
  xiiViewHandle                              m_hRenderView;
  xiiUniquePtr<xiiWorld>                     m_World;
  xiiDynamicArray<GeometryAsset>             m_GeometryAssets;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiGameObjectHandle                        m_hAssemblyRoot;
  xiiDynamicArray<xiiGameObjectHandle>       m_Objects;
  xiiDynamicArray<xiiVec3>                   m_BasePositions;
  float                                      m_fAnimationTime                   = 0.0f;
};
