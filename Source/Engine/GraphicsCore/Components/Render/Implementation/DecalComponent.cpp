#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/DecalComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiDecalBlendMode, 1)
  XII_ENUM_CONSTANTS(xiiDecalBlendMode::Opaque, xiiDecalBlendMode::AlphaBlend, xiiDecalBlendMode::Additive)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalRenderData, 1, xiiRTTIDefaultAllocator<xiiDecalRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiDecalComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Decal", GetDecalFile, SetDecalFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Decal")),
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0.5f))),
    XII_ENUM_ACCESSOR_PROPERTY("BlendMode", xiiDecalBlendMode, GetBlendMode, SetBlendMode),
    XII_ACCESSOR_PROPERTY("Alpha", GetAlpha, SetAlpha)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("AngleFade", GetAngleFade, SetAngleFade)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("BaseColor", GetBaseColor, SetBaseColor),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Decals"),
    new xiiBoxVisualizerAttribute("Extents", 1.0f, nullptr),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiDecalComponent::xiiDecalComponent()  = default;
xiiDecalComponent::~xiiDecalComponent() = default;

void xiiDecalComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_hDecal;
  s << m_vExtents;
  s << m_BlendMode.GetValue();
  s << m_fAlpha;
  s << m_fAngleFade;
  s << m_BaseColor;
}

void xiiDecalComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_hDecal;
  s >> m_vExtents;
  xiiUInt8 blendMode = 0;
  s >> blendMode;
  m_BlendMode = static_cast<xiiDecalBlendMode::Enum>(blendMode);
  s >> m_fAlpha;
  s >> m_fAngleFade;
  s >> m_BaseColor;
}

xiiResult xiiDecalComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);

  ref_bounds = xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromMinMax(-m_vExtents, m_vExtents));
  return XII_SUCCESS;
}

void xiiDecalComponent::SetDecalFile(xiiStringView sFile)
{
  xiiDecalResourceHandle hDecal;
  if (!sFile.IsEmpty())
    hDecal = xiiResourceManager::LoadResource<xiiDecalResource>(sFile);
  SetDecal(hDecal);
}

xiiStringView xiiDecalComponent::GetDecalFile() const
{
  if (m_hDecal.IsValid())
    return xiiResourceManager::GetResourceIDOrDescription(m_hDecal);
  return {};
}

void xiiDecalComponent::SetDecal(const xiiDecalResourceHandle& hDecal)
{
  m_hDecal = hDecal;
  InvalidateCachedRenderData();
}

void xiiDecalComponent::SetBlendMode(xiiEnum<xiiDecalBlendMode> mode)
{
  m_BlendMode = mode;
  InvalidateCachedRenderData();
}

void xiiDecalComponent::SetAlpha(float fAlpha)
{
  m_fAlpha = xiiMath::Clamp(fAlpha, 0.0f, 1.0f);
  InvalidateCachedRenderData();
}

void xiiDecalComponent::SetAngleFade(float fFade)
{
  m_fAngleFade = xiiMath::Clamp(fFade, 0.0f, 1.0f);
  InvalidateCachedRenderData();
}

void xiiDecalComponent::SetBaseColor(const xiiColor& color)
{
  m_BaseColor = xiiColorLinearUB(color);
  InvalidateCachedRenderData();
}

xiiColor xiiDecalComponent::GetBaseColor() const
{
  return xiiColor(m_BaseColor);
}

void xiiDecalComponent::SetExtents(const xiiVec3& vExtents)
{
  m_vExtents = vExtents.CompMax(xiiVec3(0.001f));
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiDecalComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!m_hDecal.IsValid() || ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  // Build OBB projection matrix: scale the local-to-world by the extents
  xiiMat4 mProjection = GetOwner()->GetGlobalTransform().GetAsMat4();

  xiiDecalRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiDecalRenderData>(this);
  pRenderData->m_GlobalTransform  = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds     = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject     = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent  = GetHandle();
  pRenderData->m_hDecal           = m_hDecal;
  pRenderData->m_ProjectionMatrix = mProjection;
  pRenderData->m_BlendMode        = m_BlendMode;
  pRenderData->m_fAlpha           = m_fAlpha;
  pRenderData->m_fAngleFade       = m_fAngleFade;
  pRenderData->m_BaseColor        = m_BaseColor;
  pRenderData->m_uiSortingKey     = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_DecalComponent);
