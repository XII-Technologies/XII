#include <RendererDiligent/RendererDiligentPCH.h>

#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererDiligent/CommandEncoder/CommandEncoderImplDiligent.h>
#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Device/PassDiligent.h>
#include <RendererDiligent/Device/SwapChainDiligent.h>
#include <RendererDiligent/Resources/BufferDiligent.h>
#include <RendererDiligent/Resources/QueryDiligent.h>
#include <RendererDiligent/Resources/RenderTargetViewDiligent.h>
#include <RendererDiligent/Resources/ResourceViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>
#include <RendererDiligent/Resources/UnorderedAccessViewDiligent.h>
#include <RendererDiligent/Shader/ShaderDiligent.h>
#include <RendererDiligent/Shader/VertexDeclarationDiligent.h>
#include <RendererDiligent/State/StateDiligent.h>
#include <RendererDiligent/Utilities/DiligentConversions.h>

#include <Graphics/GraphicsTools/interface/DurationQueryHelper.hpp>
#include <Graphics/GraphicsTools/interface/ScopedQueryHelper.hpp>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#if D3D11_SUPPORTED
#  include <Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>
#endif

#if D3D12_SUPPORTED
#  include <Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
#  include <Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h>
#endif

#if VULKAN_SUPPORTED
#  include <Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>
#endif

#if METAL_SUPPORTED
#  include <Graphics/GraphicsEngineMetal/interface/EngineFactoryMtl.h>
#endif

struct xiiDiligentMemoryAllocator : public Diligent::IMemoryAllocator
{
public:
  xiiDiligentMemoryAllocator(const char* szName) :
    m_ProxyAlloc(szName, xiiFoundation::GetDefaultAllocator())
  {
  }

  /// Allocates block of memory
  virtual void* Allocate(size_t Size, const Diligent::Char* dbgDescription, const char* dbgFileName, const Diligent::Int32 dbgLineNumber) override
  {
    return m_ProxyAlloc.Allocate(Size, 8u);
  }

  /// Releases memory
  virtual void Free(void* Ptr) override
  {
    m_ProxyAlloc.Deallocate(Ptr);
  }

  xiiProxyAllocator m_ProxyAlloc;
};

void XIILogDiligent(enum Diligent::DEBUG_MESSAGE_SEVERITY Severity,
                    const Diligent::Char*                 Message,
                    const Diligent::Char*                 Function,
                    const Diligent::Char*                 File,
                    int                                   Line)
{
  // Format Diligent string as it is in printf format
  switch (Severity)
  {
    case Diligent::DEBUG_MESSAGE_SEVERITY_INFO:
      xiiLog::Info("{}", Message);
      break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_WARNING:
      xiiLog::Warning("{}", Message);
      break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_ERROR:
      xiiLog::SeriousWarning("{}", Message);
      break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR:
      xiiLog::Error("{}", Message);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }
}

xiiInternal::NewInstance<xiiGALDevice> CreateDiligentDeviceD3D11(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description)
{
  xiiGraphicsDevice::Default = xiiGraphicsDevice::D3D11;
  return XII_NEW(pAllocator, xiiGALDeviceDiligent, Description);
}

xiiInternal::NewInstance<xiiGALDevice> CreateDiligentDeviceD3D12(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description)
{
  xiiGraphicsDevice::Default = xiiGraphicsDevice::D3D12;
  return XII_NEW(pAllocator, xiiGALDeviceDiligent, Description);
}

xiiInternal::NewInstance<xiiGALDevice> CreateDiligentDeviceVulkan(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description)
{
  xiiGraphicsDevice::Default = xiiGraphicsDevice::Vulkan;
  return XII_NEW(pAllocator, xiiGALDeviceDiligent, Description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RendererDiligent, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  xiiGALDeviceFactory::RegisterCreatorFunc("D3D11", &CreateDiligentDeviceD3D11, "D3D_SM50", "xiiShaderCompiler");
  xiiGALDeviceFactory::RegisterCreatorFunc("D3D12", &CreateDiligentDeviceD3D12, "D3D_SM60", "xiiShaderCompiler");
  xiiGALDeviceFactory::RegisterCreatorFunc("Vulkan", &CreateDiligentDeviceVulkan, "VK_SM60", "xiiShaderCompiler");
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiGALDeviceFactory::UnregisterCreatorFunc("D3D11");
  xiiGALDeviceFactory::UnregisterCreatorFunc("D3D12");
  xiiGALDeviceFactory::UnregisterCreatorFunc("Vulkan");
}

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiGALDeviceDiligent::xiiGALDeviceDiligent(const xiiGALDeviceCreationDescription& Description) :
  xiiGALDevice(Description), m_pDevice(nullptr), m_pEngineFactory(nullptr)
{
}

xiiGALDeviceDiligent::~xiiGALDeviceDiligent() = default;

// Init & shutdown functions

xiiResult xiiGALDeviceDiligent::InitPlatform()
{
  XII_LOG_BLOCK("xiiGALDeviceDiligent::InitPlatform");

  m_DeviceType = xiiDiligentUtils::GetDiligentRenderDeviceType();

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  // Using our memory allocator crashes on Linux allocating 64 byte aligned buffers.
  m_pMemoryAllocator = std::make_unique<xiiDiligentMemoryAllocator>("Diligent Engine Memory Allocator");
#endif

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  m_iValidationLevel = Diligent::VALIDATION_LEVEL_2;
#elif XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_iValidationLevel = Diligent::VALIDATION_LEVEL_1;
#else
  m_iValidationLevel = Diligent::VALIDATION_LEVEL_DISABLED;
#endif

  xiiUInt32 NumImmediateContexts = 0;

#if D3D11_SUPPORTED || D3D12_SUPPORTED || VULKAN_SUPPORTED
  auto FindAdapter = [this](auto* pFactory, Diligent::Version GraphicsAPIVersion, Diligent::GraphicsAdapterInfo& AdapterAttribs) -> xiiUInt32 {
    xiiUInt32 NumAdapters = 0;
    pFactory->EnumerateAdapters(GraphicsAPIVersion, NumAdapters, nullptr);
    xiiHybridArray<Diligent::GraphicsAdapterInfo, 2> Adapters;
    Adapters.Reserve(NumAdapters);

    if (NumAdapters > 0)
      pFactory->EnumerateAdapters(GraphicsAPIVersion, NumAdapters, Adapters.GetData());
    else
      XII_ASSERT_DEV(false, "Failed to find compatible hardware adapters");

    auto AdapterId = m_uiAdapterId;
    if (AdapterId != Diligent::DEFAULT_ADAPTER_ID)
    {
      if (AdapterId < Adapters.GetCount())
      {
        m_AdapterType = Adapters[AdapterId].Type;
      }
      else
      {
        xiiLog::Info("Adapter ID ('{0}') is invalid. Only {1} compatible {2} present in the system.", AdapterId, Adapters.GetCount(), (Adapters.GetCount() == 1 ? "adapter" : "adapters"));
        AdapterId = Diligent::DEFAULT_ADAPTER_ID;
      }
    }

    if (AdapterId == Diligent::DEFAULT_ADAPTER_ID && m_AdapterType != Diligent::ADAPTER_TYPE_UNKNOWN)
    {
      for (xiiUInt32 i = 0; i < Adapters.GetCount(); ++i)
      {
        if (Adapters[i].Type == m_AdapterType)
        {
          AdapterId = i;
          break;
        }
      }

      if (AdapterId == Diligent::DEFAULT_ADAPTER_ID)
        xiiLog::Warning("Unable to find the requested adapter type. Using default adapter.");
    }

    if (AdapterId == Diligent::DEFAULT_ADAPTER_ID)
    {
      m_AdapterType = Diligent::ADAPTER_TYPE_UNKNOWN;
      for (xiiUInt32 i = 0; i < Adapters.GetCount(); ++i)
      {
        const Diligent::GraphicsAdapterInfo& AdapterInfo = Adapters[i];
        const Diligent::ADAPTER_TYPE         AdapterType = AdapterInfo.Type;

        static_assert((Diligent::ADAPTER_TYPE_DISCRETE > Diligent::ADAPTER_TYPE_INTEGRATED &&
                       Diligent::ADAPTER_TYPE_INTEGRATED > Diligent::ADAPTER_TYPE_SOFTWARE &&
                       Diligent::ADAPTER_TYPE_SOFTWARE > Diligent::ADAPTER_TYPE_UNKNOWN),
                      "Unexpected ADAPTER_TYPE enum ordering");
        if (AdapterType > m_AdapterType)
        {
          // Prefer Discrete over Integrated over Software
          m_AdapterType = AdapterType;
          AdapterId     = i;
        }
        else if (AdapterType == m_AdapterType)
        {
          // Select adapter with more memory
          const auto& NewAdapterMem   = AdapterInfo.Memory;
          const auto  NewTotalMemory  = NewAdapterMem.LocalMemory + NewAdapterMem.HostVisibleMemory + NewAdapterMem.UnifiedMemory;
          const auto& CurrAdapterMem  = Adapters[AdapterId].Memory;
          const auto  CurrTotalMemory = CurrAdapterMem.LocalMemory + CurrAdapterMem.HostVisibleMemory + CurrAdapterMem.UnifiedMemory;
          if (NewTotalMemory > CurrTotalMemory)
          {
            AdapterId = i;
          }
        }
      }
    }

    if (AdapterId != Diligent::DEFAULT_ADAPTER_ID)
    {
      AdapterAttribs = Adapters[AdapterId];
      xiiLog::Info("Using Adapter {0}: '{1}'", AdapterId, AdapterAttribs.Description);
    }

    return AdapterId;
  };
#endif

  xiiHybridArray<Diligent::IDeviceContext*, 1> ppContexts;

  switch (m_DeviceType)
  {
#if D3D11_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D11:
    {
    CreateDeviceD3D11:
#  if ENGINE_DLL
      // Load the dll and import GetEngineFactoryD3D11() function
      auto GetEngineFactoryD3D11 = Diligent::LoadGraphicsEngineD3D11();
#  endif
      auto* pFactoryD3D11 = GetEngineFactoryD3D11();
      m_pEngineFactory    = pFactoryD3D11;

      // Register custom message callback
      m_pEngineFactory->SetMessageCallback(XIILogDiligent);

      Diligent::EngineD3D11CreateInfo EngineCI;
      EngineCI.GraphicsAPIVersion = {11, 0};
      EngineCI.pRawMemAllocator   = m_pMemoryAllocator.get();
      EngineCI.EnableValidation   = m_Description.m_bDebugDevice;

      EngineCI.Features.OcclusionQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.BinaryOcclusionQueries    = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.TimestampQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.PipelineStatisticsQueries = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.DurationQueries           = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;

      if (m_iValidationLevel >= 0)
        EngineCI.SetValidationLevel(static_cast<Diligent::VALIDATION_LEVEL>(m_iValidationLevel));

      EngineCI.AdapterId = FindAdapter(pFactoryD3D11, EngineCI.GraphicsAPIVersion, m_AdapterAttribs);

      if (m_AdapterType != Diligent::ADAPTER_TYPE_SOFTWARE && EngineCI.AdapterId != Diligent::DEFAULT_ADAPTER_ID)
      {
        // Display mode enumeration fails with error for software adapter
        xiiUInt32 NumDisplayModes = 0;
        pFactoryD3D11->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, EngineCI.AdapterId, 0, Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, nullptr);
        m_DisplayModes.SetCount(NumDisplayModes);
        pFactoryD3D11->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, EngineCI.AdapterId, 0, Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, m_DisplayModes.GetData());
      }

      NumImmediateContexts = xiiMath::Max(1u, EngineCI.NumImmediateContexts);
      ppContexts.SetCount(size_t{NumImmediateContexts} + size_t{EngineCI.NumDeferredContexts});
      pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &m_pDevice, ppContexts.GetData());

      XII_ASSERT_DEV(m_pDevice != nullptr, "Unable to initialize Diligent Engine in Direct3D11 mode. The API may not be available, "
                                           "or required features may not be supported by this GPU/driver/OS version.");
    }
    break;
#endif

#if D3D12_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D12:
    {
#  if ENGINE_DLL
      // Load the dll and import GetEngineFactoryD3D12() function
      auto GetEngineFactoryD3D12 = Diligent::LoadGraphicsEngineD3D12();
#  endif

      auto* pFactoryD3D12 = GetEngineFactoryD3D12();
      XII_ASSERT_DEV(pFactoryD3D12->LoadD3D12() == Diligent::True, "Failed to load Direct3D12");
      m_pEngineFactory = pFactoryD3D12;

      // Register custom message callback
      m_pEngineFactory->SetMessageCallback(XIILogDiligent);

      Diligent::EngineD3D12CreateInfo EngineCI;
      EngineCI.GraphicsAPIVersion                 = {11, 0};
      EngineCI.pRawMemAllocator                   = m_pMemoryAllocator.get();
      EngineCI.EnableValidation                   = m_Description.m_bDebugDevice;
      EngineCI.Features.OcclusionQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.BinaryOcclusionQueries    = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.TimestampQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.PipelineStatisticsQueries = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.DurationQueries           = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      if (m_iValidationLevel >= 0)
        EngineCI.SetValidationLevel(static_cast<Diligent::VALIDATION_LEVEL>(m_iValidationLevel));

      try
      {
        EngineCI.AdapterId = FindAdapter(pFactoryD3D12, EngineCI.GraphicsAPIVersion, m_AdapterAttribs);
      }
      catch (...)
      {
#  if D3D11_SUPPORTED
        xiiLog::Error("Failed to find Direct3D12-compatible hardware adapters. Attempting to initialize the engine in Direct3D11 mode.");
        xiiGraphicsDevice::Default = xiiGraphicsDevice::D3D11;
        m_DeviceType               = Diligent::RENDER_DEVICE_TYPE_D3D11;
        goto CreateDeviceD3D11;
#  else
        throw;
#  endif
      }

      if (m_AdapterType != Diligent::ADAPTER_TYPE_SOFTWARE && EngineCI.AdapterId != Diligent::DEFAULT_ADAPTER_ID)
      {
        // Display mode enumeration fails with error for software adapter
        xiiUInt32 NumDisplayModes = 0;
        pFactoryD3D12->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, EngineCI.AdapterId, 0, Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, nullptr);
        m_DisplayModes.SetCount(NumDisplayModes);
        pFactoryD3D12->EnumerateDisplayModes(EngineCI.GraphicsAPIVersion, EngineCI.AdapterId, 0, Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, m_DisplayModes.GetData());
      }

      NumImmediateContexts = xiiMath::Max(1u, EngineCI.NumImmediateContexts);
      ppContexts.SetCount(NumImmediateContexts + EngineCI.NumDeferredContexts);
      pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &m_pDevice, ppContexts.GetData());

      XII_ASSERT_DEV(m_pDevice != nullptr, "Unable to initialize Diligent Engine in Direct3D12 mode. The API may not be available, "
                                           "or required features may not be supported by this GPU/driver/OS version.");
    }
    break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_GL:
    case Diligent::RENDER_DEVICE_TYPE_GLES:
    {
#  if EXPLICITLY_LOAD_ENGINE_GL_DLL
      // Load the dll and import GetEngineFactoryOpenGL() function
      auto GetEngineFactoryOpenGL = Diligent::LoadGraphicsEngineOpenGL();
#  endif

      auto* pFactoryGL = GetEngineFactoryOpenGL();
      XII_ASSERT_DEV(pFactoryGL != nullptr, "Failed To Load OpenGL");
      m_pEngineFactory = pFactoryGL;

      // Register custom message callback
      m_pEngineFactory->SetMessageCallback(XIILogDiligent);

      NumImmediateContexts = 1; // + EngineCI.NumImmediateContexts which is zero
      ppContexts.SetCount(NumImmediateContexts);
    }
    break;
#endif

#if VULKAN_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_VULKAN:
    {
#  if EXPLICITLY_LOAD_ENGINE_VK_DLL
      // Load the dll and import GetEngineFactoryVk() function
      auto GetEngineFactoryVk = Diligent::LoadGraphicsEngineVk();
#  endif

      auto* pFactoryVk = GetEngineFactoryVk();
      XII_ASSERT_DEV(pFactoryVk != nullptr, "Failed to load Vulkan");
      m_pEngineFactory = pFactoryVk;

      // Register custom message callback
      m_pEngineFactory->SetMessageCallback(XIILogDiligent);

      Diligent::EngineVkCreateInfo EngineCI;
      EngineCI.pRawMemAllocator                   = m_pMemoryAllocator.get();
      EngineCI.EnableValidation                   = m_Description.m_bDebugDevice;
      EngineCI.Features.OcclusionQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.BinaryOcclusionQueries    = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.TimestampQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.PipelineStatisticsQueries = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.DurationQueries           = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;

      if (m_iValidationLevel >= 0)
        EngineCI.SetValidationLevel(static_cast<Diligent::VALIDATION_LEVEL>(m_iValidationLevel));

      const char* const ppIgnoreDebugMessages[] = //
        {
          // Validation Performance Warning: [ UNASSIGNED-CoreValidation-Shader-OutputNotConsumed ]
          // vertex shader writes to output location 1.0 which is not consumed by fragment shader
          "UNASSIGNED-CoreValidation-Shader-OutputNotConsumed" //
        };
      EngineCI.ppIgnoreDebugMessageNames = ppIgnoreDebugMessages;
      EngineCI.IgnoreDebugMessageCount   = _countof(ppIgnoreDebugMessages);

      EngineCI.AdapterId = FindAdapter(pFactoryVk, EngineCI.GraphicsAPIVersion, m_AdapterAttribs);

      NumImmediateContexts = xiiMath::Max(1u, EngineCI.NumImmediateContexts);
      ppContexts.SetCount(NumImmediateContexts + EngineCI.NumDeferredContexts);
      pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &m_pDevice, ppContexts.GetData());

      XII_ASSERT_DEV(m_pDevice != nullptr, "Unable to initialize Diligent Engine in Vulkan mode. The API may not be available, "
                                           "or required features may not be supported by this GPU/driver/OS version.");
    }
    break;
#endif

#if METAL_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_METAL:
    {
      Diligent::EngineMtlCreateInfo EngineCI;
      EngineCI.Features.OcclusionQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.BinaryOcclusionQueries    = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.TimestampQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.PipelineStatisticsQueries = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.DurationQueries           = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;

      if (m_iValidationLevel >= 0)
        EngineCI.SetValidationLevel(static_cast<Diligent::VALIDATION_LEVEL>(m_iValidationLevel));

      auto* pFactoryMtl = GetEngineFactoryMtl();
      m_pEngineFactory  = pFactoryMtl;

      NumImmediateContexts = xiiMath::Max(1u, EngineCI.NumImmediateContexts);
      ppContexts.resize(NumImmediateContexts + EngineCI.NumDeferredContexts);
      pFactoryMtl->CreateDeviceAndContextsMtl(EngineCI, &m_pDevice, ppContexts.GetData());

      XII_ASSERT_DEV(m_pDevice != nullptr, "Unable to initialize Diligent Engine in Metal mode. The API may not be available, "
                                           "or required features may not be supported by this GPU/driver/OS version.");
    }
    break;
#endif

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  m_uiNumImmediateContexts = NumImmediateContexts;
  m_pDeviceContexts.SetCount(ppContexts.GetCount());
  for (xiiUInt32 i = 0; i < ppContexts.GetCount(); ++i)
    m_pDeviceContexts[i].Attach(ppContexts[i]);

  // Create default pass
  m_pDefaultPass = XII_NEW(&m_Allocator, xiiGALPassDiligent, *this);

  // Fill lookup table
  FillFormatLookupTable();

  xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  m_SyncTimeDiff.SetZero();

  xiiGALWindowSwapChain::SetFactoryMethod([this](const xiiGALWindowSwapChainCreationDescription& desc) -> xiiGALSwapChainHandle { return CreateSwapChain([this, &desc](xiiAllocatorBase* pAllocator) -> xiiGALSwapChain* { return XII_NEW(pAllocator, xiiGALSwapChainDiligent, desc); }); });

  return XII_SUCCESS;
}

xiiResult xiiGALDeviceDiligent::ShutdownPlatform()
{
  xiiGALWindowSwapChain::SetFactoryMethod({});

  for (xiiUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    for (auto it = m_FreeTempResources[type].GetIterator(); it.IsValid(); ++it)
    {
      xiiDynamicArray<Diligent::IDeviceObject*>& resources = it.Value();
      for (auto pResource : resources)
      {
        switch ((TempResourceType::Enum)type)
        {
          case TempResourceType::Texture:
          {
            Diligent::ITexture* pTexture = static_cast<Diligent::ITexture*>(pResource);
            XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pTexture);
          }
          break;
          case TempResourceType::Buffer:
          {
            Diligent::IBuffer* pBuffer = static_cast<Diligent::IBuffer*>(pResource);
            XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pBuffer);
          }
          break;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    m_FreeTempResources[type].Clear();

    for (auto& tempResource : m_UsedTempResources[type])
    {
      switch ((TempResourceType::Enum)type)
      {
        case TempResourceType::Texture:
        {
          Diligent::ITexture* pTexture = static_cast<Diligent::ITexture*>(tempResource.m_pResource);
          XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pTexture);
        }
        break;
        case TempResourceType::Buffer:
        {
          Diligent::IBuffer* pBuffer = static_cast<Diligent::IBuffer*>(tempResource.m_pResource);
          XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pBuffer);
        }
        break;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    m_UsedTempResources[type].Clear();
  }

  if (!m_pDeviceContexts.IsEmpty())
  {
    for (xiiUInt32 q = 0; q < m_uiNumImmediateContexts; ++q)
    {
      m_pDeviceContexts[q]->Flush();

      XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pDeviceContexts[q]);
    }

    m_pDeviceContexts.Clear();
  }

  m_uiNumImmediateContexts = 0;

  m_pDefaultPass = nullptr;

  m_pDevice->ReleaseStaleResources(true);

  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pDevice);

  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pEngineFactory);

  m_pMemoryAllocator.reset();

  ReportLiveGpuObjects();

  return XII_SUCCESS;
}

void xiiGALDeviceDiligent::ReportLiveGpuObjects()
{
  // Implement detailed live GPU Object information
}

void xiiGALDeviceDiligent::FlushDeadObjects()
{
  DestroyDeadObjects();
}

// Pipeline & Pass functions

void xiiGALDeviceDiligent::BeginPipelinePlatform(const char* szName, xiiGALSwapChain* pSwapChain)
{
  XII_PROFILE_SCOPE("BeginPipelinePlatform");

  if (pSwapChain)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }

#if XII_ENABLED(XII_USE_PROFILING)
  m_pPipelineTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif
}

void xiiGALDeviceDiligent::EndPipelinePlatform(xiiGALSwapChain* pSwapChain)
{
  XII_PROFILE_SCOPE("EndPipelinePlatform");

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPipelineTimingScope);
#endif

  if (pSwapChain)
  {
    pSwapChain->PresentRenderTarget(this);
  }

  m_pDefaultPass->Reset();
}

xiiGALPass* xiiGALDeviceDiligent::BeginPassPlatform(const char* szName)
{
#if XII_ENABLED(XII_USE_PROFILING)
  m_pPassTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  return m_pDefaultPass.Borrow();
}

void xiiGALDeviceDiligent::EndPassPlatform(xiiGALPass* pPass)
{
  XII_ASSERT_DEV(m_pDefaultPass.Borrow() == pPass, "Invalid pass");

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPassTimingScope);
#endif
}

// State creation functions

xiiGALBlendState* xiiGALDeviceDiligent::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& Description)
{
  xiiGALBlendStateDiligent* pState = XII_NEW(&m_Allocator, xiiGALBlendStateDiligent, Description);

  if (pState->InitPlatform(this).Succeeded())
  {
    return pState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pState);
    return nullptr;
  }
}

void xiiGALDeviceDiligent::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
  xiiGALBlendStateDiligent* pState = static_cast<xiiGALBlendStateDiligent*>(pBlendState);
  pState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pState);
}

xiiGALDepthStencilState* xiiGALDeviceDiligent::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& Description)
{
  xiiGALDepthStencilStateDiligent* pDiligentDepthStencilState = XII_NEW(&m_Allocator, xiiGALDepthStencilStateDiligent, Description);

  if (pDiligentDepthStencilState->InitPlatform(this).Succeeded())
  {
    return pDiligentDepthStencilState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pDiligentDepthStencilState);
    return nullptr;
  }
}

void xiiGALDeviceDiligent::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
  xiiGALDepthStencilStateDiligent* pDiligentDepthStencilState = static_cast<xiiGALDepthStencilStateDiligent*>(pDepthStencilState);
  pDiligentDepthStencilState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDiligentDepthStencilState);
}

xiiGALRasterizerState* xiiGALDeviceDiligent::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& Description)
{
  xiiGALRasterizerStateDiligent* pDiligentRasterizerState = XII_NEW(&m_Allocator, xiiGALRasterizerStateDiligent, Description);

  if (pDiligentRasterizerState->InitPlatform(this).Succeeded())
  {
    return pDiligentRasterizerState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pDiligentRasterizerState);
    return nullptr;
  }
}

void xiiGALDeviceDiligent::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  xiiGALRasterizerStateDiligent* pDiligentRasterizerState = static_cast<xiiGALRasterizerStateDiligent*>(pRasterizerState);
  pDiligentRasterizerState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDiligentRasterizerState);
}

xiiGALSamplerState* xiiGALDeviceDiligent::CreateSamplerStatePlatform(const xiiGALSamplerStateCreationDescription& Description)
{
  xiiGALSamplerStateDiligent* pDiligentSamplerState = XII_NEW(&m_Allocator, xiiGALSamplerStateDiligent, Description);

  if (pDiligentSamplerState->InitPlatform(this).Succeeded())
  {
    return pDiligentSamplerState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pDiligentSamplerState);
    return nullptr;
  }
}

void xiiGALDeviceDiligent::DestroySamplerStatePlatform(xiiGALSamplerState* pSamplerState)
{
  xiiGALSamplerStateDiligent* pDiligentSamplerState = static_cast<xiiGALSamplerStateDiligent*>(pSamplerState);
  pDiligentSamplerState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDiligentSamplerState);
}


// Resource creation functions

xiiGALShader* xiiGALDeviceDiligent::CreateShaderPlatform(const xiiGALShaderCreationDescription& Description)
{
  xiiGALShaderDiligent* pShader = XII_NEW(&m_Allocator, xiiGALShaderDiligent, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void xiiGALDeviceDiligent::DestroyShaderPlatform(xiiGALShader* pShader)
{
  xiiGALShaderDiligent* pDiligentShader = static_cast<xiiGALShaderDiligent*>(pShader);
  pDiligentShader->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDiligentShader);
}

xiiGALBuffer* xiiGALDeviceDiligent::CreateBufferPlatform(const xiiGALBufferCreationDescription& Description, xiiArrayPtr<const xiiUInt8> pInitialData)
{
  xiiGALBufferDiligent* pBuffer = XII_NEW(&m_Allocator, xiiGALBufferDiligent, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    XII_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void xiiGALDeviceDiligent::DestroyBufferPlatform(xiiGALBuffer* pBuffer)
{
  xiiGALBufferDiligent* pDiligentBuffer = static_cast<xiiGALBufferDiligent*>(pBuffer);
  pDiligentBuffer->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDiligentBuffer);
}

xiiGALTexture* xiiGALDeviceDiligent::CreateTexturePlatform(const xiiGALTextureCreationDescription& Description, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData)
{
  xiiGALTextureDiligent* pTexture = XII_NEW(&m_Allocator, xiiGALTextureDiligent, Description);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    XII_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void xiiGALDeviceDiligent::DestroyTexturePlatform(xiiGALTexture* pTexture)
{
  xiiGALTextureDiligent* pDiligentTexture = static_cast<xiiGALTextureDiligent*>(pTexture);
  pDiligentTexture->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDiligentTexture);
}

xiiGALResourceView* xiiGALDeviceDiligent::CreateResourceViewPlatform(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description)
{
  xiiGALResourceViewDiligent* pResourceView = XII_NEW(&m_Allocator, xiiGALResourceViewDiligent, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void xiiGALDeviceDiligent::DestroyResourceViewPlatform(xiiGALResourceView* pResourceView)
{
  xiiGALResourceViewDiligent* pDiligentResourceView = static_cast<xiiGALResourceViewDiligent*>(pResourceView);
  pDiligentResourceView->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDiligentResourceView);
}

xiiGALRenderTargetView* xiiGALDeviceDiligent::CreateRenderTargetViewPlatform(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description)
{
  xiiGALRenderTargetViewDiligent* pRTView = XII_NEW(&m_Allocator, xiiGALRenderTargetViewDiligent, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void xiiGALDeviceDiligent::DestroyRenderTargetViewPlatform(xiiGALRenderTargetView* pRenderTargetView)
{
  xiiGALRenderTargetViewDiligent* pDiligentRenderTargetView = static_cast<xiiGALRenderTargetViewDiligent*>(pRenderTargetView);
  pDiligentRenderTargetView->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDiligentRenderTargetView);
}

xiiGALUnorderedAccessView* xiiGALDeviceDiligent::CreateUnorderedAccessViewPlatform(xiiGALResourceBase* pTextureOfBuffer, const xiiGALUnorderedAccessViewCreationDescription& Description)
{
  xiiGALUnorderedAccessViewDiligent* pUnorderedAccessView = XII_NEW(&m_Allocator, xiiGALUnorderedAccessViewDiligent, pTextureOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void xiiGALDeviceDiligent::DestroyUnorderedAccessViewPlatform(xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  xiiGALUnorderedAccessViewDiligent* pUnorderedAccessViewDiligent = static_cast<xiiGALUnorderedAccessViewDiligent*>(pUnorderedAccessView);
  pUnorderedAccessViewDiligent->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pUnorderedAccessViewDiligent);
}



// Other rendering creation functions

xiiGALQuery* xiiGALDeviceDiligent::CreateQueryPlatform(const xiiGALQueryCreationDescription& Description)
{
  xiiGALQueryDiligent* pQuery = XII_NEW(&m_Allocator, xiiGALQueryDiligent, Description);

  if (!pQuery->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pQuery);
    return nullptr;
  }

  return pQuery;
}

void xiiGALDeviceDiligent::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
  xiiGALQueryDiligent* pQueryDiligent = static_cast<xiiGALQueryDiligent*>(pQuery);
  pQueryDiligent->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pQueryDiligent);
}

xiiGALVertexDeclaration* xiiGALDeviceDiligent::CreateVertexDeclarationPlatform(const xiiGALVertexDeclarationCreationDescription& Description)
{
  xiiGALVertexDeclarationDiligent* pVertexDeclaration = XII_NEW(&m_Allocator, xiiGALVertexDeclarationDiligent, Description);

  if (pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    XII_DELETE(&m_Allocator, pVertexDeclaration);
    return nullptr;
  }
}

void xiiGALDeviceDiligent::DestroyVertexDeclarationPlatform(xiiGALVertexDeclaration* pVertexDeclaration)
{
  xiiGALVertexDeclarationDiligent* pVertexDeclarationDiligent = static_cast<xiiGALVertexDeclarationDiligent*>(pVertexDeclaration);
  pVertexDeclarationDiligent->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVertexDeclarationDiligent);
}

xiiGALTimestampHandle xiiGALDeviceDiligent::GetTimestampPlatform()
{
  return {(xiiUInt64)-1, GetImmediateContext()->GetFrameNumber()};
}

xiiResult xiiGALDeviceDiligent::GetTimestampResultPlatform(xiiGALTimestampHandle hTimestamp, xiiTime& result)
{
  return XII_SUCCESS;
}

// Swap chain functions


// Misc functions

void xiiGALDeviceDiligent::BeginFramePlatform(const xiiUInt64 uiRenderFrame)
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

#if 0
#  if XII_ENABLED(XII_USE_PROFILING)
  xiiStringBuilder sb;
  sb.Format("Frame {}", uiRenderFrame);
  m_pFrameTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), sb);
#  endif
#endif
}

void xiiGALDeviceDiligent::EndFramePlatform()
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  FreeTempResources(GetImmediateContext()->GetFrameNumber());

#if 0
#  if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pFrameTimingScope);
#  endif
#endif
}

void xiiGALDeviceDiligent::FillCapabilitiesPlatform()
{
  const Diligent::GraphicsAdapterInfo& adapterInfo = m_pDevice->GetAdapterInfo();
  {
    m_Capabilities.m_sAdapterName         = xiiStringUtf8(adapterInfo.Description).GetData();
    m_Capabilities.m_uiDedicatedVRAM      = adapterInfo.Memory.LocalMemory;
    m_Capabilities.m_uiDedicatedSystemRAM = adapterInfo.Memory.HostVisibleMemory;
    m_Capabilities.m_uiSharedSystemRAM    = adapterInfo.Memory.UnifiedMemory;
    m_Capabilities.m_bHardwareAccelerated = adapterInfo.Type == Diligent::ADAPTER_TYPE_DISCRETE;
  }

  const Diligent::RenderDeviceInfo& deviceInfo = GetDevice()->GetDeviceInfo();
  {
    m_Capabilities.m_bMultithreadedResourceCreation = deviceInfo.Features.MultithreadedResourceCreation == Diligent::DEVICE_FEATURE_STATE_ENABLED;
    m_Capabilities.m_bB5G6R5Textures                = true; // TODO How to check?
    m_Capabilities.m_bNoOverwriteBufferUpdate       = true; // TODO How to check?

    m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::VertexShader]   = true;
    m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::HullShader]     = true;
    m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::DomainShader]   = true;
    m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::GeometryShader] = adapterInfo.Features.GeometryShaders == Diligent::DEVICE_FEATURE_STATE_ENABLED;
    m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::PixelShader]    = true;
    m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::ComputeShader]  = adapterInfo.Features.ComputeShaders == Diligent::DEVICE_FEATURE_STATE_ENABLED;

    m_Capabilities.m_bStreamOut              = true;
    m_Capabilities.m_bInstancing             = true;
    m_Capabilities.m_b32BitIndices           = true;
    m_Capabilities.m_bIndirectDraw           = true;
    m_Capabilities.m_bTextureArrays          = true;
    m_Capabilities.m_bCubemapArrays          = true;
    m_Capabilities.m_uiMaxConstantBuffers    = XII_GAL_MAX_CONSTANT_BUFFER_COUNT;
    m_Capabilities.m_uiMaxTextureDimension   = static_cast<xiiUInt16>(adapterInfo.Texture.MaxTexture1DDimension);
    m_Capabilities.m_uiMaxCubemapDimension   = static_cast<xiiUInt16>(adapterInfo.Texture.MaxTextureCubeDimension);
    m_Capabilities.m_uiMax3DTextureDimension = static_cast<xiiUInt16>(adapterInfo.Texture.MaxTexture3DDimension);
    m_Capabilities.m_uiMaxRendertargets      = Diligent::MAX_RENDER_TARGETS;
    m_Capabilities.m_bAlphaToCoverage        = true;

    m_Capabilities.m_uiUAVCount                          = 8;
    m_Capabilities.m_uiMaxAnisotropy                     = 16;
    m_Capabilities.m_bVertexShaderRenderTargetArrayIndex = false; // TODO How to check?
  }
}

void xiiGALDeviceDiligent::WaitIdlePlatform()
{
  DestroyDeadObjects();

  m_pDevice->IdleGPU();
}

void xiiGALDeviceDiligent::FillFormatLookupTable()
{
  /// The list below is in the same order as the xiiGALResourceFormat enum. No format should be missing except the ones that are just
  /// different names for the same enum value.

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAFloat, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_FLOAT).VA(Diligent::TEX_FORMAT_RGBA32_FLOAT).RV(Diligent::TEX_FORMAT_RGBA32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUInt, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_UINT).VA(Diligent::TEX_FORMAT_RGBA32_UINT).RV(Diligent::TEX_FORMAT_RGBA32_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAInt, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_SINT).VA(Diligent::TEX_FORMAT_RGBA32_SINT).RV(Diligent::TEX_FORMAT_RGBA32_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBFloat, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_FLOAT).VA(Diligent::TEX_FORMAT_RGB32_FLOAT).RV(Diligent::TEX_FORMAT_RGB32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBUInt, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_UINT).VA(Diligent::TEX_FORMAT_RGB32_UINT).RV(Diligent::TEX_FORMAT_RGB32_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBInt, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_SINT).VA(Diligent::TEX_FORMAT_RGB32_SINT).RV(Diligent::TEX_FORMAT_RGB32_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::B5G6R5UNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_B5G6R5_UNORM).RT(Diligent::TEX_FORMAT_B5G6R5_UNORM).VA(Diligent::TEX_FORMAT_B5G6R5_UNORM).RV(Diligent::TEX_FORMAT_B5G6R5_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BGRAUByteNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BGRA8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRA8_UNORM).VA(Diligent::TEX_FORMAT_BGRA8_UNORM).RV(Diligent::TEX_FORMAT_BGRA8_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BGRAUByteNormalizedsRGB, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BGRA8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAHalf, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_FLOAT).VA(Diligent::TEX_FORMAT_RGBA16_FLOAT).RV(Diligent::TEX_FORMAT_RGBA16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUShort, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_UINT).VA(Diligent::TEX_FORMAT_RGBA16_UINT).RV(Diligent::TEX_FORMAT_RGBA16_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUShortNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_UNORM).VA(Diligent::TEX_FORMAT_RGBA16_UNORM).RV(Diligent::TEX_FORMAT_RGBA16_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAShort, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_SINT).VA(Diligent::TEX_FORMAT_RGBA16_SINT).RV(Diligent::TEX_FORMAT_RGBA16_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAShortNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_SNORM).VA(Diligent::TEX_FORMAT_RGBA16_SNORM).RV(Diligent::TEX_FORMAT_RGBA16_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGFloat, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_FLOAT).VA(Diligent::TEX_FORMAT_RG32_FLOAT).RV(Diligent::TEX_FORMAT_RG32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUInt, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_UINT).VA(Diligent::TEX_FORMAT_RG32_UINT).RV(Diligent::TEX_FORMAT_RG32_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGInt, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_SINT).VA(Diligent::TEX_FORMAT_RG32_SINT).RV(Diligent::TEX_FORMAT_RG32_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGB10A2UInt, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).RT(Diligent::TEX_FORMAT_RGB10A2_UINT).VA(Diligent::TEX_FORMAT_RGB10A2_UINT).RV(Diligent::TEX_FORMAT_RGB10A2_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGB10A2UIntNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).RT(Diligent::TEX_FORMAT_RGB10A2_UNORM).VA(Diligent::TEX_FORMAT_RGB10A2_UNORM).RV(Diligent::TEX_FORMAT_RGB10A2_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RG11B10Float, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R11G11B10_FLOAT).RT(Diligent::TEX_FORMAT_R11G11B10_FLOAT).VA(Diligent::TEX_FORMAT_R11G11B10_FLOAT).RV(Diligent::TEX_FORMAT_R11G11B10_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUByteNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_UNORM).VA(Diligent::TEX_FORMAT_RGBA8_UNORM).RV(Diligent::TEX_FORMAT_RGBA8_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUByteNormalizedsRGB, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB).RV(Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUByte, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_UINT).VA(Diligent::TEX_FORMAT_RGBA8_UINT).RV(Diligent::TEX_FORMAT_RGBA8_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAByteNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_SNORM).VA(Diligent::TEX_FORMAT_RGBA8_SNORM).RV(Diligent::TEX_FORMAT_RGBA8_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAByte, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_SINT).VA(Diligent::TEX_FORMAT_RGBA8_SINT).RV(Diligent::TEX_FORMAT_RGBA8_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGHalf, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_FLOAT).VA(Diligent::TEX_FORMAT_RG16_FLOAT).RV(Diligent::TEX_FORMAT_RG16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUShort, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_UINT).VA(Diligent::TEX_FORMAT_RG16_UINT).RV(Diligent::TEX_FORMAT_RG16_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUShortNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_UNORM).VA(Diligent::TEX_FORMAT_RG16_UNORM).RV(Diligent::TEX_FORMAT_RG16_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGShort, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_SINT).VA(Diligent::TEX_FORMAT_RG16_SINT).RV(Diligent::TEX_FORMAT_RG16_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGShortNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_SNORM).VA(Diligent::TEX_FORMAT_RG16_SNORM).RV(Diligent::TEX_FORMAT_RG16_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUByte, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_UINT).VA(Diligent::TEX_FORMAT_RG8_UINT).RV(Diligent::TEX_FORMAT_RG8_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUByteNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_UNORM).VA(Diligent::TEX_FORMAT_RG8_UNORM).RV(Diligent::TEX_FORMAT_RG8_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGByte, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_SINT).VA(Diligent::TEX_FORMAT_RG8_SINT).RV(Diligent::TEX_FORMAT_RG8_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGByteNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_SNORM).VA(Diligent::TEX_FORMAT_RG8_SNORM).RV(Diligent::TEX_FORMAT_RG8_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::DFloat, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R32_TYPELESS).RV(Diligent::TEX_FORMAT_R32_FLOAT).D(Diligent::TEX_FORMAT_R32_FLOAT).DS(Diligent::TEX_FORMAT_D32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RFloat, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_FLOAT).VA(Diligent::TEX_FORMAT_R32_FLOAT).RV(Diligent::TEX_FORMAT_R32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUInt, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_UINT).VA(Diligent::TEX_FORMAT_R32_UINT).RV(Diligent::TEX_FORMAT_R32_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RInt, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_SINT).VA(Diligent::TEX_FORMAT_R32_SINT).RV(Diligent::TEX_FORMAT_R32_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RHalf, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_FLOAT).VA(Diligent::TEX_FORMAT_R16_FLOAT).RV(Diligent::TEX_FORMAT_R16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUShort, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_UINT).VA(Diligent::TEX_FORMAT_R16_UINT).RV(Diligent::TEX_FORMAT_R16_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUShortNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_UNORM).VA(Diligent::TEX_FORMAT_R16_UNORM).RV(Diligent::TEX_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RShort, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_SINT).VA(Diligent::TEX_FORMAT_R16_SINT).RV(Diligent::TEX_FORMAT_R16_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RShortNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_SNORM).VA(Diligent::TEX_FORMAT_R16_SNORM).RV(Diligent::TEX_FORMAT_R16_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUByte, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_UINT).VA(Diligent::TEX_FORMAT_R8_UINT).RV(Diligent::TEX_FORMAT_R8_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUByteNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_UNORM).VA(Diligent::TEX_FORMAT_R8_UNORM).RV(Diligent::TEX_FORMAT_R8_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RByte, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_SINT).VA(Diligent::TEX_FORMAT_R8_SINT).RV(Diligent::TEX_FORMAT_R8_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RByteNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_SNORM).VA(Diligent::TEX_FORMAT_R8_SNORM).RV(Diligent::TEX_FORMAT_R8_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::AUByteNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_A8_UNORM).RT(Diligent::TEX_FORMAT_A8_UNORM).VA(Diligent::TEX_FORMAT_A8_UNORM).RV(Diligent::TEX_FORMAT_A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::D16, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R16_TYPELESS).RV(Diligent::TEX_FORMAT_R16_UNORM).DS(Diligent::TEX_FORMAT_D16_UNORM).D(Diligent::TEX_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::D24S8, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_R24G8_TYPELESS).DS(Diligent::TEX_FORMAT_D24_UNORM_S8_UINT).D(Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS).S(Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC1, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC1_TYPELESS).RV(Diligent::TEX_FORMAT_BC1_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC1sRGB, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC1_TYPELESS).RV(Diligent::TEX_FORMAT_BC1_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC2, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC2_TYPELESS).RV(Diligent::TEX_FORMAT_BC2_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC2sRGB, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC2_TYPELESS).RV(Diligent::TEX_FORMAT_BC2_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC3, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC3_TYPELESS).RV(Diligent::TEX_FORMAT_BC3_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC3sRGB, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC3_TYPELESS).RV(Diligent::TEX_FORMAT_BC3_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC4UNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC4_TYPELESS).RV(Diligent::TEX_FORMAT_BC4_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC4Normalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC4_TYPELESS).RV(Diligent::TEX_FORMAT_BC4_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC5UNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC5_TYPELESS).RV(Diligent::TEX_FORMAT_BC5_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC5Normalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC5_TYPELESS).RV(Diligent::TEX_FORMAT_BC5_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC6UFloat, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC6H_TYPELESS).RV(Diligent::TEX_FORMAT_BC6H_UF16));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC6Float, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC6H_TYPELESS).RV(Diligent::TEX_FORMAT_BC6H_SF16));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC7UNormalized, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC7_TYPELESS).RV(Diligent::TEX_FORMAT_BC7_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC7UNormalizedsRGB, xiiGALFormatLookupEntryDiligent(Diligent::TEX_FORMAT_BC7_TYPELESS).RV(Diligent::TEX_FORMAT_BC7_UNORM_SRGB));
}

bool xiiGALDeviceDiligent::IsFenceReachedPlatform(Diligent::IDeviceContext* pContext, Diligent::IQuery* pFence)
{
  const Diligent::QueryDesc& queryDesc = pFence->GetDesc();
  switch (queryDesc.Type)
  {
    case Diligent::QUERY_TYPE_OCCLUSION:
    {
      Diligent::QueryDataOcclusion queryData;
      return pFence->GetData(&queryData, sizeof(queryData), false);
    }

    case Diligent::QUERY_TYPE_BINARY_OCCLUSION:
    {
      Diligent::QueryDataBinaryOcclusion queryData;
      return pFence->GetData(&queryData, sizeof(queryData), false);
    }

    case Diligent::QUERY_TYPE_TIMESTAMP:
    {
      Diligent::QueryDataTimestamp queryData;
      return pFence->GetData(&queryData, sizeof(queryData), false);
    }

    case Diligent::QUERY_TYPE_PIPELINE_STATISTICS:
    {
      Diligent::QueryDataPipelineStatistics queryData;
      return pFence->GetData(&queryData, sizeof(queryData), false);
    }

    case Diligent::QUERY_TYPE_DURATION:
    {
      Diligent::QueryDataDuration queryData;
      return pFence->GetData(&queryData, sizeof(queryData), false);
    }
  }

  return false;
}

void xiiGALDeviceDiligent::WaitForFencePlatform(Diligent::IDeviceContext* pContext, Diligent::IQuery* pFence)
{
  const Diligent::QueryDesc& queryDesc = pFence->GetDesc();
  switch (queryDesc.Type)
  {
    case Diligent::QUERY_TYPE_OCCLUSION:
    {
      Diligent::QueryDataOcclusion queryData;
      while (!pFence->GetData(&queryData, sizeof(queryData), false))
      {
        xiiThreadUtils::YieldTimeSlice();
      }
    }
    break;

    case Diligent::QUERY_TYPE_BINARY_OCCLUSION:
    {
      Diligent::QueryDataBinaryOcclusion queryData;
      while (!pFence->GetData(&queryData, sizeof(queryData), false))
      {
        xiiThreadUtils::YieldTimeSlice();
      }
    }
    break;

    case Diligent::QUERY_TYPE_TIMESTAMP:
    {
      Diligent::QueryDataTimestamp queryData;
      while (!pFence->GetData(&queryData, sizeof(queryData), false))
      {
        xiiThreadUtils::YieldTimeSlice();
      }
    }
    break;

    case Diligent::QUERY_TYPE_PIPELINE_STATISTICS:
    {
      Diligent::QueryDataPipelineStatistics queryData;
      while (!pFence->GetData(&queryData, sizeof(queryData), false))
      {
        xiiThreadUtils::YieldTimeSlice();
      }
    }
    break;

    case Diligent::QUERY_TYPE_DURATION:
    {
      Diligent::QueryDataDuration queryData;
      while (!pFence->GetData(&queryData, sizeof(queryData), false))
      {
        xiiThreadUtils::YieldTimeSlice();
      }
    }
    break;
  }
}

Diligent::IBuffer* xiiGALDeviceDiligent::FindTempBuffer(xiiUInt32 uiSize)
{
  const xiiUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  uiSize = xiiMath::Max(uiSize, 256U);
  if (uiSize < uiExpGrowthLimit)
  {
    uiSize = xiiMath::PowerOfTwo_Ceil(uiSize);
  }
  else
  {
    uiSize = xiiMemoryUtils::AlignSize(uiSize, uiExpGrowthLimit);
  }

  Diligent::IBuffer* pBuffer = nullptr;
  auto               it      = m_FreeTempResources[TempResourceType::Buffer].Find(uiSize);
  if (it.IsValid())
  {
    xiiDynamicArray<Diligent::IDeviceObject*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pBuffer = static_cast<Diligent::IBuffer*>(resources[0]);
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pBuffer == nullptr)
  {
    Diligent::BufferDesc bufferDesc;
    bufferDesc.Size              = uiSize;
    bufferDesc.Usage             = Diligent::USAGE_STAGING;
    bufferDesc.BindFlags         = Diligent::BIND_NONE;
    bufferDesc.CPUAccessFlags    = Diligent::CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags         = Diligent::MISC_BUFFER_FLAG_NONE;
    bufferDesc.ElementByteStride = 0;

    Diligent::IBuffer* pBufferNew = nullptr;
    GetDevice()->CreateBuffer(bufferDesc, nullptr, &pBufferNew);

    if (pBufferNew == nullptr)
    {
      return nullptr;
    }

    pBuffer = pBufferNew;
  }

  auto& tempResource       = m_UsedTempResources[TempResourceType::Buffer].ExpandAndGetRef();
  tempResource.m_pResource = pBuffer;
  tempResource.m_uiFrame   = GetImmediateContext()->GetFrameNumber();
  tempResource.m_uiHash    = uiSize;

  return pBuffer;
}

Diligent::ITexture* xiiGALDeviceDiligent::FindTempTexture(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiDepth, xiiGALResourceFormat::Enum format)
{
  xiiUInt32 data[] = {uiWidth, uiHeight, uiDepth, (xiiUInt32)format};
  xiiUInt32 uiHash = xiiHashingUtils::xxHash32(data, sizeof(data));

  Diligent::ITexture* pTexture = nullptr;
  auto                it       = m_FreeTempResources[TempResourceType::Texture].Find(uiHash);
  if (it.IsValid())
  {
    xiiDynamicArray<Diligent::IDeviceObject*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pTexture = static_cast<Diligent::ITexture*>(resources[0]);
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pTexture == nullptr)
  {
    if (uiDepth == 1)
    {
      Diligent::TextureDesc textureDesc;
      textureDesc.Width          = uiWidth;
      textureDesc.Height         = uiHeight;
      textureDesc.MipLevels      = 1;
      textureDesc.ArraySize      = 1;
      textureDesc.Format         = GetFormatLookupTable().GetFormatInfo(format).m_eStorage;
      textureDesc.SampleCount    = 1;
      textureDesc.Usage          = Diligent::USAGE_STAGING;
      textureDesc.BindFlags      = Diligent::BIND_NONE;
      textureDesc.CPUAccessFlags = Diligent::CPU_ACCESS_NONE;
      textureDesc.MiscFlags      = Diligent::MISC_TEXTURE_FLAG_NONE;

      Diligent::ITexture* pTextureNew = nullptr;
      m_pDevice->CreateTexture(textureDesc, nullptr, &pTextureNew);
      if (pTextureNew == nullptr)
      {
        return nullptr;
      }

      pTexture = pTextureNew;
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
      return nullptr;
    }
  }

  auto& tempResource       = m_UsedTempResources[TempResourceType::Texture].ExpandAndGetRef();
  tempResource.m_pResource = pTexture;
  tempResource.m_uiFrame   = GetImmediateContext()->GetFrameNumber();
  tempResource.m_uiHash    = uiHash;

  return pTexture;
}

void xiiGALDeviceDiligent::FreeTempResources(xiiUInt64 uiFrame)
{
  for (xiiUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    while (!m_UsedTempResources[type].IsEmpty())
    {
      auto& usedTempResource = m_UsedTempResources[type].PeekFront();
      if (usedTempResource.m_uiFrame == uiFrame)
      {
        auto it = m_FreeTempResources[type].Find(usedTempResource.m_uiHash);
        if (!it.IsValid())
        {
          it = m_FreeTempResources[type].Insert(usedTempResource.m_uiHash, xiiDynamicArray<Diligent::IDeviceObject*>(&m_Allocator));
        }

        it.Value().PushBack(usedTempResource.m_pResource);
        m_UsedTempResources[type].PopFront();
      }
      else
      {
        break;
      }
    }
  }
}


XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Device_Implementation_DeviceDiligent);
