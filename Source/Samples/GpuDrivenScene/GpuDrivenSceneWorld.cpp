/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include "GpuDrivenSceneWorld.h"

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuDrivenSceneConfiguration, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuDrivenSceneConfiguration>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("MeshDetail", m_uiMeshDetail),
      XII_MEMBER_PROPERTY("Texture", m_sTexture),
      XII_MEMBER_PROPERTY("AuthoredMaterials", m_bAuthoredMaterials),
      XII_MEMBER_PROPERTY("PopulateScene", m_bPopulateScene),
      XII_MEMBER_PROPERTY("GridWidth", m_uiGridWidth)->AddAttributes(new xiiDefaultValueAttribute(24U), new xiiClampValueAttribute(1U, 512U)),
      XII_MEMBER_PROPERTY("GridHeight", m_uiGridHeight)->AddAttributes(new xiiDefaultValueAttribute(16U), new xiiClampValueAttribute(1U, 512U)),
      XII_MEMBER_PROPERTY("ObjectSpacing", m_fObjectSpacing)->AddAttributes(new xiiDefaultValueAttribute(2.4f), new xiiClampValueAttribute(0.25f, 20.0f)),
      XII_MEMBER_PROPERTY("MaxVisibleMeshlets", m_uiMaxVisibleMeshlets)->AddAttributes(new xiiDefaultValueAttribute(262144U)),
      XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight)->AddAttributes(new xiiDefaultValueAttribute(3U), new xiiClampValueAttribute(2U, 8U)),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

xiiResult xiiGpuDrivenSceneWorld::ConfigureSubsystems(const xiiGpuDrivenSceneConfiguration& configuration)
{
  if (!xiiGALBindlessResourceTable::IsInitialized())
    return XII_FAILURE;

  xiiGALBindlessResourceTableDescription bindlessDescription;
  bindlessDescription.m_uiBufferSRVCapacity = 256U;
  XII_SUCCEED_OR_RETURN(xiiGALBindlessResourceTable::Configure(bindlessDescription));

  xiiGpuResidencyDescription geometryDescription;
  geometryDescription.m_uiMaxGeometries             = 64U;
  geometryDescription.m_uiFramesInFlight            = configuration.m_uiFramesInFlight;
  geometryDescription.m_uiBudgetBytes               = 128ULL * 1024ULL * 1024ULL;
  geometryDescription.m_uiUploadBudgetPerFrameBytes = 8ULL * 1024ULL * 1024ULL;
  geometryDescription.m_uiMaxMeshlets               = configuration.m_uiMaxVisibleMeshlets;
  XII_SUCCEED_OR_RETURN(xiiGpuResidencyManager::Configure(geometryDescription));

  xiiMaterialGpuStorageDescription materialDescription;
  materialDescription.m_uiMaxMaterials   = 64U;
  materialDescription.m_uiFramesInFlight = configuration.m_uiFramesInFlight;
  XII_SUCCEED_OR_RETURN(xiiMaterialManager::Configure(materialDescription));

  return XII_SUCCESS;
}

xiiResult xiiGpuDrivenSceneWorld::Initialize(xiiGALDevice* pDevice, const xiiGpuDrivenSceneConfiguration& configuration)
{
  if (pDevice == nullptr)
    return XII_FAILURE;

  m_Configuration = configuration;
  xiiWorldDescription worldDescription("GPU-driven component world");
  m_World = XII_DEFAULT_NEW(xiiWorld, worldDescription);
  XII_LOCK(m_World->GetWriteMarker());
  m_World->SetWorldSimulationEnabled(false);
  const xiiGpuDrivenSceneLight sunlight;
  xiiGameObjectDescription     lightDescription;
  lightDescription.m_LocalRotation = xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), sunlight.m_vDirection.GetNormalized());
  xiiGameObject* lightObject       = nullptr;
  m_World->CreateObject(lightDescription, lightObject);
  xiiDirectionalLightComponent* light = nullptr;
  m_hSunLight                         = xiiDirectionalLightComponent::CreateComponent(lightObject, light);
  light->SetLightColor(xiiColorGammaUB(sunlight.m_Color));
  light->SetIntensity(sunlight.m_fIntensity);

  if (m_Configuration.m_bPopulateScene)
  {
    XII_SUCCEED_OR_RETURN(CreateMaterials());
    XII_SUCCEED_OR_RETURN(CreateGeometry());
    XII_SUCCEED_OR_RETURN(CreateSceneObjects());
  }
  else
    m_World->GetOrCreateModule<xiiRenderWorldModule>();
  xiiView* view = nullptr;
  m_hRenderView = m_World->GetModule<xiiRenderWorldModule>()->CreateView("GPU-driven authored extraction", view);
  return view ? XII_SUCCESS : XII_FAILURE;
}

void xiiGpuDrivenSceneWorld::Shutdown()
{
  m_hRenderView.Invalidate();
  m_World.Clear();
  m_GeometryAssets.Clear();
  m_Materials.Clear();
  m_Objects.Clear();
  m_BasePositions.Clear();
  m_hAssemblyRoot.Invalidate();
  m_hSunLight.Invalidate();
}

xiiView* xiiGpuDrivenSceneWorld::GetRenderView() const
{
  if (!m_World)
    return nullptr;
  XII_LOCK(m_World->GetReadMarker());
  xiiView* view = nullptr;
  static_cast<const xiiWorld*>(m_World.Borrow())->GetModule<xiiRenderWorldModule>()->TryGetView(m_hRenderView, view);
  return view;
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
    xiiMaterialResourceDescriptor material;
    material.m_BaseColor = colors[i];
    if (!m_Configuration.m_sTexture.IsEmpty())
      material.m_hBaseColorTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(m_Configuration.m_sTexture);
    if (m_Configuration.m_bAuthoredMaterials)
    {
      material.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Materials/DefaultMaterial.xiiShader");
      auto& binding      = material.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name.Assign("BaseTexture");
      binding.m_Value = material.m_hBaseColorTexture;
      auto& enabled   = material.m_Parameters.ExpandAndGetRef();
      enabled.m_Name.Assign("UseBaseTexture");
      enabled.m_Value = material.m_hBaseColorTexture.IsValid();
    }
    material.m_FeatureFlags = xiiMaterialFeatureFlags::ReceivesLighting | xiiMaterialFeatureFlags::CastsShadows;
    xiiStringBuilder name;
    name.SetFormat("GpuDrivenScene.Material{}", i);
    m_Materials.PushBack(xiiResourceManager::GetOrCreateResource<xiiMaterialResource>(name, std::move(material)));
  }
  return XII_SUCCESS;
}

xiiResult xiiGpuDrivenSceneWorld::CreateGeometry()
{
  if (!m_Configuration.m_sMesh.IsEmpty())
  {
    m_GeometryAssets.SetCount(1U);
    m_GeometryAssets[0].m_hMesh = xiiResourceManager::LoadResource<xiiMeshResource>(m_Configuration.m_sMesh);
    xiiResourceLock<xiiMeshResource> mesh(m_GeometryAssets[0].m_hMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (mesh.IsValid() && mesh.GetAcquireResult() == xiiResourceAcquireResult::Final)
    {
      xiiResourceLock<xiiMeshBufferResource> buffer(mesh->GetMeshBuffer(), xiiResourceAcquireMode::PointerOnly);
      const auto&                            descriptor = buffer->GetDescriptor();
      xiiLog::Info("Compiled geometry: {} pages, {} clusters, {} retained page payload bytes.", descriptor.m_GeometryPages.GetCount(), descriptor.m_Meshlets.GetCount(), descriptor.m_GeometryPageData.GetCount());
      for (const auto& page : descriptor.m_GeometryPages)
        if (page.m_bRequired)
          xiiLog::Info("Geometry root: {} clusters, {} children, {} object-space error.", page.m_uiClusterCount, page.m_uiChildCount, page.m_fGeometricError);
    }
    return mesh.IsValid() && mesh.GetAcquireResult() == xiiResourceAcquireResult::Final ? XII_SUCCESS : XII_FAILURE;
  }
  m_GeometryAssets.SetCount(2);
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiGeometry high, low;
    if (i == 0)
    {
      high.AddStackedSphere(0.8f, m_Configuration.m_uiMeshDetail, m_Configuration.m_uiMeshDetail / 2U);
      low.AddStackedSphere(0.8f, 12, 6);
    }
    else
    {
      high.AddBox(xiiVec3(1.5f), true);
      low.AddBox(xiiVec3(1.5f), false);
    }
    high.TriangulatePolygons();
    low.TriangulatePolygons();
    const auto fineCount   = high.GetPolygons().GetCount();
    const auto coarseCount = low.GetPolygons().GetCount();
    high.Merge(low);
    xiiMeshResourceDescriptor descriptor;
    descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(high, xiiGALPrimitiveTopology::TriangleList, true);
    descriptor.GetMeshBufferDescriptor().m_bAllowGpuDrivenDraw = true;
    descriptor.AddMaterialSlot("");
    descriptor.GetLODs()[0].m_fScreenSize = 80.0f / 1080.0f;
    descriptor.AddSection(fineCount, 0, 0, 0);
    descriptor.AddLOD(0);
    descriptor.AddSection(coarseCount, fineCount, 0, 1);
    descriptor.ComputeBounds();
    if (i == 0U && !m_Configuration.m_sExportMesh.IsEmpty())
    {
      XII_SUCCEED_OR_RETURN(descriptor.BuildMeshlets());
      const auto&   buffer = static_cast<const xiiMeshResourceDescriptor&>(descriptor).GetMeshBufferDescriptor();
      xiiFileWriter file;
      XII_SUCCEED_OR_RETURN(file.Open(m_Configuration.m_sExportMesh));
      xiiAssetFileHeader header;
      header.SetFileHashAndVersion(xiiHashingUtils::xxHash64(buffer.m_GeometryPageData.GetData(), buffer.m_GeometryPageData.GetCount()), 18U);
      header.SetGenerator("GpuDrivenScene");
      XII_SUCCEED_OR_RETURN(header.Write(file));
      XII_SUCCEED_OR_RETURN(descriptor.Save(file));
      XII_SUCCEED_OR_RETURN(file.Flush());
    }
    xiiStringBuilder name;
    name.SetFormat("GpuDrivenScene.Mesh{}", i);
    auto& asset   = m_GeometryAssets[i];
    asset.m_hMesh = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(name, std::move(descriptor));
  }
  return XII_SUCCESS;
}

xiiResult xiiGpuDrivenSceneWorld::CreateSceneObjects()
{
  XII_LOCK(m_World->GetWriteMarker());
  m_World->GetOrCreateModule<xiiRenderWorldModule>();
  xiiGameObject*           root = nullptr;
  xiiGameObjectDescription rootDescription;
  rootDescription.m_bDynamic = true;
  m_hAssemblyRoot            = m_World->CreateObject(rootDescription, root);
  for (xiiUInt32 y = 0; y < m_Configuration.m_uiGridHeight; ++y)
    for (xiiUInt32 x = 0; x < m_Configuration.m_uiGridWidth; ++x)
    {
      const auto               index = m_Objects.GetCount();
      const xiiVec3            position(6 + y * m_Configuration.m_fObjectSpacing, (float(x) - float(m_Configuration.m_uiGridWidth - 1) * 0.5f) * m_Configuration.m_fObjectSpacing, (float(index % 5) - 2) * 0.32f);
      xiiGameObjectDescription description;
      description.m_bDynamic      = true;
      description.m_LocalPosition = position;
      if (index < 64)
        description.m_hParent = m_hAssemblyRoot;
      xiiGameObject* object = nullptr;
      m_Objects.PushBack(m_World->CreateObject(description, object));
      m_BasePositions.PushBack(position);
      xiiLODMeshComponent* mesh = nullptr;
      xiiLODMeshComponent::CreateComponent(object, mesh);
      mesh->SetMesh(m_GeometryAssets[index % m_GeometryAssets.GetCount()].m_hMesh);
      mesh->SetMaterialOverride(0, m_Materials[index % m_Materials.GetCount()]);
    }
  m_World->Update();
  return XII_SUCCESS;
}

void xiiGpuDrivenSceneWorld::Update(xiiTime deltaTime)
{
  XII_LOCK(m_World->GetWriteMarker());
  m_fAnimationTime += static_cast<float>(deltaTime.GetSeconds());
  const auto animated = xiiMath::Min<xiiUInt32>(m_Objects.GetCount(), 64);
  for (xiiUInt32 i = 0; i < animated; ++i)
  {
    xiiGameObject* object = nullptr;
    if (!m_World->TryGetObject(m_Objects[i], object))
      continue;
    xiiTransform transform = xiiTransform::MakeIdentity();
    transform.m_vPosition  = m_BasePositions[i];
    transform.m_vPosition.z += 0.45f * xiiMath::Sin(xiiAngle::MakeFromRadian(m_fAnimationTime * 1.7f + float(i) * 0.31f));
    transform.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromRadian(m_fAnimationTime * 0.3f + float(i) * 0.01f));
    if (i == 0)
      transform.m_vScale = xiiVec3(1, 0.65f, 1.35f);
    object->SetLocalPosition(transform.m_vPosition);
    object->SetLocalRotation(transform.m_qRotation);
    object->SetLocalScaling(transform.m_vScale);
  }
  xiiGameObject* root = nullptr;
  if (m_World->TryGetObject(m_hAssemblyRoot, root))
  {
    root->SetLocalPosition(xiiVec3(0, 0, 0.15f * xiiMath::Sin(xiiAngle::MakeFromRadian(m_fAnimationTime * 0.5f))));
    root->SetLocalRotation(xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromRadian(0.035f * xiiMath::Sin(xiiAngle::MakeFromRadian(m_fAnimationTime * 0.25f)))));
  }
  m_World->Update();
}
