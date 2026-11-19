/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>

XII_STATICLINK_LIBRARY(MCPPlugin)
{
  if (bReturn)
  {
    return;
  }

  XII_STATICLINK_REFERENCE(MCPPlugin_Implementation_Bridge);
  XII_STATICLINK_REFERENCE(MCPPlugin_Implementation_Runtime);
}
