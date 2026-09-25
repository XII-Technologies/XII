/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include "GpuDrivenSceneWorld.h"

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuDrivenSceneConfiguration, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuDrivenSceneConfiguration>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GridWidth", m_uiGridWidth)->AddAttributes(new xiiDefaultValueAttribute(24U), new xiiClampValueAttribute(1U, 512U)),
    XII_MEMBER_PROPERTY("GridHeight", m_uiGridHeight)->AddAttributes(new xiiDefaultValueAttribute(16U), new xiiClampValueAttribute(1U, 512U)),
    XII_MEMBER_PROPERTY("ObjectSpacing", m_fObjectSpacing)->AddAttributes(new xiiDefaultValueAttribute(2.4f), new xiiClampValueAttribute(0.25f, 20.0f)),
    XII_MEMBER_PROPERTY("MaxVisibleMeshlets", m_uiMaxVisibleMeshlets)->AddAttributes(new xiiDefaultValueAttribute(262144U)),
    XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight)->AddAttributes(new xiiDefaultValueAttribute(3U), new xiiClampValueAttribute(2U, 8U)),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuDrivenSceneLight, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuDrivenSceneLight>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Direction", m_vDirection),
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.35f), new xiiClampValueAttribute(0.0f, 100.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

namespace
{
  xiiMeshBufferResourceHandle CreateMeshResource(xiiStringView sName, xiiGeometry&& geometry)
  {
    xiiMeshBufferResourceDescriptor descriptor;
    descriptor.AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, true);
    descriptor.m_bAllowGpuDrivenDraw = true;
    descriptor.m_bKeepCpuMeshData    = false;
    return xiiResourceManager::CreateResource<xiiMeshBufferResource>(sName, std::move(descriptor), sName);
  }
}

xiiResult xiiGpuDrivenSceneWorld::Initialize(xiiGALDevice* pDevice, const xiiGpuDrivenSceneConfiguration& configuration)
{
  if (pDevice == nullptr)
    return XII_FAILURE;

  m_Configuration = configuration;
  m_Scene.Reserve(configuration.m_uiGridWidth * configuration.m_uiGridHeight + 1U);
  m_SpatialHierarchy.Reserve(configuration.m_uiGridWidth * configuration.m_uiGridHeight);

  xiiGALBindlessResourceTableDescription bindlessDescription;
  bindlessDescription.m_uiBufferSRVCapacity = 256U;
  m_BindlessResources.Initialize(bindlessDescription);

  if (m_GeometryResidency.Initialize(pDevice, 64U, configuration.m_uiFramesInFlight, 128ULL * 1024ULL * 1024ULL, configuration.m_uiMaxVisibleMeshlets).Failed())
    return XII_FAILURE;

  xiiMaterialGpuStorageDescription materialDescription;
  materialDescription.m_uiMaxMaterials = 64U;
  materialDescription.m_uiMaxParameterBytes = 64U;
  materialDescription.m_uiFramesInFlight = configuration.m_uiFramesInFlight;
  if (m_MaterialSystem.Initialize(pDevice, materialDescription).Failed())
    return XII_FAILURE;

  XII_SUCCEED_OR_RETURN(CreateMaterials());
  XII_SUCCEED_OR_RETURN(CreateGeometry());
  XII_SUCCEED_OR_RETURN(CreateSceneObjects());
  return XII_SUCCESS;
}

void xiiGpuDrivenSceneWorld::Shutdown(xiiUInt64 uiLastSubmittedFrame)
{
  for (GeometryAsset& asset : m_GeometryAssets)
  {
    for (xiiGALBindlessResourceHandle handle : asset.m_BindlessBuffers)
      m_BindlessResources.RetireBufferSRV(handle, uiLastSubmittedFrame);
    m_GeometryResidency.UnregisterGeometry(asset.m_hGeometry, uiLastSubmittedFrame);
  }
  for (xiiMaterialGpuHandle handle : m_Materials)
    m_MaterialSystem.UnregisterMaterial(handle);

  m_BindlessResources.Collect(uiLastSubmittedFrame);
  m_BindlessResources.Clear();
  m_MaterialSystem.Shutdown();
  m_GeometryResidency.Shutdown();
  m_SpatialHierarchy.Clear();
  m_GeometryAssets.Clear();
  m_Materials.Clear();
  m_MaterialSchemas.Clear();
  m_MaterialInstances.Clear();
  m_Objects.Clear();
  m_BasePositions.Clear();
}

xiiResult xiiGpuDrivenSceneWorld::CreateMaterials()
{
  const xiiColor colors[] = {
    xiiColor(0.10f, 0.55f, 1.0f),
    xiiColor(1.0f, 0.28f, 0.08f),
    xiiColor(0.18f, 0.95f, 0.42f),
    xiiColor(0.85f, 0.18f, 0.95f),
  };

  for (xiiUInt32 i = 0U; i < XII_ARRAY_SIZE(colors); ++i)
  {
    xiiMaterialSchemaDescription schemaDescription;
    xiiStringBuilder materialName;
    materialName.SetFormat("GPU Driven Material {0}", i);
    schemaDescription.m_sName = materialName;
    schemaDescription.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/GpuDrivenScene.xiiShader");
    schemaDescription.m_Domain = xiiMaterialDomain::Surface;
    schemaDescription.m_ShadingModel = xiiMaterialShadingModel::Lit;
    schemaDescription.AddParameter("BaseColor", xiiMaterialParameterType::Float4, xiiVec4(colors[i].r, colors[i].g, colors[i].b, colors[i].a));

    xiiMaterialRuntimeState runtimeState;
    runtimeState.m_Domain = xiiMaterialDomain::Surface;
    runtimeState.m_ShadingModel = xiiMaterialShadingModel::Lit;
    runtimeState.m_FeatureFlags = xiiMaterialFeatureFlags::ReceivesLighting | xiiMaterialFeatureFlags::CastsShadows | xiiMaterialFeatureFlags::RuntimeGenerated | xiiMaterialFeatureFlags::UsesBindlessResources;

    xiiSharedPtr<xiiMaterialSchema> schema;
    xiiSharedPtr<xiiMaterialInstance> instance;
    xiiStringBuilder error;
    if (xiiMaterialSystem::CreateRuntimeMaterial(schemaDescription, runtimeState, schema, instance, &error).Failed())
    {
      xiiLog::Error("Failed to create GPU-driven sample material: {0}", error);
      return XII_FAILURE;
    }

    instance->SetParameter("BaseColor", xiiVec4(colors[i].r, colors[i].g, colors[i].b, colors[i].a)).AssertSuccess();
    const xiiMaterialGpuHandle handle = m_MaterialSystem.RegisterMaterial(instance);
    if (!handle.IsValid())
      return XII_FAILURE;

    m_MaterialSchemas.PushBack(std::move(schema));
    m_MaterialInstances.PushBack(std::move(instance));
    m_Materials.PushBack(handle);
  }
  return XII_SUCCESS;
}

xiiResult xiiGpuDrivenSceneWorld::CreateGeometry()
{
  m_GeometryAssets.SetCount(2U);

  {
    xiiGeometry high;
    high.AddStackedSphere(0.8f, 32U, 16U);
    xiiGeometry low;
    low.AddStackedSphere(0.8f, 12U, 6U);
    GeometryAsset& asset = m_GeometryAssets[0];
    asset.m_Lods.PushBack(CreateMeshResource("GpuDrivenScene.Sphere.High", std::move(high)));
    asset.m_Lods.PushBack(CreateMeshResource("GpuDrivenScene.Sphere.Low", std::move(low)));
  }
  {
    xiiGeometry high;
    high.AddBox(xiiVec3(1.5f), true);
    xiiGeometry low;
    low.AddBox(xiiVec3(1.5f), false);
    GeometryAsset& asset = m_GeometryAssets[1];
    asset.m_Lods.PushBack(CreateMeshResource("GpuDrivenScene.Box.High", std::move(high)));
    asset.m_Lods.PushBack(CreateMeshResource("GpuDrivenScene.Box.Low", std::move(low)));
  }

  for (GeometryAsset& asset : m_GeometryAssets)
  {
    xiiGeometryDescription description;
    description.m_bPinned = true;
    description.m_uiStreamingPriority = 100U;
    for (xiiUInt32 lod = 0U; lod < asset.m_Lods.GetCount(); ++lod)
    {
      xiiGeometryLodSource& source = description.m_Lods.ExpandAndGetRef();
      source.m_hMeshBuffer = asset.m_Lods[lod];
      source.m_fMinimumScreenCoverage = lod == 0U ? 80.0f : 0.0f;
      xiiResourceManager::PreloadResource(source.m_hMeshBuffer);
      xiiResourceLock<xiiMeshBufferResource> mesh(source.m_hMeshBuffer, xiiResourceAcquireMode::BlockTillLoaded);
      if (!mesh.IsValid())
        return XII_FAILURE;
    }
    asset.m_hGeometry = m_GeometryResidency.RegisterGeometry(description);
    if (!asset.m_hGeometry.IsValid())
      return XII_FAILURE;
    m_GeometryResidency.RequestResidency(asset.m_hGeometry, 0U, 0U);
  }

  m_GeometryResidency.ProcessStreaming(0U, 0U, 128ULL * 1024ULL * 1024ULL);
  for (GeometryAsset& asset : m_GeometryAssets)
  {
    if (m_GeometryResidency.GetState(asset.m_hGeometry) != xiiGeometryResidencyState::Resident)
      return XII_FAILURE;
    XII_SUCCEED_OR_RETURN(RegisterGeometryBuffers(asset));
  }
  return XII_SUCCESS;
}

xiiResult xiiGpuDrivenSceneWorld::RegisterGeometryBuffers(GeometryAsset& asset)
{
  for (xiiUInt32 lod = 0U; lod < asset.m_Lods.GetCount(); ++lod)
  {
    xiiResourceLock<xiiMeshBufferResource> mesh(asset.m_Lods[lod], xiiResourceAcquireMode::BlockTillLoaded);
    if (!mesh.IsValid())
      return XII_FAILURE;

    xiiGALBindlessResourceHandle handles[5] = {
      m_BindlessResources.RegisterBufferSRV(mesh->GetVertexBuffer()->GetDefaultView(xiiGALBufferViewType::ShaderResource)),
      m_BindlessResources.RegisterBufferSRV(mesh->GetIndexBuffer()->GetDefaultView(xiiGALBufferViewType::ShaderResource)),
      m_BindlessResources.RegisterBufferSRV(mesh->GetMeshletBuffer()->GetDefaultView(xiiGALBufferViewType::ShaderResource)),
      m_BindlessResources.RegisterBufferSRV(mesh->GetMeshletVertexRemapBuffer()->GetDefaultView(xiiGALBufferViewType::ShaderResource)),
      m_BindlessResources.RegisterBufferSRV(mesh->GetMeshletPrimitiveIndexBuffer()->GetDefaultView(xiiGALBufferViewType::ShaderResource)),
    };
    for (const xiiGALBindlessResourceHandle handle : handles)
    {
      if (!handle.IsValid())
        return XII_FAILURE;
      asset.m_BindlessBuffers.PushBack(handle);
    }

    if (!m_GeometryResidency.SetBindlessIndices(asset.m_hGeometry, lod, handles[0].m_uiIndex, handles[1].m_uiIndex, handles[2].m_uiIndex, handles[3].m_uiIndex, handles[4].m_uiIndex))
      return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGpuDrivenSceneWorld::CreateSceneObjects()
{
  const xiiUInt32 objectCount = m_Configuration.m_uiGridWidth * m_Configuration.m_uiGridHeight;
  m_Objects.Reserve(objectCount);
  m_BasePositions.Reserve(objectCount);

  for (xiiUInt32 y = 0U; y < m_Configuration.m_uiGridHeight; ++y)
  {
    for (xiiUInt32 x = 0U; x < m_Configuration.m_uiGridWidth; ++x)
    {
      const xiiUInt32 objectIndex = y * m_Configuration.m_uiGridWidth + x;
      const xiiUInt32 geometryIndex = objectIndex % m_GeometryAssets.GetCount();
      const xiiVec3 position(
        6.0f + static_cast<float>(y) * m_Configuration.m_fObjectSpacing,
        (static_cast<float>(x) - static_cast<float>(m_Configuration.m_uiGridWidth - 1U) * 0.5f) * m_Configuration.m_fObjectSpacing,
        (static_cast<float>(objectIndex % 5U) - 2.0f) * 0.32f);

      const xiiGpuGeometryRecord* geometry = m_GeometryResidency.GetGpuRecord(m_GeometryAssets[geometryIndex].m_hGeometry);
      if (geometry == nullptr)
        return XII_FAILURE;

      xiiSceneObjectDesc description;
      description.m_LocalTransform = xiiMat4::MakeTranslation(position);
      description.m_LocalBounds = xiiBoundingBoxSphere::MakeFromCenterExtents(
        geometry->m_BoundsCenterRadius.GetAsVec3(),
        geometry->m_BoundsExtents.GetAsVec3(),
        geometry->m_BoundsCenterRadius.w);
      description.m_uiGeometryIndex = m_GeometryAssets[geometryIndex].m_hGeometry.m_uiIndex;
      description.m_uiMaterialIndex = m_Materials[objectIndex % m_Materials.GetCount()].m_uiSlot;
      description.m_uiUserData = objectIndex;
      description.m_Flags = xiiSceneObjectFlags::Enabled | xiiSceneObjectFlags::CastShadows | xiiSceneObjectFlags::ReceiveShadows | xiiSceneObjectFlags::SensorVisible | (geometryIndex == 1U ? xiiSceneObjectFlags::Occluder : xiiSceneObjectFlags::None);

      const xiiSceneObjectHandle object = m_Scene.CreateObject(description);
      if (!object.IsValid())
        return XII_FAILURE;
      m_Objects.PushBack(object);
      m_BasePositions.PushBack(position);
    }
  }

  m_Scene.CommitFrame(0U);
  for (xiiSceneObjectHandle object : m_Objects)
  {
    if (!m_SpatialHierarchy.Insert(object, m_Scene.GetGlobalBounds(object).GetBox(), 0xFFFFFFFFU, xiiSceneObjectFlags::Default))
      return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiGpuDrivenSceneWorld::Update(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, xiiTime deltaTime)
{
  m_fAnimationTime += static_cast<float>(deltaTime.GetSeconds());
  const xiiUInt32 animatedCount = xiiMath::Min<xiiUInt32>(m_Objects.GetCount(), 64U);
  for (xiiUInt32 i = 0U; i < animatedCount; ++i)
  {
    xiiVec3 position = m_BasePositions[i];
    position.z += 0.45f * xiiMath::Sin(xiiAngle::MakeFromRadian(m_fAnimationTime * 1.7f + static_cast<float>(i) * 0.31f));
    const xiiMat4 transform = xiiMat4::MakeTranslation(position) * xiiMat4::MakeAxisRotation(xiiVec3(0.0f, 0.0f, 1.0f), xiiAngle::MakeFromRadian(m_fAnimationTime * 0.3f + static_cast<float>(i) * 0.01f));
    m_Scene.SetLocalTransform(m_Objects[i], transform);
  }

  m_Scene.CommitFrame(uiFrameIndex);
  for (xiiUInt32 i = 0U; i < animatedCount; ++i)
    m_SpatialHierarchy.Update(m_Objects[i], m_Scene.GetGlobalBounds(m_Objects[i]).GetBox(), xiiVec3::MakeZero(), 0xFFFFFFFFU, xiiSceneObjectFlags::Default);

  m_MaterialSystem.BeginFrame(uiFrameIndex, uiCompletedFrame);
  m_GeometryResidency.ProcessStreaming(uiFrameIndex, uiCompletedFrame, 8ULL * 1024ULL * 1024ULL);
  for (const GeometryAsset& asset : m_GeometryAssets)
    m_GeometryResidency.Touch(asset.m_hGeometry, uiFrameIndex);
  m_BindlessResources.Collect(uiCompletedFrame);
}

xiiUInt32 xiiGpuDrivenSceneWorld::GetMaterialFrameBase(xiiUInt64 uiFrameIndex) const
{
  if (m_Materials.IsEmpty())
    return 0U;
  const xiiUInt32 stride = m_MaterialSystem.GetGpuStorage().GetMaterialStride();
  return m_MaterialSystem.GetGpuStorage().GetGpuOffset(m_Materials[0], uiFrameIndex) - m_Materials[0].m_uiSlot * stride;
}
