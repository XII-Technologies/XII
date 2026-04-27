#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Camera/StereoCameraComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStereoCameraRenderData, 1, xiiRTTIDefaultAllocator<xiiStereoCameraRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiStereoCameraComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("EyeSeparation", GetEyeSeparation, SetEyeSeparation)->AddAttributes(new xiiDefaultValueAttribute(0.064f), new xiiClampValueAttribute(0.0f, 0.5f)),
    XII_ACCESSOR_PROPERTY("ConvergenceDist", GetConvergenceDist, SetConvergenceDist)->AddAttributes(new xiiDefaultValueAttribute(2.0f), new xiiClampValueAttribute(0.1f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Camera"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiStereoCameraComponent::xiiStereoCameraComponent()  = default;
xiiStereoCameraComponent::~xiiStereoCameraComponent() = default;

void xiiStereoCameraComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_fEyeSeparation;
  s << m_fConvergenceDist;
}

void xiiStereoCameraComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_fEyeSeparation;
  s >> m_fConvergenceDist;
}

void xiiStereoCameraComponent::SetEyeSeparation(float f) { m_fEyeSeparation = xiiMath::Clamp(f, 0.0f, 0.5f); }
void xiiStereoCameraComponent::SetConvergenceDist(float f) { m_fConvergenceDist = xiiMath::Max(f, 0.1f); }

void xiiStereoCameraComponent::SetEyeMatrices(const xiiMat4& mLeftViewProj, const xiiMat4& mRightViewProj)
{
  m_mLeftViewProj  = mLeftViewProj;
  m_mRightViewProj = mRightViewProj;
}

void xiiStereoCameraComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiStereoCameraRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiStereoCameraRenderData>(this);
  pRenderData->m_GlobalTransform         = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds            = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject            = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent         = GetHandle();
  pRenderData->m_mLeftEyeViewProj        = m_mLeftViewProj;
  pRenderData->m_mRightEyeViewProj       = m_mRightViewProj;
  pRenderData->m_fEyeSeparation          = m_fEyeSeparation;
  pRenderData->m_fConvergenceDist        = m_fConvergenceDist;
  pRenderData->m_uiSortingKey            = GetUniqueIdForRendering(0);

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Camera_Implementation_StereoCameraComponent);
