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

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneExportModifier_RemoveShapeIconComponents, 1, xiiRTTIDefaultAllocator<xiiSceneExportModifier_RemoveShapeIconComponents>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSceneExportModifier_RemoveShapeIconComponents::ModifyWorld(xiiWorld& world, const xiiUuid& documentGuid)
{
  XII_LOCK(world.GetWriteMarker());

  if (xiiShapeIconComponentManager* pSiMan = world.GetComponentManager<xiiShapeIconComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      pSiMan->DeleteComponent(it->GetHandle());
    }
  }
}
