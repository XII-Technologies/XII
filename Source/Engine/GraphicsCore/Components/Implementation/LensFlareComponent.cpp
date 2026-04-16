#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/LensFlareComponent.h>
#include <GraphicsCore/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Lights/SpotLightComponent.h>
#include <GraphicsCore/Pipeline/RenderData/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLensFlareRenderData, 1, xiiRTTIDefaultAllocator<xiiLensFlareRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiLensFlareRenderData::FillSortingKey()
{
  // ignore upper 32 bit of the resource ID hash
  const xiiUInt32 uiTextureIDHash = static_cast<xiiUInt32>(m_hTexture.GetResourceIDHash());

  // Sort by texture
  m_uiSortingKey = uiTextureIDHash;
}

bool xiiLensFlareRenderData::CanBatch(const xiiRenderData& other0) const
{
  const auto& other = xiiStaticCast<const xiiLensFlareRenderData&>(other0);

  return m_hTexture == other.m_hTexture;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiLensFlareElement, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiLensFlareElement>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_MEMBER_PROPERTY("Texture", m_hTexture)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    XII_MEMBER_PROPERTY("GreyscaleTexture", m_bGreyscaleTexture),
    XII_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_MEMBER_PROPERTY("ModulateByLightColor", m_bModulateByLightColor)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(10000.0f), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("MaxScreenSize", m_fMaxScreenSize)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("AspectRatio", m_fAspectRatio)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("ShiftToCenter", m_fShiftToCenter),
    XII_MEMBER_PROPERTY("InverseTonemap", m_bInverseTonemap),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiLensFlareElement::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_hTexture;
  inout_stream << m_Color;
  inout_stream << m_fSize;
  inout_stream << m_fMaxScreenSize;
  inout_stream << m_fAspectRatio;
  inout_stream << m_fShiftToCenter;
  inout_stream << m_bInverseTonemap;
  inout_stream << m_bModulateByLightColor;
  inout_stream << m_bGreyscaleTexture;

  return XII_SUCCESS;
}

xiiResult xiiLensFlareElement::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_hTexture;
  inout_stream >> m_Color;
  inout_stream >> m_fSize;
  inout_stream >> m_fMaxScreenSize;
  inout_stream >> m_fAspectRatio;
  inout_stream >> m_fShiftToCenter;
  inout_stream >> m_bInverseTonemap;
  inout_stream >> m_bModulateByLightColor;
  inout_stream >> m_bGreyscaleTexture;

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiLensFlareComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("LinkToLightShape", GetLinkToLightShape, SetLinkToLightShape)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("OcclusionSampleRadius", GetOcclusionSampleRadius, SetOcclusionSampleRadius)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(0.1f), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("OcclusionSampleSpread", m_fOcclusionSampleSpread)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f), new xiiDefaultValueAttribute(0.5f)),
    XII_MEMBER_PROPERTY("OcclusionDepthOffset", m_fOcclusionDepthOffset)->AddAttributes(new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("ApplyFog", m_bApplyFog)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ARRAY_MEMBER_PROPERTY("Elements", m_Elements)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
    new xiiSphereManipulatorAttribute("OcclusionSampleRadius"),
    new xiiSphereVisualizerAttribute("OcclusionSampleRadius", xiiColor::White)
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiLensFlareComponent::xiiLensFlareComponent()  = default;
xiiLensFlareComponent::~xiiLensFlareComponent() = default;

void xiiLensFlareComponent::OnActivated()
{
  SUPER::OnActivated();

  FindLightComponent();
}

void xiiLensFlareComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  m_bDirectionalLight = false;
  m_hLightComponent.Invalidate();
}

void xiiLensFlareComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s.WriteArray(m_Elements).IgnoreResult();
  s << m_fIntensity;
  s << m_fOcclusionSampleRadius;
  s << m_fOcclusionSampleSpread;
  s << m_fOcclusionDepthOffset;
  s << m_bLinkToLightShape;
  s << m_bApplyFog;
}

void xiiLensFlareComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);

  xiiStreamReader& s = inout_stream.GetStream();

  s.ReadArray(m_Elements).IgnoreResult();
  s >> m_fIntensity;
  s >> m_fOcclusionSampleRadius;
  s >> m_fOcclusionSampleSpread;
  s >> m_fOcclusionDepthOffset;
  s >> m_bLinkToLightShape;
  s >> m_bApplyFog;
}

xiiResult xiiLensFlareComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  if (m_bDirectionalLight)
  {
    ref_bAlwaysVisible = true;
  }
  else
  {
    ref_bounds = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fOcclusionSampleRadius);
  }
  return XII_SUCCESS;
}

void xiiLensFlareComponent::SetLinkToLightShape(bool bLink)
{
  if (m_bLinkToLightShape == bLink)
    return;

  m_bLinkToLightShape = bLink;
  if (IsActiveAndInitialized())
  {
    FindLightComponent();
  }

  TriggerLocalBoundsUpdate();
}

void xiiLensFlareComponent::SetOcclusionSampleRadius(float fRadius)
{
  m_fOcclusionSampleRadius = fRadius;

  TriggerLocalBoundsUpdate();
}

void xiiLensFlareComponent::FindLightComponent()
{
  xiiLightComponent* pLightComponent = nullptr;

  if (m_bLinkToLightShape)
  {
    xiiGameObject* pObject = GetOwner();
    while (pObject != nullptr)
    {
      if (pObject->TryGetComponentOfBaseType(pLightComponent))
        break;

      pObject = pObject->GetParent();
    }
  }

  if (pLightComponent != nullptr)
  {
    m_bDirectionalLight = pLightComponent->IsInstanceOf<xiiDirectionalLightComponent>();
    m_hLightComponent   = pLightComponent->GetHandle();
  }
  else
  {
    m_bDirectionalLight = false;
    m_hLightComponent.Invalidate();
  }
}

void xiiLensFlareComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // Don't render in shadow views
  if (msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  const xiiCamera*     pCamera         = msg.m_pView->GetCamera();
  xiiTransform         globalTransform = GetOwner()->GetGlobalTransform();
  xiiBoundingBoxSphere globalBounds    = GetOwner()->GetGlobalBounds();
  float                fScale          = globalTransform.GetMaxScale();
  xiiColor             lightColor      = xiiColor::White;

  const xiiLightComponent* pLightComponent = nullptr;
  if (GetWorld()->TryGetComponent(m_hLightComponent, pLightComponent))
  {
    lightColor = pLightComponent->GetLightColor();
    lightColor *= pLightComponent->GetIntensity() * 0.1f;
  }

  float fFade = 1.0f;
  if (auto pDirectionalLight = xiiDynamicCast<const xiiDirectionalLightComponent*>(pLightComponent))
  {
    xiiTransform localOffset = xiiTransform::MakeIdentity();
    localOffset.m_vPosition  = xiiVec3(pCamera->GetFarPlane() * -0.999f, 0.0f, 0.0f);

    globalTransform = xiiTransform::MakeGlobalTransform(globalTransform, localOffset);
    globalTransform.m_vPosition += pCamera->GetCenterPosition();

    if (pCamera->IsPerspective())
    {
      float fHalfHeight = xiiMath::Tan(pCamera->GetFovY(1.0f) * 0.5f) * pCamera->GetFarPlane();
      fScale *= fHalfHeight;
    }

    lightColor *= 10.0f;
  }
  else if (auto pSpotLight = xiiDynamicCast<const xiiSpotLightComponent*>(pLightComponent))
  {
    const xiiVec3 lightDir  = globalTransform.TransformDirection(xiiVec3::MakeAxisX());
    const xiiVec3 cameraDir = (pCamera->GetCenterPosition() - globalTransform.m_vPosition).GetNormalized();

    const float cosAngle  = lightDir.Dot(cameraDir);
    const float fCosInner = xiiMath::Cos(pSpotLight->GetInnerSpotAngle() * 0.5f);
    const float fCosOuter = xiiMath::Cos(pSpotLight->GetOuterSpotAngle() * 0.5f);
    fFade                 = xiiMath::Saturate((cosAngle - fCosOuter) / xiiMath::Max(0.001f, (fCosInner - fCosOuter)));
    fFade *= fFade;
  }

  for (auto& element : m_Elements)
  {
    if (element.m_hTexture.IsValid() == false)
      continue;

    xiiColor color = element.m_Color * m_fIntensity;
    if (element.m_bModulateByLightColor)
    {
      color *= lightColor;
    }
    color.a = element.m_Color.a * fFade;

    if (color.GetLuminance() <= 0.0f || color.a <= 0.0f)
      continue;

    xiiLensFlareRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiLensFlareRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform        = globalTransform;
      pRenderData->m_GlobalBounds           = globalBounds;
      pRenderData->m_hTexture               = element.m_hTexture;
      pRenderData->m_Color                  = color.GetAsVec4();
      pRenderData->m_fSize                  = element.m_fSize * fScale;
      pRenderData->m_fMaxScreenSize         = element.m_fMaxScreenSize * 2.0f;
      pRenderData->m_fAspectRatio           = 1.0f / element.m_fAspectRatio;
      pRenderData->m_fShiftToCenter         = element.m_fShiftToCenter;
      pRenderData->m_fOcclusionSampleRadius = m_fOcclusionSampleRadius * fScale;
      pRenderData->m_fOcclusionSampleSpread = m_fOcclusionSampleSpread;
      pRenderData->m_fOcclusionDepthOffset  = m_fOcclusionDepthOffset * fScale;
      pRenderData->m_bInverseTonemap        = element.m_bInverseTonemap;
      pRenderData->m_bGreyscaleTexture      = element.m_bGreyscaleTexture;
      pRenderData->m_bApplyFog              = m_bApplyFog;

      pRenderData->FillSortingKey();
    }

    pRenderData->m_RoutingFlags = xiiRenderDataRoutingFlags::Transparent;
    msg.AddRenderData(pRenderData, pLightComponent != nullptr ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_LensFlareComponent);
