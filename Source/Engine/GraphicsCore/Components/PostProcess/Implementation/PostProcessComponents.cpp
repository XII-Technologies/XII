#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/PostProcess/PostProcessComponents.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTonemapOperator, 1)
  XII_ENUM_CONSTANTS(xiiTonemapOperator::Reinhard, xiiTonemapOperator::ACES, xiiTonemapOperator::AgX, xiiTonemapOperator::Custom)
XII_END_STATIC_REFLECTED_ENUM;

#define XII_PP_REFLECT(TypeName) \
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(TypeName, 1, xiiRTTIDefaultAllocator<TypeName>) \
  XII_END_DYNAMIC_REFLECTED_TYPE

XII_PP_REFLECT(xiiPostProcessVolumeRenderData);
XII_PP_REFLECT(xiiBloomRenderData);
XII_PP_REFLECT(xiiToneMappingRenderData);
XII_PP_REFLECT(xiiTemporalAARenderData);
XII_PP_REFLECT(xiiMotionBlurRenderData);
XII_PP_REFLECT(xiiDepthOfFieldRenderData);
XII_PP_REFLECT(xiiSSRRenderData);
XII_PP_REFLECT(xiiAORenderData);
XII_PP_REFLECT(xiiColorGradingRenderData);

XII_BEGIN_COMPONENT_TYPE(xiiPostProcessVolumeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlendRadius", GetBlendRadius, SetBlendRadius),
    XII_ACCESSOR_PROPERTY("Priority",    GetPriority,    SetPriority),
    XII_ACCESSOR_PROPERTY("IsGlobal",    GetIsGlobal,    SetIsGlobal)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/PostProcess"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiBloomComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Threshold", GetThreshold, SetThreshold)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("Radius",    GetRadius,    SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.003f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/PostProcess"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiToneMappingComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("Operator",   xiiTonemapOperator, GetOperator, SetOperator),
    XII_ACCESSOR_PROPERTY("Exposure",    GetExposure,    SetExposure),
    XII_ACCESSOR_PROPERTY("WhitePoint",  GetWhitePoint,  SetWhitePoint)->AddAttributes(new xiiDefaultValueAttribute(4.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/PostProcess"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiTemporalAAComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("JitterScale",  GetJitterScale,  SetJitterScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("FeedbackMin",  GetFeedbackMin,  SetFeedbackMin)->AddAttributes(new xiiDefaultValueAttribute(0.88f)),
    XII_ACCESSOR_PROPERTY("FeedbackMax",  GetFeedbackMax,  SetFeedbackMax)->AddAttributes(new xiiDefaultValueAttribute(0.97f)),
    XII_ACCESSOR_PROPERTY("AntiGhosting", GetAntiGhosting, SetAntiGhosting)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/PostProcess"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiMotionBlurComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ShutterAngle", GetShutterAngle, SetShutterAngle)->AddAttributes(new xiiDefaultValueAttribute(180.0f)),
    XII_ACCESSOR_PROPERTY("MaxSamples",   GetMaxSamples,   SetMaxSamples)->AddAttributes(new xiiDefaultValueAttribute(16u)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/PostProcess"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiDepthOfFieldComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("FocusDistance", GetFocusDistance, SetFocusDistance)->AddAttributes(new xiiDefaultValueAttribute(5.0f)),
    XII_ACCESSOR_PROPERTY("NearBlur",      GetNearBlur,      SetNearBlur)->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
    XII_ACCESSOR_PROPERTY("FarBlur",       GetFarBlur,       SetFarBlur)->AddAttributes(new xiiDefaultValueAttribute(1.5f)),
    XII_ACCESSOR_PROPERTY("BokehSize",     GetBokehSize,     SetBokehSize)->AddAttributes(new xiiDefaultValueAttribute(0.02f)),
    XII_ACCESSOR_PROPERTY("BokehBlades",   GetBokehBlades,   SetBokehBlades)->AddAttributes(new xiiDefaultValueAttribute(6u)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/PostProcess"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiScreenSpaceReflectionComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("MaxSteps",     GetMaxSteps,     SetMaxSteps)->AddAttributes(new xiiDefaultValueAttribute(64u)),
    XII_ACCESSOR_PROPERTY("Thickness",    GetThickness,    SetThickness)->AddAttributes(new xiiDefaultValueAttribute(0.05f)),
    XII_ACCESSOR_PROPERTY("MaxRoughness", GetMaxRoughness, SetMaxRoughness)->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
    XII_ACCESSOR_PROPERTY("HalfRes",      GetHalfRes,      SetHalfRes)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/PostProcess"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiAmbientOcclusionComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius",      GetRadius,      SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
    XII_ACCESSOR_PROPERTY("NumSamples",  GetNumSamples,  SetNumSamples)->AddAttributes(new xiiDefaultValueAttribute(16u)),
    XII_ACCESSOR_PROPERTY("Strength",    GetStrength,    SetStrength)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("BentNormals", GetBentNormals, SetBentNormals),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/PostProcess"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiColorGradingComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("LUT",        GetLUTFile,     SetLUTFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    XII_ACCESSOR_PROPERTY("LUTBlend",   GetLUTBlend,    SetLUTBlend)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("Saturation", GetSaturation,  SetSaturation)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("Contrast",   GetContrast,    SetContrast)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/PostProcess"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

// Helper macro for trivial GetLocalBounds (always invisible — no geometry)
#define XII_PP_NO_BOUNDS(ClassName)                                                               \
  xiiResult ClassName::GetLocalBounds(xiiBoundingBoxSphere&, bool& bAV, xiiMsgUpdateLocalBounds&) \
  {                                                                                               \
    bAV = true;                                                                                   \
    return XII_SUCCESS;                                                                           \
  }

// Helper macro for trivial Serialize/Deserialize of common PP params
#define XII_PP_SERIALIZE2(ClassName, a, b)                    \
  void ClassName::SerializeComponent(xiiWorldWriter& s) const \
  {                                                           \
    SUPER::SerializeComponent(s);                             \
    s.GetStream() << a << b;                                  \
  }                                                           \
  void ClassName::DeserializeComponent(xiiWorldReader& s)     \
  {                                                           \
    SUPER::DeserializeComponent(s);                           \
    s.GetStream() >> a >> b;                                  \
  }
#define XII_PP_SERIALIZE3(ClassName, a, b, c)                 \
  void ClassName::SerializeComponent(xiiWorldWriter& s) const \
  {                                                           \
    SUPER::SerializeComponent(s);                             \
    s.GetStream() << a << b << c;                             \
  }                                                           \
  void ClassName::DeserializeComponent(xiiWorldReader& s)     \
  {                                                           \
    SUPER::DeserializeComponent(s);                           \
    s.GetStream() >> a >> b >> c;                             \
  }
#define XII_PP_SERIALIZE4(ClassName, a, b, c, d)              \
  void ClassName::SerializeComponent(xiiWorldWriter& s) const \
  {                                                           \
    SUPER::SerializeComponent(s);                             \
    s.GetStream() << a << b << c << d;                        \
  }                                                           \
  void ClassName::DeserializeComponent(xiiWorldReader& s)     \
  {                                                           \
    SUPER::DeserializeComponent(s);                           \
    s.GetStream() >> a >> b >> c >> d;                        \
  }

// Helper macro for extract render data
#define XII_PP_EXTRACT(ClassName, RDType, ...)                                   \
  void ClassName::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const \
  {                                                                              \
    if (!ref_msg.m_pView || !ref_msg.m_pExtractedRenderData) return;             \
    auto* pWM = GetWorld()->GetModule<xiiRenderWorldModule>();                   \
    if (!pWM) return;                                                            \
    auto* pRD              = pWM->CreateRenderDataForThisFrame<RDType>(this);    \
    pRD->m_GlobalTransform = GetOwner()->GetGlobalTransform();                   \
    pRD->m_GlobalBounds    = GetOwner()->GetGlobalBounds();                      \
    pRD->m_hOwnerObject    = GetOwner()->GetHandle();                            \
    pRD->m_hOwnerComponent = GetHandle();                                        \
    pRD->m_uiSortingKey    = GetUniqueIdForRendering();                          \
    __VA_ARGS__                                                                  \
    ref_msg.AddRenderData(pRD, xiiRenderData::Caching::Never);                   \
  }

// ====== PostProcessVolumeComponent ======
xiiPostProcessVolumeComponent::xiiPostProcessVolumeComponent()  = default;
xiiPostProcessVolumeComponent::~xiiPostProcessVolumeComponent() = default;
XII_PP_NO_BOUNDS(xiiPostProcessVolumeComponent)
XII_PP_SERIALIZE3(xiiPostProcessVolumeComponent, m_fBlendRadius, m_fPriority, m_bIsGlobal)
void xiiPostProcessVolumeComponent::SetBlendRadius(float f) { m_fBlendRadius = xiiMath::Max(f, 0.0f); }
void xiiPostProcessVolumeComponent::SetPriority(float f) { m_fPriority = f; }
void xiiPostProcessVolumeComponent::SetIsGlobal(bool b) { m_bIsGlobal = b; }
XII_PP_EXTRACT(xiiPostProcessVolumeComponent, xiiPostProcessVolumeRenderData,
               pRD->m_fBlendRadius = m_fBlendRadius;
               pRD->m_fPriority = m_fPriority; pRD->m_bIsGlobal = m_bIsGlobal;)

// ====== BloomComponent ======
xiiBloomComponent::xiiBloomComponent()  = default;
xiiBloomComponent::~xiiBloomComponent() = default;
XII_PP_NO_BOUNDS(xiiBloomComponent)
XII_PP_SERIALIZE3(xiiBloomComponent, m_fThreshold, m_fIntensity, m_fRadius)
void xiiBloomComponent::SetThreshold(float f) { m_fThreshold = xiiMath::Max(f, 0.0f); }
void xiiBloomComponent::SetIntensity(float f) { m_fIntensity = xiiMath::Max(f, 0.0f); }
void xiiBloomComponent::SetRadius(float f) { m_fRadius = xiiMath::Clamp(f, 0.0001f, 1.0f); }
XII_PP_EXTRACT(xiiBloomComponent, xiiBloomRenderData,
               pRD->m_fThreshold = m_fThreshold;
               pRD->m_fIntensity = m_fIntensity; pRD->m_fRadius = m_fRadius;)

// ====== ToneMappingComponent ======
xiiToneMappingComponent::xiiToneMappingComponent()  = default;
xiiToneMappingComponent::~xiiToneMappingComponent() = default;
XII_PP_NO_BOUNDS(xiiToneMappingComponent)
void xiiToneMappingComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_Operator.GetValue() << m_fExposure << m_fWhitePoint;
}
void xiiToneMappingComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  xiiUInt8 op = 0;
  s.GetStream() >> op >> m_fExposure >> m_fWhitePoint;
  m_Operator = static_cast<xiiTonemapOperator::Enum>(op);
}
void xiiToneMappingComponent::SetOperator(xiiEnum<xiiTonemapOperator> op) { m_Operator = op; }
void xiiToneMappingComponent::SetExposure(float f) { m_fExposure = f; }
void xiiToneMappingComponent::SetWhitePoint(float f) { m_fWhitePoint = xiiMath::Max(f, 0.001f); }
XII_PP_EXTRACT(xiiToneMappingComponent, xiiToneMappingRenderData,
               pRD->m_Operator  = m_Operator;
               pRD->m_fExposure = m_fExposure; pRD->m_fWhitePoint = m_fWhitePoint;)

// ====== TemporalAAComponent ======
xiiTemporalAAComponent::xiiTemporalAAComponent()  = default;
xiiTemporalAAComponent::~xiiTemporalAAComponent() = default;
XII_PP_NO_BOUNDS(xiiTemporalAAComponent)
XII_PP_SERIALIZE4(xiiTemporalAAComponent, m_fJitterScale, m_fFeedbackMin, m_fFeedbackMax, m_bAntiGhosting)
void xiiTemporalAAComponent::SetJitterScale(float f) { m_fJitterScale = xiiMath::Max(f, 0.0f); }
void xiiTemporalAAComponent::SetFeedbackMin(float f) { m_fFeedbackMin = xiiMath::Clamp(f, 0.0f, 1.0f); }
void xiiTemporalAAComponent::SetFeedbackMax(float f) { m_fFeedbackMax = xiiMath::Clamp(f, 0.0f, 1.0f); }
void xiiTemporalAAComponent::SetAntiGhosting(bool b) { m_bAntiGhosting = b; }
XII_PP_EXTRACT(xiiTemporalAAComponent, xiiTemporalAARenderData,
               pRD->m_fJitterScale = m_fJitterScale;
               pRD->m_fFeedbackMin = m_fFeedbackMin;
               pRD->m_fFeedbackMax = m_fFeedbackMax; pRD->m_bAntiGhosting = m_bAntiGhosting;)

// ====== MotionBlurComponent ======
xiiMotionBlurComponent::xiiMotionBlurComponent()  = default;
xiiMotionBlurComponent::~xiiMotionBlurComponent() = default;
XII_PP_NO_BOUNDS(xiiMotionBlurComponent)
XII_PP_SERIALIZE2(xiiMotionBlurComponent, m_fShutterAngle, m_uiMaxSamples)
void xiiMotionBlurComponent::SetShutterAngle(float f) { m_fShutterAngle = xiiMath::Clamp(f, 0.0f, 360.0f); }
void xiiMotionBlurComponent::SetMaxSamples(xiiUInt8 n) { m_uiMaxSamples = xiiMath::Max<xiiUInt8>(n, 1); }
XII_PP_EXTRACT(xiiMotionBlurComponent, xiiMotionBlurRenderData,
               pRD->m_fShutterAngle = m_fShutterAngle;
               pRD->m_uiMaxSamples  = m_uiMaxSamples;)

// ====== DepthOfFieldComponent ======
xiiDepthOfFieldComponent::xiiDepthOfFieldComponent()  = default;
xiiDepthOfFieldComponent::~xiiDepthOfFieldComponent() = default;
XII_PP_NO_BOUNDS(xiiDepthOfFieldComponent)
void xiiDepthOfFieldComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_fFocusDistance << m_fNearBlur << m_fFarBlur << m_fBokehSize << m_uiBokehBlades;
}
void xiiDepthOfFieldComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_fFocusDistance >> m_fNearBlur >> m_fFarBlur >> m_fBokehSize >> m_uiBokehBlades;
}
void xiiDepthOfFieldComponent::SetFocusDistance(float f) { m_fFocusDistance = xiiMath::Max(f, 0.0f); }
void xiiDepthOfFieldComponent::SetNearBlur(float f) { m_fNearBlur = xiiMath::Max(f, 0.0f); }
void xiiDepthOfFieldComponent::SetFarBlur(float f) { m_fFarBlur = xiiMath::Max(f, 0.0f); }
void xiiDepthOfFieldComponent::SetBokehSize(float f) { m_fBokehSize = xiiMath::Max(f, 0.0f); }
void xiiDepthOfFieldComponent::SetBokehBlades(xiiUInt8 n) { m_uiBokehBlades = xiiMath::Clamp<xiiUInt8>(n, 3, 12); }
XII_PP_EXTRACT(xiiDepthOfFieldComponent, xiiDepthOfFieldRenderData,
               pRD->m_fFocusDistance = m_fFocusDistance;
               pRD->m_fNearBlur      = m_fNearBlur;
               pRD->m_fFarBlur = m_fFarBlur; pRD->m_fBokehSize = m_fBokehSize; pRD->m_uiBokehBlades = m_uiBokehBlades;)

// ====== ScreenSpaceReflectionComponent ======
xiiScreenSpaceReflectionComponent::xiiScreenSpaceReflectionComponent()  = default;
xiiScreenSpaceReflectionComponent::~xiiScreenSpaceReflectionComponent() = default;
XII_PP_NO_BOUNDS(xiiScreenSpaceReflectionComponent)
XII_PP_SERIALIZE4(xiiScreenSpaceReflectionComponent, m_uiMaxSteps, m_fThickness, m_fMaxRoughness, m_bHalfRes)
void xiiScreenSpaceReflectionComponent::SetMaxSteps(xiiUInt8 n) { m_uiMaxSteps = xiiMath::Max<xiiUInt8>(n, 1); }
void xiiScreenSpaceReflectionComponent::SetThickness(float f) { m_fThickness = xiiMath::Max(f, 0.0f); }
void xiiScreenSpaceReflectionComponent::SetMaxRoughness(float f) { m_fMaxRoughness = xiiMath::Clamp(f, 0.0f, 1.0f); }
void xiiScreenSpaceReflectionComponent::SetHalfRes(bool b) { m_bHalfRes = b; }
XII_PP_EXTRACT(xiiScreenSpaceReflectionComponent, xiiSSRRenderData,
               pRD->m_uiMaxSteps    = m_uiMaxSteps;
               pRD->m_fThickness    = m_fThickness;
               pRD->m_fMaxRoughness = m_fMaxRoughness; pRD->m_bHalfRes = m_bHalfRes;)

// ====== AmbientOcclusionComponent ======
xiiAmbientOcclusionComponent::xiiAmbientOcclusionComponent()  = default;
xiiAmbientOcclusionComponent::~xiiAmbientOcclusionComponent() = default;
XII_PP_NO_BOUNDS(xiiAmbientOcclusionComponent)
XII_PP_SERIALIZE4(xiiAmbientOcclusionComponent, m_fRadius, m_uiNumSamples, m_fStrength, m_bBentNormals)
void xiiAmbientOcclusionComponent::SetRadius(float f) { m_fRadius = xiiMath::Max(f, 0.0f); }
void xiiAmbientOcclusionComponent::SetNumSamples(xiiUInt8 n) { m_uiNumSamples = xiiMath::Max<xiiUInt8>(n, 1); }
void xiiAmbientOcclusionComponent::SetStrength(float f) { m_fStrength = xiiMath::Max(f, 0.0f); }
void xiiAmbientOcclusionComponent::SetBentNormals(bool b) { m_bBentNormals = b; }
XII_PP_EXTRACT(xiiAmbientOcclusionComponent, xiiAORenderData,
               pRD->m_fRadius      = m_fRadius;
               pRD->m_uiNumSamples = m_uiNumSamples;
               pRD->m_fStrength = m_fStrength; pRD->m_bBentNormals = m_bBentNormals;)

// ====== ColorGradingComponent ======
xiiColorGradingComponent::xiiColorGradingComponent()  = default;
xiiColorGradingComponent::~xiiColorGradingComponent() = default;
XII_PP_NO_BOUNDS(xiiColorGradingComponent)
void xiiColorGradingComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_hLUT << m_fLUTBlend << m_fSaturation << m_fContrast;
}
void xiiColorGradingComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_hLUT >> m_fLUTBlend >> m_fSaturation >> m_fContrast;
}
void xiiColorGradingComponent::SetLUTFile(xiiStringView sFile)
{
  m_hLUT = sFile.IsEmpty() ? xiiTexture3DResourceHandle{} : xiiResourceManager::LoadResource<xiiTexture3DResource>(sFile);
}
xiiStringView xiiColorGradingComponent::GetLUTFile() const
{
  return m_hLUT.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hLUT) : xiiStringView{};
}
void xiiColorGradingComponent::SetLUTBlend(float f) { m_fLUTBlend = xiiMath::Clamp(f, 0.0f, 1.0f); }
void xiiColorGradingComponent::SetSaturation(float f) { m_fSaturation = xiiMath::Max(f, 0.0f); }
void xiiColorGradingComponent::SetContrast(float f) { m_fContrast = xiiMath::Max(f, 0.0f); }
XII_PP_EXTRACT(xiiColorGradingComponent, xiiColorGradingRenderData,
               pRD->m_hLUT        = m_hLUT;
               pRD->m_fLUTBlend   = m_fLUTBlend;
               pRD->m_fSaturation = m_fSaturation; pRD->m_fContrast = m_fContrast;)

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_PostProcess_Implementation_PostProcessComponents);
