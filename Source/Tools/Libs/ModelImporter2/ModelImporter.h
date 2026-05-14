/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <ModelImporter2/Importer/Importer.h>

namespace xiiModelImporter2
{
  XII_MODELIMPORTER2_DLL xiiUniquePtr<Importer> RequestImporterForFileType(xiiStringView sFile);
} // namespace xiiModelImporter2
