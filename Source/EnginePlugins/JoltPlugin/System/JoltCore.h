#pragma once

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Types/UniquePtr.h>
#include <JoltPlugin/JoltPluginDLL.h>

class xiiJoltMaterial;
struct xiiSurfaceResourceEvent;
class xiiJoltDebugRenderer;
class xiiWorld;

namespace JPH
{
  class JobSystem;
}

class XII_JOLTPLUGIN_DLL xiiJoltCore
{
public:
  static JPH::JobSystem*        GetJoltJobSystem() { return s_pJobSystem.get(); }
  static const xiiJoltMaterial* GetDefaultMaterial() { return s_pDefaultMaterial; }

  static void DebugDraw(xiiWorld* pWorld);

#ifdef JPH_DEBUG_RENDERER
  static std::unique_ptr<xiiJoltDebugRenderer> s_pDebugRenderer;
#endif

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Jolt, JoltPlugin);

  static void Startup();
  static void Shutdown();

  static void SurfaceResourceEventHandler(const xiiSurfaceResourceEvent& e);

  static void* JoltMalloc(size_t inSize);
  static void  JoltFree(void* inBlock);
  static void* JoltAlignedMalloc(size_t inSize, size_t inAlignment);
  static void  JoltAlignedFree(void* inBlock);

  static xiiJoltMaterial*                s_pDefaultMaterial;
  static std::unique_ptr<JPH::JobSystem> s_pJobSystem;

  static xiiUniquePtr<xiiProxyAllocator> s_pAllocator;
};
