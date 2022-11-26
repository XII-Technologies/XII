#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Components/JoltVisColMeshComponent.h>
#include <JoltPlugin/Shapes/JoltShapeConvexHullComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltVisColMeshComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Triangle;CompatibleAsset_Jolt_Colmesh_Convex")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Misc"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltVisColMeshComponent::xiiJoltVisColMeshComponent()  = default;
xiiJoltVisColMeshComponent::~xiiJoltVisColMeshComponent() = default;

void xiiJoltVisColMeshComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hCollisionMesh;
}


void xiiJoltVisColMeshComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hCollisionMesh;

  GetWorld()->GetOrCreateComponentManager<xiiJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
}

xiiResult xiiJoltVisColMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  // have to assume this isn't thread safe
  // CreateCollisionRenderMesh();

  if (m_hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);
    bounds = pMesh->GetBounds();
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiJoltVisColMeshComponent::SetMeshFile(const char* szFile)
{
  xiiJoltMeshResourceHandle hMesh;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = xiiResourceManager::LoadResource<xiiJoltMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* xiiJoltVisColMeshComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void xiiJoltVisColMeshComponent::SetMesh(const xiiJoltMeshResourceHandle& hMesh)
{
  if (m_hCollisionMesh != hMesh)
  {
    m_hCollisionMesh = hMesh;
    m_hMesh.Invalidate();

    GetWorld()->GetOrCreateComponentManager<xiiJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void xiiJoltVisColMeshComponent::CreateCollisionRenderMesh()
{
  if (!m_hCollisionMesh.IsValid())
  {
    xiiJoltStaticActorComponent* pSibling = nullptr;
    if (GetOwner()->TryGetComponentOfBaseType(pSibling))
    {
      m_hCollisionMesh = pSibling->GetMesh();
    }
  }

  if (!m_hCollisionMesh.IsValid())
  {
    xiiJoltShapeConvexHullComponent* pSibling = nullptr;
    if (GetOwner()->TryGetComponentOfBaseType(pSibling))
    {
      m_hCollisionMesh = pSibling->GetMesh();
    }
  }

  if (!m_hCollisionMesh.IsValid())
    return;

  xiiResourceLock<xiiJoltMeshResource> pMesh(m_hCollisionMesh, xiiResourceAcquireMode::BlockTillLoaded);

  if (pMesh.GetAcquireResult() == xiiResourceAcquireResult::MissingFallback)
    return;

  xiiStringBuilder sColMeshName = pMesh->GetResourceID();
  sColMeshName.AppendFormat("_{0}_JoltVisColMesh",
                            pMesh->GetCurrentResourceChangeCounter()); // the change counter allows to react to resource updates

  m_hMesh = xiiResourceManager::GetExistingResource<xiiMeshResource>(sColMeshName);

  if (m_hMesh.IsValid())
  {
    TriggerLocalBoundsUpdate();
    return;
  }

  xiiCpuMeshResourceHandle hCpuMesh = pMesh->ConvertToCpuMesh();

  if (!hCpuMesh.IsValid())
    return;

  xiiResourceLock<xiiCpuMeshResource> pCpuMesh(hCpuMesh, xiiResourceAcquireMode::BlockTillLoaded);

  xiiMeshResourceDescriptor md = pCpuMesh->GetDescriptor();

  md.SetMaterial(0, "Materials/Common/ColMesh.xiiMaterial");

  m_hMesh = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(sColMeshName, std::move(md), "Collision Mesh Visualization");

  TriggerLocalBoundsUpdate();
}

void xiiJoltVisColMeshComponent::Initialize()
{
  SUPER::Initialize();

  GetWorld()->GetOrCreateComponentManager<xiiJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
}

void xiiJoltVisColMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);

  xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::IfStatic;

  if (pMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    caching = xiiRenderData::Caching::Never;
  }

  xiiArrayPtr<const xiiMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (xiiUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const xiiUInt32           uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    xiiMaterialResourceHandle hMaterial       = pMesh->GetMaterials()[uiMaterialIndex];

    xiiMeshRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiMeshRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds    = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh           = m_hMesh;
      pRenderData->m_hMaterial       = hMaterial;
      pRenderData->m_uiSubMeshIndex  = uiPartIndex;
      pRenderData->m_uiUniqueID      = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::LitOpaque, caching);
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiJoltVisColMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  xiiWorldModule::UpdateFunctionDesc desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltVisColMeshComponentManager::Update, this);
  desc.m_Phase                            = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiJoltVisColMeshComponentManager::ResourceEventHandler, this));
}

void xiiJoltVisColMeshComponentManager::Deinitialize()
{
  XII_LOCK(m_Mutex);

  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiJoltVisColMeshComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void xiiJoltVisColMeshComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  xiiDeque<xiiComponentHandle> requireUpdate;
  m_RequireUpdate.Swap(requireUpdate);

  for (const auto& hComp : requireUpdate)
  {
    xiiJoltVisColMeshComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp))
      continue;

    pComp->CreateCollisionRenderMesh();
  }
}

void xiiJoltVisColMeshComponentManager::EnqueueUpdate(xiiComponentHandle hComponent)
{
  m_RequireUpdate.PushBack(hComponent);
}

void xiiJoltVisColMeshComponentManager::ResourceEventHandler(const xiiResourceEvent& e)
{
  if ((e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading || e.m_Type == xiiResourceEvent::Type::ResourceContentUpdated) && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiJoltMeshResource>())
  {
    XII_LOCK(m_Mutex);

    xiiJoltMeshResourceHandle hResource((xiiJoltMeshResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const xiiJoltVisColMeshComponent* pComponent = static_cast<xiiJoltVisColMeshComponent*>(it.Value());

      if (pComponent->GetMesh() == hResource)
      {
        m_RequireUpdate.PushBack(pComponent->GetHandle());
      }
    }
  }
}
