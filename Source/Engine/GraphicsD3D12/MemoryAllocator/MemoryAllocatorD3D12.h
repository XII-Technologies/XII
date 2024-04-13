#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Types/UniquePtr.h>

namespace D3D12MA
{
  class Allocator;
}

class XII_GRAPHICSD3D12_DLL xiiMemoryAllocatorD3D12
{
public:
  xiiProxyAllocator*  GetProxyAllocator() { return m_pAllocator.Borrow(); }
  D3D12MA::Allocator* GetD3D12Allocator() { return m_pD3D12MAAllocator; }

private:
  friend class xiiMemoryAllocator;
  friend class xiiGALDeviceD3D12;

  xiiMemoryAllocatorD3D12(IDXGIAdapter1* pDXGIAdapter, ID3D12Device* pDeviceD3D12);
  ~xiiMemoryAllocatorD3D12();

  xiiUniquePtr<xiiProxyAllocator> m_pAllocator;
  D3D12MA::Allocator*             m_pD3D12MAAllocator = nullptr;
};
