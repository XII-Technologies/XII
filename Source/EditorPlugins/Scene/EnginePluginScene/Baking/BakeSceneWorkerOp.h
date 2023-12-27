#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class xiiBakingScene;

class xiiLongOpWorker_BakeScene : public xiiLongOpWorker
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLongOpWorker_BakeScene, xiiLongOpWorker);

public:
  virtual xiiResult InitializeExecution(xiiStreamReader& ref_config, const xiiUuid& documentGuid) override;
  virtual xiiResult Execute(xiiProgress& ref_progress, xiiStreamWriter& ref_proxydata) override;

  xiiString       m_sOutputPath;
  xiiBakingScene* m_pScene;
};
