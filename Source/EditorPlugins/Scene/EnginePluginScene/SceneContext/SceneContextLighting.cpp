/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <SharedPluginScene/Scene/ScenePreviewLighting.h>

void xiiSceneContext::AddAmbientLight(bool bSetEditorTag, bool bForce)
{
  if (m_pPreviewLighting == nullptr)
    m_pPreviewLighting = XII_DEFAULT_NEW(xiiScenePreviewLighting, *GetWorld());

  if (m_pPreviewLighting->EnsureCreated(bSetEditorTag, bForce).Failed())
    xiiLog::Error("Failed to create scene preview lighting.");
}

void xiiSceneContext::RemoveAmbientLight()
{
  m_pPreviewLighting.Clear();
}
