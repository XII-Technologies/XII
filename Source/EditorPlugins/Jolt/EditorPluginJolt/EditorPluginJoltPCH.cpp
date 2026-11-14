/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginJolt/EditorPluginJoltPCH.h>

XII_STATICLINK_LIBRARY(EditorPluginJolt)
{
  if (bReturn)
  {
    return;
  }

  XII_STATICLINK_REFERENCE(EditorPluginJolt_CollisionMeshAsset_Implementation_JoltCollisionMeshAsset);
  XII_STATICLINK_REFERENCE(EditorPluginJolt_Authoring_Implementation_JoltComponentAdapters);
  XII_STATICLINK_REFERENCE(EditorPluginJolt_Authoring_Implementation_JoltEditorActions);
  XII_STATICLINK_REFERENCE(EditorPluginJolt_Authoring_Implementation_JoltEditorIntegration);
}
