/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ModelImporter2/Importer/Importer.h>

namespace xiiModelImporter2
{
  /// Importer implementation to import Source engine BSP files.
  class ImporterMagicaVoxel : public Importer
  {
  public:
    ImporterMagicaVoxel();
    ~ImporterMagicaVoxel();

  protected:
    virtual xiiResult DoImport() override;
  };
} // namespace xiiModelImporter2
