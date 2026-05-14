/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <GraphicsCore/AnimationSystem/EditableSkeleton.h>
#include <GraphicsCore/Meshes/MeshResourceDescriptor.h>
#include <ModelImporter/Importer/Importer.h>

namespace xiiModelImporter
{
  Importer::Importer()  = default;
  Importer::~Importer() = default;

  xiiResult Importer::Import(const ImportOptions& options, xiiLogInterface* pLogInterface /*= nullptr*/, xiiProgress* pProgress /*= nullptr*/)
  {
    xiiResult res = XII_FAILURE;

    xiiLogInterface* pPrevLogSystem = xiiLog::GetThreadLocalLogSystem();

    if (pLogInterface)
    {
      xiiLog::SetThreadLocalLogSystem(pLogInterface);
    }

    {
      m_pProgress = pProgress;
      m_Options   = options;

      XII_LOG_BLOCK("ModelImport", m_Options.m_sSourceFile);

      res = DoImport();
    }

    xiiLog::SetThreadLocalLogSystem(pPrevLogSystem);

    return res;
  }

  void OutputTexture::GenerateFileName(xiiStringBuilder& out_sName) const
  {
    xiiStringBuilder tmp("Embedded_", m_sFilename);

    xiiPathUtils::MakeValidFilename(tmp.GetFileName(), '_', out_sName);
    out_sName.ChangeFileExtension(m_sFileFormatExtension);
  }

} // namespace xiiModelImporter
