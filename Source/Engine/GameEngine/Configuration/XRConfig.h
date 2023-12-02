#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Configuration/PlatformProfile.h>

class XII_GAMEENGINE_DLL xiiXRConfig : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiXRConfig, xiiProfileConfigData);

public:
  virtual void SaveRuntimeData(xiiChunkStreamWriter& inout_stream) const override;
  virtual void LoadRuntimeData(xiiChunkStreamReader& inout_stream) override;

  bool      m_bEnableXR = false;
  xiiString m_sXRRenderPipeline;
};
