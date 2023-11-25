#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <GraphicsCore/Components/BeamComponent.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshComponentBase.h>
#include <GraphicsFoundation/Device/Device.h>


// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiBeamComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("TargetObject", DummyGetter, SetTargetObject)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::White)),
    XII_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new xiiDefaultValueAttribute(0.1f), new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("UVUnitsPerWorldUnit", GetUVUnitsPerWorldUnit, SetUVUnitsPerWorldUnit)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.01f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Effects"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBeamComponent::xiiBeamComponent() = default;

xiiBeamComponent::~xiiBeamComponent() = default;

void xiiBeamComponent::Update()
{
  xiiGameObject* pTargetObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pTargetObject))
  {
    xiiVec3 currentOwnerPosition  = GetOwner()->GetGlobalPosition();
    xiiVec3 currentTargetPosition = pTargetObject->GetGlobalPosition();

    if (!pTargetObject->IsActive())
    {
      currentTargetPosition = currentOwnerPosition;
    }

    bool updateMesh = false;

    if ((currentOwnerPosition - m_vLastOwnerPosition).GetLengthSquared() > m_fDistanceUpdateEpsilon)
    {
      updateMesh           = true;
      m_vLastOwnerPosition = currentOwnerPosition;
    }

    if ((currentTargetPosition - m_vLastTargetPosition).GetLengthSquared() > m_fDistanceUpdateEpsilon)
    {
      updateMesh            = true;
      m_vLastTargetPosition = currentTargetPosition;
    }

    if (updateMesh)
    {
      ReinitMeshes();
    }
  }
  else
  {
    m_hMesh.Invalidate();
  }
}

void xiiBeamComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  inout_stream.WriteGameObjectHandle(m_hTargetObject);

  s << m_fWidth;
  s << m_fUVUnitsPerWorldUnit;
  s << m_hMaterial;
  s << m_Color;
}

void xiiBeamComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s         = inout_stream.GetStream();
  m_hTargetObject = inout_stream.ReadGameObjectHandle();

  s >> m_fWidth;
  s >> m_fUVUnitsPerWorldUnit;
  s >> m_hMaterial;
  s >> m_Color;
}

xiiResult xiiBeamComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  xiiGameObject* pTargetObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pTargetObject))
  {
    const xiiVec3 currentTargetPosition      = pTargetObject->GetGlobalPosition();
    const xiiVec3 targetPositionInOwnerSpace = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(currentTargetPosition);

    xiiVec3 pts[] = {xiiVec3::ZeroVector(), targetPositionInOwnerSpace};

    xiiBoundingBox box        = xiiBoundingBox::MakeFromPoints(pts, 2);
    const float    fHalfWidth = m_fWidth * 0.5f;
    box.m_vMin -= xiiVec3(0, fHalfWidth, fHalfWidth);
    box.m_vMax += xiiVec3(0, fHalfWidth, fHalfWidth);
    ref_bounds = xiiBoundingBoxSphere::MakeFromBox(box);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}


void xiiBeamComponent::OnActivated()
{
  SUPER::OnActivated();

  ReinitMeshes();
}

void xiiBeamComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  Cleanup();
}

void xiiBeamComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  xiiMeshRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds    = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh           = m_hMesh;
    pRenderData->m_hMaterial       = m_hMaterial;
    pRenderData->m_Color           = m_Color;
    pRenderData->m_uiSubMeshIndex  = 0;
    pRenderData->m_uiUniqueID      = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  // Determine render data category.
  xiiResourceLock<xiiMaterialResource> pMaterial(m_hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);
  xiiRenderData::Category              category = pMaterial->GetRenderDataCategory();

  msg.AddRenderData(pRenderData, category, xiiRenderData::Caching::Never);
}

void xiiBeamComponent::SetTargetObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hTargetObject = resolver(szReference, GetHandle(), "TargetObject");

  ReinitMeshes();
}

void xiiBeamComponent::SetWidth(float fWidth)
{
  if (fWidth <= 0.0f)
    return;

  m_fWidth = fWidth;

  ReinitMeshes();
}

float xiiBeamComponent::GetWidth() const
{
  return m_fWidth;
}

void xiiBeamComponent::SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit)
{
  if (fUVUnitsPerWorldUnit <= 0.0f)
    return;

  m_fUVUnitsPerWorldUnit = fUVUnitsPerWorldUnit;

  ReinitMeshes();
}

float xiiBeamComponent::GetUVUnitsPerWorldUnit() const
{
  return m_fUVUnitsPerWorldUnit;
}

void xiiBeamComponent::SetMaterialFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* xiiBeamComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

xiiMaterialResourceHandle xiiBeamComponent::GetMaterial() const
{
  return m_hMaterial;
}

void xiiBeamComponent::CreateMeshes()
{
  xiiVec3 targetPositionInOwnerSpace = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(m_vLastTargetPosition);

  if (targetPositionInOwnerSpace.IsZero(0.01f))
    return;

  // Create the beam mesh name, it expresses the beam in local space with it's width
  // this way multiple beams in a corridor can share the same mesh for example.
  xiiStringBuilder meshName;
  meshName.Format("xiiBeamComponent_{0}_{1}_{2}_{3}.createdAtRuntime.xiiMesh", m_fWidth, xiiArgF(targetPositionInOwnerSpace.x, 2), xiiArgF(targetPositionInOwnerSpace.y, 2), xiiArgF(targetPositionInOwnerSpace.z, 2));

  m_hMesh = xiiResourceManager::GetExistingResource<xiiMeshResource>(meshName);

  // We build a cross mesh, thus we need the following vectors, x is the origin and we need to construct
  // the star points.
  //
  //  3        1
  //
  //      x
  //
  //  4        2
  xiiVec3 crossVector1 = (0.5f * xiiVec3::MakeAxisY() + 0.5f * xiiVec3::MakeAxisZ());
  crossVector1.SetLength(m_fWidth * 0.5f).IgnoreResult();

  xiiVec3 crossVector2 = (0.5f * xiiVec3::MakeAxisY() - 0.5f * xiiVec3::MakeAxisZ());
  crossVector2.SetLength(m_fWidth * 0.5f).IgnoreResult();

  xiiVec3 crossVector3 = (-0.5f * xiiVec3::MakeAxisY() + 0.5f * xiiVec3::MakeAxisZ());
  crossVector3.SetLength(m_fWidth * 0.5f).IgnoreResult();

  xiiVec3 crossVector4 = (-0.5f * xiiVec3::MakeAxisY() - 0.5f * xiiVec3::MakeAxisZ());
  crossVector4.SetLength(m_fWidth * 0.5f).IgnoreResult();

  const float fDistance = (m_vLastOwnerPosition - m_vLastTargetPosition).GetLength();



  // Build mesh if no existing one is found
  if (!m_hMesh.IsValid())
  {
    xiiGeometry g;

    // Quad 1
    {
      xiiUInt32 index0 = g.AddVertex(xiiVec3::ZeroVector() + crossVector1, xiiVec3::MakeAxisX(), xiiVec2(0, 0), xiiColor::White);
      xiiUInt32 index1 = g.AddVertex(xiiVec3::ZeroVector() + crossVector4, xiiVec3::MakeAxisX(), xiiVec2(0, 1), xiiColor::White);
      xiiUInt32 index2 = g.AddVertex(targetPositionInOwnerSpace + crossVector1, xiiVec3::MakeAxisX(), xiiVec2(fDistance * m_fUVUnitsPerWorldUnit, 0), xiiColor::White);
      xiiUInt32 index3 = g.AddVertex(targetPositionInOwnerSpace + crossVector4, xiiVec3::MakeAxisX(), xiiVec2(fDistance * m_fUVUnitsPerWorldUnit, 1), xiiColor::White);

      xiiUInt32 indices[] = {index0, index2, index3, index1};
      g.AddPolygon(xiiArrayPtr(indices), false);
      g.AddPolygon(xiiArrayPtr(indices), true);
    }

    // Quad 2
    {
      xiiUInt32 index0 = g.AddVertex(xiiVec3::ZeroVector() + crossVector2, xiiVec3::MakeAxisX(), xiiVec2(0, 0), xiiColor::White);
      xiiUInt32 index1 = g.AddVertex(xiiVec3::ZeroVector() + crossVector3, xiiVec3::MakeAxisX(), xiiVec2(0, 1), xiiColor::White);
      xiiUInt32 index2 = g.AddVertex(targetPositionInOwnerSpace + crossVector2, xiiVec3::MakeAxisX(), xiiVec2(fDistance * m_fUVUnitsPerWorldUnit, 0), xiiColor::White);
      xiiUInt32 index3 = g.AddVertex(targetPositionInOwnerSpace + crossVector3, xiiVec3::MakeAxisX(), xiiVec2(fDistance * m_fUVUnitsPerWorldUnit, 1), xiiColor::White);

      xiiUInt32 indices[] = {index0, index2, index3, index1};
      g.AddPolygon(xiiArrayPtr(indices), false);
      g.AddPolygon(xiiArrayPtr(indices), true);
    }

    g.ComputeTangents();

    xiiMeshResourceDescriptor desc;
    BuildMeshResourceFromGeometry(g, desc);

    m_hMesh = xiiResourceManager::CreateResource<xiiMeshResource>(meshName, std::move(desc));
  }
}

void xiiBeamComponent::BuildMeshResourceFromGeometry(xiiGeometry& Geometry, xiiMeshResourceDescriptor& MeshDesc) const
{
  auto& MeshBufferDesc = MeshDesc.MeshBufferDesc();

  MeshBufferDesc.AddCommonStreams();
  MeshBufferDesc.AllocateStreamsFromGeometry(Geometry, xiiGALPrimitiveTopology::TriangleList);

  MeshDesc.AddSubMesh(MeshBufferDesc.GetPrimitiveCount(), 0, 0);

  MeshDesc.ComputeBounds();
}

void xiiBeamComponent::ReinitMeshes()
{
  Cleanup();

  if (IsActiveAndInitialized())
  {
    CreateMeshes();
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiBeamComponent::Cleanup()
{
  m_hMesh.Invalidate();
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_BeamComponent);
