#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Camera/FrameGraphNodeComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFrameGraphNodeRenderData, 1, xiiRTTIDefaultAllocator<xiiFrameGraphNodeRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiFrameGraphNodeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("PassName", GetPassName, SetPassName),
    XII_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority)->AddAttributes(new xiiDefaultValueAttribute(0)),
    XII_ACCESSOR_PROPERTY("Enabled", GetEnabled, SetEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/FrameGraph"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiFrameGraphNodeComponent::xiiFrameGraphNodeComponent()  = default;
xiiFrameGraphNodeComponent::~xiiFrameGraphNodeComponent() = default;

void xiiFrameGraphNodeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_sPassName.GetString();
  s << m_iPriority;
  s << m_bEnabled;
}

void xiiFrameGraphNodeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto&     s = inout_stream.GetStream();
  xiiString sName;
  s >> sName;
  m_sPassName.Assign(sName);
  s >> m_iPriority;
  s >> m_bEnabled;
}

void          xiiFrameGraphNodeComponent::SetPassName(xiiStringView sName) { m_sPassName.Assign(sName); }
xiiStringView xiiFrameGraphNodeComponent::GetPassName() const { return m_sPassName.GetString(); }
void          xiiFrameGraphNodeComponent::SetPriority(xiiInt32 i) { m_iPriority = i; }
void          xiiFrameGraphNodeComponent::SetEnabled(bool b) { m_bEnabled = b; }

void xiiFrameGraphNodeComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiFrameGraphNodeRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiFrameGraphNodeRenderData>(this);
  pRenderData->m_GlobalTransform           = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds              = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject              = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent           = GetHandle();
  pRenderData->m_sPassName                 = m_sPassName;
  pRenderData->m_iPriority                 = m_iPriority;
  pRenderData->m_bEnabled                  = m_bEnabled;
  pRenderData->m_uiSortingKey              = GetUniqueIdForRendering(0);

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Camera_Implementation_FrameGraphNodeComponent);
