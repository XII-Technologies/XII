#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>

class XII_CORE_DLL xiiActorApiService : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiActorApiService, xiiReflectedClass);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiActorApiService);

public:
  xiiActorApiService();
  ~xiiActorApiService();

protected:
  virtual void Activate() = 0;
  virtual void Update()   = 0;

private: // directly accessed by xiiActorManager
  friend class xiiActorManager;

  enum class State
  {
    New,
    Active,
    QueuedForDestruction
  };

  State m_State = State::New;
};
