/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

using xiiCommentComponentManager = xiiComponentManager<class xiiCommentComponent, xiiBlockStorageType::Compact>;

/// This component is for adding notes to objects in a scene.
///
/// These comments are solely to explain things to other people that look at the scene or prefab structure.
/// They are not meant for use at runtime. Therefore, all instances of xiiCommentComponent are automatically stripped from a scene during export.
class XII_ENGINEPLUGINSCENE_DLL xiiCommentComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiCommentComponent, xiiComponent, xiiCommentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiCommentComponent

public:
  xiiCommentComponent();
  ~xiiCommentComponent();

  void        SetComment(const char* szText);
  const char* GetComment() const;

private:
  xiiHashedString m_sComment;
};

//////////////////////////////////////////////////////////////////////////

class XII_ENGINEPLUGINSCENE_DLL xiiSceneExportModifier_RemoveCommentComponents : public xiiSceneExportModifier
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneExportModifier_RemoveCommentComponents, xiiSceneExportModifier);

public:
  virtual void ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport) override;
};
