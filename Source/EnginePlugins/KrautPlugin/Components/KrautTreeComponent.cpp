#include <KrautPlugin/KrautPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiKrautTreeComponent, 3, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("KrautTree", GetKrautFile, SetKrautFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Kraut_Tree")),
    XII_ACCESSOR_PROPERTY("VariationIndex", GetVariationIndex, SetVariationIndex)->AddAttributes(new xiiDefaultValueAttribute(0xFFFF)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgExtractGeometry, OnMsgExtractGeometry),
    XII_MESSAGE_HANDLER(xiiMsgBuildStaticMesh, OnBuildStaticMesh),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Terrain"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiKrautTreeComponent::xiiKrautTreeComponent()  = default;
xiiKrautTreeComponent::~xiiKrautTreeComponent() = default;

void xiiKrautTreeComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hKrautGenerator;
  s << m_uiVariationIndex;
  s << m_uiCustomRandomSeed;
}

void xiiKrautTreeComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion <= 1)
  {
    s >> m_hKrautTree;
  }
  else
  {
    s >> m_hKrautGenerator;
  }

  s >> m_uiVariationIndex;
  s >> m_uiCustomRandomSeed;

  if (uiVersion == 2)
  {
    xiiUInt16 m_uiDefaultVariationIndex;
    s >> m_uiDefaultVariationIndex;
  }

  GetWorld()->GetOrCreateComponentManager<xiiKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
}

xiiResult xiiKrautTreeComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  if (m_hKrautTree.IsValid())
  {
    xiiResourceLock<xiiKrautTreeResource> pTree(m_hKrautTree, xiiResourceAcquireMode::AllowLoadingFallback);
    // TODO: handle fallback case properly

    bounds = pTree->GetDetails().m_Bounds;

    {
      // this is a work around to make shadows and LODing work better
      // shadows do not affect the maximum LOD of a tree that is being rendered,
      // otherwise moving/rotating light-sources would cause LOD popping artifacts
      // and would generally result in more detailed tree rendering than typically necessary
      // however, that means when one is facing away from a tree, but can see its shadow,
      // the shadow may disappear entirely, because no view is setting a decent LOD level
      //
      // by artificially increasing its bbox the main camera will affect the LOD much longer,
      // even when not looking at the tree, thus resulting in decent shadows

      bounds.m_fSphereRadius *= s_iLocalBoundsScale;
      bounds.m_vBoxHalfExtends *= (float)s_iLocalBoundsScale;
    }

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiKrautTreeComponent::SetKrautFile(const char* szFile)
{
  xiiKrautGeneratorResourceHandle hTree;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hTree = xiiResourceManager::LoadResource<xiiKrautGeneratorResource>(szFile);
  }

  SetKrautGeneratorResource(hTree);
}

const char* xiiKrautTreeComponent::GetKrautFile() const
{
  if (!m_hKrautGenerator.IsValid())
    return "";

  return m_hKrautGenerator.GetResourceID();
}

void xiiKrautTreeComponent::SetVariationIndex(xiiUInt16 uiIndex)
{
  if (m_uiVariationIndex == uiIndex)
    return;

  m_uiVariationIndex = uiIndex;

  if (IsActiveAndInitialized() && m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<xiiKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

xiiUInt16 xiiKrautTreeComponent::GetVariationIndex() const
{
  return m_uiVariationIndex;
}

void xiiKrautTreeComponent::SetCustomRandomSeed(xiiUInt16 uiSeed)
{
  if (m_uiCustomRandomSeed == uiSeed)
    return;

  m_uiCustomRandomSeed = uiSeed;

  if (IsActiveAndInitialized() && m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<xiiKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

xiiUInt16 xiiKrautTreeComponent::GetCustomRandomSeed() const
{
  return m_uiCustomRandomSeed;
}

void xiiKrautTreeComponent::SetKrautGeneratorResource(const xiiKrautGeneratorResourceHandle& hTree)
{
  if (m_hKrautGenerator == hTree)
    return;

  m_hKrautGenerator = hTree;

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetOrCreateComponentManager<xiiKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void xiiKrautTreeComponent::OnActivated()
{
  SUPER::OnActivated();

  m_hKrautTree.Invalidate();
  m_vWindSpringPos.SetZero();
  m_vWindSpringVel.SetZero();

  if (m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<xiiKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void xiiKrautTreeComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (!m_hKrautTree.IsValid())
    return;

  xiiResourceLock<xiiKrautTreeResource> pTree(m_hKrautTree, xiiResourceAcquireMode::AllowLoadingFallback);

  // if (pTree.GetAcquireResult() != xiiResourceAcquireResult::Final)
  //   return;

  ComputeWind();

  // ignore scale, the shader expects the wind strength in the global 0-20 m/sec range
  const xiiVec3 vLocalWind = -GetOwner()->GetGlobalRotation() * m_vWindSpringPos;

  const xiiUInt8 uiMaxLods = static_cast<xiiUInt8>(pTree->GetTreeLODs().GetCount());
  for (xiiUInt8 uiCurLod = 0; uiCurLod < uiMaxLods; ++uiCurLod)
  {
    const auto& lodData = pTree->GetTreeLODs()[uiCurLod];

    if (!lodData.m_hMesh.IsValid())
      continue;

    const xiiUInt32 uiMeshIDHash = xiiHashingUtils::StringHashTo32(lodData.m_hMesh.GetResourceIDHash());

    xiiResourceLock<xiiMeshResource>                      pMesh(lodData.m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    xiiArrayPtr<const xiiMeshResourceDescriptor::SubMesh> subMeshes = pMesh->GetSubMeshes();

    const auto materials = pMesh->GetMaterials();

    const xiiGameObject* pOwner = GetOwner();

    float fGlobalUniformScale = pOwner->GetGlobalScalingSimd().HorizontalSum<3>() * xiiSimdFloat(1.0f / 3.0f);

    const xiiTransform         tOwner      = pOwner->GetGlobalTransform();
    const xiiBoundingBoxSphere bounds      = pOwner->GetGlobalBounds();
    const float                fMinDistSQR = xiiMath::Square(fGlobalUniformScale * lodData.m_fMinLodDistance);
    const float                fMaxDistSQR = xiiMath::Square(fGlobalUniformScale * lodData.m_fMaxLodDistance);

    for (xiiUInt32 subMeshIdx = 0; subMeshIdx < subMeshes.GetCount(); ++subMeshIdx)
    {
      const auto& subMesh = subMeshes[subMeshIdx];

      const xiiUInt32 uiMaterialIndex = subMesh.m_uiMaterialIndex;

      if (uiMaterialIndex >= materials.GetCount())
        continue;

      const xiiMaterialResourceHandle& hMaterial        = materials[uiMaterialIndex];
      const xiiUInt32                  uiMaterialIDHash = hMaterial.IsValid() ? xiiHashingUtils::StringHashTo32(hMaterial.GetResourceIDHash()) : 0;

      // Generate batch id from mesh, material and part index.
      const xiiUInt32 data[]    = {uiMeshIDHash, uiMaterialIDHash, subMeshIdx, 0};
      const xiiUInt32 uiBatchId = xiiHashingUtils::xxHash32(data, sizeof(data));

      xiiKrautRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiKrautRenderData>(GetOwner());

      {
        pRenderData->m_uiBatchId    = uiBatchId;
        pRenderData->m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + subMeshIdx) & 0xFFFF);

        pRenderData->m_uiThisLodIndex = uiCurLod;

        pRenderData->m_GlobalTransform = tOwner;
        pRenderData->m_GlobalBounds    = bounds;
        pRenderData->m_hMesh           = lodData.m_hMesh;
        pRenderData->m_uiSubMeshIndex  = static_cast<xiiUInt8>(subMeshIdx);
        pRenderData->m_uiUniqueID      = GetUniqueIdForRendering(uiMaterialIndex);
        pRenderData->m_bCastShadows    = (lodData.m_LodType == xiiKrautLodType::Mesh);

        pRenderData->m_vLeafCenter        = pTree->GetDetails().m_vLeafCenter;
        pRenderData->m_fLodDistanceMinSQR = fMinDistSQR;
        pRenderData->m_fLodDistanceMaxSQR = fMaxDistSQR;

        pRenderData->m_vWindTrunk    = vLocalWind;
        pRenderData->m_vWindBranches = vLocalWind;
      }

      // TODO: somehow make Kraut render data static again and pass along the wind vectors differently
      msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::LitOpaque, xiiRenderData::Caching::Never);
    }
  }
}

xiiResult xiiKrautTreeComponent::CreateGeometry(xiiGeometry& geo, xiiWorldGeoExtractionUtil::ExtractionMode mode) const
{
  if (GetOwner()->IsDynamic())
    return XII_FAILURE;

  // EnsureTreeIsGenerated(); // not const

  if (!m_hKrautTree.IsValid())
    return XII_FAILURE;

  xiiResourceLock<xiiKrautTreeResource> pTree(m_hKrautTree, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTree.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;

  const auto& details = pTree->GetDetails();

  if (mode == xiiWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
  {
    // TODO: support to load the actual tree mesh and return it
  }
  // else
  {
    const float fHeightScale = GetOwner()->GetGlobalScalingSimd().z();
    const float fMaxScale    = GetOwner()->GetGlobalScalingSimd().HorizontalMax<3>();

    if (details.m_fStaticColliderRadius * fMaxScale <= 0.0f)
      return XII_FAILURE;

    const float fTreeHeight = (details.m_Bounds.m_vCenter.z + details.m_Bounds.m_vBoxHalfExtends.z) * 0.9f;

    if (fHeightScale * fTreeHeight <= 0.0f)
      return XII_FAILURE;

    // using a cone or even a cylinder with a thinner top results in the character controller getting stuck while sliding along the geometry
    // TODO: instead of triangle geometry it would maybe be better to use actual physics capsules

    // due to 'transform' this will already include the tree scale
    geo.AddCylinderOnePiece(details.m_fStaticColliderRadius, details.m_fStaticColliderRadius, fTreeHeight, 0.0f, 8);

    geo.TriangulatePolygons();
  }

  return XII_SUCCESS;
}

void xiiKrautTreeComponent::EnsureTreeIsGenerated()
{
  if (!m_hKrautGenerator.IsValid())
    return;

  xiiResourceLock<xiiKrautGeneratorResource> pResource(m_hKrautGenerator, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pResource.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  xiiKrautTreeResourceHandle hNewTree;

  if (m_uiCustomRandomSeed != 0xFFFF)
  {
    hNewTree = pResource->GenerateTree(m_uiCustomRandomSeed);
  }
  else
  {
    if (m_uiVariationIndex == 0xFFFF)
    {
      hNewTree = pResource->GenerateTreeWithGoodSeed(GetOwner()->GetStableRandomSeed() & 0xFFFF);
    }
    else
    {
      hNewTree = pResource->GenerateTreeWithGoodSeed(m_uiVariationIndex);
    }
  }

  if (m_hKrautTree != hNewTree)
  {
    m_hKrautTree = hNewTree;
    TriggerLocalBoundsUpdate();
  }
}

void xiiKrautTreeComponent::ComputeWind() const
{
  if (!IsActiveAndSimulating())
    return;

  // ComputeWind() is called by the renderer extraction, which happens once for every view
  // make sure the wind update happens only once per frame, otherwise the spring would behave differently
  // depending on how many light sources (with shadows) shine on a tree
  if (xiiRenderWorld::GetFrameCounter() == m_uiLastWindUpdate)
    return;

  m_uiLastWindUpdate = xiiRenderWorld::GetFrameCounter();

  const xiiWindWorldModuleInterface* pWindInterface = GetWorld()->GetModuleReadOnly<xiiWindWorldModuleInterface>();

  if (!pWindInterface)
    return;

  auto pOwnder = GetOwner();

  const xiiVec3 vOwnerPos      = pOwnder->GetGlobalPosition();
  const xiiVec3 vSampleWindPos = vOwnerPos + xiiVec3(0, 0, 2);
  const xiiVec3 vWindForce     = pWindInterface->GetWindAt(vSampleWindPos);

  const float realTimeStep = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  // springy wind force
  {
    const float fOverallStrength = 4.0f;

    const float fSpringConstant = 1.0f;
    const float fSpringDamping  = 0.5f;
    const float fTreeMass       = 1.0f;

    const xiiVec3 vSpringForce = -(fSpringConstant * m_vWindSpringPos + fSpringDamping * m_vWindSpringVel);

    const xiiVec3 vTotalForce = vWindForce + vSpringForce;

    // F = mass*acc
    // acc = F / mass
    const xiiVec3 vTreeAcceleration = vTotalForce / fTreeMass;

    m_vWindSpringVel += vTreeAcceleration * realTimeStep * fOverallStrength;
    m_vWindSpringPos += m_vWindSpringVel * realTimeStep * fOverallStrength;
  }

  // debug draw wind vectors
  if (false)
  {
    const xiiVec3 offset = GetOwner()->GetGlobalPosition() + xiiVec3(2, 0, 1);

    xiiHybridArray<xiiDebugRenderer::Line, 2> lines;

    // actual wind
    {
      auto& l        = lines.ExpandAndGetRef();
      l.m_start      = offset;
      l.m_end        = offset + vWindForce;
      l.m_startColor = xiiColor::BlueViolet;
      l.m_endColor   = xiiColor::PowderBlue;
    }

    // springy wind
    {
      auto& l        = lines.ExpandAndGetRef();
      l.m_start      = offset;
      l.m_end        = offset + m_vWindSpringPos;
      l.m_startColor = xiiColor::BlueViolet;
      l.m_endColor   = xiiColor::MediumVioletRed;
    }

    // springy wind 2
    {
      auto& l        = lines.ExpandAndGetRef();
      l.m_start      = offset;
      l.m_end        = offset + m_vWindSpringPos;
      l.m_startColor = xiiColor::LightGoldenRodYellow;
      l.m_endColor   = xiiColor::MediumVioletRed;
    }

    xiiDebugRenderer::DrawLines(GetWorld(), lines, xiiColor::White);

    xiiStringBuilder tmp;
    tmp.Format("Wind: {}m/s", m_vWindSpringPos.GetLength());

    xiiDebugRenderer::Draw3DText(GetWorld(), tmp, GetOwner()->GetGlobalPosition() + xiiVec3(0, 0, 1), xiiColor::DeepSkyBlue);
  }
}

void xiiKrautTreeComponent::OnMsgExtractGeometry(xiiMsgExtractGeometry& msg) const
{
  xiiStringBuilder sResourceName;
  sResourceName.Format("KrautTreeCpu:{}", m_hKrautGenerator.GetResourceID());

  xiiCpuMeshResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiCpuMeshResource>(sResourceName);
  if (!hMesh.IsValid())
  {
    xiiGeometry geo;
    if (CreateGeometry(geo, msg.m_Mode).Failed())
      return;

    xiiMeshResourceDescriptor desc;

    desc.MeshBufferDesc().AddCommonStreams();
    desc.MeshBufferDesc().AllocateStreamsFromGeometry(geo, xiiGALPrimitiveTopology::Triangles);

    desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

    desc.ComputeBounds();

    hMesh = xiiResourceManager::GetOrCreateResource<xiiCpuMeshResource>(sResourceName, std::move(desc), sResourceName);
  }

  msg.AddMeshObject(GetOwner()->GetGlobalTransform(), hMesh);
}

void xiiKrautTreeComponent::OnBuildStaticMesh(xiiMsgBuildStaticMesh& msg) const
{
  xiiGeometry geo;
  if (CreateGeometry(geo, xiiWorldGeoExtractionUtil::ExtractionMode::CollisionMesh).Failed())
    return;

  auto& desc    = *msg.m_pStaticMeshDescription;
  auto& subMesh = msg.m_pStaticMeshDescription->m_SubMeshes.ExpandAndGetRef();

  {
    xiiResourceLock<xiiKrautTreeResource> pTree(m_hKrautTree, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pTree.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return;

    const auto& details = pTree->GetDetails();

    if (!details.m_sSurfaceResource.IsEmpty())
    {
      subMesh.m_uiSurfaceIndex = static_cast<xiiUInt16>(desc.m_Surfaces.GetCount());
      desc.m_Surfaces.PushBack(details.m_sSurfaceResource);
    }
  }

  const xiiTransform transform = GetOwner()->GetGlobalTransform();

  subMesh.m_uiFirstTriangle = desc.m_Triangles.GetCount();
  subMesh.m_uiNumTriangles  = geo.GetPolygons().GetCount();

  const xiiUInt32 uiFirstVertex = desc.m_Vertices.GetCount();

  for (const auto& vtx : geo.GetVertices())
  {
    desc.m_Vertices.ExpandAndGetRef() = transform.TransformPosition(vtx.m_vPosition);
  }

  for (const auto& tri : geo.GetPolygons())
  {
    auto& t                = desc.m_Triangles.ExpandAndGetRef();
    t.m_uiVertexIndices[0] = uiFirstVertex + tri.m_Vertices[0];
    t.m_uiVertexIndices[1] = uiFirstVertex + tri.m_Vertices[1];
    t.m_uiVertexIndices[2] = uiFirstVertex + tri.m_Vertices[2];
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiKrautTreeComponentManager::Initialize()
{
  SUPER::Initialize();

  xiiWorldModule::UpdateFunctionDesc desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiKrautTreeComponentManager::Update, this);
  desc.m_Phase                            = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiKrautTreeComponentManager::ResourceEventHandler, this));
}

void xiiKrautTreeComponentManager::Deinitialize()
{
  XII_LOCK(m_Mutex);

  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiKrautTreeComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void xiiKrautTreeComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  xiiDeque<xiiComponentHandle> requireUpdate;

  {
    XII_LOCK(m_Mutex);
    requireUpdate.Swap(m_RequireUpdate);
  }

  for (const auto& hComp : requireUpdate)
  {
    xiiKrautTreeComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp) || !pComp->IsActiveAndInitialized())
      continue;

    // TODO: this could be wrapped into a task
    pComp->EnsureTreeIsGenerated();
  }
}

void xiiKrautTreeComponentManager::EnqueueUpdate(xiiComponentHandle hComponent)
{
  XII_LOCK(m_Mutex);

  if (m_RequireUpdate.IndexOf(hComponent) != xiiInvalidIndex)
    return;

  m_RequireUpdate.PushBack(hComponent);
}

void xiiKrautTreeComponentManager::ResourceEventHandler(const xiiResourceEvent& e)
{
  if ((e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading || e.m_Type == xiiResourceEvent::Type::ResourceContentUpdated) && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiKrautGeneratorResource>())
  {
    XII_LOCK(m_Mutex);

    xiiKrautGeneratorResourceHandle hResource((xiiKrautGeneratorResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const xiiKrautTreeComponent* pComponent = static_cast<xiiKrautTreeComponent*>(it.Value());

      if (pComponent->GetKrautGeneratorResource() == hResource)
      {
        EnqueueUpdate(pComponent->GetHandle());
      }
    }
  }
}
