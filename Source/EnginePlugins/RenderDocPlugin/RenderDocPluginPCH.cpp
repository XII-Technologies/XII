/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <RenderDocPlugin/RenderDocPluginPCH.h>

XII_STATICLINK_LIBRARY(RenderDocPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(RenderDocPlugin_Implementation_RenderDocSingleton);
}
