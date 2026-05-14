/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <ModelImporter/ImporterAssimp/ImporterAssimp.h>
#include <ModelImporter/ModelImporter.h>

namespace xiiModelImporter
{
  xiiUniquePtr<Importer> RequestImporterForFileType(xiiStringView sFile)
  {
    if (sFile.HasExtension(".fbx") || sFile.HasExtension(".obj") || sFile.HasExtension(".gltf") || sFile.HasExtension(".glb") || sFile.HasExtension(".blend"))
    {
      return XII_DEFAULT_NEW(ImporterAssimp);
    }
    return nullptr;
  }
} // namespace xiiModelImporter
