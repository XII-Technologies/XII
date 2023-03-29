#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiShapeIconComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Editing Utilities"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiShapeIconComponent::xiiShapeIconComponent()  = default;
xiiShapeIconComponent::~xiiShapeIconComponent() = default;
