#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorPlugin.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActorPlugin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActorPlugin::xiiActorPlugin()  = default;
xiiActorPlugin::~xiiActorPlugin() = default;

xiiActor* xiiActorPlugin::GetActor() const
{
  return m_pOwningActor;
}


XII_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorPlugin);
