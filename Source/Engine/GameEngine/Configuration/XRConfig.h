#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <GameEngine/GameEngineDLL.h>

class XII_GAMEENGINE_DLL xiiXRConfig : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiXRConfig, xiiProfileConfigData);

public:
  virtual void SaveRuntimeData(xiiChunkStreamWriter& stream) const override;
  virtual void LoadRuntimeData(xiiChunkStreamReader& stream) override;

  bool      m_bEnableXR = false;
  xiiString m_sXRRenderPipeline;
};
