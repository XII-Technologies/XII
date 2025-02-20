#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/SpriteComponent.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSpriteBlendMode, 1)
  XII_ENUM_CONSTANTS(xiiSpriteBlendMode::Masked, xiiSpriteBlendMode::Transparent, xiiSpriteBlendMode::Additive)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
xiiTempHashedString xiiSpriteBlendMode::GetPermutationValue(Enum blendMode)
{
  switch (blendMode)
  {
    case xiiSpriteBlendMode::Masked:
    case xiiSpriteBlendMode::ShapeIcon:
      return "BLEND_MODE_MASKED";
    case xiiSpriteBlendMode::Transparent:
      return "BLEND_MODE_TRANSPARENT";
    case xiiSpriteBlendMode::Additive:
      return "BLEND_MODE_ADDITIVE";
  }

  return "";
}

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpriteRenderData, 1, xiiRTTIDefaultAllocator<xiiSpriteRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSpriteRenderData::FillSortingKey()
{
  // ignore upper 32 bit of the resource ID hash
  const xiiUInt32 uiTextureIDHash = static_cast<xiiUInt32>(m_hTexture.GetResourceIDHash());

  // Sort by mode and then by texture
  m_uiSortingKey = (m_BlendMode << 30) | (uiTextureIDHash & 0x3FFFFFFF);
}

bool xiiSpriteRenderData::CanBatch(const xiiRenderData& other0) const
{
  const auto& other = xiiStaticCast<const xiiSpriteRenderData&>(other0);

  return m_BlendMode == other.m_BlendMode && m_hTexture == other.m_hTexture;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSpriteComponent, 3, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("Texture", GetTexture, SetTexture)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    XII_ENUM_MEMBER_PROPERTY("BlendMode", xiiSpriteBlendMode, m_BlendMode),
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("MaxScreenSize", GetMaxScreenSize, SetMaxScreenSize)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(64.0f), new xiiSuffixAttribute(" px")),
    XII_MEMBER_PROPERTY("AspectRatio", m_fAspectRatio)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
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

xiiSpriteComponent::xiiSpriteComponent()  = default;
xiiSpriteComponent::~xiiSpriteComponent() = default;

xiiResult xiiSpriteComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fSize * 0.5f);
  return XII_SUCCESS;
}

void xiiSpriteComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // Don't render in shadow views
  if (msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow)
    return;

  if (!m_hTexture.IsValid())
    return;

  xiiSpriteRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiSpriteRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds    = GetOwner()->GetGlobalBounds();
    pRenderData->m_hTexture        = m_hTexture;
    pRenderData->m_fSize           = m_fSize;
    pRenderData->m_fMaxScreenSize  = m_fMaxScreenSize;
    pRenderData->m_fAspectRatio    = m_fAspectRatio;
    pRenderData->m_BlendMode       = m_BlendMode;
    pRenderData->m_color           = m_Color;
    pRenderData->m_texCoordScale   = xiiVec2(1.0f);
    pRenderData->m_texCoordOffset  = xiiVec2(0.0f);
    pRenderData->m_uiUniqueID      = GetUniqueIdForRendering();

    pRenderData->FillSortingKey();
  }

  // Determine render data category.
  xiiRenderData::Category category = xiiDefaultRenderDataCategories::LitTransparent;
  if (m_BlendMode == xiiSpriteBlendMode::Masked)
  {
    category = xiiDefaultRenderDataCategories::LitMasked;
  }

  msg.AddRenderData(pRenderData, category, xiiRenderData::Caching::IfStatic);
}

void xiiSpriteComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_hTexture;
  s << m_fSize;
  s << m_fMaxScreenSize;

  // Version 3
  s << m_Color; // HDR now
  s << m_fAspectRatio;
  s << m_BlendMode;
}

void xiiSpriteComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_hTexture;

  if (uiVersion < 3)
  {
    xiiColorGammaUB color;
    s >> color;
    m_Color = color;
  }

  s >> m_fSize;
  s >> m_fMaxScreenSize;

  if (uiVersion >= 3)
  {
    s >> m_Color;
    s >> m_fAspectRatio;
    s >> m_BlendMode;
  }
}

void xiiSpriteComponent::SetTexture(const xiiTexture2DResourceHandle& hTexture)
{
  m_hTexture = hTexture;
}

const xiiTexture2DResourceHandle& xiiSpriteComponent::GetTexture() const
{
  return m_hTexture;
}

void xiiSpriteComponent::SetColor(xiiColor color)
{
  m_Color = color;
}

xiiColor xiiSpriteComponent::GetColor() const
{
  return m_Color;
}

void xiiSpriteComponent::SetSize(float fSize)
{
  m_fSize = fSize;

  TriggerLocalBoundsUpdate();
}

float xiiSpriteComponent::GetSize() const
{
  return m_fSize;
}

void xiiSpriteComponent::SetMaxScreenSize(float fSize)
{
  m_fMaxScreenSize = fSize;
}

float xiiSpriteComponent::GetMaxScreenSize() const
{
  return m_fMaxScreenSize;
}

void xiiSpriteComponent::OnMsgSetColor(xiiMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_SpriteComponent);
