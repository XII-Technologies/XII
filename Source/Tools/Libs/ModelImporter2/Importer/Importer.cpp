#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/Importer/Importer.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

namespace xiiModelImporter2
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

} // namespace xiiModelImporter2
