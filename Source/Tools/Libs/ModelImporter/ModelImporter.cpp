/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <ModelImporter/ImporterAssimp/ImporterAssimp.h>
#include <ModelImporter/ModelImporter.h>

namespace xiiModelImporter
{
  xiiUniquePtr<Importer> RequestImporterForFileType(xiiStringView sFileName)
  {
    if (sFileName.HasExtension(".fbx") || sFileName.HasExtension(".obj") || sFileName.HasExtension(".gltf") || sFileName.HasExtension(".glb") || sFileName.HasExtension(".stl") || sFileName.HasExtension(".ply"))
    {
      return XII_DEFAULT_NEW(ImporterAssimp);
    }
    return nullptr;
  }
} // namespace xiiModelImporter
