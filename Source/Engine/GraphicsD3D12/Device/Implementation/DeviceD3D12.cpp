/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Device/SwapChainD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsD3D12/Pools/CommandListPoolD3D12.h>
#include <GraphicsD3D12/Pools/DescriptorSetPoolD3D12.h>
#include <GraphicsD3D12/Pools/FencePoolD3D12.h>
#include <GraphicsD3D12/Pools/QueryPoolD3D12.h>
#include <GraphicsD3D12/Resources/BottomLevelASD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>
#include <GraphicsD3D12/Resources/FramebufferD3D12.h>
#include <GraphicsD3D12/Resources/QueryD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Resources/SamplerD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TopLevelASD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>
#include <GraphicsD3D12/States/BlendStateD3D12.h>
#include <GraphicsD3D12/States/ComputePipelineStateD3D12.h>
#include <GraphicsD3D12/States/DepthStencilStateD3D12.h>
#include <GraphicsD3D12/States/GraphicsPipelineStateD3D12.h>
#include <GraphicsD3D12/States/PipelineResourceSignatureD3D12.h>
#include <GraphicsD3D12/States/RasterizerStateD3D12.h>
#include <GraphicsD3D12/States/RayTracingPipelineStateD3D12.h>
#include <GraphicsD3D12/States/TilePipelineStateD3D12.h>

#include <dxgi1_4.h>
#include <dxgidebug.h>

namespace
{
  XII_ALWAYS_INLINE const char* GetD3D12FeatureLevelName(D3D_FEATURE_LEVEL featureLevel)
  {
    switch (featureLevel)
    {
      case D3D_FEATURE_LEVEL_12_2:
        return "12.2";
      case D3D_FEATURE_LEVEL_12_1:
        return "12.1";
      case D3D_FEATURE_LEVEL_12_0:
        return "12.0";
      case D3D_FEATURE_LEVEL_11_1:
        return "11.1";
      case D3D_FEATURE_LEVEL_11_0:
        return "11.0";
      default:
        return "Unknown";
    }
  }
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDeviceD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiInternal::NewInstance<xiiGALDevice> CreateD3D12Device(xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  return XII_NEW(pAllocator, xiiGALDeviceD3D12, pAllocator, description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsD3D12, DeviceFactory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    const xiiGALDeviceImplementationDescription implementation = {.m_APIType = xiiGALGraphicsDeviceType::Direct3D12, .m_sShaderModel = "D3D_SM60", .m_sShaderCompiler = "xiiShaderCompilerDXIL" };

    xiiGALDeviceFactory::RegisterImplementation("D3D12", &CreateD3D12Device, implementation);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGALDeviceFactory::UnregisterImplementation("D3D12");
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

///////////////////////////////////////////////////////////////////////////

class xiiGALDeviceD3D12::DeferredDeletionQueue
{
public:
  explicit DeferredDeletionQueue(xiiGALDeviceD3D12* pDeviceD3D12) :
    m_pDeviceD3D12(pDeviceD3D12)
  {
  }

  ~DeferredDeletionQueue()
  {
    ReleaseResources(true);
  }

  void EnqueueObject(IUnknown* pObject)
  {
    XII_ASSERT_DEV(pObject != nullptr, "D3D12 object must be valid.");

    XII_LOCK(m_DeletionQueueMutex);

    DeletionEntry& entry = m_DeletionQueue.ExpandAndGetRef();
    entry.m_Type         = DeletionEntry::Type::Object;
    entry.m_pObject      = pObject;
    entry.m_FenceValues  = CaptureSubmittedFenceValues();
  }

  void EnqueueResource(ID3D12Resource* pResource, xiiD3D12Allocation allocation, bool bIsBuffer)
  {
    XII_ASSERT_DEV(pResource != nullptr, "D3D12 resource must be valid.");

    XII_LOCK(m_DeletionQueueMutex);

    DeletionEntry& entry = m_DeletionQueue.ExpandAndGetRef();
    entry.m_Type         = bIsBuffer ? DeletionEntry::Type::Buffer : DeletionEntry::Type::Image;
    entry.m_pObject      = pResource;
    entry.m_Allocation   = allocation;
    entry.m_FenceValues  = CaptureSubmittedFenceValues();
  }

  void ReleaseResources(bool bForceReleaseAll = false)
  {
    while (true)
    {
      FenceValues completedFenceValues = {};
      if (!bForceReleaseAll)
      {
        completedFenceValues = CaptureCompletedFenceValues();
      }

      DeletionEntry entryToDestroy;
      bool          bHasEntryToDestroy = false;

      {
        XII_LOCK(m_DeletionQueueMutex);

        if (m_DeletionQueue.IsEmpty())
          break;

        if (!bForceReleaseAll && !IsReadyForDeletion(m_DeletionQueue.PeekFront(), completedFenceValues))
          break;

        entryToDestroy = m_DeletionQueue.PeekFront();
        m_DeletionQueue.PopFront();
        bHasEntryToDestroy = true;
      }

      if (bHasEntryToDestroy)
      {
        DestroyEntry(entryToDestroy);
      }
    }
  }

private:
  struct FenceValues
  {
    xiiUInt64 m_uiGraphics = 0ULL;
    xiiUInt64 m_uiCompute  = 0ULL;
    xiiUInt64 m_uiTransfer = 0ULL;
  };

  struct DeletionEntry
  {
    enum class Type : xiiUInt8
    {
      Object,
      Buffer,
      Image
    };

    Type               m_Type        = Type::Object;
    IUnknown*          m_pObject     = nullptr;
    xiiD3D12Allocation m_Allocation  = nullptr;
    FenceValues        m_FenceValues = {};
  };

  [[nodiscard]] static xiiUInt64 GetRequiredFenceValue(const xiiGALCommandQueueD3D12* pCommandQueue)
  {
    if (pCommandQueue == nullptr)
      return 0ULL;

    return pCommandQueue->GetNextFenceValue();
  }

  [[nodiscard]] static xiiUInt64 GetCompletedFenceValue(xiiGALCommandQueueD3D12* pCommandQueue)
  {
    return pCommandQueue != nullptr ? pCommandQueue->GetCompletedFenceValue() : 0ULL;
  }

  [[nodiscard]] FenceValues CaptureSubmittedFenceValues() const
  {
    FenceValues fenceValues = {};

    fenceValues.m_uiGraphics = GetRequiredFenceValue(m_pDeviceD3D12->m_pGraphicsCommandQueue.Borrow());
    fenceValues.m_uiCompute  = GetRequiredFenceValue(m_pDeviceD3D12->m_pComputeCommandQueue.Borrow());
    fenceValues.m_uiTransfer = GetRequiredFenceValue(m_pDeviceD3D12->m_pTransferCommandQueue.Borrow());

    return fenceValues;
  }

  [[nodiscard]] FenceValues CaptureCompletedFenceValues() const
  {
    FenceValues fenceValues = {};

    fenceValues.m_uiGraphics = GetCompletedFenceValue(m_pDeviceD3D12->m_pGraphicsCommandQueue.Borrow());
    fenceValues.m_uiCompute  = GetCompletedFenceValue(m_pDeviceD3D12->m_pComputeCommandQueue.Borrow());
    fenceValues.m_uiTransfer = GetCompletedFenceValue(m_pDeviceD3D12->m_pTransferCommandQueue.Borrow());

    return fenceValues;
  }

  [[nodiscard]] static bool IsReadyForDeletion(const DeletionEntry& entry, const FenceValues& completedFenceValues)
  {
    return completedFenceValues.m_uiGraphics >= entry.m_FenceValues.m_uiGraphics && completedFenceValues.m_uiCompute >= entry.m_FenceValues.m_uiCompute && completedFenceValues.m_uiTransfer >= entry.m_FenceValues.m_uiTransfer;
  }

  void DestroyEntry(DeletionEntry& entry)
  {
    if (entry.m_pObject == nullptr)
      return;

    xiiD3D12MemoryAllocator* pAllocatorD3D12 = m_pDeviceD3D12->GetD3D12Allocator();

    switch (entry.m_Type)
    {
      case DeletionEntry::Type::Object:
      {
        XII_GAL_D3D12_RELEASE(entry.m_pObject);
      }
      break;
      case DeletionEntry::Type::Buffer:
      case DeletionEntry::Type::Image:
      {
        ID3D12Resource*    pResource  = static_cast<ID3D12Resource*>(entry.m_pObject);
        xiiD3D12Allocation allocation = entry.m_Allocation;

        if (allocation != nullptr && pAllocatorD3D12 != nullptr)
        {
          if (entry.m_Type == DeletionEntry::Type::Buffer)
          {
            pAllocatorD3D12->DestroyBuffer(pResource, allocation);
          }
          else
          {
            pAllocatorD3D12->DestroyImage(pResource, allocation);
          }
        }
        else
        {
          XII_GAL_D3D12_RELEASE(pResource);
          XII_GAL_D3D12_RELEASE(allocation);
        }

        entry.m_pObject    = pResource;
        entry.m_Allocation = allocation;
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  xiiGALDeviceD3D12*      m_pDeviceD3D12 = nullptr;
  xiiDeque<DeletionEntry> m_DeletionQueue;
  mutable xiiMutex        m_DeletionQueueMutex;
};

///////////////////////////////////////////////////////////////////////////

xiiGALDeviceD3D12::xiiGALDeviceD3D12(xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description) :
  xiiGALDevice(pAllocator, description)
{
}

xiiGALDeviceD3D12::~xiiGALDeviceD3D12()
{
  WaitIdlePlatform();

  m_pTransferCommandListPool.Clear();
  m_pTransferCommandQueueQueryPool.Clear();
  m_pTransferCommandQueue.Clear();

  m_pComputeCommandListPool.Clear();
  m_pComputeCommandQueueQueryPool.Clear();
  m_pComputeCommandQueue.Clear();

  m_pGraphicsCommandListPool.Clear();
  m_pGraphicsCommandQueueQueryPool.Clear();
  m_pGraphicsCommandQueue.Clear();

  m_pResourceDescriptorPool.Clear();
  m_pFencePool.Clear();
  m_pDeferredDeletionQueue.Clear();

  XII_GAL_D3D12_RELEASE(m_pD3D12Device);
  XII_GAL_D3D12_RELEASE(m_pDXGIAdapter);
  XII_GAL_D3D12_RELEASE(m_pDXGIFactory);

  ReportLiveGPUObjects();
}

void xiiGALDeviceD3D12::SafeReleaseDeviceObject(IUnknown*& pObject)
{
  if (pObject == nullptr)
    return;

  if (m_pDeferredDeletionQueue != nullptr)
  {
    m_pDeferredDeletionQueue->EnqueueObject(pObject);
    pObject = nullptr;
    return;
  }

  XII_GAL_D3D12_RELEASE(pObject);
}

void xiiGALDeviceD3D12::SafeReleaseBuffer(ID3D12Resource*& pResource, xiiD3D12Allocation& allocation)
{
  if (pResource == nullptr)
    return;

  if (allocation == nullptr)
  {
    IUnknown* pObject = pResource;
    SafeReleaseDeviceObject(pObject);
    pResource = nullptr;
    return;
  }

  if (m_pDeferredDeletionQueue != nullptr)
  {
    m_pDeferredDeletionQueue->EnqueueResource(pResource, allocation, true);
    pResource  = nullptr;
    allocation = nullptr;
    return;
  }

  if (m_pAllocatorD3D12 != nullptr)
  {
    m_pAllocatorD3D12->DestroyBuffer(pResource, allocation);
  }
  else
  {
    XII_GAL_D3D12_RELEASE(pResource);
    XII_GAL_D3D12_RELEASE(allocation);
  }
}

void xiiGALDeviceD3D12::SafeReleaseTexture(ID3D12Resource*& pResource, xiiD3D12Allocation& allocation, bool bIsStagingTexture)
{
  if (pResource == nullptr)
    return;

  if (allocation == nullptr)
  {
    IUnknown* pObject = pResource;
    SafeReleaseDeviceObject(pObject);
    pResource = nullptr;
    return;
  }

  if (m_pDeferredDeletionQueue != nullptr)
  {
    m_pDeferredDeletionQueue->EnqueueResource(pResource, allocation, bIsStagingTexture);
    pResource  = nullptr;
    allocation = nullptr;
    return;
  }

  if (m_pAllocatorD3D12 != nullptr)
  {
    if (bIsStagingTexture)
    {
      m_pAllocatorD3D12->DestroyBuffer(pResource, allocation);
    }
    else
    {
      m_pAllocatorD3D12->DestroyImage(pResource, allocation);
    }
  }
  else
  {
    XII_GAL_D3D12_RELEASE(pResource);
    XII_GAL_D3D12_RELEASE(allocation);
  }
}

xiiResult xiiGALDeviceD3D12::InitializePlatform()
{
  XII_LOG_BLOCK("xiiGALDeviceD3D12::InitializePlatform");

  // Enable the D3D12 debug layer.
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (m_Description.m_ValidationLevel != xiiGALDeviceValidationLevel::Disabled)
  {
    ID3D12Debug* pDebugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(pDebugController), reinterpret_cast<void**>(static_cast<ID3D12Debug**>(&pDebugController)))))
    {
      pDebugController->EnableDebugLayer();

      if (m_Description.m_ValidationLevel == xiiGALDeviceValidationLevel::All)
      {
        ID3D12Debug1* pDebugController1 = nullptr;
        if (SUCCEEDED(pDebugController->QueryInterface(__uuidof(pDebugController1), reinterpret_cast<void**>(static_cast<ID3D12Debug1**>(&pDebugController1)))))
        {
          pDebugController1->SetEnableGPUBasedValidation(TRUE);
          pDebugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
        }
        XII_GAL_D3D12_RELEASE(pDebugController1);
      }
    }
    XII_GAL_D3D12_RELEASE(pDebugController);
  }
#endif

  // Create the DXGI factory to select a compatible adapter to create the D3D12 device with.
  {
    HRESULT hResult = CreateDXGIFactory1(__uuidof(m_pDXGIFactory), reinterpret_cast<void**>(static_cast<IDXGIFactory4**>(&m_pDXGIFactory)));

    if (FAILED(hResult))
    {
      xiiLog::Error("Failed to create DXGI factory. Error: '{}'.", xiiHRESULTtoString(hResult));
    }
  }

  {
    // Direct3D12 does not allow feature levels below 11.0.
    constexpr D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

    // Probe list (highest -> lowest).
    constexpr D3D_FEATURE_LEVEL probeFeatureLevels[] =
      {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0};

    for (const D3D_FEATURE_LEVEL level : probeFeatureLevels)
    {
      if (level < minFeatureLevel)
        continue;

      IDXGIAdapter1* pDXGIAdapter = nullptr;

      if (m_Description.m_uiAdapterID != xiiInvalidIndex)
      {
        if (SelectAdapterByIndex(m_Description.m_uiAdapterID, level, &pDXGIAdapter, false, false).Failed())
          continue;
      }
      else
      {
        xiiTemporaryArray<IDXGIAdapter1*> compatibleAdapters;
        XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE_ARRAY(compatibleAdapters));

        if (GetCompatibleAdapters(level, compatibleAdapters, true).Succeeded() && !compatibleAdapters.IsEmpty())
        {
          pDXGIAdapter = SelectBestAdapter(compatibleAdapters);

          // Remove from the array so we keep ownership of the adapter.
          XII_VERIFY(compatibleAdapters.RemoveAndSwap(pDXGIAdapter), "Unexpectedly failed to remove the selected adapter from the compatible adapters array.");
        }
      }

      ID3D12Device1* pD3D12Device = nullptr;
      if (SUCCEEDED(D3D12CreateDevice(pDXGIAdapter, level, __uuidof(pD3D12Device), reinterpret_cast<void**>(static_cast<ID3D12Device1**>(&pD3D12Device)))))
      {
        m_pDXGIAdapter = pDXGIAdapter;
        m_pD3D12Device = pD3D12Device;

        xiiLog::Info("Created D3D12 device with feature level {0}.", GetD3D12FeatureLevelName(level));

        break;
      }

      XII_GAL_D3D12_RELEASE(pDXGIAdapter);
    }

    if (m_pD3D12Device == nullptr)
    {
      xiiLog::Error("Failed to create a D3D12 device with the required feature level. Ensure that a compatible GPU is installed and the latest drivers are updated.");

      return XII_FAILURE;
    }
  }

  // Set validation and debugging options.
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (m_Description.m_ValidationLevel != xiiGALDeviceValidationLevel::Disabled)
  {
    ID3D12InfoQueue* pD3D12InfoQueue = nullptr;
    XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pD3D12InfoQueue));

    if (SUCCEEDED(m_pD3D12Device->QueryInterface(__uuidof(ID3D12InfoQueue), reinterpret_cast<void**>(static_cast<ID3D12InfoQueue**>(&pD3D12InfoQueue)))))
    {
      // Suppress messages based on their severity level.
      D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

      // Suppress individual messages by their ID.
      D3D12_MESSAGE_ID denyIDs[] =
        {
          // D3D12 WARNING: ID3D12CommandList::ClearRenderTargetView: The clear values do not match those passed to resource creation.
          // The clear operation is typically slower as a result; but will still clear to the desired value.
          // [ EXECUTION WARNING #820: CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE]
          D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,

          // D3D12 WARNING: ID3D12CommandList::ClearDepthStencilView: The clear values do not match those passed to resource creation.
          // The clear operation is typically slower as a result; but will still clear to the desired value.
          // [ EXECUTION WARNING #821: CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE]
          D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE //
        };

      D3D12_INFO_QUEUE_FILTER queueFilter = {};
      queueFilter.DenyList.NumSeverities  = XII_ARRAY_SIZE(severities);
      queueFilter.DenyList.pSeverityList  = severities;
      queueFilter.DenyList.NumIDs         = XII_ARRAY_SIZE(denyIDs);
      queueFilter.DenyList.pIDList        = denyIDs;

      XII_VERIFY(SUCCEEDED(pD3D12InfoQueue->PushStorageFilter(&queueFilter)), "Failed to push storage filter.");

#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      XII_VERIFY(SUCCEEDED(pD3D12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE)), "Failed to set break on corruption.");
      XII_VERIFY(SUCCEEDED(pD3D12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE)), "Failed to set break on error.");
      XII_VERIFY(SUCCEEDED(pD3D12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE)), "Failed to set break on warning.");
#  endif
    }

    // We can prevent the GPU from overclocking or underclocking to get consistent timings.
    m_pD3D12Device->SetStablePowerState(TRUE);
  }
#endif

  return XII_SUCCESS;
}

xiiResult xiiGALDeviceD3D12::PostInitializePlatform()
{
  // Initialize Direct3D 12 Memory Allocator (D3D12MA).
  {
    m_pAllocatorD3D12 = XII_NEW(&m_Allocator, xiiD3D12MemoryAllocator);

    XII_SUCCEED_OR_RETURN(m_pAllocatorD3D12->Initialize(this));
  }

  // Create pools.
  {
    m_pFencePool              = XII_NEW(&m_Allocator, xiiGALFencePoolD3D12, this, 16U);
    m_pResourceDescriptorPool = XII_NEW(&m_Allocator, xiiGALDescriptorSetPoolD3D12, this, 2048U, false);
  }

  // Create command queues.
  {
    {
      D3D12_COMMAND_QUEUE_DESC queueDescriptionD3D12 = {};
      queueDescriptionD3D12.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
      queueDescriptionD3D12.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      queueDescriptionD3D12.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
      queueDescriptionD3D12.NodeMask                 = 0U;

      HRESULT hResult = m_pD3D12Device->CreateCommandQueue(&queueDescriptionD3D12, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(static_cast<ID3D12CommandQueue**>(&m_GraphicsQueueInformation.m_pCommandQueue)));
      if (FAILED(hResult))
      {
        xiiLog::Error("Failed to create D3D12 graphics command queue: {}.", xiiHRESULTtoString(hResult));

        return XII_FAILURE;
      }

      xiiGALCommandQueueCreationDescription queueDescription = {.m_QueueFlags = xiiGALCommandQueueFlags::Graphics};
      m_pGraphicsCommandQueue                                = XII_NEW(&m_Allocator, xiiGALCommandQueueD3D12, this, queueDescription, m_GraphicsQueueInformation);
      m_pGraphicsCommandListPool                             = XII_NEW(&m_Allocator, xiiGALCommandListPoolD3D12, this, m_pGraphicsCommandQueue.Borrow(), D3D12_COMMAND_LIST_TYPE_DIRECT);
      m_pGraphicsCommandQueueQueryPool                       = XII_NEW(&m_Allocator, xiiGALQueryPoolD3D12, this, m_pGraphicsCommandQueue.Borrow(), m_GraphicsQueueInformation);

      m_pGraphicsCommandQueue->SetDebugName("Command Queue (Default Graphics)");

      xiiLog::Dev("Created {}", m_pGraphicsCommandQueue->GetDebugName());
    }

    {
      D3D12_COMMAND_QUEUE_DESC queueDescriptionD3D12 = {};
      queueDescriptionD3D12.Type                     = D3D12_COMMAND_LIST_TYPE_COMPUTE;
      queueDescriptionD3D12.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      queueDescriptionD3D12.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
      queueDescriptionD3D12.NodeMask                 = 0U;

      if (SUCCEEDED(m_pD3D12Device->CreateCommandQueue(&queueDescriptionD3D12, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(static_cast<ID3D12CommandQueue**>(&m_ComputeQueueInformation.m_pCommandQueue)))))
      {
        xiiGALCommandQueueCreationDescription queueDescription = {.m_QueueFlags = xiiGALCommandQueueFlags::Compute};
        m_pComputeCommandQueue                                 = XII_NEW(&m_Allocator, xiiGALCommandQueueD3D12, this, queueDescription, m_ComputeQueueInformation);
        m_pComputeCommandListPool                              = XII_NEW(&m_Allocator, xiiGALCommandListPoolD3D12, this, m_pComputeCommandQueue.Borrow(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
        m_pComputeCommandQueueQueryPool                        = XII_NEW(&m_Allocator, xiiGALQueryPoolD3D12, this, m_pComputeCommandQueue.Borrow(), m_ComputeQueueInformation);

        m_pComputeCommandQueue->SetDebugName("Command Queue (Default Compute)");

        xiiLog::Dev("Created {}", m_pComputeCommandQueue->GetDebugName());
      }
    }

    {
      D3D12_COMMAND_QUEUE_DESC queueDescriptionD3D12 = {};
      queueDescriptionD3D12.Type                     = D3D12_COMMAND_LIST_TYPE_COPY;
      queueDescriptionD3D12.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      queueDescriptionD3D12.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
      queueDescriptionD3D12.NodeMask                 = 0U;

      if (SUCCEEDED(m_pD3D12Device->CreateCommandQueue(&queueDescriptionD3D12, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(static_cast<ID3D12CommandQueue**>(&m_TransferQueueInformation.m_pCommandQueue)))))
      {
        xiiGALCommandQueueCreationDescription queueDescription = {.m_QueueFlags = xiiGALCommandQueueFlags::Transfer};
        m_pTransferCommandQueue                                = XII_NEW(&m_Allocator, xiiGALCommandQueueD3D12, this, queueDescription, m_TransferQueueInformation);
        m_pTransferCommandListPool                             = XII_NEW(&m_Allocator, xiiGALCommandListPoolD3D12, this, m_pTransferCommandQueue.Borrow(), D3D12_COMMAND_LIST_TYPE_COPY);
        m_pTransferCommandQueueQueryPool                       = XII_NEW(&m_Allocator, xiiGALQueryPoolD3D12, this, m_pTransferCommandQueue.Borrow(), m_TransferQueueInformation);

        m_pTransferCommandQueue->SetDebugName("Command Queue (Default Transfer)");

        xiiLog::Dev("Created {}", m_pTransferCommandQueue->GetDebugName());
      }
    }
  }

  m_pDeferredDeletionQueue = XII_NEW(&m_Allocator, DeferredDeletionQueue, this);

  xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  return XII_SUCCESS;
}

void xiiGALDeviceD3D12::ReportLiveGPUObjects()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  IDXGIDebug1* pDXGIDebug;
  XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pDXGIDebug));
  if (SUCCEEDED(DXGIGetDebugInterface1(0U, __uuidof(IDXGIDebug1), reinterpret_cast<void**>(static_cast<IDXGIDebug1**>(&pDXGIDebug)))))
  {
    OutputDebugStringW(L" +++++ Live D3D12 Objects: +++++\n");

    pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);

    OutputDebugStringW(L" ----- Live D3D12 Objects: -----\n");
  }

  ID3D12DebugDevice* pD3D12DebugDevice;
  XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pD3D12DebugDevice));
  if (SUCCEEDED(m_pD3D12Device->QueryInterface(__uuidof(ID3D12DebugDevice), reinterpret_cast<void**>(static_cast<ID3D12DebugDevice**>(&pD3D12DebugDevice)))))
  {
    OutputDebugStringW(L" +++++ Live D3D12 Objects (DETAIL): +++++\n");

    pD3D12DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);

    OutputDebugStringW(L" ----- Live D3D12 Objects: -----\n");
  }
#endif
}

void xiiGALDeviceD3D12::BeginFramePlatform()
{
}

void xiiGALDeviceD3D12::EndFramePlatform()
{
  if (m_pTransferCommandListPool != nullptr)
  {
    m_pTransferCommandListPool->ReclaimCompleted();
  }
  if (m_pComputeCommandListPool != nullptr)
  {
    m_pComputeCommandListPool->ReclaimCompleted();
  }
  if (m_pGraphicsCommandListPool != nullptr)
  {
    m_pGraphicsCommandListPool->ReclaimCompleted();
  }

  if (m_pTransferCommandQueueQueryPool != nullptr)
  {
    m_pTransferCommandQueueQueryPool->ResetStaleQueries();
  }
  if (m_pComputeCommandQueueQueryPool != nullptr)
  {
    m_pComputeCommandQueueQueryPool->ResetStaleQueries();
  }
  if (m_pGraphicsCommandQueueQueryPool != nullptr)
  {
    m_pGraphicsCommandQueueQueryPool->ResetStaleQueries();
  }

  if (m_pDeferredDeletionQueue != nullptr)
  {
    m_pDeferredDeletionQueue->ReleaseResources();
  }
}

xiiGALCommandQueue* xiiGALDeviceD3D12::GetCommandQueue(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Graphics))
    return m_pGraphicsCommandQueue.Borrow();

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Compute) && m_pComputeCommandQueue != nullptr)
    return m_pComputeCommandQueue.Borrow();

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Transfer) && m_pTransferCommandQueue != nullptr)
    return m_pTransferCommandQueue.Borrow();

  return m_pGraphicsCommandQueue.Borrow();
}

xiiInternal::NewInstance<xiiGALSwapChain> xiiGALDeviceD3D12::CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALSwapChainD3D12> pSwapChainD3D12 = XII_NEW(&m_Allocator, xiiGALSwapChainD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pSwapChainD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pSwapChainD3D12.m_pAllocator, pSwapChainD3D12.m_pInstance);
    return nullptr;
  }

  return pSwapChainD3D12;
}

xiiInternal::NewInstance<xiiGALCommandList> xiiGALDeviceD3D12::CreateCommandListPlatform(const xiiGALCommandListCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALCommandListD3D12> pCommandListD3D12 = XII_NEW(&m_Allocator, xiiGALCommandListD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pCommandListD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pCommandListD3D12.m_pAllocator, pCommandListD3D12.m_pInstance);
    return nullptr;
  }

  return pCommandListD3D12;
}

xiiInternal::NewInstance<xiiGALBlendState> xiiGALDeviceD3D12::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALBlendStateD3D12> pBlendStateD3D12 = XII_NEW(&m_Allocator, xiiGALBlendStateD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pBlendStateD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pBlendStateD3D12.m_pAllocator, pBlendStateD3D12.m_pInstance);
    return nullptr;
  }

  return pBlendStateD3D12;
}

xiiInternal::NewInstance<xiiGALDepthStencilState> xiiGALDeviceD3D12::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALDepthStencilStateD3D12> pDepthStencilStateD3D12 = XII_NEW(&m_Allocator, xiiGALDepthStencilStateD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pDepthStencilStateD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pDepthStencilStateD3D12.m_pAllocator, pDepthStencilStateD3D12.m_pInstance);
    return nullptr;
  }

  return pDepthStencilStateD3D12;
}

xiiInternal::NewInstance<xiiGALRasterizerState> xiiGALDeviceD3D12::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALRasterizerStateD3D12> pRasterizerStateD3D12 = XII_NEW(&m_Allocator, xiiGALRasterizerStateD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pRasterizerStateD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pRasterizerStateD3D12.m_pAllocator, pRasterizerStateD3D12.m_pInstance);
    return nullptr;
  }

  return pRasterizerStateD3D12;
}

xiiInternal::NewInstance<xiiGALShader> xiiGALDeviceD3D12::CreateShaderPlatform(const xiiGALShaderCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALShaderD3D12> pShaderD3D12 = XII_NEW(&m_Allocator, xiiGALShaderD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pShaderD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pShaderD3D12.m_pAllocator, pShaderD3D12.m_pInstance);
    return nullptr;
  }

  return pShaderD3D12;
}

xiiInternal::NewInstance<xiiGALBuffer> xiiGALDeviceD3D12::CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  xiiInternal::NewInstance<xiiGALBufferD3D12> pBufferD3D12 = XII_NEW(&m_Allocator, xiiGALBufferD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pBufferD3D12->InitPlatform(pInitialData, externalMemoryKind).Failed())
  {
    XII_DELETE(pBufferD3D12.m_pAllocator, pBufferD3D12.m_pInstance);
    return nullptr;
  }

  return pBufferD3D12;
}

xiiInternal::NewInstance<xiiGALTexture> xiiGALDeviceD3D12::CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  xiiInternal::NewInstance<xiiGALTextureD3D12> pTextureD3D12 = XII_NEW(&m_Allocator, xiiGALTextureD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pTextureD3D12->InitPlatform(pInitialData, externalMemoryKind).Failed())
  {
    XII_DELETE(pTextureD3D12.m_pAllocator, pTextureD3D12.m_pInstance);
    return nullptr;
  }

  return pTextureD3D12;
}

xiiInternal::NewInstance<xiiGALSampler> xiiGALDeviceD3D12::CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALSamplerD3D12> pSamplerD3D12 = XII_NEW(&m_Allocator, xiiGALSamplerD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pSamplerD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pSamplerD3D12.m_pAllocator, pSamplerD3D12.m_pInstance);
    return nullptr;
  }

  return pSamplerD3D12;
}

xiiInternal::NewInstance<xiiGALQuery> xiiGALDeviceD3D12::CreateQueryPlatform(const xiiGALQueryCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALQueryD3D12> pQueryD3D12 = XII_NEW(&m_Allocator, xiiGALQueryD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pQueryD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pQueryD3D12.m_pAllocator, pQueryD3D12.m_pInstance);
    return nullptr;
  }

  return pQueryD3D12;
}

xiiInternal::NewInstance<xiiGALFence> xiiGALDeviceD3D12::CreateFencePlatform(const xiiGALFenceCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALFenceD3D12> pFenceD3D12 = XII_NEW(&m_Allocator, xiiGALFenceD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pFenceD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pFenceD3D12.m_pAllocator, pFenceD3D12.m_pInstance);
    return nullptr;
  }

  return pFenceD3D12;
}

xiiInternal::NewInstance<xiiGALRenderPass> xiiGALDeviceD3D12::CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALRenderPassD3D12> pRenderPassD3D12 = XII_NEW(&m_Allocator, xiiGALRenderPassD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pRenderPassD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pRenderPassD3D12.m_pAllocator, pRenderPassD3D12.m_pInstance);
    return nullptr;
  }

  return pRenderPassD3D12;
}

xiiInternal::NewInstance<xiiGALFramebuffer> xiiGALDeviceD3D12::CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALFramebufferD3D12> pFramebufferD3D12 = XII_NEW(&m_Allocator, xiiGALFramebufferD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pFramebufferD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pFramebufferD3D12.m_pAllocator, pFramebufferD3D12.m_pInstance);
    return nullptr;
  }

  return pFramebufferD3D12;
}

xiiInternal::NewInstance<xiiGALBottomLevelAS> xiiGALDeviceD3D12::CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALBottomLevelASD3D12> pBottomLevelASD3D12 = XII_NEW(&m_Allocator, xiiGALBottomLevelASD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pBottomLevelASD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pBottomLevelASD3D12.m_pAllocator, pBottomLevelASD3D12.m_pInstance);
    return nullptr;
  }

  return pBottomLevelASD3D12;
}

xiiInternal::NewInstance<xiiGALTopLevelAS> xiiGALDeviceD3D12::CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALTopLevelASD3D12> pTopLevelASD3D12 = XII_NEW(&m_Allocator, xiiGALTopLevelASD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pTopLevelASD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pTopLevelASD3D12.m_pAllocator, pTopLevelASD3D12.m_pInstance);
    return nullptr;
  }

  return pTopLevelASD3D12;
}

xiiInternal::NewInstance<xiiGALPipelineResourceSignature> xiiGALDeviceD3D12::CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALPipelineResourceSignatureD3D12> pPipelineResourceSignatureD3D12 = XII_NEW(&m_Allocator, xiiGALPipelineResourceSignatureD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pPipelineResourceSignatureD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pPipelineResourceSignatureD3D12.m_pAllocator, pPipelineResourceSignatureD3D12.m_pInstance);
    return nullptr;
  }

  return pPipelineResourceSignatureD3D12;
}

xiiInternal::NewInstance<xiiGALGraphicsPipelineState> xiiGALDeviceD3D12::CreateGraphicsPipelineStatePlatform(const xiiGALGraphicsPipelineStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALGraphicsPipelineStateD3D12> pGraphicsPipelineStateD3D12 = XII_NEW(&m_Allocator, xiiGALGraphicsPipelineStateD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pGraphicsPipelineStateD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pGraphicsPipelineStateD3D12.m_pAllocator, pGraphicsPipelineStateD3D12.m_pInstance);
    return nullptr;
  }

  return pGraphicsPipelineStateD3D12;
}

xiiInternal::NewInstance<xiiGALComputePipelineState> xiiGALDeviceD3D12::CreateComputePipelineStatePlatform(const xiiGALComputePipelineStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALComputePipelineStateD3D12> pComputePipelineStateD3D12 = XII_NEW(&m_Allocator, xiiGALComputePipelineStateD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pComputePipelineStateD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pComputePipelineStateD3D12.m_pAllocator, pComputePipelineStateD3D12.m_pInstance);
    return nullptr;
  }

  return pComputePipelineStateD3D12;
}

xiiInternal::NewInstance<xiiGALRayTracingPipelineState> xiiGALDeviceD3D12::CreateRayTracingPipelineStatePlatform(const xiiGALRayTracingPipelineStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALRayTracingPipelineStateD3D12> pRayTracingPipelineStateD3D12 = XII_NEW(&m_Allocator, xiiGALRayTracingPipelineStateD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pRayTracingPipelineStateD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pRayTracingPipelineStateD3D12.m_pAllocator, pRayTracingPipelineStateD3D12.m_pInstance);
    return nullptr;
  }

  return pRayTracingPipelineStateD3D12;
}

xiiInternal::NewInstance<xiiGALTilePipelineState> xiiGALDeviceD3D12::CreateTilePipelineStatePlatform(const xiiGALTilePipelineStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALTilePipelineStateD3D12> pTilePipelineStateD3D12 = XII_NEW(&m_Allocator, xiiGALTilePipelineStateD3D12, xiiSharedPtr<xiiGALDeviceD3D12>(this, m_Allocator.GetParent()), description);

  if (pTilePipelineStateD3D12->InitPlatform().Failed())
  {
    XII_DELETE(pTilePipelineStateD3D12.m_pAllocator, pTilePipelineStateD3D12.m_pInstance);
    return nullptr;
  }

  return pTilePipelineStateD3D12;
}

void xiiGALDeviceD3D12::WaitIdlePlatform()
{
  if (m_pGraphicsCommandQueue != nullptr)
    m_pGraphicsCommandQueue->WaitForIdle();
  if (m_pComputeCommandQueue != nullptr)
    m_pComputeCommandQueue->WaitForIdle();
  if (m_pTransferCommandQueue != nullptr)
    m_pTransferCommandQueue->WaitForIdle();

  if (m_pTransferCommandListPool != nullptr)
  {
    m_pTransferCommandListPool->ReclaimCompleted();
    m_pTransferCommandListPool->ResetPools();
  }
  if (m_pComputeCommandListPool != nullptr)
  {
    m_pComputeCommandListPool->ReclaimCompleted();
    m_pComputeCommandListPool->ResetPools();
  }
  if (m_pGraphicsCommandListPool != nullptr)
  {
    m_pGraphicsCommandListPool->ReclaimCompleted();
    m_pGraphicsCommandListPool->ResetPools();
  }

  if (m_pDeferredDeletionQueue != nullptr)
  {
    m_pDeferredDeletionQueue->ReleaseResources(true);
  }
}

xiiResult xiiGALDeviceD3D12::FillCapabilitiesPlatform()
{
  m_Description.m_GraphicsDeviceType = xiiGALGraphicsDeviceType::Direct3D12;

  // Set graphics adapter properties.
  {
    DXGI_ADAPTER_DESC1 dxgiAdapterDescription = {};
    m_pDXGIAdapter->GetDesc1(&dxgiAdapterDescription);
    m_AdapterDescription.m_sAdapterName = xiiStringUtf8(dxgiAdapterDescription.Description).GetData();

    if (dxgiAdapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Software;
    else if (dxgiAdapterDescription.DedicatedVideoMemory != 0U)
      m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Discrete;
    else
      m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Integrated;

    {
      D3D12_FEATURE_DATA_ARCHITECTURE dataArchitecture = {};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &dataArchitecture, sizeof(dataArchitecture))))
      {
        if (m_AdapterDescription.m_Type != xiiGALDeviceAdapterType::Software && (dataArchitecture.UMA || dataArchitecture.CacheCoherentUMA))
          m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Integrated;
      }
    }

    m_AdapterDescription.m_Vendor             = xiiGALDeviceUtilities::GetVendorFromID(dxgiAdapterDescription.VendorId);
    m_AdapterDescription.m_uiVendorID         = dxgiAdapterDescription.VendorId;
    m_AdapterDescription.m_uiDeviceID         = dxgiAdapterDescription.DeviceId;
    m_AdapterDescription.m_uiVideoOutputCount = 0U;

    // Enable features.
    m_AdapterDescription.m_Features.m_WireframeFill                 = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_MultithreadedResourceCreation = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_ComputeShaders                = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_GeometryShaders               = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_Tessellation                  = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_OcclusionQueries              = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_BinaryOcclusionQueries        = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_TimestampQueries              = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_PipelineStatisticsQueries     = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_DurationQueries               = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_DepthBiasClamp                = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_DepthClamp                    = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_IndependentBlend              = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_DualSourceBlend               = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_MultiViewport                 = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_TextureCompressionBC          = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_PixelUAVWritesAndAtomics      = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_TextureUAVExtendedFormats     = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_InstanceDataStepRate          = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_TileShaders                   = xiiGALDeviceFeatureState::Disabled;
    m_AdapterDescription.m_Features.m_SubpassFramebufferFetch       = xiiGALDeviceFeatureState::Disabled;
    m_AdapterDescription.m_Features.m_TextureComponentSwizzle       = xiiGALDeviceFeatureState::Disabled;

    // Set memory properties.
    m_AdapterDescription.m_MemoryProperties.m_uiLocalMemory         = dxgiAdapterDescription.DedicatedVideoMemory;
    m_AdapterDescription.m_MemoryProperties.m_uiHostVisibleMemory   = dxgiAdapterDescription.SharedSystemMemory;
    m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory       = 0U;
    m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation = 0U; // Unable to query.

    // Set draw command properties.
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxIndexValue        = 0U;
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxDrawIndirectCount = ~0U;
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags        = xiiGALDrawCommandCapabilityFlags::DrawIndirect | xiiGALDrawCommandCapabilityFlags::DrawIndirectFirstInstance;

    // Set queue information.
    xiiGALCommandQueueFlags::Enum queueIndexType[] = {xiiGALCommandQueueFlags::Graphics, xiiGALCommandQueueFlags::Compute, xiiGALCommandQueueFlags::Transfer};
    m_AdapterDescription.m_CommandQueueProperties.SetCountUninitialized(XII_ARRAY_SIZE(queueIndexType));

    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      xiiGALCommandQueueProperties& queueProperty = m_AdapterDescription.m_CommandQueueProperties[i];
      queueProperty.m_Flags                       = queueIndexType[i];
      queueProperty.m_uiMaxDeviceContexts         = 0xFFU;

      queueProperty.m_TextureCopyGranularity[0] = 1U;
      queueProperty.m_TextureCopyGranularity[1] = 1U;
      queueProperty.m_TextureCopyGranularity[2] = 1U;
    }
  }

  // Enable features and set properties.
  {
    xiiGALDeviceFeatures& deviceFeatures = m_AdapterDescription.m_Features;

    // Direct3D12 supports shader model 5.1 on all feature levels (even on 11.0), so bindless resources are always available.
    // https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels#feature-level-support
    deviceFeatures.m_BindlessResources = xiiGALDeviceFeatureState::Enabled;

    deviceFeatures.m_VertexPipelineUAVWritesAndAtomics = xiiGALDeviceFeatureState::Enabled;
    deviceFeatures.m_NativeFence                       = xiiGALDeviceFeatureState::Optional; // This can be disabled.
    deviceFeatures.m_TextureComponentSwizzle           = xiiGALDeviceFeatureState::Enabled;

    // Check if mesh shader is supported.
    bool bMeshShadersSupported = false;
#ifdef D3D12_H_HAS_MESH_SHADER
    {
      D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {static_cast<D3D_SHADER_MODEL>(0x65)};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
      {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureData = {};
        bMeshShadersSupported                         = SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureData, sizeof(featureData))) && featureData.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
      }
    }
#endif

    if (bMeshShadersSupported)
    {
      deviceFeatures.m_MeshShaders = xiiGALDeviceFeatureState::Enabled;

      m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountX     = 65536U; // From specification: https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html#dispatchmesh-api
      m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountY     = 65536U;
      m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountZ     = 65536U;
      m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupTotalCount = XII_BIT(22U);
    }

    deviceFeatures.m_ShaderResourceRuntimeArray = xiiGALDeviceFeatureState::Enabled;

    {
      D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOptions = {};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureDataOptions, sizeof(featureDataOptions))))
      {
        if (featureDataOptions.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT)
        {
          deviceFeatures.m_ShaderFloat16 = xiiGALDeviceFeatureState::Enabled;
        }

        if (featureDataOptions.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_1)
        {
          deviceFeatures.m_SparseResources = xiiGALDeviceFeatureState::Enabled;

          m_AdapterDescription.m_SparseResourceProperties.m_uiStandardBlockSize = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;

          D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT featureDataGPUVirtualAddress = {};
          if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &featureDataGPUVirtualAddress, sizeof(featureDataGPUVirtualAddress))))
          {
            m_AdapterDescription.m_SparseResourceProperties.m_uiAddressSpaceSize  = XII_BIT(featureDataGPUVirtualAddress.MaxGPUVirtualAddressBitsPerProcess);
            m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize = XII_BIT(featureDataGPUVirtualAddress.MaxGPUVirtualAddressBitsPerResource);
          }
          else
          {
            m_AdapterDescription.m_SparseResourceProperties.m_uiAddressSpaceSize  = XII_BIT(featureDataOptions.MaxGPUVirtualAddressBitsPerResource);
            m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize = XII_BIT(featureDataOptions.MaxGPUVirtualAddressBitsPerResource);
          }

          m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags = xiiGALSparseResourceCapabilityFlags::Buffer | xiiGALSparseResourceCapabilityFlags::BufferStandardBlock | xiiGALSparseResourceCapabilityFlags::Texture2D |
            xiiGALSparseResourceCapabilityFlags::Standard2DTileShape | xiiGALSparseResourceCapabilityFlags::Aliased | xiiGALSparseResourceCapabilityFlags::NonResidentSafe;

          // No 2, 8 or 16 sample multisample antialiasing (MSAA) support. Only 4x is required, except no 128 bpp formats.
          m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture4Samples | xiiGALSparseResourceCapabilityFlags::Standard2DMSTileShape;

          if (featureDataOptions.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_2)
          {
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::ShaderResourceResidency | xiiGALSparseResourceCapabilityFlags::NonResidentStrict;
          }
          if (featureDataOptions.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_3)
          {
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture3D | xiiGALSparseResourceCapabilityFlags::Standard3DTileShape;
          }
#if 0 // We currently do not use NVAPI on Nvidia graphics cards.
          if (pNVAPI)
          {
              m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture2DArrayMipTail;
          }
#endif
          if (featureDataOptions.ResourceHeapTier >= D3D12_RESOURCE_HEAP_TIER_2)
          {
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::MixedResourceTypeSupport;
          }

          // Some features are not correctly working in software renderer.
          if (m_AdapterDescription.m_Type == xiiGALDeviceAdapterType::Software)
          {
            // Reading from null-mapped tile does not return zero.
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.Remove(xiiGALSparseResourceCapabilityFlags::NonResidentStrict);
            // CheckAccessFullyMapped() in shader does not work.
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.Remove(xiiGALSparseResourceCapabilityFlags::ShaderResourceResidency);
            // Mip tails are not supported at all.
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.Remove(xiiGALSparseResourceCapabilityFlags::AlignedMipSize);
          }

          m_AdapterDescription.m_SparseResourceProperties.m_BindFlags = xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::IndexBuffer | xiiGALBindFlags::UniformBuffer |
            xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments | xiiGALBindFlags::RayTracing;

          for (xiiUInt32 i = 0; i < m_AdapterDescription.m_CommandQueueProperties.GetCount(); ++i)
          {
            m_AdapterDescription.m_CommandQueueProperties[i].m_Flags |= xiiGALCommandQueueFlags::SparseBinding;
          }
        }
      }

      D3D12_FEATURE_DATA_D3D12_OPTIONS1 featureDataOptions1 = {};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &featureDataOptions1, sizeof(featureDataOptions1))))
      {
        if (featureDataOptions1.WaveOps != FALSE)
        {
          deviceFeatures.m_WaveOperation = xiiGALDeviceFeatureState::Enabled;

          m_AdapterDescription.m_WaveOperationProperties.m_uiMinSize             = featureDataOptions1.WaveLaneCountMin;
          m_AdapterDescription.m_WaveOperationProperties.m_uiMaxSize             = featureDataOptions1.WaveLaneCountMax;
          m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages = xiiGALShaderType::Pixel | xiiGALShaderType::Compute;
          m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures          = xiiGALWaveFeature::Basic | xiiGALWaveFeature::Vote | xiiGALWaveFeature::Arithmetic | xiiGALWaveFeature::Ballot | xiiGALWaveFeature::Quad;
          if (bMeshShadersSupported)
            m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderType::Amplification | xiiGALShaderType::Mesh;
        }
      }

      D3D12_FEATURE_DATA_D3D12_OPTIONS3 featureDataOptions3 = {};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &featureDataOptions3, sizeof(featureDataOptions3))))
      {
        if (featureDataOptions3.CopyQueueTimestampQueriesSupported)
          deviceFeatures.m_TransferQueueTimestampQueries = xiiGALDeviceFeatureState::Enabled;
      }

      D3D12_FEATURE_DATA_D3D12_OPTIONS4 featureDataOptions4{};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &featureDataOptions4, sizeof(featureDataOptions4))))
      {
        if (featureDataOptions4.Native16BitShaderOpsSupported)
        {
          deviceFeatures.m_ResourceBuffer16BitAccess = xiiGALDeviceFeatureState::Enabled;
          deviceFeatures.m_UniformBuffer16BitAccess  = xiiGALDeviceFeatureState::Enabled;
          deviceFeatures.m_ShaderInputOutput16       = xiiGALDeviceFeatureState::Enabled;
        }
      }

      D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureDataOptions5{};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureDataOptions5, sizeof(featureDataOptions5))))
      {
        if (featureDataOptions5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
        {
          deviceFeatures.m_RayTracing = xiiGALDeviceFeatureState::Enabled;

          m_AdapterDescription.m_RayTracingProperties.m_uiMaxRecursionDepth        = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
          m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupHandleSize    = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
          m_AdapterDescription.m_RayTracingProperties.m_uiMaxShaderRecordStride    = D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE;
          m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupBaseAlignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
          m_AdapterDescription.m_RayTracingProperties.m_uiMaxRayGenThreads         = D3D12_RAYTRACING_MAX_RAY_GENERATION_SHADER_THREADS;
          m_AdapterDescription.m_RayTracingProperties.m_uiMaxInstancesPerTLAS      = D3D12_RAYTRACING_MAX_INSTANCES_PER_TOP_LEVEL_ACCELERATION_STRUCTURE;
          m_AdapterDescription.m_RayTracingProperties.m_uiMaxPrimitivesPerBLAS     = D3D12_RAYTRACING_MAX_PRIMITIVES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE;
          m_AdapterDescription.m_RayTracingProperties.m_uiMaxGeometriesPerBLAS     = D3D12_RAYTRACING_MAX_GEOMETRIES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE;
          m_AdapterDescription.m_RayTracingProperties.m_uiVertexBufferAlignment    = 1U;
          m_AdapterDescription.m_RayTracingProperties.m_uiIndexBufferAlignment     = 1U;
          m_AdapterDescription.m_RayTracingProperties.m_uiTransformBufferAlignment = D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT;
          m_AdapterDescription.m_RayTracingProperties.m_uiBoxBufferAlignment       = D3D12_RAYTRACING_AABB_BYTE_ALIGNMENT;
          m_AdapterDescription.m_RayTracingProperties.m_uiScratchBufferAlignment   = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
          m_AdapterDescription.m_RayTracingProperties.m_uiInstanceBufferAlignment  = D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT;
          m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::StandaloneShaders;
        }
        if (featureDataOptions5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
        {
          m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::InlineRayTracing | xiiGALRayTracingCapabilityFlags::IndirectRayTracing;
        }
      }

#if defined(NTDDI_WIN10_19H1) || defined(FORCE_NTDDI_WIN10_19H1)
      D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureDataOptions6{};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureDataOptions6, sizeof(featureDataOptions6))))
      {
        // https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html#feature-tiering
        xiiGALShadingRateProperties& shadingRateProperties = m_AdapterDescription.m_ShadingRateProperties;
        auto                         AddShadingRate        = [&shadingRateProperties](xiiBitflags<xiiGALShadingRateFlags> shadingRate, xiiBitflags<xiiGALSampleCount> sampleBits) -> void {
          xiiGALShadingRateMode& mode = shadingRateProperties.m_Modes.ExpandAndGetRef();
          mode.m_ShadingRate          = shadingRate;
          mode.m_SampleBits           = sampleBits;
        };

        if (featureDataOptions6.AdditionalShadingRatesSupported != FALSE)
        {
          AddShadingRate(xiiGALShadingRateFlags::_4X4, xiiGALSampleCount::OneSample);
          AddShadingRate(xiiGALShadingRateFlags::_4X2, xiiGALSampleCount::OneSample | xiiGALSampleCount::TwoSamples);
          AddShadingRate(xiiGALShadingRateFlags::_2X4, xiiGALSampleCount::OneSample | xiiGALSampleCount::TwoSamples);
        }
        if (featureDataOptions6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1)
        {
          deviceFeatures.m_VariableRateShading = xiiGALDeviceFeatureState::Enabled;

          shadingRateProperties.m_Format = xiiGALShadingRateFormat::Palette;
          shadingRateProperties.m_CombinerFlags |= xiiGALShadingRateCombinerFlags::PassThrough;
          shadingRateProperties.m_CapabilityFlags |= xiiGALShadingRateCapabilityFlags::PerDraw;

          // 1x1, 1x2, 2x1, 2x2 are always supported
          AddShadingRate(xiiGALShadingRateFlags::_2X2, xiiGALSampleCount::OneSample | xiiGALSampleCount::TwoSamples | xiiGALSampleCount::FourSamples);
          AddShadingRate(xiiGALShadingRateFlags::_2X1, xiiGALSampleCount::OneSample | xiiGALSampleCount::TwoSamples | xiiGALSampleCount::FourSamples);
          AddShadingRate(xiiGALShadingRateFlags::_1X2, xiiGALSampleCount::OneSample | xiiGALSampleCount::TwoSamples | xiiGALSampleCount::FourSamples);
          AddShadingRate(xiiGALShadingRateFlags::_1X1, xiiGALSampleCount::AllSamples);
        }
        if (featureDataOptions6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
        {
          shadingRateProperties.m_CapabilityFlags = xiiGALShadingRateCapabilityFlags::PerPrimitive | xiiGALShadingRateCapabilityFlags::TextureBased | xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget | xiiGALShadingRateCapabilityFlags::SampleMask |
            xiiGALShadingRateCapabilityFlags::ShaderSampleMask | xiiGALShadingRateCapabilityFlags::ShadingRateShaderInput;

          shadingRateProperties.m_MinTileSize = xiiSizeU32(featureDataOptions6.ShadingRateImageTileSize, featureDataOptions6.ShadingRateImageTileSize);
          shadingRateProperties.m_MaxTileSize = xiiSizeU32(featureDataOptions6.ShadingRateImageTileSize, featureDataOptions6.ShadingRateImageTileSize);
          shadingRateProperties.m_CombinerFlags |= xiiGALShadingRateCombinerFlags::CombinerOverride | xiiGALShadingRateCombinerFlags::CombinerMin | xiiGALShadingRateCombinerFlags::CombinerMax | xiiGALShadingRateCombinerFlags::CombinerSum;
          shadingRateProperties.m_BindFlags |= xiiGALBindFlags::ShadingRate | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShadingRate;
          shadingRateProperties.m_TextureAccess = xiiGALShadingRateTextureAccess::OnGPU;
        }
        if (featureDataOptions6.PerPrimitiveShadingRateSupportedWithViewportIndexing != FALSE)
        {
          shadingRateProperties.m_CapabilityFlags |= xiiGALShadingRateCapabilityFlags::PerPrimitiveWithMultipleViewports;
        }
        // Export of depth and stencil is not supported
        // https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html#export-of-depth-and-stencil

        // Perhaps add support for D3D12_FEATURE_DATA_D3D12_OPTIONS10?
      }
#endif // NTDDI_WIN10_19H1
    }

    // Buffer properties.
    {
      m_AdapterDescription.m_BufferProperties.m_uiConstantBufferAlignment         = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
      m_AdapterDescription.m_BufferProperties.m_uiStructuredBufferOffsetAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
    }
  }

  // Texture properties.
  {
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DDimension     = D3D12_REQ_TEXTURE1D_U_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DArraySlices   = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DDimension     = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DArraySlices   = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture3DDimension     = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTextureCubeDimension   = D3D12_REQ_TEXTURECUBE_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_bTexture2DMSSupported       = true;
    m_AdapterDescription.m_TextureProperties.m_bTexture2DMSArraySupported  = true;
    m_AdapterDescription.m_TextureProperties.m_bTextureViewSupported       = true;
    m_AdapterDescription.m_TextureProperties.m_bCubeMapArraysSupported     = true;
    m_AdapterDescription.m_TextureProperties.m_bTextureView2DOn3DSupported = true;
  }

  // Sampler properties.
  {
    m_AdapterDescription.m_SamplerProperties.m_bBorderSamplingModeSupported = true;
    m_AdapterDescription.m_SamplerProperties.m_uiMaxAnisotropy              = D3D12_DEFAULT_MAX_ANISOTROPY;
    m_AdapterDescription.m_SamplerProperties.m_bLODBiasSupported            = true;
  }

  // Compute shader properties.
  {
    m_AdapterDescription.m_ComputeShaderProperties.m_uiSharedMemorySize          = 32U << 10U;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupInvocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeX       = D3D12_CS_THREAD_GROUP_MAX_X;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeY       = D3D12_CS_THREAD_GROUP_MAX_Y;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeZ       = D3D12_CS_THREAD_GROUP_MAX_Z;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountX      = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountY      = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountZ      = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
  }

  // Draw command properties.
  {
#if D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP >= 32
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxIndexValue = ~0U;
#else
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxIndexValue = 1U << D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP;
#endif
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::BaseVertex | xiiGALDrawCommandCapabilityFlags::NativeMultiDrawIndirect | xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALDeviceD3D12::EnumerateAdapters(xiiDynamicArray<IDXGIAdapter1*>& out_adapters)
{
  XII_ASSERT_DEV(m_pDXGIFactory != nullptr, "DXGI factory is not initialized.");

  out_adapters.Clear();

  xiiUInt32      uiIndex  = 0;
  IDXGIAdapter1* pAdapter = nullptr;
  while (SUCCEEDED(m_pDXGIFactory->EnumAdapters1(uiIndex, &pAdapter))) // If EnumAdapters1 returned DXGI_ERROR_NOT_FOUND, that's expected when enumeration ends.
  {
    out_adapters.PushBack(pAdapter);

    ++uiIndex;
  }

  return XII_SUCCESS;
}

bool xiiGALDeviceD3D12::IsAdapterCompatible(IDXGIAdapter1* pAdapter, D3D_FEATURE_LEVEL minFeatureLevel, bool bPermitSoftwareAdapters)
{
  if (pAdapter == nullptr)
    return false;

  DXGI_ADAPTER_DESC1 adapterDescription;
  if (FAILED(pAdapter->GetDesc1(&adapterDescription)))
    return false;

  // Skip software adapters if not permitted.
  if (!bPermitSoftwareAdapters && (adapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
    return false;

  ID3D12Device* pD3D12Device;
  XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pD3D12Device));

  // Try to create a D3D12 device with the given adapter and feature level to check compatibility.
  if (FAILED(D3D12CreateDevice(pAdapter, minFeatureLevel, __uuidof(ID3D12Device), reinterpret_cast<void**>(&pD3D12Device))))
    return false;

  return true;
}

xiiResult xiiGALDeviceD3D12::GetCompatibleAdapters(D3D_FEATURE_LEVEL minFeatureLevel, xiiDynamicArray<IDXGIAdapter1*>& out_CompatibleAdapters, bool bPermitSoftwareAdapters)
{
  XII_ASSERT_DEV(m_pDXGIFactory != nullptr, "DXGI factory is not initialized.");

  xiiTemporaryArray<IDXGIAdapter1*> allAdapters;
  XII_SUCCEED_OR_RETURN(EnumerateAdapters(allAdapters));

  out_CompatibleAdapters.Clear();
  out_CompatibleAdapters.Reserve(allAdapters.GetCount());

  for (xiiUInt32 i = 0; i < allAdapters.GetCount(); ++i)
  {
    if (IsAdapterCompatible(allAdapters[i], minFeatureLevel, bPermitSoftwareAdapters))
    {
      out_CompatibleAdapters.PushBack(allAdapters[i]);
    }
    else
    {
      // Release incompatible adapter.
      XII_GAL_D3D12_RELEASE(allAdapters[i]);
    }
  }

  return XII_SUCCESS;
}

IDXGIAdapter1* xiiGALDeviceD3D12::SelectBestAdapter(xiiArrayPtr<IDXGIAdapter1*> pCompatibleAdapters)
{
  // Rank adapters and pick the best one. Our strategy is to:
  // 1. Prefer discrete GPUs (non-software, non-integrated).
  // 2. Prefer higher dedicated video memory.
  // 3. Prefer adapters with more outputs (useful heuristic).

  if (pCompatibleAdapters.IsEmpty())
    return nullptr;

  struct AdapterRank
  {
    XII_DECLARE_POD_TYPE();

    IDXGIAdapter1* m_pAdapter;
    xiiUInt64      m_uiScore; // Higher score means better adapter.
  };

  xiiTemporaryArray<AdapterRank> rankedAdapters;
  rankedAdapters.Reserve(pCompatibleAdapters.GetCount());

  for (xiiUInt32 i = 0; i < pCompatibleAdapters.GetCount(); ++i)
  {
    IDXGIAdapter1* pAdapter = pCompatibleAdapters[i];

    DXGI_ADAPTER_DESC1 adapterDescription;
    if (FAILED(pAdapter->GetDesc1(&adapterDescription)))
      continue;

    xiiUInt64 uiScore = 0ULL;

    // Heuristic 1: Discrete GPUs get a big score boost.
    if (!(adapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
    {
      if (adapterDescription.DedicatedVideoMemory != 0U)
      {
        uiScore += 1000000ULL; // Discrete GPU.
      }
      else
      {
        uiScore += 500000ULL; // Integrated GPU.
      }
    }

    // Heuristic 2: Prefer more dedicated video memory.
    uiScore += static_cast<xiiUInt64>(adapterDescription.DedicatedVideoMemory / (1024 * 1024)); // MB weight.

    // Heuristic 3: Prefer adapters with more outputs (monitors).
    xiiUInt32    uiOutputCount = 0U;
    IDXGIOutput* pOutput       = nullptr;
    for (xiiUInt32 j = 0; SUCCEEDED(pAdapter->EnumOutputs(j, &pOutput)); ++j)
    {
      ++uiOutputCount;

      XII_GAL_D3D12_RELEASE(pOutput);
    }

    uiScore += static_cast<xiiUInt64>(uiOutputCount) * 100ULL; // Output count weight.

    rankedAdapters.PushBack({pAdapter, uiScore});
  }

  if (rankedAdapters.IsEmpty())
    return nullptr;

  rankedAdapters.Sort([](const AdapterRank& a, const AdapterRank& b) { return a.m_uiScore > b.m_uiScore; });

  return rankedAdapters[0].m_pAdapter;
}

xiiResult xiiGALDeviceD3D12::SelectAdapterByIndex(xiiUInt32 uiAdapterIndex, D3D_FEATURE_LEVEL minFeatureLevel, IDXGIAdapter1** out_ppAdapter, bool bPermitSoftwareAdapter, bool bPreferBestIfIndexInvalid)
{
  XII_ASSERT_DEV(m_pDXGIFactory != nullptr, "DXGI factory is not initialized.");

  *out_ppAdapter = nullptr;

  xiiTemporaryArray<IDXGIAdapter1*> compatibleAdapters;
  XII_SUCCEED_OR_RETURN(GetCompatibleAdapters(minFeatureLevel, compatibleAdapters, bPermitSoftwareAdapter));
  XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE_ARRAY(compatibleAdapters));

  if (compatibleAdapters.IsEmpty())
  {
    xiiLog::Warning("No compatible adapters found for feature level {}.", GetD3D12FeatureLevelName(minFeatureLevel));

    // Try to create a WARP device (a high-performance software device that has the capabilities of a hardware device).
    IDXGIAdapter1* pWARPAdapter = nullptr;
    if (SUCCEEDED(m_pDXGIFactory->EnumWarpAdapter(__uuidof(IDXGIAdapter1), reinterpret_cast<void**>(&pWARPAdapter))))
    {
      if (IsAdapterCompatible(pWARPAdapter, minFeatureLevel, bPermitSoftwareAdapter))
      {
        *out_ppAdapter = pWARPAdapter;

        return XII_SUCCESS;
      }
      else
      {
        xiiLog::Warning("WARP adapter is not compatible with feature level {}.", GetD3D12FeatureLevelName(minFeatureLevel));

        XII_GAL_D3D12_RELEASE(pWARPAdapter);
      }
    }
    else
    {
      xiiLog::Warning("Failed to enumerate WARP adapter.");
    }
  }

  if (uiAdapterIndex < compatibleAdapters.GetCount())
  {
    *out_ppAdapter = compatibleAdapters[uiAdapterIndex];

    compatibleAdapters.RemoveAndSwap(*out_ppAdapter); // Remove the selected adapter from the list to avoid releasing it in the scope exit.

    return XII_SUCCESS;
  }
  else
  {
    xiiLog::Warning("Requested adapter index {} is out of range. {} compatible adapter(s) found for feature level {}.", uiAdapterIndex, compatibleAdapters.GetCount(), GetD3D12FeatureLevelName(minFeatureLevel));

    if (bPreferBestIfIndexInvalid)
    {
      IDXGIAdapter1* pBestAdapter = SelectBestAdapter(compatibleAdapters);

      if (pBestAdapter != nullptr)
      {
        *out_ppAdapter = pBestAdapter;

        xiiLog::Info("Selected the best available adapter instead: {}.", GetD3D12FeatureLevelName(minFeatureLevel));

        compatibleAdapters.RemoveAndSwap(*out_ppAdapter); // Remove the selected adapter from the list to avoid releasing it in the scope exit.

        return XII_SUCCESS;
      }
      else
      {
        xiiLog::Warning("No suitable adapter found to select as best.");
      }
    }
  }

  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Device_Implementation_DeviceD3D12);
