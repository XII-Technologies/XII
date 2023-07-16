#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class xiiActor;

class XII_CORE_DLL xiiActorPlugin : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiActorPlugin, xiiReflectedClass);

public:
  xiiActorPlugin();
  ~xiiActorPlugin();

  xiiActor* GetActor() const;

protected:
  friend class xiiActor;
  virtual void Update() {}

private:
  xiiActor* m_pOwningActor = nullptr;
};
