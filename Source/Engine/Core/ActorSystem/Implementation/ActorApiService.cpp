#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorApiService.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActorApiService, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActorApiService::xiiActorApiService()  = default;
xiiActorApiService::~xiiActorApiService() = default;

XII_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorApiService);
