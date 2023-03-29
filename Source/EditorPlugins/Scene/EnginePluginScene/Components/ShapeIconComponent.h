#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <GameEngine/GameEngineDLL.h>

typedef xiiComponentManager<class xiiShapeIconComponent, xiiBlockStorageType::Compact> xiiShapeIconComponentManager;

/// \brief This is a dummy component that the editor creates on all 'empty' nodes for the sole purpose to render a shape icon and enable picking.
///
/// Though in the future one could potentially use them for other editor functionality, such as displaying the object name or some other useful text.
class XII_ENGINEPLUGINSCENE_DLL xiiShapeIconComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiShapeIconComponent, xiiComponent, xiiShapeIconComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiShapeIconComponent

public:
  xiiShapeIconComponent();
  ~xiiShapeIconComponent();
};

//////////////////////////////////////////////////////////////////////////

class XII_ENGINEPLUGINSCENE_DLL xiiSceneExportModifier_RemoveShapeIconComponents : public xiiSceneExportModifier
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneExportModifier_RemoveShapeIconComponents, xiiSceneExportModifier);

public:
  virtual void ModifyWorld(xiiWorld& world, const xiiUuid& documentGuid, bool bForExport) override;
};
