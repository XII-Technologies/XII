#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltStaticActorComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Triangle", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("IncludeInNavmesh", m_bIncludeInNavmesh)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("PullSurfacesFromGraphicsMesh", m_bPullSurfacesFromGraphicsMesh),
    XII_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface", xiiDependencyFlags::Package)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractGeometry, OnMsgExtractGeometry),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltStaticActorComponent::xiiJoltStaticActorComponent()
{
  m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
}

xiiJoltStaticActorComponent::~xiiJoltStaticActorComponent() = default;

void xiiJoltStaticActorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hCollisionMesh;
  s << m_bIncludeInNavmesh;
  s << m_bPullSurfacesFromGraphicsMesh;
  s << m_hSurface;
}


void xiiJoltStaticActorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hCollisionMesh;
  s >> m_bIncludeInNavmesh;
  s >> m_bPullSurfacesFromGraphicsMesh;
  s >> m_hSurface;
}

void xiiJoltStaticActorComponent::OnDeactivated()
{
  m_UsedSurfaces.Clear();

  SUPER::OnDeactivated();
}

void xiiJoltStaticActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  xiiJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex          = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  auto* pMaterial = GetJoltMaterial();

  JPH::BodyCreationSettings bodyCfg;
  if (CreateShape(&bodyCfg, 1.0f, pMaterial).Failed())
  {
    xiiLog::Error("Jolt static actor component '{}' has no valid shape.", GetOwner()->GetName());
    return;
  }

  const xiiSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  if (pMaterial == nullptr)
    pMaterial = xiiJoltCore::GetDefaultMaterial();

  bodyCfg.mPosition    = xiiJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation    = xiiJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType  = JPH::EMotionType::Static;
  bodyCfg.mObjectLayer = xiiJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, xiiJoltBroadphaseLayer::Static);
  bodyCfg.mRestitution = pMaterial->m_fRestitution;
  bodyCfg.mFriction    = pMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<xiiUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID   = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, true);
}

void xiiJoltStaticActorComponent::CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial)
{
  if (!m_hCollisionMesh.IsValid())
    return;

  xiiResourceLock<xiiJoltMeshResource> pMesh(m_hCollisionMesh, xiiResourceAcquireMode::BlockTillLoaded);

  if (pMesh->GetNumConvexParts() > 0)
  {
    for (xiiUInt32 i = 0; i < pMesh->GetNumConvexParts(); ++i)
    {
      auto pShape = pMesh->InstantiateConvexPart(i, reinterpret_cast<xiiUInt64>(GetUserData()), pMaterial, fDensity);

      xiiJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
      sub.m_pShape         = pShape;
      sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
    }
  }

  if (auto pTriMesh = pMesh->HasTriangleMesh())
  {
    xiiHybridArray<const xiiJoltMaterial*, 32> materials;

    if (pMaterial != nullptr)
    {
      materials.SetCount(pMesh->GetSurfaces().GetCount(), pMaterial);
    }

    if (m_bPullSurfacesFromGraphicsMesh)
    {
      materials.SetCount(pMesh->GetSurfaces().GetCount());
      PullSurfacesFromGraphicsMesh(materials);
    }

    auto pNewShape = pMesh->InstantiateTriangleMesh(reinterpret_cast<xiiUInt64>(GetUserData()), materials);

    xiiJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
    sub.m_pShape         = pNewShape;
    sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
  }
}

void xiiJoltStaticActorComponent::PullSurfacesFromGraphicsMesh(xiiDynamicArray<const xiiJoltMaterial*>& ref_materials)
{
  // the materials don't hold a handle to the surfaces, so they don't keep them alive
  // therefore, we need to keep them alive by storing a handle
  m_UsedSurfaces.Clear();

  xiiMeshComponent* pMeshComp;
  if (!GetOwner()->TryGetComponentOfBaseType(pMeshComp))
    return;

  auto hMeshRes = pMeshComp->GetMesh();
  if (!hMeshRes.IsValid())
    return;

  xiiResourceLock<xiiMeshResource> pMeshRes(hMeshRes, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pMeshRes.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  if (pMeshRes->GetMaterials().GetCount() != ref_materials.GetCount())
    return;

  const xiiUInt32 uiNumMats = ref_materials.GetCount();
  m_UsedSurfaces.SetCount(uiNumMats);

  for (xiiUInt32 s = 0; s < uiNumMats; ++s)
  {
    // first check whether the component has a material override
    auto hMat = pMeshComp->GetMaterial(s);

    if (!hMat.IsValid())
    {
      // otherwise ask the mesh resource about the material
      hMat = pMeshRes->GetMaterials()[s];
    }

    if (!hMat.IsValid())
      continue;

    xiiResourceLock<xiiMaterialResource> pMat(hMat, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pMat.GetAcquireResult() != xiiResourceAcquireResult::Final)
      continue;

    if (pMat->GetSurface().IsEmpty())
      continue;

    m_UsedSurfaces[s] = xiiResourceManager::LoadResource<xiiSurfaceResource>(pMat->GetSurface().GetString());

    xiiResourceLock<xiiSurfaceResource> pSurface(m_UsedSurfaces[s], xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSurface.GetAcquireResult() != xiiResourceAcquireResult::Final)
      continue;

    XII_ASSERT_DEV(pSurface->m_pPhysicsMaterialJolt != nullptr, "Invalid Jolt material pointer on surface");
    ref_materials[s] = static_cast<xiiJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
  }
}

void xiiJoltStaticActorComponent::OnMsgExtractGeometry(xiiMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == xiiWorldGeoExtractionUtil::ExtractionMode::CollisionMesh || (msg.m_Mode == xiiWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration && m_bIncludeInNavmesh))
  {
    if (m_hCollisionMesh.IsValid())
    {
      xiiResourceLock<xiiJoltMeshResource> pMesh(m_hCollisionMesh, xiiResourceAcquireMode::BlockTillLoaded);

      msg.AddMeshObject(GetOwner()->GetGlobalTransform(), pMesh->ConvertToCpuMesh());
    }

    ExtractSubShapeGeometry(GetOwner(), msg);
  }
}

void xiiJoltStaticActorComponent::SetMeshFile(const char* szFile)
{
  xiiJoltMeshResourceHandle hMesh;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = xiiResourceManager::LoadResource<xiiJoltMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* xiiJoltStaticActorComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void xiiJoltStaticActorComponent::SetMesh(const xiiJoltMeshResourceHandle& hMesh)
{
  m_hCollisionMesh = hMesh;
}

const xiiJoltMaterial* xiiJoltStaticActorComponent::GetJoltMaterial() const
{
  if (m_hSurface.IsValid())
  {
    xiiResourceLock<xiiSurfaceResource> pSurface(m_hSurface, xiiResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<xiiJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return nullptr;
}

void xiiJoltStaticActorComponent::SetSurfaceFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = xiiResourceManager::LoadResource<xiiSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    xiiResourceManager::PreloadResource(m_hSurface);
}

const char* xiiJoltStaticActorComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltStaticActorComponent);
