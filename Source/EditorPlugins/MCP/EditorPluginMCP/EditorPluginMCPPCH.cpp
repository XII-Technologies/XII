/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <Foundation/Configuration/Plugin.h>

XII_STATICLINK_LIBRARY(EditorPluginMCP)
{
  if (bReturn)
  {
    return;
  }

  XII_STATICLINK_REFERENCE(EditorPluginMCP_Implementation_EditorMCP);
}
