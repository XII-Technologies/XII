#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class xiiBakingScene;

class xiiLongOpWorker_BakeScene : public xiiLongOpWorker
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLongOpWorker_BakeScene, xiiLongOpWorker);

public:
  virtual xiiResult InitializeExecution(xiiStreamReader& config, const xiiUuid& DocumentGuid) override;
  virtual xiiResult Execute(xiiProgress& progress, xiiStreamWriter& proxydata) override;

  xiiString       m_sOutputPath;
  xiiBakingScene* m_pScene;
};
