#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/PlanarReflectionComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlanarReflectionRenderData, 1, xiiRTTIDefaultAllocator<xiiPlanarReflectionRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiPlanarReflectionComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ClipOffset", GetClipOffset, SetClipOffset)->AddAttributes(new xiiDefaultValueAttribute(0.01f)),
    XII_ACCESSOR_PROPERTY("Width",      GetWidth,      SetWidth)->AddAttributes(new xiiDefaultValueAttribute(512u)),
    XII_ACCESSOR_PROPERTY("Height",     GetHeight,     SetHeight)->AddAttributes(new xiiDefaultValueAttribute(512u)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Reflections"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiPlanarReflectionComponent::xiiPlanarReflectionComponent()  = default;
xiiPlanarReflectionComponent::~xiiPlanarReflectionComponent() = default;

void xiiPlanarReflectionComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_fClipOffset << m_uiWidth << m_uiHeight;
}

void xiiPlanarReflectionComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_fClipOffset >> m_uiWidth >> m_uiHeight;
}

xiiResult xiiPlanarReflectionComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bounds);
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = true;
  return XII_SUCCESS;
}

void xiiPlanarReflectionComponent::SetClipOffset(float f) { m_fClipOffset = f; }
void xiiPlanarReflectionComponent::SetWidth(xiiUInt32 w) { m_uiWidth = xiiMath::Clamp(w, 1u, 16384u); }
void xiiPlanarReflectionComponent::SetHeight(xiiUInt32 h) { m_uiHeight = xiiMath::Clamp(h, 1u, 16384u); }

void xiiPlanarReflectionComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  // The reflection plane is the owner's local XY plane transformed to world space.
  const xiiVec3 vNormal = GetOwner()->GetGlobalRotation() * xiiVec3(0, 0, 1);
  const float   fD      = -vNormal.Dot(GetOwner()->GetGlobalPosition());

  xiiPlanarReflectionRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiPlanarReflectionRenderData>(this);
  pRenderData->m_GlobalTransform             = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds                = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject                = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent             = GetHandle();
  pRenderData->m_hRenderTarget               = m_hRenderTarget;
  pRenderData->m_ReflectionPlane             = xiiVec4(vNormal.x, vNormal.y, vNormal.z, fD);
  pRenderData->m_fClipOffset                 = m_fClipOffset;
  pRenderData->m_uiWidth                     = m_uiWidth;
  pRenderData->m_uiHeight                    = m_uiHeight;
  pRenderData->m_uiSortingKey                = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_PlanarReflectionComponent);
