#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/ApplyOnlyToMessage.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Decals/DecalResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

xiiDecalComponentManager::xiiDecalComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiDecalComponent, xiiBlockStorageType::Compact>(pWorld)
{
}

void xiiDecalComponentManager::Initialize()
{
  m_hDecalAtlas = xiiDecalAtlasResource::GetDecalAtlasResource();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalRenderData, 1, xiiRTTIDefaultAllocator<xiiDecalRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiDecalComponent, 8, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_ACCESSOR_PROPERTY("Decals", DecalFile_GetCount, DecalFile_Get, DecalFile_Set, DecalFile_Insert, DecalFile_Remove)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Decal")),
    XII_ENUM_ACCESSOR_PROPERTY("ProjectionAxis", xiiBasisAxis, GetProjectionAxis, SetProjectionAxis),
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f)), new xiiClampValueAttribute(xiiVec3(0.01f), xiiVariant(25.0f))),
    XII_ACCESSOR_PROPERTY("SizeVariance", GetSizeVariance, SetSizeVariance)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_ACCESSOR_PROPERTY("EmissiveColor", GetEmissiveColor, SetEmissiveColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::Black)),
    XII_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new xiiClampValueAttribute(-64.0f, 64.0f)),
    XII_ACCESSOR_PROPERTY("WrapAround", GetWrapAround, SetWrapAround),
    XII_ACCESSOR_PROPERTY("MapNormalToGeometry", GetMapNormalToGeometry, SetMapNormalToGeometry)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("InnerFadeAngle", GetInnerFadeAngle, SetInnerFadeAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(0.0f), xiiAngle::Degree(89.0f)), new xiiDefaultValueAttribute(xiiAngle::Degree(50.0f))),
    XII_ACCESSOR_PROPERTY("OuterFadeAngle", GetOuterFadeAngle, SetOuterFadeAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(0.0f), xiiAngle::Degree(89.0f)), new xiiDefaultValueAttribute(xiiAngle::Degree(80.0f))),
    XII_MEMBER_PROPERTY("FadeOutDelay", m_FadeOutDelay),
    XII_MEMBER_PROPERTY("FadeOutDuration", m_FadeOutDuration),
    XII_ENUM_MEMBER_PROPERTY("OnFinishedAction", xiiOnComponentFinishedAction, m_OnFinishedAction),
    XII_ACCESSOR_PROPERTY("ApplyToDynamic", DummyGetter, SetApplyToRef)->AddAttributes(new xiiGameObjectReferenceAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Effects"),
    new xiiDirectionVisualizerAttribute("ProjectionAxis", 0.5f, xiiColorScheme::LightUI(xiiColorScheme::Blue)),
    new xiiBoxManipulatorAttribute("Extents", 1.0f, true),
    new xiiBoxVisualizerAttribute("Extents"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgComponentInternalTrigger, OnTriggered),
    XII_MESSAGE_HANDLER(xiiMsgDeleteGameObject, OnMsgDeleteGameObject),
    XII_MESSAGE_HANDLER(xiiMsgOnlyApplyToObject, OnMsgOnlyApplyToObject),
    XII_MESSAGE_HANDLER(xiiMsgSetColor, OnMsgSetColor),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiDecalComponent::xiiDecalComponent() = default;

xiiDecalComponent::~xiiDecalComponent() = default;

void xiiDecalComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  xiiStreamWriter& s = ref_stream.GetStream();

  s << m_vExtents;
  s << m_Color;
  s << m_EmissiveColor;
  s << m_InnerFadeAngle;
  s << m_OuterFadeAngle;
  s << m_fSortOrder;
  s << m_FadeOutDelay.m_Value;
  s << m_FadeOutDelay.m_fVariance;
  s << m_FadeOutDuration;
  s << m_StartFadeOutTime;
  s << m_fSizeVariance;
  s << m_OnFinishedAction;
  s << m_bWrapAround;
  s << m_bMapNormalToGeometry;

  // version 5
  s << m_ProjectionAxis;

  // version 6
  ref_stream.WriteGameObjectHandle(m_hApplyOnlyToObject);

  // version 7
  s << m_uiRandomDecalIdx;
  s.WriteArray(m_Decals).IgnoreResult();
}

void xiiDecalComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = ref_stream.GetStream();

  s >> m_vExtents;

  if (uiVersion >= 4)
  {
    s >> m_Color;
    s >> m_EmissiveColor;
  }
  else
  {
    xiiColor tmp;
    s >> tmp;
    m_Color = tmp;
  }

  s >> m_InnerFadeAngle;
  s >> m_OuterFadeAngle;
  s >> m_fSortOrder;

  if (uiVersion <= 7)
  {
    xiiUInt32 dummy;
    s >> dummy;
  }

  m_uiInternalSortKey = GetOwner()->GetStableRandomSeed();
  m_uiInternalSortKey = (m_uiInternalSortKey >> 16) ^ (m_uiInternalSortKey & 0xFFFF);

  if (uiVersion < 7)
  {
    m_Decals.SetCount(1);
    s >> m_Decals[0];
  }

  s >> m_FadeOutDelay.m_Value;
  s >> m_FadeOutDelay.m_fVariance;
  s >> m_FadeOutDuration;
  s >> m_StartFadeOutTime;
  s >> m_fSizeVariance;
  s >> m_OnFinishedAction;

  if (uiVersion >= 3)
  {
    s >> m_bWrapAround;
  }

  if (uiVersion >= 4)
  {
    s >> m_bMapNormalToGeometry;
  }

  if (uiVersion >= 5)
  {
    s >> m_ProjectionAxis;
  }

  if (uiVersion >= 6)
  {
    SetApplyOnlyTo(ref_stream.ReadGameObjectHandle());
  }

  if (uiVersion >= 7)
  {
    s >> m_uiRandomDecalIdx;
    s.ReadArray(m_Decals).IgnoreResult();
  }
}

xiiResult xiiDecalComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  if (m_Decals.IsEmpty())
    return XII_FAILURE;

  m_uiRandomDecalIdx = (GetOwner()->GetStableRandomSeed() % m_Decals.GetCount()) & 0xFF;

  const xiiUInt32 uiDecalIndex = xiiMath::Min<xiiUInt32>(m_uiRandomDecalIdx, m_Decals.GetCount() - 1);

  if (!m_Decals[uiDecalIndex].IsValid() || m_vExtents.IsZero())
    return XII_FAILURE;

  float fAspectRatio = 1.0f;

  {
    auto                                   hDecalAtlas = GetWorld()->GetComponentManager<xiiDecalComponentManager>()->m_hDecalAtlas;
    xiiResourceLock<xiiDecalAtlasResource> pDecalAtlas(hDecalAtlas, xiiResourceAcquireMode::BlockTillLoaded);

    const auto&     atlas    = pDecalAtlas->GetAtlas();
    const xiiUInt32 decalIdx = atlas.m_Items.Find(xiiHashingUtils::StringHashTo32(m_Decals[uiDecalIndex].GetResourceIDHash()));

    if (decalIdx != xiiInvalidIndex)
    {
      const auto& item = atlas.m_Items.GetValue(decalIdx);
      fAspectRatio     = (float)item.m_LayerRects[0].width / item.m_LayerRects[0].height;
    }
  }

  xiiVec3 vAspectCorrection = xiiVec3(1.0f);
  if (!xiiMath::IsEqual(fAspectRatio, 1.0f, 0.001f))
  {
    if (fAspectRatio > 1.0f)
    {
      vAspectCorrection.z /= fAspectRatio;
    }
    else
    {
      vAspectCorrection.y *= fAspectRatio;
    }
  }

  const xiiQuat axisRotation = xiiBasisAxis::GetBasisRotation_PosX(m_ProjectionAxis);
  xiiVec3       vHalfExtents = (axisRotation * vAspectCorrection).Abs().CompMul(m_vExtents * 0.5f);

  bounds = xiiBoundingBox(-vHalfExtents, vHalfExtents);
  return XII_SUCCESS;
}

void xiiDecalComponent::SetExtents(const xiiVec3& value)
{
  m_vExtents = value.CompMax(xiiVec3::ZeroVector());

  TriggerLocalBoundsUpdate();
}

const xiiVec3& xiiDecalComponent::GetExtents() const
{
  return m_vExtents;
}

void xiiDecalComponent::SetSizeVariance(float fVariance)
{
  m_fSizeVariance = xiiMath::Clamp(fVariance, 0.0f, 1.0f);
}

float xiiDecalComponent::GetSizeVariance() const
{
  return m_fSizeVariance;
}

void xiiDecalComponent::SetColor(xiiColorGammaUB color)
{
  m_Color = color;
}

xiiColorGammaUB xiiDecalComponent::GetColor() const
{
  return m_Color;
}

void xiiDecalComponent::SetEmissiveColor(xiiColor color)
{
  m_EmissiveColor = color;
}

xiiColor xiiDecalComponent::GetEmissiveColor() const
{
  return m_EmissiveColor;
}

void xiiDecalComponent::SetInnerFadeAngle(xiiAngle spotAngle)
{
  m_InnerFadeAngle = xiiMath::Clamp(spotAngle, xiiAngle::Degree(0.0f), m_OuterFadeAngle);
}

xiiAngle xiiDecalComponent::GetInnerFadeAngle() const
{
  return m_InnerFadeAngle;
}

void xiiDecalComponent::SetOuterFadeAngle(xiiAngle spotAngle)
{
  m_OuterFadeAngle = xiiMath::Clamp(spotAngle, m_InnerFadeAngle, xiiAngle::Degree(90.0f));
}

xiiAngle xiiDecalComponent::GetOuterFadeAngle() const
{
  return m_OuterFadeAngle;
}

void xiiDecalComponent::SetSortOrder(float fOrder)
{
  m_fSortOrder = fOrder;
}

float xiiDecalComponent::GetSortOrder() const
{
  return m_fSortOrder;
}

void xiiDecalComponent::SetWrapAround(bool bWrapAround)
{
  m_bWrapAround = bWrapAround;
}

bool xiiDecalComponent::GetWrapAround() const
{
  return m_bWrapAround;
}

void xiiDecalComponent::SetMapNormalToGeometry(bool bMapNormal)
{
  m_bMapNormalToGeometry = bMapNormal;
}

bool xiiDecalComponent::GetMapNormalToGeometry() const
{
  return m_bMapNormalToGeometry;
}

void xiiDecalComponent::SetDecal(xiiUInt32 uiIndex, const xiiDecalResourceHandle& hDecal)
{
  m_Decals[uiIndex] = hDecal;

  TriggerLocalBoundsUpdate();
}

const xiiDecalResourceHandle& xiiDecalComponent::GetDecal(xiiUInt32 uiIndex) const
{
  return m_Decals[uiIndex];
}

void xiiDecalComponent::SetProjectionAxis(xiiEnum<xiiBasisAxis> projectionAxis)
{
  m_ProjectionAxis = projectionAxis;

  TriggerLocalBoundsUpdate();
}

xiiEnum<xiiBasisAxis> xiiDecalComponent::GetProjectionAxis() const
{
  return m_ProjectionAxis;
}

void xiiDecalComponent::SetApplyOnlyTo(xiiGameObjectHandle hObject)
{
  if (m_hApplyOnlyToObject != hObject)
  {
    m_hApplyOnlyToObject = hObject;
    UpdateApplyTo();
  }
}

xiiGameObjectHandle xiiDecalComponent::GetApplyOnlyTo() const
{
  return m_hApplyOnlyToObject;
}

void xiiDecalComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // Don't extract decal render data for selection.
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory)
    return;

  if (m_Decals.IsEmpty())
    return;

  const xiiUInt32 uiDecalIndex = xiiMath::Min<xiiUInt32>(m_uiRandomDecalIdx, m_Decals.GetCount() - 1);

  if (!m_Decals[uiDecalIndex].IsValid() || m_vExtents.IsZero() || GetOwner()->GetLocalScaling().IsZero())
    return;

  float fFade = 1.0f;

  const xiiTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
  if (tNow > m_StartFadeOutTime)
  {
    fFade -= xiiMath::Min<float>(1.0f, (float)((tNow - m_StartFadeOutTime).GetSeconds() / m_FadeOutDuration.GetSeconds()));
  }

  xiiColor finalColor = m_Color;
  finalColor.a *= fFade;

  if (finalColor.a <= 0.0f)
    return;

  const bool  bNoFade          = m_InnerFadeAngle == xiiAngle::Radian(0.0f) && m_OuterFadeAngle == xiiAngle::Radian(0.0f);
  const float fCosInner        = xiiMath::Cos(m_InnerFadeAngle);
  const float fCosOuter        = xiiMath::Cos(m_OuterFadeAngle);
  const float fFadeParamScale  = bNoFade ? 0.0f : (1.0f / xiiMath::Max(0.001f, (fCosInner - fCosOuter)));
  const float fFadeParamOffset = bNoFade ? 1.0f : (-fCosOuter * fFadeParamScale);

  auto      hDecalAtlas            = GetWorld()->GetComponentManager<xiiDecalComponentManager>()->m_hDecalAtlas;
  xiiVec4   baseAtlasScaleOffset   = xiiVec4(0.5f);
  xiiVec4   normalAtlasScaleOffset = xiiVec4(0.5f);
  xiiVec4   ormAtlasScaleOffset    = xiiVec4(0.5f);
  xiiUInt32 uiDecalFlags           = 0;

  float fAspectRatio = 1.0f;

  {
    xiiResourceLock<xiiDecalAtlasResource> pDecalAtlas(hDecalAtlas, xiiResourceAcquireMode::BlockTillLoaded);

    const auto&     atlas    = pDecalAtlas->GetAtlas();
    const xiiUInt32 decalIdx = atlas.m_Items.Find(xiiHashingUtils::StringHashTo32(m_Decals[uiDecalIndex].GetResourceIDHash()));

    if (decalIdx != xiiInvalidIndex)
    {
      const auto& item = atlas.m_Items.GetValue(decalIdx);
      uiDecalFlags     = item.m_uiFlags;

      auto layerRectToScaleOffset = [](xiiRectU32 layerRect, xiiVec2U32 vTextureSize) {
        xiiVec4 result;
        result.x = (float)layerRect.width / vTextureSize.x * 0.5f;
        result.y = (float)layerRect.height / vTextureSize.y * 0.5f;
        result.z = (float)layerRect.x / vTextureSize.x + result.x;
        result.w = (float)layerRect.y / vTextureSize.y + result.y;
        return result;
      };

      baseAtlasScaleOffset   = layerRectToScaleOffset(item.m_LayerRects[0], pDecalAtlas->GetBaseColorTextureSize());
      normalAtlasScaleOffset = layerRectToScaleOffset(item.m_LayerRects[1], pDecalAtlas->GetNormalTextureSize());
      ormAtlasScaleOffset    = layerRectToScaleOffset(item.m_LayerRects[2], pDecalAtlas->GetORMTextureSize());

      fAspectRatio = (float)item.m_LayerRects[0].width / item.m_LayerRects[0].height;
    }
  }

  auto pRenderData = xiiCreateRenderDataForThisFrame<xiiDecalRenderData>(GetOwner());

  xiiUInt32 uiSortingId       = (xiiUInt32)(xiiMath::Min(m_fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  pRenderData->m_uiSortingKey = (uiSortingId << 16) | (m_uiInternalSortKey & 0xFFFF);

  const xiiQuat axisRotation = xiiBasisAxis::GetBasisRotation_PosX(m_ProjectionAxis);

  pRenderData->m_GlobalTransform             = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalTransform.m_vScale    = (axisRotation * (pRenderData->m_GlobalTransform.m_vScale.CompMul(m_vExtents * 0.5f))).Abs();
  pRenderData->m_GlobalTransform.m_qRotation = pRenderData->m_GlobalTransform.m_qRotation * axisRotation;

  if (!xiiMath::IsEqual(fAspectRatio, 1.0f, 0.001f))
  {
    if (fAspectRatio > 1.0f)
    {
      pRenderData->m_GlobalTransform.m_vScale.z /= fAspectRatio;
    }
    else
    {
      pRenderData->m_GlobalTransform.m_vScale.y *= fAspectRatio;
    }
  }

  pRenderData->m_uiApplyOnlyToId = m_uiApplyOnlyToId;
  pRenderData->m_uiFlags         = uiDecalFlags;
  pRenderData->m_uiFlags |= (m_bWrapAround ? DECAL_WRAP_AROUND : 0);
  pRenderData->m_uiFlags |= (m_bMapNormalToGeometry ? DECAL_MAP_NORMAL_TO_GEOMETRY : 0);
  pRenderData->m_uiAngleFadeParams = xiiShaderUtils::Float2ToRG16F(xiiVec2(fFadeParamScale, fFadeParamOffset));
  pRenderData->m_BaseColor         = finalColor;
  pRenderData->m_EmissiveColor     = m_EmissiveColor;
  xiiShaderUtils::Float4ToRGBA16F(baseAtlasScaleOffset, pRenderData->m_uiBaseColorAtlasScale, pRenderData->m_uiBaseColorAtlasOffset);
  xiiShaderUtils::Float4ToRGBA16F(normalAtlasScaleOffset, pRenderData->m_uiNormalAtlasScale, pRenderData->m_uiNormalAtlasOffset);
  xiiShaderUtils::Float4ToRGBA16F(ormAtlasScaleOffset, pRenderData->m_uiORMAtlasScale, pRenderData->m_uiORMAtlasOffset);

  xiiRenderData::Caching::Enum caching = (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0) ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::Decal, caching);
}

void xiiDecalComponent::SetApplyToRef(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  xiiGameObjectHandle hTarget = resolver(szReference, GetHandle(), "ApplyTo");

  if (m_hApplyOnlyToObject == hTarget)
    return;

  m_hApplyOnlyToObject = hTarget;

  if (IsActiveAndInitialized())
  {
    UpdateApplyTo();
  }
}

void xiiDecalComponent::UpdateApplyTo()
{
  xiiUInt32 uiPrevId = m_uiApplyOnlyToId;

  m_uiApplyOnlyToId = 0;

  if (!m_hApplyOnlyToObject.IsInvalidated())
  {
    m_uiApplyOnlyToId = xiiInvalidIndex;

    xiiGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hApplyOnlyToObject, pObject))
    {
      xiiRenderComponent* pRenderComponent = nullptr;
      if (pObject->TryGetComponentOfBaseType(pRenderComponent))
      {
        // this only works for dynamic objects, for static ones we must use ID 0
        if (pRenderComponent->GetOwner()->IsDynamic())
        {
          m_uiApplyOnlyToId = pRenderComponent->GetUniqueIdForRendering();
        }
      }
    }
  }

  if (uiPrevId != m_uiApplyOnlyToId && GetOwner()->IsStatic())
  {
    InvalidateCachedRenderData();
  }
}

static xiiHashedString s_sSuicide = xiiMakeHashedString("Suicide");

void xiiDecalComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  xiiWorld* pWorld = GetWorld();

  // no fade out -> fade out pretty late
  m_StartFadeOutTime = xiiTime::Hours(24.0 * 365.0 * 100.0); // 100 years should be enough for everybody (ignoring leap years)

  if (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0)
  {
    const xiiTime tFadeOutDelay = xiiTime::Seconds(pWorld->GetRandomNumberGenerator().DoubleVariance(m_FadeOutDelay.m_Value.GetSeconds(), m_FadeOutDelay.m_fVariance));
    m_StartFadeOutTime          = pWorld->GetClock().GetAccumulatedTime() + tFadeOutDelay;

    if (m_OnFinishedAction != xiiOnComponentFinishedAction::None)
    {
      xiiMsgComponentInternalTrigger msg;
      msg.m_sMessage = s_sSuicide;

      const xiiTime tKill = tFadeOutDelay + m_FadeOutDuration;

      PostMessage(msg, tKill);
    }
  }

  if (m_fSizeVariance > 0)
  {
    const float scale = (float)pWorld->GetRandomNumberGenerator().DoubleVariance(1.0, m_fSizeVariance);
    m_vExtents *= scale;

    TriggerLocalBoundsUpdate();

    InvalidateCachedRenderData();
  }
}

void xiiDecalComponent::OnActivated()
{
  SUPER::OnActivated();

  m_uiInternalSortKey = GetOwner()->GetStableRandomSeed();
  m_uiInternalSortKey = (m_uiInternalSortKey >> 16) ^ (m_uiInternalSortKey & 0xFFFF);

  UpdateApplyTo();
}

void xiiDecalComponent::OnTriggered(xiiMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != s_sSuicide)
    return;

  xiiOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);
}

void xiiDecalComponent::OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg)
{
  xiiOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

void xiiDecalComponent::OnMsgOnlyApplyToObject(xiiMsgOnlyApplyToObject& msg)
{
  SetApplyOnlyTo(msg.m_hObject);
}

void xiiDecalComponent::OnMsgSetColor(xiiMsgSetColor& msg)
{
  msg.ModifyColor(m_Color);
}

xiiUInt32 xiiDecalComponent::DecalFile_GetCount() const
{
  return m_Decals.GetCount();
}

const char* xiiDecalComponent::DecalFile_Get(xiiUInt32 uiIndex) const
{
  if (!m_Decals[uiIndex].IsValid())
    return "";

  return m_Decals[uiIndex].GetResourceID();
}

void xiiDecalComponent::DecalFile_Set(xiiUInt32 uiIndex, const char* szFile)
{
  xiiDecalResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiDecalResource>(szFile);
  }

  SetDecal(uiIndex, hResource);
}

void xiiDecalComponent::DecalFile_Insert(xiiUInt32 uiIndex, const char* szFile)
{
  m_Decals.Insert(xiiDecalResourceHandle(), uiIndex);
  DecalFile_Set(uiIndex, szFile);
}

void xiiDecalComponent::DecalFile_Remove(xiiUInt32 uiIndex)
{
  m_Decals.RemoveAtAndCopy(uiIndex);
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiDecalComponent_6_7 : public xiiGraphPatch
{
public:
  xiiDecalComponent_6_7() :
    xiiGraphPatch("xiiDecalComponent", 7)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pDecal = pNode->FindProperty("Decal");
    if (pDecal && pDecal->m_Value.IsA<xiiString>())
    {
      xiiVariantArray ar;
      ar.PushBack(pDecal->m_Value.Get<xiiString>());
      pNode->AddProperty("Decals", ar);
    }
  }
};

xiiDecalComponent_6_7 g_xiiDecalComponent_6_7;

XII_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalComponent);
