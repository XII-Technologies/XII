/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FileservePlugin/FileservePluginPCH.h>

XII_STATICLINK_LIBRARY(FileservePlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(FileservePlugin_Client_FileserveClient);
  XII_STATICLINK_REFERENCE(FileservePlugin_Implementation_FileservePlugin);
}
