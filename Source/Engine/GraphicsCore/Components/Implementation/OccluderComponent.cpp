#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/OccluderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiOccluderComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiClampValueAttribute(xiiVec3(0.0f), {}), new xiiDefaultValueAttribute(xiiVec3(1.0f))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgExtractOccluderData, OnMsgExtractOccluderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
    new xiiBoxVisualizerAttribute("Extents", 1.0f, xiiColorScheme::LightUI(xiiColorScheme::Blue)),
    new xiiBoxManipulatorAttribute("Extents", 1.0f, true),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiOccluderComponentManager::xiiOccluderComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiOccluderComponent, xiiBlockStorageType::FreeList>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

xiiOccluderComponent::xiiOccluderComponent()  = default;
xiiOccluderComponent::~xiiOccluderComponent() = default;

void xiiOccluderComponent::SetExtents(const xiiVec3& vExtents)
{
  m_vExtents = vExtents;
  m_pOccluderObject.Clear();

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiOccluderComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  if (GetOwner()->IsStatic())
    msg.AddBounds(xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), xiiDefaultSpatialDataCategories::OcclusionStatic);
  else
    msg.AddBounds(xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), xiiDefaultSpatialDataCategories::OcclusionDynamic);
}

void xiiOccluderComponent::OnMsgExtractOccluderData(xiiMsgExtractOccluderData& msg) const
{
  if (IsActiveAndInitialized())
  {
    if (m_pOccluderObject == nullptr)
    {
      m_pOccluderObject = xiiRasterizerObject::CreateBox(m_vExtents);
    }

    msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
  }
}

void xiiOccluderComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
}

void xiiOccluderComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
}

void xiiOccluderComponent::OnActivated()
{
  m_pOccluderObject.Clear();
  GetOwner()->UpdateLocalBounds();
}

void xiiOccluderComponent::OnDeactivated()
{
  m_pOccluderObject.Clear();
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_OccluderComponent);
