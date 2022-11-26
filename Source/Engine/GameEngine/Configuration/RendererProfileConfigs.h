#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <GameEngine/GameEngineDLL.h>

class XII_GAMEENGINE_DLL xiiRenderPipelineProfileConfig : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineProfileConfig, xiiProfileConfigData);

public:
  virtual void SaveRuntimeData(xiiChunkStreamWriter& stream) const override;
  virtual void LoadRuntimeData(xiiChunkStreamReader& stream) override;

  xiiString m_sMainRenderPipeline;
  // xiiString m_sEditorRenderPipeline;
  // xiiString m_sDebugRenderPipeline;

  xiiMap<xiiString, xiiString> m_CameraPipelines;
};
