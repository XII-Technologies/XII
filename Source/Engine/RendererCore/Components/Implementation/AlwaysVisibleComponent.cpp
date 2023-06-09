#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/AlwaysVisibleComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiAlwaysVisibleComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiAlwaysVisibleComponent::xiiAlwaysVisibleComponent()  = default;
xiiAlwaysVisibleComponent::~xiiAlwaysVisibleComponent() = default;

xiiResult xiiAlwaysVisibleComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return XII_SUCCESS;
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_AlwaysVisibleComponent);
