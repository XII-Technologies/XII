#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Meshes/LodMeshComponent.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiLodMeshLod, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiLodMeshLod>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_MEMBER_PROPERTY("Mesh", m_hMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    XII_MEMBER_PROPERTY("Threshold", m_fThreshold)
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiLodMeshComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
    XII_MEMBER_PROPERTY("BoundsOffset", m_vBoundsOffset),
    XII_MEMBER_PROPERTY("BoundsRadius", m_fBoundsRadius)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.01f, 100.0f)),
    XII_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    XII_ACCESSOR_PROPERTY("OverlapRanges", GetOverlapRanges, SetOverlapRanges)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ARRAY_MEMBER_PROPERTY("Meshes", m_Meshes),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
    new xiiSphereVisualizerAttribute("BoundsRadius", xiiColor::MediumVioletRed, nullptr, xiiVisualizerAnchor::Center, xiiVec3(1.0f), "BoundsOffset"),
    new xiiTransformManipulatorAttribute("BoundsOffset"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgSetColor, OnMsgSetColor),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

struct LodMeshCompFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    OverlapRanges = 1,
  };
};

xiiLodMeshComponent::xiiLodMeshComponent()  = default;
xiiLodMeshComponent::~xiiLodMeshComponent() = default;

void xiiLodMeshComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(LodMeshCompFlags::ShowDebugInfo, bShow);
}

bool xiiLodMeshComponent::GetShowDebugInfo() const
{
  return GetUserFlag(LodMeshCompFlags::ShowDebugInfo);
}

void xiiLodMeshComponent::SetOverlapRanges(bool bShow)
{
  SetUserFlag(LodMeshCompFlags::OverlapRanges, bShow);
}

bool xiiLodMeshComponent::GetOverlapRanges() const
{
  return GetUserFlag(LodMeshCompFlags::OverlapRanges);
}

void xiiLodMeshComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_Meshes.GetCount();
  for (const auto& mesh : m_Meshes)
  {
    s << mesh.m_hMesh;
    s << mesh.m_fThreshold;
  }

  s << m_Color;
  s << m_fSortingDepthOffset;

  s << m_vBoundsOffset;
  s << m_fBoundsRadius;
}

void xiiLodMeshComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = inout_stream.GetStream();

  xiiUInt32 uiMeshes = 0;
  s >> uiMeshes;

  m_Meshes.SetCount(uiMeshes);

  for (auto& mesh : m_Meshes)
  {
    s >> mesh.m_hMesh;
    s >> mesh.m_fThreshold;
  }

  s >> m_Color;
  s >> m_fSortingDepthOffset;

  s >> m_vBoundsOffset;
  s >> m_fBoundsRadius;
}

xiiResult xiiLodMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  out_bounds         = xiiBoundingSphere::MakeFromCenterAndRadius(m_vBoundsOffset, m_fBoundsRadius);
  out_bAlwaysVisible = false;
  return XII_SUCCESS;
}

void xiiLodMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (m_Meshes.IsEmpty())
    return;

  if (msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::EditorView || msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::MainView)
  {
    UpdateSelectedLod(*msg.m_pView);
  }

  if (m_iCurLod >= (xiiInt32)m_Meshes.GetCount())
    return;

  auto hMesh = m_Meshes[m_iCurLod].m_hMesh;

  if (!hMesh.IsValid())
    return;

  xiiResourceLock<xiiMeshResource>                      pMesh(hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
  xiiArrayPtr<const xiiMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (xiiUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const xiiUInt32           uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    xiiMaterialResourceHandle hMaterial;

    hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    xiiMeshRenderData* pRenderData = CreateRenderData();
    {
      pRenderData->m_GlobalTransform     = GetOwner()->GetGlobalTransform() * pRenderData->m_GlobalTransform;
      pRenderData->m_GlobalBounds        = GetOwner()->GetGlobalBounds();
      pRenderData->m_fSortingDepthOffset = m_fSortingDepthOffset;
      pRenderData->m_hMesh               = hMesh;
      pRenderData->m_hMaterial           = hMaterial;
      pRenderData->m_Color               = m_Color;
      pRenderData->m_uiSubMeshIndex      = uiPartIndex;
      pRenderData->m_uiUniqueID          = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillSortingKey();
    }

    // Determine render data category.
    xiiRenderData::Category category = xiiDefaultRenderDataCategories::Opaque;
    if (hMaterial.IsValid())
    {
      xiiResourceLock<xiiMaterialResource> pMaterial(hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);

      category = pMaterial->GetRenderDataCategory();
    }

    msg.AddRenderData(pRenderData, category, xiiRenderData::Caching::Never);
  }
}

void xiiLodMeshComponent::SetColor(const xiiColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const xiiColor& xiiLodMeshComponent::GetColor() const
{
  return m_Color;
}

void xiiLodMeshComponent::SetSortingDepthOffset(float fOffset)
{
  m_fSortingDepthOffset = fOffset;

  InvalidateCachedRenderData();
}

float xiiLodMeshComponent::GetSortingDepthOffset() const
{
  return m_fSortingDepthOffset;
}

void xiiLodMeshComponent::OnMsgSetColor(xiiMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

xiiMeshRenderData* xiiLodMeshComponent::CreateRenderData() const
{
  return xiiCreateRenderDataForThisFrame<xiiMeshRenderData>(GetOwner());
}

static float CalculateSphereScreenSpaceCoverage(const xiiBoundingSphere& sphere, const xiiCamera& camera)
{
  if (camera.IsPerspective())
  {
    return xiiGraphicsUtils::CalculateSphereScreenCoverage(sphere, camera.GetCenterPosition(), camera.GetFovY(1.0f));
  }
  else
  {
    return xiiGraphicsUtils::CalculateSphereScreenCoverage(sphere.m_fRadius, camera.GetDimensionY(1.0f));
  }
}

void xiiLodMeshComponent::UpdateSelectedLod(const xiiView& view) const
{
  const xiiInt32 iNumLods = (xiiInt32)m_Meshes.GetCount();

  const xiiVec3 vScale  = GetOwner()->GetGlobalScaling();
  const float   fScale  = xiiMath::Max(vScale.x, vScale.y, vScale.z);
  const xiiVec3 vCenter = GetOwner()->GetGlobalTransform() * m_vBoundsOffset;

  const float fCoverage = CalculateSphereScreenSpaceCoverage(xiiBoundingSphere::MakeFromCenterAndRadius(vCenter, fScale * m_fBoundsRadius), *view.GetLodCamera());

  // clamp the input value, this is to prevent issues while editing the threshold array
  xiiInt32 iNewLod = xiiMath::Clamp<xiiInt32>(m_iCurLod, 0, iNumLods);

  float fCoverageP = 1;
  float fCoverageN = 0;

  if (iNewLod > 0)
  {
    fCoverageP = m_Meshes[iNewLod - 1].m_fThreshold;
  }

  if (iNewLod < iNumLods)
  {
    fCoverageN = m_Meshes[iNewLod].m_fThreshold;
  }

  if (GetOverlapRanges())
  {
    const float fLodRangeOverlap = 0.40f;

    if (iNewLod + 1 < iNumLods)
    {
      float range = (fCoverageN - m_Meshes[iNewLod + 1].m_fThreshold);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
    else
    {
      float range = (fCoverageN - 0.0f);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
  }

  if (fCoverage < fCoverageN)
  {
    ++iNewLod;
  }
  else if (fCoverage > fCoverageP)
  {
    --iNewLod;
  }

  iNewLod   = xiiMath::Clamp(iNewLod, 0, iNumLods);
  m_iCurLod = iNewLod;

  if (GetShowDebugInfo())
  {
    xiiStringBuilder sb;
    sb.SetFormat("Coverage: {}\nLOD {}\nRange: {} - {}", xiiArgF(fCoverage, 3), iNewLod, xiiArgF(fCoverageP, 3), xiiArgF(fCoverageN, 3));
    xiiDebugRenderer::Draw3DText(view.GetHandle(), sb, GetOwner()->GetGlobalPosition(), xiiColor::White);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_LodMeshComponent);
