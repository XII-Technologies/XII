#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Types/UniquePtr.h>

namespace Diligent
{
  struct IMemoryAllocator;
}

class XII_GRAPHICSD3D12_DLL xiiDiligentCore
{
public:
  static xiiProxyAllocator*          GetProxyAllocator() { return s_pAllocator.Borrow(); }
  static Diligent::IMemoryAllocator* GetDiligentMemoryAllocator() { return s_pDiligentMemoryAllocator.Borrow(); }

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(xiiGAL, GraphicsD3D12);

  static void Startup();
  static void Shutdown();

  static xiiUniquePtr<xiiProxyAllocator>          s_pAllocator;
  static xiiUniquePtr<Diligent::IMemoryAllocator> s_pDiligentMemoryAllocator;
};
