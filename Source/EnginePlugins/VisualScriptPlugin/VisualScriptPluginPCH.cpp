/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/VisualScriptPluginDLL.h>

#include <Foundation/Configuration/Plugin.h>

XII_STATICLINK_LIBRARY(VisualScriptPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(VisualScriptPlugin_Resources_VisualScriptClassResource);
  XII_STATICLINK_REFERENCE(VisualScriptPlugin_Runtime_VisualScript);
  XII_STATICLINK_REFERENCE(VisualScriptPlugin_Runtime_VisualScriptDataType);
}
