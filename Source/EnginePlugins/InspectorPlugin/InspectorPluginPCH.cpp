#include <InspectorPlugin/InspectorPluginPCH.h>

XII_STATICLINK_LIBRARY(InspectorPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(InspectorPlugin_App);
  XII_STATICLINK_REFERENCE(InspectorPlugin_CVars);
  XII_STATICLINK_REFERENCE(InspectorPlugin_GlobalEvents);
  XII_STATICLINK_REFERENCE(InspectorPlugin_Input);
  XII_STATICLINK_REFERENCE(InspectorPlugin_Log);
  XII_STATICLINK_REFERENCE(InspectorPlugin_Main);
  XII_STATICLINK_REFERENCE(InspectorPlugin_Memory);
  XII_STATICLINK_REFERENCE(InspectorPlugin_OSFile);
  XII_STATICLINK_REFERENCE(InspectorPlugin_Plugins);
  XII_STATICLINK_REFERENCE(InspectorPlugin_Startup);
  XII_STATICLINK_REFERENCE(InspectorPlugin_Stats);
  XII_STATICLINK_REFERENCE(InspectorPlugin_Time);
}
