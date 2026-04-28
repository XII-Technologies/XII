/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Interfaces/WindWorldModule.h>

class XII_GAMEENGINE_DLL xiiSimpleWindWorldModule : public xiiWindWorldModuleInterface
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiSimpleWindWorldModule, xiiWindWorldModuleInterface);

public:
  xiiSimpleWindWorldModule(xiiWorld* pWorld);
  ~xiiSimpleWindWorldModule();

  virtual xiiVec3 GetWindAt(const xiiVec3& vPosition) const override;

  void SetFallbackWind(const xiiVec3& vWind);

private:
  xiiVec3 m_vFallbackWind;
};
