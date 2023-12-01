#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DiligentCore.h>

xiiUniquePtr<xiiProxyAllocator>          xiiDiligentCore::s_pAllocator;
xiiUniquePtr<Diligent::IMemoryAllocator> xiiDiligentCore::s_pDiligentMemoryAllocator;

struct xiiAllocatorDiligent final : public Diligent::IMemoryAllocator
{
public:
  xiiAllocatorDiligent(xiiProxyAllocator* pProxyAllocator) :
    m_pAllocator(pProxyAllocator) {}

  virtual void* Allocate(size_t Size, const Diligent::Char* dbgDescription, const char* dbgFileName, const Diligent::Int32 dbgLineNumber) override { return m_pAllocator->Allocate(Size, 16U); }

  virtual void Free(void* Ptr) override { return m_pAllocator->Deallocate(Ptr); }

  xiiProxyAllocator* m_pAllocator = nullptr;
};

void xiiDiligentCore::Startup()
{
  s_pAllocator               = XII_DEFAULT_NEW(xiiProxyAllocator, "Vulkan-Allocator", xiiFoundation::GetAlignedAllocator());
  s_pDiligentMemoryAllocator = XII_DEFAULT_NEW(xiiAllocatorDiligent, s_pAllocator.Borrow());
}

void xiiDiligentCore::Shutdown()
{
  s_pDiligentMemoryAllocator.Clear();
  s_pAllocator.Clear();
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_DiligentCore);
