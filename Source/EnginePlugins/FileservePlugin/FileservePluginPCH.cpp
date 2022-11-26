#include <FileservePlugin/FileservePluginPCH.h>

XII_STATICLINK_LIBRARY(FileservePlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(FileservePlugin_Client_FileserveClient);
  XII_STATICLINK_REFERENCE(FileservePlugin_Client_FileserveDataDir);
  XII_STATICLINK_REFERENCE(FileservePlugin_Fileserver_ClientContext);
  XII_STATICLINK_REFERENCE(FileservePlugin_Fileserver_Fileserver);
  XII_STATICLINK_REFERENCE(FileservePlugin_Main);
}
