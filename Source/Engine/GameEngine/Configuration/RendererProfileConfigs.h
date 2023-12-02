#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Configuration/PlatformProfile.h>

class XII_GAMEENGINE_DLL xiiRenderPipelineProfileConfig : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineProfileConfig, xiiProfileConfigData);

public:
  virtual void SaveRuntimeData(xiiChunkStreamWriter& inout_stream) const override;
  virtual void LoadRuntimeData(xiiChunkStreamReader& inout_stream) override;

  xiiString m_sMainRenderPipeline;
  // xiiString m_sEditorRenderPipeline;
  // xiiString m_sDebugRenderPipeline;

  xiiMap<xiiString, xiiString> m_CameraPipelines;
};
