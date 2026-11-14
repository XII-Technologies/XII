/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMujoco/EditorPluginMujocoPCH.h>

XII_STATICLINK_LIBRARY(EditorPluginMujoco)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(EditorPluginMujoco_ModelAsset_Implementation_MuJoCoModelAsset);
  XII_STATICLINK_REFERENCE(EditorPluginMujoco_Authoring_Implementation_MuJoCoEditorIntegration);
  XII_STATICLINK_REFERENCE(EditorPluginMujoco_Authoring_Implementation_MuJoCoSceneInspector);
}
