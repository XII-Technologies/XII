#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/ReflectionProbeComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectionProbeRenderData, 1, xiiRTTIDefaultAllocator<xiiReflectionProbeRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiReflectionProbeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("CubeMap",         GetCubeMapFile,    SetCubeMapFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    XII_ACCESSOR_PROPERTY("InfluenceRadius", GetInfluenceRadius, SetInfluenceRadius)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.01f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("BlendWeight",     GetBlendWeight,     SetBlendWeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("Realtime",        GetRealtime,        SetRealtime),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Lights"),
    new xiiSphereVisualizerAttribute("InfluenceRadius"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiReflectionProbeComponent::xiiReflectionProbeComponent()  = default;
xiiReflectionProbeComponent::~xiiReflectionProbeComponent() = default;

void xiiReflectionProbeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_hCubeMap << m_fInfluenceRadius << m_fBlendWeight << m_bRealtime;
}

void xiiReflectionProbeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_hCubeMap >> m_fInfluenceRadius >> m_fBlendWeight >> m_bRealtime;
}

xiiResult xiiReflectionProbeComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);
  ref_bounds = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fInfluenceRadius));
  return XII_SUCCESS;
}

void xiiReflectionProbeComponent::SetCubeMapFile(xiiStringView sFile)
{
  if (!sFile.IsEmpty())
    m_hCubeMap = xiiResourceManager::LoadResource<xiiTextureCubeResource>(sFile);
  else
    m_hCubeMap.Invalidate();
  InvalidateCachedRenderData();
}

xiiStringView xiiReflectionProbeComponent::GetCubeMapFile() const
{
  if (m_hCubeMap.IsValid())
    return xiiResourceManager::GetResourceIDOrDescription(m_hCubeMap);
  return {};
}

void xiiReflectionProbeComponent::SetInfluenceRadius(float f)
{
  m_fInfluenceRadius = xiiMath::Max(f, 0.01f);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}
void xiiReflectionProbeComponent::SetBlendWeight(float f)
{
  m_fBlendWeight = xiiMath::Clamp(f, 0.0f, 1.0f);
  InvalidateCachedRenderData();
}
void xiiReflectionProbeComponent::SetRealtime(bool b)
{
  m_bRealtime = b;
  InvalidateCachedRenderData();
}

void xiiReflectionProbeComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiReflectionProbeRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiReflectionProbeRenderData>(this);
  pRenderData->m_GlobalTransform            = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds               = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject               = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent            = GetHandle();
  pRenderData->m_hCubeMap                   = m_hCubeMap;
  pRenderData->m_vCapturePosition           = GetOwner()->GetGlobalPosition();
  pRenderData->m_fInfluenceRadius           = m_fInfluenceRadius;
  pRenderData->m_fBlendWeight               = m_fBlendWeight;
  pRenderData->m_bRealtime                  = m_bRealtime;
  pRenderData->m_uiSortingKey               = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, m_bRealtime ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_ReflectionProbeComponent);
