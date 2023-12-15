#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/View.h>

struct GICFlags
{
  enum Enum
  {
    DebugShowPoints = 0,
  };
};

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGrabbableItemGrabPoint, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGrabbableItemGrabPoint>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    XII_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiTransformManipulatorAttribute("LocalPosition", "LocalRotation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE


XII_BEGIN_COMPONENT_TYPE(xiiGrabbableItemComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("DebugShowPoints", GetDebugShowPoints, SetDebugShowPoints),
    XII_ARRAY_MEMBER_PROPERTY("GrabPoints", m_GrabPoints),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Input"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGrabbableItemComponent::xiiGrabbableItemComponent()  = default;
xiiGrabbableItemComponent::~xiiGrabbableItemComponent() = default;

void xiiGrabbableItemComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  const xiiUInt8 uiNumGrabPoints = static_cast<xiiUInt8>(m_GrabPoints.GetCount());
  s << uiNumGrabPoints;
  for (const auto& gb : m_GrabPoints)
  {
    s << gb.m_vLocalPosition;
    s << gb.m_qLocalRotation;
  }
}

void xiiGrabbableItemComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  xiiUInt8 uiNumGrabPoints;
  s >> uiNumGrabPoints;
  m_GrabPoints.SetCount(uiNumGrabPoints);
  for (auto& gb : m_GrabPoints)
  {
    s >> gb.m_vLocalPosition;
    s >> gb.m_qLocalRotation;
  }
}

void xiiGrabbableItemComponent::SetDebugShowPoints(bool bShow)
{
  SetUserFlag(GICFlags::DebugShowPoints, bShow);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool xiiGrabbableItemComponent::GetDebugShowPoints() const
{
  return GetUserFlag(GICFlags::DebugShowPoints);
}

void xiiGrabbableItemComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const
{
  if (GetDebugShowPoints())
  {
    msg.AddBounds(xiiBoundingSphere(xiiVec3::ZeroVector(), 1.0f), xiiDefaultSpatialDataCategories::RenderDynamic);
  }
}

void xiiGrabbableItemComponent::OnExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (!GetDebugShowPoints() || m_GrabPoints.IsEmpty())
    return;

  if (msg.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::MainView &&
      msg.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::EditorView)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory)
    return;

  const xiiTransform globalTransform = GetOwner()->GetGlobalTransform();

  for (auto& grabPoint : m_GrabPoints)
  {
    xiiTransform grabPointTransform;
    grabPointTransform.SetGlobalTransform(globalTransform, xiiTransform(grabPoint.m_vLocalPosition, grabPoint.m_qLocalRotation));

    xiiDebugRenderer::DrawArrow(GetWorld(), 0.75f, xiiColorScheme::LightUI(xiiColorScheme::Red), grabPointTransform, xiiVec3::UnitXAxis());
    xiiDebugRenderer::DrawArrow(GetWorld(), 0.3f, xiiColorScheme::LightUI(xiiColorScheme::Green), grabPointTransform, xiiVec3::UnitYAxis());
    xiiDebugRenderer::DrawArrow(GetWorld(), 0.3f, xiiColorScheme::LightUI(xiiColorScheme::Blue), grabPointTransform, xiiVec3::UnitZAxis());
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GrabbableItemComponent);
