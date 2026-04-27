#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/GizmoComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGizmoType, 1)
  XII_ENUM_CONSTANTS(xiiGizmoType::TranslationAxes, xiiGizmoType::RotationRings, xiiGizmoType::ScaleHandles,
                     xiiGizmoType::BoundingBox, xiiGizmoType::Sphere, xiiGizmoType::Custom)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGizmoRenderData, 1, xiiRTTIDefaultAllocator<xiiGizmoRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiGizmoComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("GizmoType", xiiGizmoType, GetGizmoType, SetGizmoType),
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor),
    XII_ACCESSOR_PROPERTY("Scale", GetScale, SetScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.001f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Debug/Gizmos"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiGizmoComponent::xiiGizmoComponent()  = default;
xiiGizmoComponent::~xiiGizmoComponent() = default;

void xiiGizmoComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_GizmoType.GetValue();
  s << m_Color;
  s << m_fScale;
}

void xiiGizmoComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto&    s         = inout_stream.GetStream();
  xiiUInt8 gizmoType = 0;
  s >> gizmoType;
  m_GizmoType = static_cast<xiiGizmoType::Enum>(gizmoType);
  s >> m_Color;
  s >> m_fScale;
}

xiiResult xiiGizmoComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bounds);
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = true;
  return XII_SUCCESS;
}

void xiiGizmoComponent::SetGizmoType(xiiEnum<xiiGizmoType> type)
{
  m_GizmoType = type;
  InvalidateCachedRenderData();
}

void xiiGizmoComponent::SetColor(const xiiColor& color)
{
  m_Color = color;
  InvalidateCachedRenderData();
}

void xiiGizmoComponent::SetScale(float fScale)
{
  m_fScale = xiiMath::Max(fScale, 0.001f);
  InvalidateCachedRenderData();
}

void xiiGizmoComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiGizmoRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiGizmoRenderData>(this);
  pRenderData->m_GlobalTransform  = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds     = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject     = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent  = GetHandle();
  pRenderData->m_GizmoType        = m_GizmoType;
  pRenderData->m_Color            = m_Color;
  pRenderData->m_fScale           = m_fScale;
  pRenderData->m_uiSortingKey     = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_GizmoComponent);
