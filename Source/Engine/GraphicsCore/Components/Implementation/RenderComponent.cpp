#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Components/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderDataManager.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiRenderComponent, 1)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

xiiRenderComponent::xiiRenderComponent()  = default;
xiiRenderComponent::~xiiRenderComponent() = default;

void xiiRenderComponent::Deinitialize()
{
  GetWorld()->GetOrCreateModule<xiiRenderDataManager>()->DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void xiiRenderComponent::OnActivated()
{
  TriggerLocalBoundsUpdate();
}

void xiiRenderComponent::OnDeactivated()
{
  // Can't call TriggerLocalBoundsUpdate because it checks whether we are active, which is not the case anymore.
  GetOwner()->UpdateLocalBounds();
}

void xiiRenderComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  xiiBoundingBoxSphere bounds = xiiBoundingBoxSphere::MakeInvalid();

  bool bAlwaysVisible = false;

  if (GetLocalBounds(bounds, bAlwaysVisible, msg).Succeeded())
  {
    xiiSpatialData::Category category = GetOwner()->IsDynamic() ? xiiDefaultSpatialDataCategories::RenderDynamic : xiiDefaultSpatialDataCategories::RenderStatic;

    if (bounds.IsValid())
    {
      msg.AddBounds(bounds, category);
    }

    if (bAlwaysVisible)
    {
      msg.SetAlwaysVisible(category);
    }
  }
}

void xiiRenderComponent::InvalidateCachedRenderData()
{
  if (IsActiveAndInitialized())
  {
    GetWorld()->GetOrCreateModule<xiiRenderDataManager>()->DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void xiiRenderComponent::TriggerLocalBoundsUpdate()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

// static
xiiUInt32 xiiRenderComponent::GetUniqueIdForRendering(const xiiComponent& component, xiiUInt32 uiInnerIndex /*= 0*/, xiiUInt32 uiInnerIndexShift /*= 24*/)
{
  xiiUInt32 uniqueId = component.GetUniqueID();
  if (uniqueId == xiiInvalidIndex)
  {
    uniqueId = component.GetOwner()->GetHandle().GetInternalID().m_InstanceIndex;
  }
  else
  {
    uniqueId |= (uiInnerIndex << uiInnerIndexShift);
  }

  const xiiUInt32 dynamicBit     = (1 << 31);
  const xiiUInt32 dynamicBitMask = ~dynamicBit;
  return (uniqueId & dynamicBitMask) | (component.GetOwner()->IsDynamic() ? dynamicBit : 0);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_RenderComponent);
