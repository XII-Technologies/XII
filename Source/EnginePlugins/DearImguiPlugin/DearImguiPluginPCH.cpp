/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <DearImguiPlugin/DearImguiPluginPCH.h>

XII_STATICLINK_LIBRARY(DearImguiPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(DearImguiPlugin_Implementation_DearImguiSubsystem);
  XII_STATICLINK_REFERENCE(DearImguiPlugin_Implementation_DearImguiContext);
  XII_STATICLINK_REFERENCE(DearImguiPlugin_Implementation_DearImguiRenderer);
  XII_STATICLINK_REFERENCE(DearImguiPlugin_Implementation_DearImguiInput);
  XII_STATICLINK_REFERENCE(DearImguiPlugin_Pipeline_Implementation_DearImguiRenderPass);
}
