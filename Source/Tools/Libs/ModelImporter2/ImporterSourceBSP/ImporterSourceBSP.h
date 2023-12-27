#pragma once

#include <ModelImporter2/Importer/Importer.h>

namespace xiiModelImporter2
{
  /// Importer implementation to import Source engine BSP files.
  class ImporterSourceBSP : public Importer
  {
  public:
    ImporterSourceBSP();
    ~ImporterSourceBSP();

  protected:
    virtual xiiResult DoImport() override;
  };
} // namespace xiiModelImporter2
