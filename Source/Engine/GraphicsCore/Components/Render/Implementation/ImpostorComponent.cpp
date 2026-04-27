#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/ImpostorComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiImpostorRenderData, 1, xiiRTTIDefaultAllocator<xiiImpostorRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiImpostorComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Texture", GetTextureFile, SetTextureFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    XII_ACCESSOR_PROPERTY("HalfWidth",  GetHalfWidth,  SetHalfWidth)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("HalfHeight", GetHalfHeight, SetHalfHeight)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("FadeInDist",  GetFadeInDist,  SetFadeInDist)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("FadeOutDist", GetFadeOutDist, SetFadeOutDist)->AddAttributes(new xiiDefaultValueAttribute(200.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Impostors"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiImpostorComponent::xiiImpostorComponent()  = default;
xiiImpostorComponent::~xiiImpostorComponent() = default;

void xiiImpostorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_hTexture;
  s << m_fHalfWidth;
  s << m_fHalfHeight;
  s << m_fFadeInDist;
  s << m_fFadeOutDist;
  s << m_Color;
}

void xiiImpostorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_hTexture;
  s >> m_fHalfWidth;
  s >> m_fHalfHeight;
  s >> m_fFadeInDist;
  s >> m_fFadeOutDist;
  s >> m_Color;
}

xiiResult xiiImpostorComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);

  const float       fR     = xiiMath::Max(m_fHalfWidth, m_fHalfHeight);
  xiiBoundingSphere sphere = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), fR);
  ref_bounds               = xiiBoundingBoxSphere::MakeFromSphere(sphere);
  return XII_SUCCESS;
}

void xiiImpostorComponent::SetTextureFile(xiiStringView sFile)
{
  xiiTexture2DResourceHandle hTex;
  if (!sFile.IsEmpty())
    hTex = xiiResourceManager::LoadResource<xiiTexture2DResource>(sFile);
  SetTexture(hTex);
}

xiiStringView xiiImpostorComponent::GetTextureFile() const
{
  if (m_hTexture.IsValid())
    return xiiResourceManager::GetResourceIDOrDescription(m_hTexture);
  return {};
}

void xiiImpostorComponent::SetTexture(const xiiTexture2DResourceHandle& hTexture)
{
  m_hTexture = hTexture;
  InvalidateCachedRenderData();
}

void xiiImpostorComponent::SetHalfWidth(float f)
{
  m_fHalfWidth = xiiMath::Max(f, 0.0f);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}
void xiiImpostorComponent::SetHalfHeight(float f)
{
  m_fHalfHeight = xiiMath::Max(f, 0.0f);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}
void xiiImpostorComponent::SetFadeInDist(float f)
{
  m_fFadeInDist = xiiMath::Max(f, 0.0f);
  InvalidateCachedRenderData();
}
void xiiImpostorComponent::SetFadeOutDist(float f)
{
  m_fFadeOutDist = xiiMath::Max(f, 0.0f);
  InvalidateCachedRenderData();
}

void xiiImpostorComponent::SetColor(const xiiColor& color)
{
  m_Color = color;
  InvalidateCachedRenderData();
}

void xiiImpostorComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!m_hTexture.IsValid() || ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiImpostorRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiImpostorRenderData>(this);
  pRenderData->m_GlobalTransform     = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds        = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject        = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent     = GetHandle();
  pRenderData->m_hTexture            = m_hTexture;
  pRenderData->m_fHalfWidth          = m_fHalfWidth;
  pRenderData->m_fHalfHeight         = m_fHalfHeight;
  pRenderData->m_fFadeInDist         = m_fFadeInDist;
  pRenderData->m_fFadeOutDist        = m_fFadeOutDist;
  pRenderData->m_Color               = m_Color;
  pRenderData->m_uiSortingKey        = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_ImpostorComponent);
