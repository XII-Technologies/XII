/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <DearImguiPlugin/DearImguiPluginPCH.h>

XII_STATICLINK_LIBRARY(DearImguiPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(DearImguiPlugin_Implementation_DearImgui);
}
