#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/JoltShapeConvexHullComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltShapeConvexHullComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Convex", xiiDependencyFlags::Package)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltShapeConvexHullComponent::xiiJoltShapeConvexHullComponent()  = default;
xiiJoltShapeConvexHullComponent::~xiiJoltShapeConvexHullComponent() = default;

void xiiJoltShapeConvexHullComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hCollisionMesh;
}

void xiiJoltShapeConvexHullComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hCollisionMesh;
}

void xiiJoltShapeConvexHullComponent::SetMeshFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hCollisionMesh = xiiResourceManager::LoadResource<xiiJoltMeshResource>(szFile);
  }
}

const char* xiiJoltShapeConvexHullComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void xiiJoltShapeConvexHullComponent::CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial)
{
  if (!m_hCollisionMesh.IsValid())
  {
    xiiLog::Warning("xiiJoltShapeConvexHullComponent '{0}' has no collision mesh set.", GetOwner()->GetName());
    return;
  }

  xiiResourceLock<xiiJoltMeshResource> pMesh(m_hCollisionMesh, xiiResourceAcquireMode::BlockTillLoaded);

  if (pMesh->GetNumConvexParts() == 0)
  {
    xiiLog::Warning("xiiJoltShapeConvexHullComponent '{0}' has a collision mesh set that does not contain a convex mesh: '{1}' ('{2}')", GetOwner()->GetName(), pMesh->GetResourceID(), pMesh->GetResourceDescription());
    return;
  }

  for (xiiUInt32 i = 0; i < pMesh->GetNumConvexParts(); ++i)
  {
    auto pShape = pMesh->InstantiateConvexPart(i, reinterpret_cast<xiiUInt64>(GetUserData()), pMaterial, fDensity);

    xiiJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
    sub.m_pShape         = pShape;
    sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
  }
}

void xiiJoltShapeConvexHullComponent::ExtractGeometry(xiiMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != xiiWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && ref_msg.m_Mode != xiiWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration)
    return;

  if (m_hCollisionMesh.IsValid())
  {
    xiiResourceLock<xiiJoltMeshResource> pMesh(m_hCollisionMesh, xiiResourceAcquireMode::BlockTillLoaded);

    ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), pMesh->ConvertToCpuMesh());
  }
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeConvexHullComponent);
