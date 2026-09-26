/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/Device.h>

#include <Foundation/Profiling/Profiling.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/BottomLevelAS.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/Query.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TopLevelAS.h>
#include <GraphicsFoundation/Shader/Shader.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/States/RasterizerState.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

namespace
{
  static constexpr xiiUInt32 s_uiMaxResourcesInSignature = XII_BIT(16) - 1U;
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDevice, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

#define XII_GAL_DEVICE_CHECK(expression, ...)  \
  do                                           \
  {                                            \
    XII_ASSERT_DEV((expression), __VA_ARGS__); \
    if (!(expression)) { return {}; }          \
  } while (false)

xiiSharedPtr<xiiGALDevice>                   xiiGALDevice::s_pDefaultDevice;
xiiEvent<const xiiGALDeviceEvent&, xiiMutex> xiiGALDevice::s_Events;

xiiGALDevice::xiiGALDevice(xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& creationDescription) :
  xiiGALObject(), m_Description(creationDescription), m_Allocator("GALDevice", pAllocator), m_AllocatorWrapper(&m_Allocator)
{
}

xiiGALDevice::~xiiGALDevice() = default;

xiiResult xiiGALDevice::Initialize()
{
  XII_LOG_BLOCK("xiiGALDevice::Initialize");

  // Initialize platform device.
  XII_SUCCEED_OR_RETURN(InitializePlatform());

  // Fill the device capabilities
  XII_SUCCEED_OR_RETURN(FillCapabilitiesPlatform());

  // Initialize device after platform capabilities have been filled.
  XII_SUCCEED_OR_RETURN(PostInitializePlatform());

  xiiLog::Info("Adapter: '{}' - {} VRAM, {} Sys RAM, {} Shared RAM.", m_AdapterDescription.m_sAdapterName, xiiArgFileSize(m_AdapterDescription.m_MemoryProperties.m_uiLocalMemory),
               xiiArgFileSize(m_AdapterDescription.m_MemoryProperties.m_uiHostVisibleMemory), xiiArgFileSize(m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory));

  if (m_AdapterDescription.m_Type != xiiGALDeviceAdapterType::Discrete)
  {
    xiiLog::Warning("Selected graphics adapter has no hardware acceleration.");
  }

  XII_GAL_DEVICE_LOCK_AND_CHECK();

  xiiProfilingSystem::InitializeGPUData();

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::AfterInitialization;
    s_Events.Broadcast(e);
  }

  return XII_SUCCESS;
}

xiiResult xiiGALDevice::PostInitialize()
{
  return PostInitializePlatform();
}

void xiiGALDevice::BeginFrame()
{
  {
    XII_PROFILE_SCOPE("BeforeBeginFrame");

    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::BeforeBeginFrame;
    s_Events.Broadcast(e);
  }

  {
    XII_GAL_DEVICE_LOCK_AND_CHECK();

    XII_ASSERT_DEV(!m_bBeginFrameCalled, "You must call xiiGALDevice::EndFrame before you can call xiiGALDevice::BeginFrame again");

    m_bBeginFrameCalled = true;

    BeginFramePlatform();
  }

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::AfterBeginFrame;
    s_Events.Broadcast(e);
  }
}

void xiiGALDevice::EndFrame()
{
  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::BeforeEndFrame;
    s_Events.Broadcast(e);
  }

  {
    XII_GAL_DEVICE_LOCK_AND_CHECK();

    XII_ASSERT_DEV(m_bBeginFrameCalled, "You must have called xiiGALDevice::Begin before you can call xiiGALDevice::EndFrame");

    EndFramePlatform();

    m_bBeginFrameCalled = false;
  }

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::AfterEndFrame;
    s_Events.Broadcast(e);
  }
}

xiiSharedPtr<xiiGALSwapChain> xiiGALDevice::CreateSwapChain(const xiiGALSwapChainCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(description.m_pWindow != nullptr, "Swap chain creation failed: Window handle (m_pWindow) is null. A valid window reference is required.");
  XII_GAL_DEVICE_CHECK(description.m_ColorBufferFormat != xiiGALResourceFormat::Unknown, "Swap chain creation failed: Color buffer format is 'Unknown'. Specify a valid format for rendering output.");
  XII_GAL_DEVICE_CHECK(!description.m_UsageFlags.IsNoFlagSet(), "Swap chain creation failed: No usage flags specified. Define at least one usage via m_UsageFlags.");
  XII_GAL_DEVICE_CHECK(description.m_fDefaultDepthValue > 0.0f, "Swap chain creation failed: Default depth value must be greater than zero. Check m_fDefaultDepthValue.");

  return CreateSwapChainPlatform(description);
}

xiiSharedPtr<xiiGALCommandList> xiiGALDevice::CreateCommandList(const xiiGALCommandListCreationDescription& description)
{
  VerifyMultithreadedAccess();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (description.m_Flags.IsSet(xiiGALCommandListFlags::Secondary))
  {
    if (description.m_pRenderPass)
    {
      XII_ASSERT_DEV(description.m_uiSubPassIndex < description.m_pRenderPass->GetDescription().m_SubPasses.GetCount(), "Subpass index is out of bounds for the provided render pass.");
    }
    else
    {
      XII_ASSERT_DEV(description.m_uiSubPassIndex == 0U, "Secondary command lists should specify a zero subpass index if no render pass is specified.");
    }
  }
  else
  {
    XII_ASSERT_DEV(description.m_pRenderPass == nullptr, "Primary command lists should not specify a render pass.");
    XII_ASSERT_DEV(description.m_pFramebuffer == nullptr, "Primary command lists should not specify a framebuffer.");
    XII_ASSERT_DEV(description.m_uiSubPassIndex == 0U, "Primary command lists should not specify a non-zero subpass index.");
  }

  XII_ASSERT_DEV(description.m_QueueFlags != xiiGALCommandQueueFlags::None, "Command list must declare at least one queue capability (Graphics, Compute, Transfer, etc.).");
#endif

  return CreateCommandListPlatform(description);
}

xiiSharedPtr<xiiGALBlendState> xiiGALDevice::CreateBlendState(const xiiGALBlendStateCreationDescription& description)
{
  VerifyMultithreadedAccess();

  for (xiiUInt32 i = 0U; i < description.m_RenderTargets.GetCount(); ++i)
  {
    const xiiGALRenderTargetBlendDescription& rtDescription = description.m_RenderTargets[i];

    const bool bBlendEnable = rtDescription.m_bBlendEnable && (i == 0U || (description.m_bIndependentBlend && i > 0U));

    if (bBlendEnable)
    {
      XII_GAL_DEVICE_CHECK(rtDescription.m_SourceBlend != xiiGALBlendFactor::Undefined, "The source blend must not be xiiGALBlendFactor::Undefined.");
      XII_GAL_DEVICE_CHECK(rtDescription.m_DestinationBlend != xiiGALBlendFactor::Undefined, "The destination blend must not be xiiGALBlendFactor::Undefined.");
      XII_GAL_DEVICE_CHECK(rtDescription.m_BlendOperation != xiiGALBlendOperation::Undefined, "The blend operation must not be xiiGALBlendOperation::Undefined.");

      XII_GAL_DEVICE_CHECK(rtDescription.m_SourceBlendAlpha != xiiGALBlendFactor::Undefined, "The alpha source blend must not be xiiGALBlendFactor::Undefined.");
      XII_GAL_DEVICE_CHECK(rtDescription.m_DestinationBlendAlpha != xiiGALBlendFactor::Undefined, "The alpha destination blend must not be xiiGALBlendFactor::Undefined.");
      XII_GAL_DEVICE_CHECK(rtDescription.m_BlendOperationAlpha != xiiGALBlendOperation::Undefined, "The alpha blend operation must not be xiiGALBlendOperation::Undefined.");
    }
  }

  return CreateBlendStatePlatform(description);
}

xiiSharedPtr<xiiGALDepthStencilState> xiiGALDevice::CreateDepthStencilState(const xiiGALDepthStencilStateCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(!(description.m_bDepthEnable && description.m_ComparisonDepthFunction == xiiGALComparisonFunction::Unknown), "The depth comparison function must not be xiiGALComparisonFunction::Unknown when depth is enabled.");

  if (description.m_bStencilEnable)
  {
    XII_GAL_DEVICE_CHECK(description.m_FrontFace.m_StencilFailOperation != xiiGALStencilOperation::Undefined, "The front face stencil fail operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_GAL_DEVICE_CHECK(description.m_FrontFace.m_StencilDepthFailOperation != xiiGALStencilOperation::Undefined, "The front face stencil depth fail operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_GAL_DEVICE_CHECK(description.m_FrontFace.m_StencilPassOperation != xiiGALStencilOperation::Undefined, "The front face stencil pass operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_GAL_DEVICE_CHECK(description.m_FrontFace.m_ComparisonFunction != xiiGALComparisonFunction::Unknown, "The front face stencil comparison function must not be xiiGALComparisonFunction::Unknown when stencil is enabled.");

    XII_GAL_DEVICE_CHECK(description.m_BackFace.m_StencilFailOperation != xiiGALStencilOperation::Undefined, "The back face stencil fail operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_GAL_DEVICE_CHECK(description.m_BackFace.m_StencilDepthFailOperation != xiiGALStencilOperation::Undefined, "The back face stencil depth fail operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_GAL_DEVICE_CHECK(description.m_BackFace.m_StencilPassOperation != xiiGALStencilOperation::Undefined, "The back face stencil pass operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_GAL_DEVICE_CHECK(description.m_BackFace.m_ComparisonFunction != xiiGALComparisonFunction::Unknown, "The back face stencil comparison function must not be xiiGALComparisonFunction::Unknown when stencil is enabled.");
  }

  return CreateDepthStencilStatePlatform(description);
}

xiiSharedPtr<xiiGALRasterizerState> xiiGALDevice::CreateRasterizerState(const xiiGALRasterizerStateCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(description.m_FillMode != xiiGALFillMode::Undefined, "The fill mode cannot be xiiGALFillMode::Undefined.");
  XII_GAL_DEVICE_CHECK(description.m_CullMode != xiiGALCullMode::Undefined, "The cull mode cannot be xiiGALCullMode::Undefined.");

  return CreateRasterizerStatePlatform(description);
}

xiiSharedPtr<xiiGALShader> xiiGALDevice::CreateShader(const xiiGALShaderCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(description.m_ShaderType != xiiGALShaderType::Unknown, "The shader type must not be xiiGALShaderType::Unknown.");
  XII_GAL_DEVICE_CHECK(description.HasValidByteCode(), "A shader cannot be created with no provided valid shader bytecode.");

  if (description.m_ShaderType == xiiGALShaderType::Geometry)
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_GeometryShaders == xiiGALDeviceFeatureState::Enabled, "Geometry shaders are not supported by this device.");
  }
  if (description.m_ShaderType.IsAnySet(xiiGALShaderType::Domain | xiiGALShaderType::Hull))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_Tessellation == xiiGALDeviceFeatureState::Enabled, "Tessellation shaders are not supported by this device.");
  }
  if (description.m_ShaderType.IsSet(xiiGALShaderType::Compute))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_ComputeShaders == xiiGALDeviceFeatureState::Enabled, "Compute shaders are not supported by this device.");
  }
  if (description.m_ShaderType.IsAnySet(xiiGALShaderType::Amplification | xiiGALShaderType::Mesh))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_MeshShaders == xiiGALDeviceFeatureState::Enabled, "Mesh shaders are not supported by this device.");
  }
  if (description.m_ShaderType.IsAnySet(xiiGALShaderType::AllRayTracing))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_RayTracing == xiiGALDeviceFeatureState::Enabled && m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags.IsSet(xiiGALRayTracingCapabilityFlags::StandaloneShaders), "Standalone ray tracing shaders are not supported by this device.");
  }
  if (description.m_ShaderType.IsSet(xiiGALShaderType::Tile))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_TileShaders == xiiGALDeviceFeatureState::Enabled, "Tile shaders are not supported by this device.");
  }

  return CreateShaderPlatform(description);
}

xiiSharedPtr<xiiGALBuffer> xiiGALDevice::CreateBuffer(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData /* = nullptr*/, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind /*= xiiGALExternalMemoryKind::None*/)
{
  VerifyMultithreadedAccess();

  // Validate buffer description.

  auto allowedBindFlags = xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::IndexBuffer | xiiGALBindFlags::UniformBuffer | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::StreamOutput | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments | xiiGALBindFlags::RayTracing;

  XII_GAL_DEVICE_CHECK((description.m_Usage == xiiGALResourceUsage::Staging && description.m_BindFlags.IsNoFlagSet()) || description.m_BindFlags.IsStrictlyAnySet(allowedBindFlags), "The buffer description bind flags contain unsupported bind flags.");

  if (description.m_BindFlags.IsAnySet(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess))
  {
    XII_GAL_DEVICE_CHECK(description.m_Mode.GetValue() > xiiGALBufferMode::Undefined && description.m_Mode.GetValue() < xiiGALBufferMode::ENUM_COUNT, "The given buffer mode is not a valid buffer mode for a buffer created with xiiGALBindFlags::ShaderResource or xiiGALBindFlags::UnorderedAccess.");

    if (description.m_Mode == xiiGALBufferMode::Structured || description.m_Mode == xiiGALBufferMode::Formatted)
    {
      XII_GAL_DEVICE_CHECK(description.m_uiElementByteStride != 0U, "The element stride must not be zero for structured and formatted buffers.");
    }
    else if (description.m_Mode == xiiGALBufferMode::Raw)
    {
      // Nothing to do.
    }
  }

  if (description.m_BindFlags.IsSet(xiiGALBindFlags::RayTracing))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_RayTracing == xiiGALDeviceFeatureState::Enabled, "xiiGALBindFlags::RayTracing flag cannot be used when the Ray Tracing feature is disabled.");
  }
  if (description.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags.IsSet(xiiGALDrawCommandCapabilityFlags::DrawIndirect), "xiiGALBindFlags::IndirectDrawArguments flag cannot be used when the xiiGALDrawCommandCapabilityFlags::DrawIndirect capability is not supported.");
  }

  switch (description.m_Usage)
  {
    case xiiGALResourceUsage::Immutable:
    case xiiGALResourceUsage::Mutable:
    {
      XII_GAL_DEVICE_CHECK(description.m_CPUAccessFlags.IsNoFlagSet(), "Static and default buffers cannot have any CPU flags set.");
    }
    break;
    case xiiGALResourceUsage::Dynamic:
    {
      XII_GAL_DEVICE_CHECK(description.m_CPUAccessFlags == xiiGALCPUAccessFlag::Write, "Dynamic buffers require the xiiGALCPUAccessFlag::Write flag.");
    }
    break;
    case xiiGALResourceUsage::Staging:
    {
      XII_GAL_DEVICE_CHECK(description.m_CPUAccessFlags == xiiGALCPUAccessFlag::Write || description.m_CPUAccessFlags == xiiGALCPUAccessFlag::Read, "Exactly one of xiiGALCPUAccessFlag::Write or xiiGALCPUAccessFlag::Read must be specified for a staging buffer.");
      XII_GAL_DEVICE_CHECK(description.m_BindFlags.IsNoFlagSet(), "Staging buffers cannot be bound to any part of the graphics pipeline and cannot have any bind flags set.");
    }
    break;
    case xiiGALResourceUsage::Unified:
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory != 0U, "Unified memory is not present in this device. Check the amount of unified memory in the device adapter information before creating unified buffers.");
      XII_GAL_DEVICE_CHECK(description.m_CPUAccessFlags.IsAnyFlagSet(), "At least one of xiiGALCPUAccessFlag::Write or xiiGALCPUAccessFlag::Read must be specified for a unified buffer.");

      if (description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
      {
        XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags.IsAnySet(xiiGALCPUAccessFlag::Write), "Unified memory on this device does not support write access. Check for available access flags in the device properties before creating unified buffers.");
      }
      if (description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read))
      {
        XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags.IsAnySet(xiiGALCPUAccessFlag::Read), "Unified memory on this device does not support read access. Check for available access flags in the device properties before creating unified buffers.");
      }
    }
    break;
    case xiiGALResourceUsage::Sparse:
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_SparseResources == xiiGALDeviceFeatureState::Enabled, "Sparse buffer requires the Sparse Resources device feature to be enabled.");
      XII_GAL_DEVICE_CHECK(description.m_CPUAccessFlags.IsNoFlagSet(), "Sparse buffers cannot have any CPU access flags set.");
      XII_GAL_DEVICE_CHECK(description.m_uiSize <= m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize, "Sparse buffer size ({0}), must not exceed the Resource Space Size ({1}).", description.m_uiSize, m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize);
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.IsSet(xiiGALSparseResourceCapabilityFlags::Buffer), "Sparse buffer requires the xiiGALSparseResourceCapabilityFlags::Buffer capability.");

      if (description.m_MiscFlags.IsSet(xiiGALMiscBufferFlags::SparseAlias))
      {
        XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.IsSet(xiiGALSparseResourceCapabilityFlags::Aliased), "xiiGALMiscBufferFlags::SparseAlias flag requires the xiiGALSparseResourceCapabilityFlags::Aliased capability.");
      }
      XII_GAL_DEVICE_CHECK(description.m_BindFlags.IsStrictlyAnySet(m_AdapterDescription.m_SparseResourceProperties.m_BindFlags), "The buffer description bind flags contain unsupported bind flags.");
    }
    break;

    default:
      xiiLog::Error("Unknown resource usage given.");
      return {};
  }

  if (description.m_Usage == xiiGALResourceUsage::Dynamic)
  {
    const bool bNeedsBackingResource = (description.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess) || description.m_Mode == xiiGALBufferMode::Formatted);
    XII_GAL_DEVICE_CHECK(!bNeedsBackingResource, "xiiGALResourceUsage::Dynamic buffers that use the Unordered Access flag or Formatted mode requires an internal backing resource. "
                                                 "This resource is implicitly transitioned by the device context and thus cannot be relied upon to be safely used in multiple contexts. Create a xiiGALResourceUsage::Dynamic buffer "
                                                 "without the xiiGALResourceUsage::UnorderedAccess flag and use xiiGALResourceUsage::Undefined mode and copy the contents to a xiiGALResourceUsage::Mutable buffer with required flags, "
                                                 "which can be shared between device contexts.");
  }

  if (description.m_Usage != xiiGALResourceUsage::Sparse)
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation == 0U || description.m_uiSize <= m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation, "Non-sparse buffer size ({0}) must not exceed the maximum allocation size ({1}).", description.m_uiSize, m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation);
    XII_GAL_DEVICE_CHECK(description.m_MiscFlags.AreNoneSet(xiiGALMiscBufferFlags::SparseAlias), "Miscellaneous flags must not have xiiGALMiscBufferFlags::SparseAlias if the buffer usage is not xiiGALResourceUsage::Sparse.");
  }

  // Validate buffer initial data.

  const bool bHasInitialData = (pInitialData != nullptr && pInitialData->m_pData != nullptr);

  if (description.m_Usage == xiiGALResourceUsage::Immutable && !bHasInitialData)
  {
    XII_GAL_DEVICE_CHECK(false, "The initial data must not be nullptr, as immutable buffers must be initialized at creation time.");
  }
  if (description.m_Usage == xiiGALResourceUsage::Dynamic && bHasInitialData)
  {
    XII_GAL_DEVICE_CHECK(false, "The initial data must be nullptr for dynamic buffers.");
  }
  if (description.m_Usage == xiiGALResourceUsage::Sparse && bHasInitialData)
  {
    XII_GAL_DEVICE_CHECK(false, "The initial data must be nullptr for sparse buffers.");
  }

  if (description.m_Usage == xiiGALResourceUsage::Staging)
  {
    if (description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
    {
      XII_GAL_DEVICE_CHECK(bHasInitialData, "Staging buffers with CPU write access must be updated via map.");
    }
  }
  else if (description.m_Usage == xiiGALResourceUsage::Unified)
  {
    XII_GAL_DEVICE_CHECK(description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write) && bHasInitialData, "xiiGALCPUAccessFlag::Write is required to initialize a unified buffer.");
  }

  if (pInitialData != nullptr && pInitialData->m_pCommandList != nullptr)
  {
    XII_GAL_DEVICE_CHECK(pInitialData->m_pCommandList->GetDescription().m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer | xiiGALCommandQueueFlags::SparseBinding), "Cannot initialize the buffer with the given command list queue flags. Only Graphics, Transfer, and Sparse Binding queues are supported.");
  }

  if (bHasInitialData)
  {
    XII_GAL_DEVICE_CHECK(pInitialData->m_uiDataSize >= description.m_uiSize, "The buffer initial data size ({0}) must be larger or equal to the buffer size ({1}).", pInitialData->m_uiDataSize, description.m_uiSize);
  }

  if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_ExternalMemory == xiiGALDeviceFeatureState::Enabled, "External memory kind flags cannot be used when the External Memory feature is disabled.");
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_NativeFence == xiiGALDeviceFeatureState::Enabled, "External memory kind flags require the Native Fence feature to be enabled.");
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_ExternalSemaphore == xiiGALDeviceFeatureState::Enabled, "External memory kind flags require the External Semaphore feature to be enabled.");
  }

  xiiSharedPtr<xiiGALBuffer> pBuffer = CreateBufferPlatform(description, pInitialData, externalMemoryKind);

  FinalizeBufferInternal(description, pBuffer);

  return pBuffer;
}

void xiiGALDevice::FinalizeBufferInternal(const xiiGALBufferCreationDescription& description, xiiSharedPtr<xiiGALBuffer>& pBuffer)
{
  XII_IGNORE_UNUSED(description);

  if (pBuffer != nullptr)
  {
    pBuffer->CreateDefaultResourceViews();
  }
}

xiiSharedPtr<xiiGALTexture> xiiGALDevice::CreateTexture(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData /* = nullptr*/, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind /*= xiiGALExternalMemoryKind::None*/)
{
  VerifyMultithreadedAccess();

  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(description.m_Format);

  // Validate texture description.

  XII_GAL_DEVICE_CHECK(description.m_Type != xiiGALResourceDimension::Undefined, "The texture resource dimension is undefined.");
  XII_GAL_DEVICE_CHECK(description.m_Type > xiiGALResourceDimension::Undefined && description.m_Type < xiiGALResourceDimension::ENUM_COUNT, "The texture resource dimension is invalid.");
  XII_GAL_DEVICE_CHECK(description.m_Size.width != 0U, "The texture width cannot be zero.");

  if (description.m_Type == xiiGALResourceDimension::Texture1D || description.m_Type == xiiGALResourceDimension::Texture1DArray)
  {
    if (description.m_Size.height != formatProperties.m_uiBlockHeight)
    {
      if (formatProperties.m_uiBlockHeight == 1U)
      {
        XII_GAL_DEVICE_CHECK(false, "The texture height ({0}) of a Texture1D or Texture1DArray must be equal to 1.");
      }
      else
      {
        XII_GAL_DEVICE_CHECK(false, "For block-compressed formats, the height ({0}) of a Texture1D or Texture1DArray must be equal to the compressed block height ({1}).", description.m_Size.height, formatProperties.m_uiBlockHeight);
      }
    }
  }
  else
  {
    XII_GAL_DEVICE_CHECK(description.m_Size.height != 0U, "The texture height cannot be zero.");
  }

  XII_GAL_DEVICE_CHECK(description.m_Type != xiiGALResourceDimension::Texture3D || description.m_uiArraySizeOrDepth != 0U, "A 3D texture depth cannot be zero.");

  if (description.m_Type == xiiGALResourceDimension::Texture1D || description.m_Type == xiiGALResourceDimension::Texture2D)
  {
    XII_GAL_DEVICE_CHECK(description.m_uiArraySizeOrDepth == 1U, "A Texture1D or Texture2D must have 1 array slice, ({0}) provided. Use Texture1DArray or Texture2DArray if more than one slice is needed.", description.m_uiArraySizeOrDepth);
  }

  if (description.m_Type == xiiGALResourceDimension::TextureCube || description.m_Type == xiiGALResourceDimension::TextureCubeArray)
  {
    XII_GAL_DEVICE_CHECK(description.m_Size.width == description.m_Size.height, "For TextureCube or TextureCubeArray textures, the width ({0} provided) must match the height ({1} provided).", description.m_Size.width, description.m_Size.height);
    XII_GAL_DEVICE_CHECK(description.m_uiArraySizeOrDepth >= 6U, "For TextureCube or TextureCubeArray textures, a minimum of 6 slices must be given ({0} provided).", description.m_uiArraySizeOrDepth);
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  {
    xiiUInt32 uiMaxDimension = 0;
    if (description.Is1D())
      uiMaxDimension = description.m_Size.width;
    else if (description.Is2D())
      uiMaxDimension = xiiMath::Max(description.m_Size.width, description.m_Size.height);
    else if (description.Is3D())
      uiMaxDimension = xiiMath::Max(xiiMath::Max(description.m_Size.width, description.m_Size.height), description.m_uiArraySizeOrDepth);

    XII_GAL_DEVICE_CHECK((uiMaxDimension >= XII_BIT(description.m_uiMipLevels - 1)), "The texture has an incorrect number of Mip levels ({0}).", description.m_uiMipLevels);
  }
#endif

  if (description.m_uiSampleCount > 1U)
  {
    XII_GAL_DEVICE_CHECK(xiiMath::IsPowerOf2(description.m_uiSampleCount), "The texture sample count must be a power of two.");
    XII_GAL_DEVICE_CHECK(description.m_Type == xiiGALResourceDimension::Texture2D || description.m_Type == xiiGALResourceDimension::Texture2DArray, "Only Texture2D and Texture2DArray can be multi-sampled.");
    XII_GAL_DEVICE_CHECK(description.m_uiMipLevels == 1U, "Multi-sampled textures must have one mip level ({0} levels specified).", description.m_uiMipLevels);
    XII_GAL_DEVICE_CHECK(description.m_BindFlags.AreNoneSet(xiiGALBindFlags::UnorderedAccess), "xiiGALBindFlags::UnorderedAccess is not allowed for multi-sampled resources.");
  }

  if (description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Memoryless))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags.IsAnyFlagSet(), "Memoryless textures are not supported by this device.");
    XII_GAL_DEVICE_CHECK(description.m_BindFlags.IsStrictlyAnySet(m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags), "Unsupported bind flags given for memoryless textures.");
    XII_GAL_DEVICE_CHECK(description.m_Usage == xiiGALResourceUsage::Mutable, "Memoryless attachment requires xiiGALResourceUsage::Mutable.");
    XII_GAL_DEVICE_CHECK(description.m_CPUAccessFlags.IsNoFlagSet(), "Memoryless attachment requires xiiGALCPUAccessFlags::None.");
    XII_GAL_DEVICE_CHECK(description.m_MiscFlags.AreNoneSet(xiiGALMiscTextureFlags::GenerateMips), "Memoryless attachment is not compatible with mip map generation.");
  }

  if (description.m_Usage == xiiGALResourceUsage::Staging)
  {
    XII_GAL_DEVICE_CHECK(description.m_BindFlags.IsNoFlagSet(), "Staging textures cannot be bound to any GPU pipeline stage.");
    XII_GAL_DEVICE_CHECK(!description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::GenerateMips), "Mipmaps cannot be automatically generated for staging textures.");
    XII_GAL_DEVICE_CHECK(description.m_CPUAccessFlags.IsAnyFlagSet(), "Staging textures must specify the CPU access flags.");
    XII_GAL_DEVICE_CHECK(description.m_CPUAccessFlags.IsStrictlyAnySet(xiiGALCPUAccessFlag::Read) || description.m_CPUAccessFlags.IsStrictlyAnySet(xiiGALCPUAccessFlag::Write), "Staging textures must use exactly one of xiiGALCPUAccessFlags::Read or xiiGALCPUAccessFlags::Write.");
  }
  else if (description.m_Usage == xiiGALResourceUsage::Unified)
  {
    XII_GAL_DEVICE_CHECK(false, "xiiGALResourceUsage::Unified textures are currently not supported.");
  }

  if (description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Subsampled))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_VariableRateShading == xiiGALDeviceFeatureState::Enabled, "xiiGALMiscTextureFlags::Subsampled requires the Variable Shading Rate device feature.");
    XII_GAL_DEVICE_CHECK(m_Description.m_GraphicsDeviceType != xiiGALGraphicsDeviceType::Metal, "xiiGALMiscTextureFlags::Subsampled is unsupported in Metal. Use IRasterizationRateMapMtl to implement Variable Rate Shading in Metal.");
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget), "xiiGALMiscTextureFlags::Subsampled requires the xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget capability.");
    XII_GAL_DEVICE_CHECK(!description.m_BindFlags.AreAllSet(xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil), "Subsampled textures must use one of xiiGALBindFlags::RenderTarget or xiiGALBindFlags::DepthStencil bind flags.");
    XII_GAL_DEVICE_CHECK(!description.m_BindFlags.IsSet(xiiGALBindFlags::ShadingRate), "xiiGALMiscTextureFlags::Subsampled is not compatible with xiiGALBindFlags::ShadingRate.");
  }

  if (description.m_BindFlags.IsSet(xiiGALBindFlags::ShadingRate))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_VariableRateShading == xiiGALDeviceFeatureState::Enabled, "xiiGALBindFlags::ShadingRate requires the Variable Shading Rate device feature.");
    XII_GAL_DEVICE_CHECK(m_Description.m_GraphicsDeviceType != xiiGALGraphicsDeviceType::Metal, "xiiGALBindFlags::ShadingRate is unsupported in Metal. Use IRasterizationRateMapMtl to implement Variable Rate Shading in Metal.");
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureBased), "xiiGALBindFlags::ShadingRate requires the xiiGALShadingRateCapabilityFlags::TextureBased capability.");
    XII_GAL_DEVICE_CHECK(description.m_uiSampleCount == 1U, "xiiGALBindFlags::ShadingRate is not allowed for multi-sampled textures.");

    if (description.m_Type == xiiGALResourceDimension::Texture2DArray && description.m_uiArraySizeOrDepth > 1U)
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureArray), "Shading rate texture arrays require the xiiGALShadingRateCapabilityFlags::TextureArray capability.");
    }

    XII_GAL_DEVICE_CHECK(description.m_Usage == xiiGALResourceUsage::Mutable || description.m_Usage == xiiGALResourceUsage::Immutable, "Shading rate textures only allow xiiGALResourceUsage::Mutable or xiiGALResourceUsage::Immutable.");

    XII_GAL_DEVICE_CHECK(description.m_uiMipLevels == 1U, "Shading rate textures must have a single mip level.");
    XII_GAL_DEVICE_CHECK(description.m_uiMipLevels != 1U, "Shading rate textures must have a single mip level."); // For Direct3D12 and Vulkan with VK_EXT_fragment_density_map
    XII_GAL_DEVICE_CHECK(description.m_BindFlags.IsStrictlyAnySet(m_AdapterDescription.m_ShadingRateProperties.m_BindFlags), "Unsupported bind flags are specified for the shading rate texture.");

    /// \todo GraphicsFoundation: Vulkan allows the creation of 2D texture arrays, and using a single slice for view even if xiiGALShadingRateCapabilityFlags::TextureArray is not supported by the device.
    if (description.m_Type != xiiGALResourceDimension::Texture2D && !(description.m_Type == xiiGALResourceDimension::Texture2DArray && m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureArray)))
    {
      XII_GAL_DEVICE_CHECK(false, "Shading rate texture must be Texture2D or Texture2DArray with the xiiGALShadingRateCapabilityFlags::TextureArray capability.");
    }

    switch (m_AdapterDescription.m_ShadingRateProperties.m_Format)
    {
      case xiiGALShadingRateFormat::Palette:
      {
        XII_GAL_DEVICE_CHECK(description.m_Format == xiiGALResourceFormat::R8UInt, "The shading rate texture format must be xiiGALResourceFormat::R8UInt.");
      }
      break;
      case xiiGALShadingRateFormat::RG8UNormalized:
      {
        XII_GAL_DEVICE_CHECK(description.m_Format == xiiGALResourceFormat::R8UNormalized, "The shading rate texture format must be xiiGALResourceFormat::R8UNormalized.");
      }
      break;

      case xiiGALShadingRateFormat::ColumnRowFloat32:
      default:
        XII_GAL_DEVICE_CHECK(false, "The shading rate texture is not supported.");
    }
  }

  if (description.m_Usage == xiiGALResourceUsage::Sparse)
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_SparseResources == xiiGALDeviceFeatureState::Enabled, "Sparse texture requires the Sparse Resources device feature.");

    if (description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::SparseAlias))
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.AreNoneSet(xiiGALSparseResourceCapabilityFlags::Aliased), "xiiGALMiscTextureFlags::SparseAlias flag requires the xiiGALSparseResourceCapabilityFlags::Aliased capability.");
    }

    switch (description.m_Type)
    {
      case xiiGALResourceDimension::Texture2D:
      {
        XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.IsSet(xiiGALSparseResourceCapabilityFlags::Texture2D), "Texture2D requires the xiiGALSparseResourceCapabilityFlags::Texture2D capability.");
      }
      break;
      case xiiGALResourceDimension::Texture2DArray:
      case xiiGALResourceDimension::TextureCube:
      case xiiGALResourceDimension::TextureCubeArray:
      {
        XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.IsSet(xiiGALSparseResourceCapabilityFlags::Texture2D), "Texture2DArray, Texture2DCube and TextureCubeArray requires the xiiGALSparseResourceCapabilityFlags::Texture2D capability.");

        if (m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.AreNoneSet(xiiGALSparseResourceCapabilityFlags::Texture2DArrayMipTail))
        {
          /// \todo GraphicsFoundation: Validate texture mip size to the tile size property.
        }
      }
      break;
      case xiiGALResourceDimension::Texture3D:
      {
        XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.IsSet(xiiGALSparseResourceCapabilityFlags::Texture3D), "Texture3D requires the xiiGALSparseResourceCapabilityFlags::Texture3D capability.");
      }
      break;
      case xiiGALResourceDimension::Texture1D:
      case xiiGALResourceDimension::Texture1DArray:
      {
        XII_GAL_DEVICE_CHECK(false, "Texture1D and Texture1DArray sparse textures are not supported.");
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }
  else
  {
    XII_GAL_DEVICE_CHECK(description.m_MiscFlags.AreNoneSet(xiiGALMiscTextureFlags::SparseAlias), "The miscellaneous flags must not have xiiGALMiscTextureFlags::SparseAlias if the usage is not xiiGALResourceUsage::Sparse.");
  }

  if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_ExternalMemory == xiiGALDeviceFeatureState::Enabled, "External memory kind flags cannot be used when the External Memory feature is disabled.");
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_NativeFence == xiiGALDeviceFeatureState::Enabled, "External memory kind flags require the Native Fence feature to be enabled.");
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_ExternalSemaphore == xiiGALDeviceFeatureState::Enabled, "External memory kind flags require the External Semaphore feature to be enabled.");
  }

  xiiTemporaryHybridArray<xiiGALTextureSubResourceData, 2U> subresourceData;
  xiiTemporaryArray<xiiUInt8>                               zeroData;
  xiiGALTextureData                                         textureData;

  if (pInitialData != nullptr)
  {
    if (pInitialData->m_pCommandList != nullptr)
    {
      XII_GAL_DEVICE_CHECK(pInitialData->m_pCommandList->GetDescription().m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer | xiiGALCommandQueueFlags::SparseBinding), "Cannot initialize the texture with the given command list queue flags. Only Graphics, Transfer, and Sparse Binding queues are supported.");
    }

    if (pInitialData->m_pSubResources.IsEmpty())
    {
      textureData  = xiiGALTextureUtilities::GetZeroMemoryInitialData(description, subresourceData, zeroData);
      pInitialData = &textureData;
    }

    const xiiUInt32 uiExpectedSubResourceCount = xiiGALTextureUtilities::GetSubResourceCount(description);
    XII_GAL_DEVICE_CHECK(pInitialData->m_pSubResources.GetCount() == uiExpectedSubResourceCount, "The number of provided initial data sub-resources ({0}) does not match the expected number of sub-resources ({1}).", pInitialData->m_pSubResources.GetCount(), uiExpectedSubResourceCount);

    xiiUInt32 uiMipLevel = 0U;
    for (const xiiGALTextureSubResourceData& subResource : pInitialData->m_pSubResources)
    {
      XII_GAL_DEVICE_CHECK(!subResource.m_pData.IsEmpty(), "A sub-resource initial data pointer is empty.");

      const xiiUInt32 uiRequiredRowPitch = xiiGALTextureUtilities::GetRequiredRowPitch(description, uiMipLevel);
      XII_GAL_DEVICE_CHECK(subResource.m_uiStride >= uiRequiredRowPitch, "The sub-resource row pitch ({0}) is smaller than the required row pitch ({1}).", subResource.m_uiStride, uiRequiredRowPitch);

      const xiiUInt32 uiRequiredSlicePitch = xiiGALTextureUtilities::GetRequiredSlicePitch(description, uiMipLevel);
      XII_GAL_DEVICE_CHECK(subResource.m_uiDepthStride >= uiRequiredSlicePitch, "The sub-resource slice pitch ({0}) is smaller than the required slice pitch ({1}).", subResource.m_uiDepthStride, uiRequiredSlicePitch);

      ++uiMipLevel;
    }
  }

  xiiSharedPtr<xiiGALTexture> pTexture = CreateTexturePlatform(description, pInitialData, externalMemoryKind);

  FinalizeTextureInternal(description, pTexture);

  return pTexture;
}

void xiiGALDevice::FinalizeTextureInternal(const xiiGALTextureCreationDescription& description, xiiSharedPtr<xiiGALTexture>& pTexture)
{
  XII_IGNORE_UNUSED(description);

  if (pTexture != nullptr)
  {
    pTexture->CreateDefaultResourceViews();
  }
}

xiiSharedPtr<xiiGALSampler> xiiGALDevice::CreateSampler(const xiiGALSamplerCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_LOCK(m_Mutex);

  const xiiUInt32 uiDescriptionHash = description.CalculateHash();
  if (auto it = m_SamplerCache.Find(uiDescriptionHash); it.IsValid())
  {
    for (xiiGALSampler* pCachedSampler : it.Value())
    {
      if (pCachedSampler->GetDescription() == description)
      {
        return xiiSharedPtr<xiiGALSampler>(pCachedSampler, &m_Allocator);
      }
    }
  }

  if (description.m_Flags.AreAllSet(xiiGALSamplerFlags::Subsampled | xiiGALSamplerFlags::SubsampledCoarseReconstruction))
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget), "Subsampled sampler requires the xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget capability.");
  }
  if (description.m_bUnormalizedCoords)
  {
    XII_GAL_DEVICE_CHECK(m_Description.m_GraphicsDeviceType == xiiGALGraphicsDeviceType::Vulkan || m_Description.m_GraphicsDeviceType == xiiGALGraphicsDeviceType::Metal, "Unnormalized coordinates are only supported in Vulkan and Metal.");
    XII_GAL_DEVICE_CHECK(description.m_MinFilter == description.m_MagFilter, "When unnormalized coordinates are enabled, the MipFilter must equal the MagFilter.");
    XII_GAL_DEVICE_CHECK(description.m_MipFilter == xiiGALFilterType::Point, "When unnormalized coordinates are enabled, the MipFilter must be xiiGALFilterType::Point.");
    XII_GAL_DEVICE_CHECK(description.m_AddressU == xiiGALTextureAddressMode::Clamp || description.m_AddressU == xiiGALTextureAddressMode::Border, "When unnormalized coordinates are enabled, the AddressU must be xiiGALTextureAddressMode::Clamp or xiiGALTextureAddressMode::Border.");
    XII_GAL_DEVICE_CHECK(description.m_AddressV == xiiGALTextureAddressMode::Clamp || description.m_AddressV == xiiGALTextureAddressMode::Border, "When unnormalized coordinates are enabled, the AddressV must be xiiGALTextureAddressMode::Clamp or xiiGALTextureAddressMode::Border.");
    XII_GAL_DEVICE_CHECK(!xiiGALFilterType::IsComparisonFilter(description.m_MinFilter), "When unnormalized coordinates are enabled, the MinFilter and MagFilter must not be of the comparison type.");
    XII_GAL_DEVICE_CHECK(!xiiGALFilterType::IsAnisotropicFilter(description.m_MinFilter), "When unnormalized coordinates are enabled, the MinFilter and MagFilter must not be of the anisotropic type.");
  }

  xiiSharedPtr<xiiGALSampler> pSampler = CreateSamplerPlatform(description);
  if (pSampler != nullptr)
  {
    m_SamplerCache[uiDescriptionHash].PushBack(pSampler.Borrow());
  }
  return pSampler;
}

void xiiGALDevice::UnregisterSampler(xiiUInt32 uiDescriptionHash, const xiiGALSampler* pSampler)
{
  XII_LOCK(m_Mutex);

  auto it = m_SamplerCache.Find(uiDescriptionHash);
  if (!it.IsValid())
    return;

  if (it.Value().RemoveAndCopy(const_cast<xiiGALSampler*>(pSampler)) && it.Value().IsEmpty())
  {
    m_SamplerCache.Remove(it);
  }
}

xiiSharedPtr<xiiGALQuery> xiiGALDevice::CreateQuery(const xiiGALQueryCreationDescription& description)
{
  VerifyMultithreadedAccess();

  switch (description.m_Type)
  {
    case xiiGALQueryType::Occlusion:
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_OcclusionQueries == xiiGALDeviceFeatureState::Enabled, "Occlusion queries are not supported by this device.");
    }
    break;
    case xiiGALQueryType::BinaryOcclusion:
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_BinaryOcclusionQueries == xiiGALDeviceFeatureState::Enabled, "Binary occlusion queries are not supported by this device.");
    }
    break;
    case xiiGALQueryType::Timestamp:
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_TimestampQueries == xiiGALDeviceFeatureState::Enabled, "Timestamp queries are not supported by this device.");
    }
    break;
    case xiiGALQueryType::PipelineStatistics:
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_PipelineStatisticsQueries == xiiGALDeviceFeatureState::Enabled, "Pipeline statistics queries are not supported by this device.");
    }
    break;
    case xiiGALQueryType::Duration:
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_DurationQueries == xiiGALDeviceFeatureState::Enabled, "Duration queries are not supported by this device.");
    }
    break;

    default:
      XII_GAL_DEVICE_CHECK(false, "Unexpected query type.");
  }

  return CreateQueryPlatform(description);
}

xiiSharedPtr<xiiGALFence> xiiGALDevice::CreateFence(const xiiGALFenceCreationDescription& description)
{
  VerifyMultithreadedAccess();

  switch (description.m_Type)
  {
    case xiiGALFenceType::CpuWaitOnly:
      break;
    case xiiGALFenceType::General:
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_NativeFence == xiiGALDeviceFeatureState::Enabled, "xiiGALFenceType::General requires the Native Fence device feature.");
    }
    break;

    default:
      XII_GAL_DEVICE_CHECK(false, "Unexpected query type.");
  }

  return CreateFencePlatform(description);
}

xiiSharedPtr<xiiGALRenderPass> xiiGALDevice::CreateRenderPass(const xiiGALRenderPassCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(description.m_SubPasses.GetCount() > 0U, "The sub pass count must be greater than 0.");

  const bool bIsVulkanDevice = m_Description.m_GraphicsDeviceType == xiiGALGraphicsDeviceType::Vulkan;

  for (xiiUInt32 uiAttachmentIndex = 0U; uiAttachmentIndex < description.m_Attachments.GetCount(); ++uiAttachmentIndex)
  {
    const xiiGALRenderPassAttachmentDescription& attachment = description.m_Attachments[uiAttachmentIndex];

    XII_GAL_DEVICE_CHECK(attachment.m_Format != xiiGALResourceFormat::Unknown, "The format of attachment {0} is unknown.", uiAttachmentIndex);
    XII_GAL_DEVICE_CHECK(attachment.m_uiSampleCount != 0U, "The sample count of attachment {0} is zero.", uiAttachmentIndex);
    XII_GAL_DEVICE_CHECK(xiiMath::IsPowerOf2(attachment.m_uiSampleCount), "The sample count ({0}) of attachment {1} is not a power of 2.", attachment.m_uiSampleCount, uiAttachmentIndex);

    const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(attachment.m_Format);
    if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Depth || formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
    {
      XII_GAL_DEVICE_CHECK(attachment.m_InitialStateFlags.IsStrictlyAnySet(xiiGALResourceStateFlags::DepthWrite | xiiGALResourceStateFlags::DepthRead | xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::ResolveDestination | xiiGALResourceStateFlags::ResolveSource | xiiGALResourceStateFlags::CopyDestination | xiiGALResourceStateFlags::CopySource | xiiGALResourceStateFlags::InputAttachment | xiiGALResourceStateFlags::Undefined) || (bIsVulkanDevice && attachment.m_InitialStateFlags.IsSet(xiiGALResourceStateFlags::Common)),
                           "The initial state of the depth-stencil attachment {0} is invalid.", uiAttachmentIndex);

      XII_GAL_DEVICE_CHECK(attachment.m_FinalStateFlags.IsStrictlyAnySet(xiiGALResourceStateFlags::DepthWrite | xiiGALResourceStateFlags::DepthRead | xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::ResolveDestination | xiiGALResourceStateFlags::ResolveSource | xiiGALResourceStateFlags::CopyDestination | xiiGALResourceStateFlags::CopySource | xiiGALResourceStateFlags::InputAttachment) || (bIsVulkanDevice && attachment.m_FinalStateFlags.IsSet(xiiGALResourceStateFlags::Common)),
                           "The final state of the depth-stencil attachment {0} is invalid.", uiAttachmentIndex);
    }
    else
    {
      XII_GAL_DEVICE_CHECK(attachment.m_InitialStateFlags.IsStrictlyAnySet(xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::ResolveDestination | xiiGALResourceStateFlags::ResolveSource | xiiGALResourceStateFlags::CopyDestination | xiiGALResourceStateFlags::CopySource | xiiGALResourceStateFlags::InputAttachment | xiiGALResourceStateFlags::Present | xiiGALResourceStateFlags::ShadingRate | xiiGALResourceStateFlags::Undefined) || (bIsVulkanDevice && attachment.m_InitialStateFlags.IsSet(xiiGALResourceStateFlags::Common)),
                           "The initial state of the color attachment {0} is invalid.", uiAttachmentIndex);

      XII_GAL_DEVICE_CHECK(attachment.m_FinalStateFlags.IsStrictlyAnySet(xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::ResolveDestination | xiiGALResourceStateFlags::ResolveSource | xiiGALResourceStateFlags::CopyDestination | xiiGALResourceStateFlags::CopySource | xiiGALResourceStateFlags::InputAttachment | xiiGALResourceStateFlags::Present | xiiGALResourceStateFlags::ShadingRate) || (bIsVulkanDevice && attachment.m_FinalStateFlags.IsSet(xiiGALResourceStateFlags::Common)),
                           "The final state of the color attachment {0} is invalid.", uiAttachmentIndex);
    }
  }

  const xiiGALShadingRateAttachmentDescription* pShadingRateAttachment = nullptr;
  for (xiiUInt32 uiSubPassIndex = 0U; uiSubPassIndex < description.m_SubPasses.GetCount(); ++uiSubPassIndex)
  {
    const xiiGALSubPassDescription& subpass = description.m_SubPasses[uiSubPassIndex];

    for (xiiUInt32 uiInputAttachmentIndex = 0U; uiInputAttachmentIndex < subpass.m_InputAttachments.GetCount(); ++uiInputAttachmentIndex)
    {
      const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_InputAttachments[uiInputAttachmentIndex];

      if (attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
        continue;

      // If the attachment member of any element of Input Attachment, Color Attachment, Resolve Attachment or Depth Stencil attachment, or any element of Preserve Attachments in any element of
      // the sub pass is not XII_GAL_ATTACHMENT_UNUSED, it must be less than the attachment count.
      // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-attachment-00834
      XII_GAL_DEVICE_CHECK(attachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The attachment index ({0}) of the input attachment reference {1} of sub pass {2} must be less than the number of attachments ({3}).", attachmentReference.m_uiAttachmentIndex, uiInputAttachmentIndex, uiSubPassIndex, description.m_Attachments.GetCount());
      XII_GAL_DEVICE_CHECK(attachmentReference.m_ResourceStateFlags == xiiGALResourceStateFlags::InputAttachment || (bIsVulkanDevice && attachmentReference.m_ResourceStateFlags == xiiGALResourceStateFlags::Common), "The attachment with index {0} referenced as an input attachment in sub pass {1} must be in {2} state.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex, (bIsVulkanDevice ? "xiiGALResourceStateFlags::InputAttachment or xiiGALResourceStateFlags::Common" : "xiiGALResourceStateFlags::InputAttachment"));
    }

    for (xiiUInt32 uiColorAttachmentIndex = 0U; uiColorAttachmentIndex < subpass.m_RenderTargetAttachments.GetCount(); ++uiColorAttachmentIndex)
    {
      const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_RenderTargetAttachments[uiColorAttachmentIndex];

      if (attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
        continue;

      // If the attachment member of any element of Input Attachment, Color Attachment, Resolve Attachment or Depth Stencil attachment, or any element of Preserve Attachments in any element of
      // the sub pass is not XII_GAL_ATTACHMENT_UNUSED, it must be less than the attachment count.
      // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-attachment-00834
      XII_GAL_DEVICE_CHECK(attachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The attachment index ({0}) of the render target attachment reference {1} of sub pass {2} must be less than the number of attachments ({3}).", attachmentReference.m_uiAttachmentIndex, uiColorAttachmentIndex, uiSubPassIndex, description.m_Attachments.GetCount());
      XII_GAL_DEVICE_CHECK(attachmentReference.m_ResourceStateFlags == xiiGALResourceStateFlags::RenderTarget || (bIsVulkanDevice && attachmentReference.m_ResourceStateFlags == xiiGALResourceStateFlags::Common), "The attachment with index {0} referenced as an input attachment in sub pass {1} must be in {2} state.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex, (bIsVulkanDevice ? "xiiGALResourceStateFlags::RenderTarget or xiiGALResourceStateFlags::Common" : "xiiGALResourceStateFlags::RenderTarget"));

      const xiiEnum<xiiGALResourceFormat>&   format             = description.m_Attachments[attachmentReference.m_uiAttachmentIndex].m_Format;
      const xiiGALResourceFormatDescription& rtFormatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(format);
      XII_GAL_DEVICE_CHECK(rtFormatProperties.m_ComponentType != xiiGALResourceFormatComponentType::Depth && rtFormatProperties.m_ComponentType != xiiGALResourceFormatComponentType::DepthStencil && rtFormatProperties.m_ComponentType != xiiGALResourceFormatComponentType::Compressed, "Attachment with index {0} referenced as a render target attachment in sub pass {1} uses format {2}, which is not a valid render target format.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex, xiiArgEnum(format));
    }

    if (!subpass.m_ResolveAttachments.IsEmpty())
    {
      for (xiiUInt32 uiResolveAttachmentIndex = 0U; uiResolveAttachmentIndex < subpass.m_ResolveAttachments.GetCount(); ++uiResolveAttachmentIndex)
      {
        const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_RenderTargetAttachments[uiResolveAttachmentIndex];

        if (attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
          continue;

        // If the attachment member of any element of Input Attachment, Color Attachment, Resolve Attachment or Depth Stencil attachment, or any element of Preserve Attachments in any element of
        // the sub pass is not XII_GAL_ATTACHMENT_UNUSED, it must be less than the attachment count.
        // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-attachment-00834
        XII_GAL_DEVICE_CHECK(attachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The attachment index ({0}) of the resolve attachment reference {1} of sub pass {2} must be less than the number of attachments ({3}).", attachmentReference.m_uiAttachmentIndex, uiResolveAttachmentIndex, uiSubPassIndex, description.m_Attachments.GetCount());
      }
    }

    if (!subpass.m_DepthStencilAttachment.IsEmpty())
    {
      for (xiiUInt32 uiDepthResolveAttachmentIndex = 0U; uiDepthResolveAttachmentIndex < subpass.m_DepthStencilAttachment.GetCount(); ++uiDepthResolveAttachmentIndex)
      {
        const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_DepthStencilAttachment[uiDepthResolveAttachmentIndex];

        if (attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
          continue;

        // If the attachment member of any element of Input Attachment, Color Attachment, Resolve Attachment or Depth Stencil attachment, or any element of Preserve Attachments in any element of
        // the sub pass is not XII_GAL_ATTACHMENT_UNUSED, it must be less than the attachment count.
        // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-attachment-00834
        XII_GAL_DEVICE_CHECK(attachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The attachment index ({0}) of the depth-stencil attachment reference of sub pass {1} must be less than the number of attachments ({3}).", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex, description.m_Attachments.GetCount());
        XII_GAL_DEVICE_CHECK(attachmentReference.m_ResourceStateFlags == xiiGALResourceStateFlags::DepthRead || attachmentReference.m_ResourceStateFlags == xiiGALResourceStateFlags::DepthWrite || (bIsVulkanDevice && attachmentReference.m_ResourceStateFlags == xiiGALResourceStateFlags::Common), "The attachment with index ({0}) of the depth-stencil attachment reference of sub pass {1} must be must be in {2} state.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex, (bIsVulkanDevice ? "xiiGALResourceStateFlags::DepthRead or xiiGALResourceStateFlags::DepthWrite or xiiGALResourceStateFlags::Common" : "xiiGALResourceStateFlags::DepthRead or xiiGALResourceStateFlags::DepthWrite"));

        const xiiEnum<xiiGALResourceFormat>&   format                = description.m_Attachments[attachmentReference.m_uiAttachmentIndex].m_Format;
        const xiiGALResourceFormatDescription& depthFormatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(format);
        XII_GAL_DEVICE_CHECK(depthFormatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Depth || depthFormatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil, "Attachment with index {0} referenced as a depth-stencil attachment in sub pass {1} uses format {2}, which is not a valid depth buffer format.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex, xiiArgEnum(format));
      }
    }

    if (!subpass.m_DepthResolveAttachment.IsEmpty())
    {
      for (xiiUInt32 uiDepthResolveAttachmentIndex = 0U; uiDepthResolveAttachmentIndex < subpass.m_DepthResolveAttachment.GetCount(); ++uiDepthResolveAttachmentIndex)
      {
        const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_DepthResolveAttachment[uiDepthResolveAttachmentIndex].m_Attachment;

        if (attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
          continue;

        // If the attachment member of any element of Input Attachment, Color Attachment, Resolve Attachment or Depth Stencil attachment, or any element of Preserve Attachments in any element of
        // the sub pass is not XII_GAL_ATTACHMENT_UNUSED, it must be less than the attachment count.
        // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-attachment-00834
        XII_GAL_DEVICE_CHECK(attachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The attachment index ({0}) of the depth-stencil resolve attachment reference {1} of sub pass {2} must be less than the number of attachments ({3}).", attachmentReference.m_uiAttachmentIndex, uiDepthResolveAttachmentIndex, uiSubPassIndex, description.m_Attachments.GetCount());
        XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_DepthStencilResolve == xiiGALDeviceFeatureState::Enabled, "Depth resolve attachment in sub pass {0} requires the DepthStencilResolve device feature.", uiSubPassIndex);
      }
    }

    for (xiiUInt32 uiPreserveAttachmentIndex = 0U; uiPreserveAttachmentIndex < subpass.m_PreserveAttachments.GetCount(); ++uiPreserveAttachmentIndex)
    {
      const xiiUInt32& attachmentReference = subpass.m_PreserveAttachments[uiPreserveAttachmentIndex];

      XII_GAL_DEVICE_CHECK(attachmentReference != XII_GAL_ATTACHMENT_UNUSED, "The attachment index of preserve attachment reference {0} of sub pass {1} is XII_GAL_ATTACHMENT_UNUSED.");
      XII_GAL_DEVICE_CHECK(attachmentReference < description.m_Attachments.GetCount(), "The attachment index ({0}) of the preserve attachment reference {1} of sub pass {2} must be less than the number of attachments ({3}).", attachmentReference, uiPreserveAttachmentIndex, uiSubPassIndex, description.m_Attachments.GetCount());
    }

    if (!subpass.m_ResolveAttachments.IsEmpty())
    {
      for (xiiUInt32 uiColorAttachmentIndex = 0U; uiColorAttachmentIndex < subpass.m_RenderTargetAttachments.GetCount(); ++uiColorAttachmentIndex)
      {
        const xiiGALAttachmentReferenceDescription& attachmentReference       = subpass.m_RenderTargetAttachments[uiColorAttachmentIndex];
        const xiiGALAttachmentReferenceDescription& resolveAttacmentReference = subpass.m_ResolveAttachments[uiColorAttachmentIndex];

        if (resolveAttacmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
        {
          // If pResolveAttachments is not NULL, for each resolve attachment that is not VK_ATTACHMENT_UNUSED, the corresponding color attachment must not be VK_ATTACHMENT_UNUSED.
          // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkSubpassDescription-pResolveAttachments-00847
          XII_GAL_DEVICE_CHECK(false, "The resolve attachment of sub pass {0} is not unused but the render target attachment reference {1} is unused.", uiSubPassIndex, uiColorAttachmentIndex);
        }

        if (resolveAttacmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && description.m_Attachments[attachmentReference.m_uiAttachmentIndex].m_uiSampleCount == 1U)
        {
          // If pResolveAttachments is not NULL, for each resolve attachment that is not VK_ATTACHMENT_UNUSED, the corresponding color attachment must not have a sample count of VK_SAMPLE_COUNT_1_BIT.
          // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkSubpassDescription-pResolveAttachments-00848
          XII_GAL_DEVICE_CHECK(false, "The render target attachment at index {0} referenced by attachment reference {1} of sub pass {2} is used as the source of a resolve operation, but its sample count is 1.", attachmentReference.m_uiAttachmentIndex, uiColorAttachmentIndex, uiSubPassIndex);
        }

        if (resolveAttacmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && description.m_Attachments[resolveAttacmentReference.m_uiAttachmentIndex].m_uiSampleCount != 1U)
        {
          // If pResolveAttachments is not NULL, each resolve attachment that is not VK_ATTACHMENT_UNUSED must have a sample count of VK_SAMPLE_COUNT_1_BIT.
          // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkSubpassDescription-pResolveAttachments-00849
          XII_GAL_DEVICE_CHECK(false, "The resolve attachment at index {0} referenced by attachment reference {1} of sub pass {2} must have a sample count of 1.", resolveAttacmentReference.m_uiAttachmentIndex, uiColorAttachmentIndex, uiSubPassIndex);
        }

        if (resolveAttacmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && attachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && description.m_Attachments[attachmentReference.m_uiAttachmentIndex].m_Format != description.m_Attachments[resolveAttacmentReference.m_uiAttachmentIndex].m_Format)
        {
          // If pResolveAttachments is not NULL, each resolve attachment that is not VK_ATTACHMENT_UNUSED must have the same VkFormat as its corresponding color attachment.
          // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkSubpassDescription-pResolveAttachments-00850
          XII_GAL_DEVICE_CHECK(false, "The format ({0}) of render target attachment at index {1} referenced by attachment reference {2} of sub pass {3} does not match the format ({4}) of the corresponding resolve attachment at index {5}.", xiiArgEnum(description.m_Attachments[attachmentReference.m_uiAttachmentIndex].m_Format), attachmentReference.m_uiAttachmentIndex, uiColorAttachmentIndex, uiSubPassIndex, xiiArgEnum(description.m_Attachments[resolveAttacmentReference.m_uiAttachmentIndex].m_Format), resolveAttacmentReference.m_uiAttachmentIndex);
        }
      }
    }

    if (!subpass.m_DepthResolveAttachment.IsEmpty())
    {
      for (xiiUInt32 uiDepthStencilAttachmentIndex = 0U; uiDepthStencilAttachmentIndex < subpass.m_DepthStencilAttachment.GetCount(); ++uiDepthStencilAttachmentIndex)
      {
        const xiiGALAttachmentReferenceDescription& depthStencilAttachmentReference = subpass.m_DepthStencilAttachment[uiDepthStencilAttachmentIndex];
        const xiiGALAttachmentReferenceDescription& depthResolveAttacmentReference  = subpass.m_DepthResolveAttachment[uiDepthStencilAttachmentIndex].m_Attachment;

        if (depthResolveAttacmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && depthStencilAttachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
        {
          XII_GAL_DEVICE_CHECK(false, "The depth-stencil resolve attachment of sub pass {0} is not unused but the depth-stencil attachment reference {1} is unused.", uiSubPassIndex, uiDepthStencilAttachmentIndex);
        }

        if (depthResolveAttacmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && description.m_Attachments[depthStencilAttachmentReference.m_uiAttachmentIndex].m_uiSampleCount == 1U)
        {
          XII_GAL_DEVICE_CHECK(false, "The depth-stencil attachment at index {0} referenced by attachment reference {1} of sub pass {2} is used as the source of a resolve operation, but its sample count is 1.", depthStencilAttachmentReference.m_uiAttachmentIndex, uiDepthStencilAttachmentIndex, uiSubPassIndex);
        }

        if (depthResolveAttacmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && description.m_Attachments[depthResolveAttacmentReference.m_uiAttachmentIndex].m_uiSampleCount != 1U)
        {
          XII_GAL_DEVICE_CHECK(false, "The depth-stencil resolve attachment at index {0} referenced by attachment reference {1} of sub pass {2} must have a sample count of 1.", depthResolveAttacmentReference.m_uiAttachmentIndex, uiDepthStencilAttachmentIndex, uiSubPassIndex);
        }

        if (depthResolveAttacmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && depthStencilAttachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && description.m_Attachments[depthStencilAttachmentReference.m_uiAttachmentIndex].m_Format != description.m_Attachments[depthResolveAttacmentReference.m_uiAttachmentIndex].m_Format)
        {
          XII_GAL_DEVICE_CHECK(false, "The format ({0}) of depth-stencil attachment at index {1} referenced by attachment reference {2} of sub pass {3} does not match the format ({4}) of the corresponding depth-stencil resolve attachment at index {5}.", xiiArgEnum(description.m_Attachments[depthStencilAttachmentReference.m_uiAttachmentIndex].m_Format), depthStencilAttachmentReference.m_uiAttachmentIndex, uiDepthStencilAttachmentIndex, uiSubPassIndex, xiiArgEnum(description.m_Attachments[depthResolveAttacmentReference.m_uiAttachmentIndex].m_Format), depthResolveAttacmentReference.m_uiAttachmentIndex);
        }
      }
    }

    if (!subpass.m_ShadingRateAttachment.IsEmpty())
    {
      pShadingRateAttachment                                          = pShadingRateAttachment == nullptr ? subpass.m_ShadingRateAttachment.GetData() : pShadingRateAttachment;
      const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_ShadingRateAttachment[0].m_AttachmentReference;

      if (attachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED)
      {
        XII_GAL_DEVICE_CHECK(attachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The attachment index ({0}) of the shading rate attachment reference of sub pass {1} must be less than the number of attachments ({3}).", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex, description.m_Attachments.GetCount());
        XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_VariableRateShading == xiiGALDeviceFeatureState::Enabled, "The sub pass at index {0} uses a shading rate attachment, but the Variable Shading Rate device feature is not enabled.", uiSubPassIndex);
        XII_GAL_DEVICE_CHECK(attachmentReference.m_ResourceStateFlags == xiiGALResourceStateFlags::ShadingRate, "The attachment with index {0} referenced as a shading rate attachment in sub pass {1} must be in the xiiGALResourceStateFlags::ShadingRate state.");

        const xiiSizeU32& tileSize    = subpass.m_ShadingRateAttachment[0].m_TileSize;
        const xiiSizeU32& minTileSize = m_AdapterDescription.m_ShadingRateProperties.m_MinTileSize;
        const xiiSizeU32& maxTileSize = m_AdapterDescription.m_ShadingRateProperties.m_MaxTileSize;
        if (tileSize.HasNonZeroArea())
        {
          XII_GAL_DEVICE_CHECK(tileSize.width >= minTileSize.width && tileSize.width <= maxTileSize.width, "The sub pass at index {0} uses a shading rate attachment with tile width {1} that is not in te allowed range [{2},{3}]. Check MinTileSize/MaxTileSize members of the Shading Rate Properties.", uiSubPassIndex, tileSize.width, minTileSize.width, maxTileSize.height);
          XII_GAL_DEVICE_CHECK(tileSize.height >= minTileSize.height && tileSize.height <= maxTileSize.height, "The sub pass at index {0} uses a shading rate attachment with tile height {1} that is not in te allowed range [{2},{3}]. Check MinTileSize/MaxTileSize members of the Shading Rate Properties.", uiSubPassIndex, tileSize.height, minTileSize.height, maxTileSize.height);

          // The tile size is only used for Vulkan shading rate and current hardware only supports aspect ratio of 1.
          /// \todo GraphicsFoundation: Use VkPhysicalDeviceFragmentShadingRatePropertiesKHR::maxFragmentShadingRateAttachmentTexelSizeAspectRatio.
          XII_GAL_DEVICE_CHECK(tileSize.width == tileSize.height, "The sub pass at index {0} uses shading rate attachment with tile width {1} that is not equal to the tile height {2}.", uiSubPassIndex, tileSize.width, tileSize.height);
          XII_GAL_DEVICE_CHECK(xiiMath::IsPowerOf2(tileSize.width) && xiiMath::IsPowerOf2(tileSize.height), "The sub pass at index {0} uses a shading rate attachment with tile sizes {1}x{2} that are not a power of two.", uiSubPassIndex, tileSize.width, tileSize.height);
        }
      }
    }
  }

  if (pShadingRateAttachment != nullptr && m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::SameTextureForWholeRenderPass))
  {
    for (xiiUInt32 uiSubPassIndex = 0U; uiSubPassIndex < description.m_SubPasses.GetCount(); ++uiSubPassIndex)
    {
      const xiiGALSubPassDescription& subpass = description.m_SubPasses[uiSubPassIndex];

      XII_GAL_DEVICE_CHECK(!subpass.m_ShadingRateAttachment.IsEmpty(), "Render pass uses a shading rate attachment, but sub pass {0} uses no shading rate attachment. A device with the xiiGALShadingRateCapabilityFlags::SameTextureForWholeRenderPass capability requires that all sub passes of a render pass use the same shading rate attachment.", uiSubPassIndex);
      XII_GAL_DEVICE_CHECK(subpass.m_ShadingRateAttachment == xiiMakeArrayPtr(pShadingRateAttachment, 1U), "The shading rate attachment in sub pass {0} does not match the shading rate attachment used by the previous sub passes. A device with the xiiGALShadingRateCapabilityFlags::SameTextureForWholeRenderPass capability requires that all sub passes of a render pass use the same shading rate attachment.", uiSubPassIndex);
    }
  }

  for (xiiUInt32 uiDependencyIndex = 0U; uiDependencyIndex < description.m_Dependencies.GetCount(); ++uiDependencyIndex)
  {
    const xiiGALSubPassDependencyDescription& dependency = description.m_Dependencies[uiDependencyIndex];

    XII_GAL_DEVICE_CHECK(dependency.m_SourceStageFlags != xiiGALPipelineStageFlags::Undefined, "The source stage mask of subpass dependency {} is undefined.", uiDependencyIndex);
    XII_GAL_DEVICE_CHECK(dependency.m_DestinationStageFlags != xiiGALPipelineStageFlags::Undefined, "The destination stage mask of subpass dependency {} is undefined.", uiDependencyIndex);
  }

  return CreateRenderPassPlatform(description);
}

xiiSharedPtr<xiiGALFramebuffer> xiiGALDevice::CreateFramebuffer(const xiiGALFramebufferCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(description.m_pRenderPass != nullptr, "The render pass handle is invalid.");

  for (xiiUInt32 i = 0U; i < description.m_Attachments.GetCount(); ++i)
  {
    const xiiSharedPtr<xiiGALTextureView>& pAttachment = description.m_Attachments[i];

    // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, and attachmentCount is not 0, pAttachments must be a valid pointer to an array of attachmentCount valid VkImageView handles.
    // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-flags-02778
    XII_GAL_DEVICE_CHECK(pAttachment != nullptr, "The framebuffer attachment at index {0} is invalid.", i);
  }

  const xiiGALRenderPassCreationDescription& renderPassDescription = description.m_pRenderPass->GetDescription();

  XII_GAL_DEVICE_CHECK(description.m_Attachments.GetCount() == renderPassDescription.m_Attachments.GetCount(), "The number of framebuffer attachments ({0}) must be equal to the number of attachments ({1}) in the render pass.", description.m_Attachments.GetCount(), renderPassDescription.m_Attachments.GetCount());

  for (xiiUInt32 uiAttachmentIndex = 0U; uiAttachmentIndex < renderPassDescription.m_Attachments.GetCount(); ++uiAttachmentIndex)
  {
    const xiiGALRenderPassAttachmentDescription& attachmentDescription = renderPassDescription.m_Attachments[uiAttachmentIndex];
    const xiiGALTextureViewCreationDescription&  viewDescription       = description.m_Attachments[uiAttachmentIndex]->GetDescription();
    const xiiGALTextureCreationDescription&      textureDescription    = description.m_Attachments[uiAttachmentIndex]->GetTexture()->GetDescription();

    // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments must have been created with a VkFormat value that matches the VkFormat specified by the corresponding VkAttachmentDescription in renderPass.
    // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-00880
    XII_GAL_DEVICE_CHECK(viewDescription.m_Format == textureDescription.m_Format, "The format ({0}) of attachment {1} does not match the format ({2}) defined by the render pass for the same attachment.", xiiArgEnum(viewDescription.m_Format), uiAttachmentIndex, xiiArgEnum(textureDescription.m_Format));

    // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments must have been created with a samples value that matches the samples value specified by the corresponding VkAttachmentDescription in renderPass
    // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-00881
    XII_GAL_DEVICE_CHECK(textureDescription.m_uiSampleCount == attachmentDescription.m_uiSampleCount, "The sample count ({0}) of attachment {1} does not match the sample count ({2}) defined by the render pass for the same attachment.", textureDescription.m_uiSampleCount, uiAttachmentIndex, attachmentDescription.m_uiSampleCount);

    if (textureDescription.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Memoryless))
    {
      const bool bHasStencilComponent = xiiGALTextureUtilities::GetResourceFormatProperties(attachmentDescription.m_Format).m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil;

      XII_GAL_DEVICE_CHECK(attachmentDescription.m_LoadOperation != xiiGALAttachmentLoadOperation::Load && !(bHasStencilComponent && attachmentDescription.m_StencilLoadOperation == xiiGALAttachmentLoadOperation::Load), "Memoryless attachment {i} is not compatible with xiiGALAttachmentLoadOperation::Load.", uiAttachmentIndex);
      XII_GAL_DEVICE_CHECK(attachmentDescription.m_StencilStoreOperation != xiiGALAttachmentStoreOperation::Store && !(bHasStencilComponent && attachmentDescription.m_StencilStoreOperation == xiiGALAttachmentStoreOperation::Store), "Memoryless attachment {i} is not compatible with xiiGALAttachmentStoreOperation::Store.", uiAttachmentIndex);

#if XII_ENABLED(XII_PLATFORM_OSX)
      {
        xiiUInt32 uiSubpassCount = 0U;

        for (xiiUInt32 j = 0; j < renderPassDescription.m_SubPasses.GetCount(); ++j)
        {
          const xiiGALSubPassDescription& subpass = renderPassDescription.m_SubPasses[j];

          bool bUsedInSubPass = false;
          for (xiiUInt32 k = 0; k < subpass.m_RenderTargetAttachments.GetCount(); ++k)
          {
            const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_RenderTargetAttachments[k];

            if (attachmentReference.m_uiAttachmentIndex == uiAttachmentIndex)
            {
              bUsedInSubPass = true;
            }
          }
          for (xiiUInt32 k = 0; k < subpass.m_InputAttachments.GetCount(); ++k)
          {
            const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_InputAttachments[k];

            if (attachmentReference.m_uiAttachmentIndex == uiAttachmentIndex)
            {
              bUsedInSubPass = true;
            }
          }
          if (!subpass.m_DepthStencilAttachment.IsEmpty() && subpass.m_DepthStencilAttachment[0].m_uiAttachmentIndex == uiAttachmentIndex)
          {
            bUsedInSubPass = true;
          }
          if (bUsedInSubPass)
          {
            ++uiSubpassCount;
          }
        }

        XII_GAL_DEVICE_CHECK(uiSubpassCount <= 1U, "Memoryless attachment {0} is used in more than one sub pass, which is not supported on MacOS/iOS as the contents of the attachment cannot be preserved between sub passes without storing it in global memory.", uiAttachmentIndex);
      }
#endif
    }
  }

  for (xiiUInt32 uiSubPassIndex = 0U; uiSubPassIndex < renderPassDescription.m_SubPasses.GetCount(); ++uiSubPassIndex)
  {
    const xiiGALSubPassDescription& subpass = renderPassDescription.m_SubPasses[uiSubPassIndex];

    for (xiiUInt32 uiInputAttachmentIndex = 0U; uiInputAttachmentIndex < subpass.m_InputAttachments.GetCount(); ++uiInputAttachmentIndex)
    {
      const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_InputAttachments[uiInputAttachmentIndex];

      if (attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
        continue;

      XII_GAL_DEVICE_CHECK(attachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The input attachment index ({0}) at {1} must be less than the attachment count ({2}).", attachmentReference.m_uiAttachmentIndex, uiInputAttachmentIndex, description.m_Attachments.GetCount());

      const xiiSharedPtr<xiiGALTextureView>& pAttachment = description.m_Attachments[attachmentReference.m_uiAttachmentIndex];
      XII_GAL_DEVICE_CHECK(pAttachment != nullptr, "The attachment at index {0} is used as an input attachment by sub pass {1} of render pass and must be valid.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);

      const xiiGALTextureCreationDescription& textureDescription = pAttachment->GetTexture()->GetDescription();

      // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments that is used as an input attachment by renderPass must have been created with a usage value including VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT.
      // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-00879
      XII_GAL_DEVICE_CHECK(textureDescription.m_BindFlags.IsSet(xiiGALBindFlags::InputAttachment), "The attachment at index {1} is used as an input attachment in sub pass {2} of render pass, but was not created with the xiiGALBindFlags::InputAttachment bind flag.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);
    }

    for (xiiUInt32 uiColorAttachmentIndex = 0U; uiColorAttachmentIndex < subpass.m_RenderTargetAttachments.GetCount(); ++uiColorAttachmentIndex)
    {
      const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_RenderTargetAttachments[uiColorAttachmentIndex];

      if (attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
        continue;

      XII_GAL_DEVICE_CHECK(attachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The render target attachment index ({0}) at {1} must be less than the attachment count ({2}).", attachmentReference.m_uiAttachmentIndex, uiColorAttachmentIndex, description.m_Attachments.GetCount());

      const xiiSharedPtr<xiiGALTextureView>& pAttachment = description.m_Attachments[attachmentReference.m_uiAttachmentIndex];
      XII_GAL_DEVICE_CHECK(pAttachment != nullptr, "The attachment at index {0} is used as a render target attachment by sub pass {1} of render pass and must be valid.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);

      const xiiGALTextureViewCreationDescription& viewDescription = pAttachment->GetDescription();
      XII_GAL_DEVICE_CHECK(viewDescription.m_ViewType == xiiGALTextureViewType::RenderTarget, "The attachment at index {0} is used as a render target attachment by sub pass {1} of render pass, but is not of the xiiGALTextureViewType::RenderTarget type.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);

      const xiiGALTextureCreationDescription& textureDescription = pAttachment->GetTexture()->GetDescription();

      // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments that is used as a color attachment or resolve attachment by renderPass must have been created with a usage value including VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT.
      // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-00877
      XII_GAL_DEVICE_CHECK(textureDescription.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget), "The attachment at index {0} is used as a render target attachment by sub pass {1} of render pass, but was not created with the xiiGALBindFlags::RenderTarget bind flag.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);
    }

    for (xiiUInt32 uiResolveAttachmentIndex = 0U; uiResolveAttachmentIndex < subpass.m_ResolveAttachments.GetCount(); ++uiResolveAttachmentIndex)
    {
      const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_ResolveAttachments[uiResolveAttachmentIndex];

      if (attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
        continue;

      XII_GAL_DEVICE_CHECK(attachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The resolve attachment index ({0}) at {1} must be less than the attachment count ({2}).", attachmentReference.m_uiAttachmentIndex, uiResolveAttachmentIndex, description.m_Attachments.GetCount());

      const xiiSharedPtr<xiiGALTextureView>& pAttachment = description.m_Attachments[attachmentReference.m_uiAttachmentIndex];
      XII_GAL_DEVICE_CHECK(pAttachment != nullptr, "The attachment at index {0} is used as a resolve attachment by sub pass {1} of render pass and must be valid.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);

      const xiiGALTextureViewCreationDescription& viewDescription = pAttachment->GetDescription();
      XII_GAL_DEVICE_CHECK(viewDescription.m_ViewType == xiiGALTextureViewType::RenderTarget, "The attachment at index {0} is used as a resolve attachment by sub pass {1} of render pass, but is not of the xiiGALTextureViewType::RenderTarget type.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);

      const xiiGALTextureCreationDescription& textureDescription = pAttachment->GetTexture()->GetDescription();

      // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments that is used as a color attachment or resolve attachment by renderPass must have been created with a usage value including VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT.
      // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-00877
      XII_GAL_DEVICE_CHECK(textureDescription.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget), "The attachment at index {0} is used as a resolve attachment by sub pass {1} of render pass, but was not created with the xiiGALBindFlags::RenderTarget bind flag.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);
    }

    if (!subpass.m_DepthStencilAttachment.IsEmpty())
    {
      const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_DepthStencilAttachment[0];

      if (attachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED)
      {
        XII_GAL_DEVICE_CHECK(attachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The depth-stencil attachment index ({0}) must be less than the attachment count ({1}).", attachmentReference.m_uiAttachmentIndex, description.m_Attachments.GetCount());

        const xiiSharedPtr<xiiGALTextureView>& pAttachment = description.m_Attachments[attachmentReference.m_uiAttachmentIndex];
        XII_GAL_DEVICE_CHECK(pAttachment != nullptr, "The attachment at index {0} is used as a depth-stencil attachment by sub pass {1} of render pass and must be valid.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);

        const xiiGALTextureViewCreationDescription& viewDescription = pAttachment->GetDescription();
        XII_GAL_DEVICE_CHECK(viewDescription.m_ViewType == xiiGALTextureViewType::DepthStencil, "The attachment at index {0} is used as a depth-stencil attachment by sub pass {1} of render pass, but is not of the xiiGALTextureViewType::DepthStencil type.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);

        const xiiGALTextureCreationDescription& textureDescription = pAttachment->GetTexture()->GetDescription();

        // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments that is used as a depth/stencil attachment by renderPass must have been created with a usage value including VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT.
        // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-02633
        XII_GAL_DEVICE_CHECK(textureDescription.m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil), "The attachment at index {0} is used as a depth-stencil attachment by sub pass {1} of render pass, but was not created with the xiiGALBindFlags::DepthStencil bind flag.", attachmentReference.m_uiAttachmentIndex, uiSubPassIndex);
      }
    }
  }

  bool bIsVRSEnabled = false;
  for (xiiUInt32 uiSubPassIndex = 0U; uiSubPassIndex < renderPassDescription.m_SubPasses.GetCount(); ++uiSubPassIndex)
  {
    const xiiGALSubPassDescription& subpass = renderPassDescription.m_SubPasses[uiSubPassIndex];

    if (!subpass.m_ShadingRateAttachment.IsEmpty())
    {
      const xiiGALShadingRateAttachmentDescription& attachmentReference = subpass.m_ShadingRateAttachment[0];
      if (attachmentReference.m_AttachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED)
      {
        XII_GAL_DEVICE_CHECK(attachmentReference.m_AttachmentReference.m_uiAttachmentIndex < description.m_Attachments.GetCount(), "The shading rate attachment index ({0}) must be less than the attachment count ({1}).", attachmentReference.m_AttachmentReference.m_uiAttachmentIndex, description.m_Attachments.GetCount());

        const xiiSharedPtr<xiiGALTextureView>& pAttachment = description.m_Attachments[attachmentReference.m_AttachmentReference.m_uiAttachmentIndex];
        XII_GAL_DEVICE_CHECK(pAttachment != nullptr, "The attachment at index {0} is used as a shading rate attachment by sub pass {1} of render pass and must be valid.", attachmentReference.m_AttachmentReference.m_uiAttachmentIndex, uiSubPassIndex);

        const xiiGALTextureViewCreationDescription& viewDescription = pAttachment->GetDescription();
        XII_GAL_DEVICE_CHECK(viewDescription.m_ViewType == xiiGALTextureViewType::ShadingRate, "The attachment at index {0} is used as a shading rate attachment by sub pass {1} of render pass, but is not of the xiiGALTextureViewType::ShadingRate type.", attachmentReference.m_AttachmentReference.m_uiAttachmentIndex, uiSubPassIndex);

        const xiiGALTextureCreationDescription& textureDescription = pAttachment->GetTexture()->GetDescription();

        // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments that is used as a depth/stencil attachment by renderPass must have been created with a usage value including VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT.
        // Link: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-02633
        XII_GAL_DEVICE_CHECK(textureDescription.m_BindFlags.IsSet(xiiGALBindFlags::ShadingRate), "The attachment at index {0} is used as a shading rate attachment by sub pass {1} of render pass, but was not created with the xiiGALBindFlags::ShadingRate bind flag.", attachmentReference.m_AttachmentReference.m_uiAttachmentIndex, uiSubPassIndex);

        bIsVRSEnabled = true;
      }
    }
  }

  if (bIsVRSEnabled && !m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget))
  {
    XII_GAL_DEVICE_CHECK(!m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget), "One of xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget or xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget capabilities must be presented if texture based variable rate shading is supported.");

    for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < renderPassDescription.m_Attachments.GetCount(); ++uiAttachmentIndex)
    {
      const xiiSharedPtr<xiiGALTextureView>& pAttachmentReference = description.m_Attachments[uiAttachmentIndex];

      if (pAttachmentReference->GetDescription().m_ViewType == xiiGALTextureViewType::ShadingRate)
        continue;

      const xiiGALTextureCreationDescription& textureDescription = pAttachmentReference->GetTexture()->GetDescription();

      XII_GAL_DEVICE_CHECK(textureDescription.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Subsampled), "The attachment at index {0} must be created with the xiiGALMiscTextureFlags::Subsampled flag. If the xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget capability is not supported, all attachments except the shading rate texture must have been created with a xiiGALMiscTextureFlags::Subsampled flag.");
    }
  }

  return CreateFramebufferPlatform(description);
}

xiiSharedPtr<xiiGALBottomLevelAS> xiiGALDevice::CreateBottomLevelAS(const xiiGALBottomLevelASCreationDescription& description)
{
  VerifyMultithreadedAccess();

  if (description.m_uiCompactedSize > 0U)
  {
    XII_GAL_DEVICE_CHECK(!description.m_Triangles.IsEmpty() && !description.m_BoundingBoxes.IsEmpty(), "If a non-zero compacted size is given, the Triangles and Bounding Boxes must not be empty.");
    XII_GAL_DEVICE_CHECK(description.m_BuildASFlags == xiiGALRayTracingBuildASFlags::None, "If a non-zero compacted size is given, the Flags must be xiiGALRayTracingBuildASFlags::None.");
  }
  else
  {
    XII_GAL_DEVICE_CHECK(((description.m_BoundingBoxes.GetCount() != 0U) ^ (description.m_Triangles.GetCount() != 0U)), "Exactly one of the Bounding Box count {0}, and the Triangles count {1} must be non-zero.", description.m_BoundingBoxes.GetCount(), description.m_Triangles.GetCount());
    XII_GAL_DEVICE_CHECK(!description.m_BuildASFlags.AreAllSet(xiiGALRayTracingBuildASFlags::PreferFastTrace | xiiGALRayTracingBuildASFlags::PreferFastBuild), "xiiGALRayTracingBuildASFlags::PreferFastTrace and xiiGALRayTracingBuildASFlags::PreferFastBuild are mutually exclusive.");

    for (xiiUInt32 i = 0U; i < description.m_Triangles.GetCount(); ++i)
    {
      const xiiGALBLASTriangleDescription& triangle = description.m_Triangles[i];

      XII_GAL_DEVICE_CHECK(!triangle.m_sGeometryName.IsEmpty(), "The geometry name at triangle {0} must not be empty.", i);
      XII_GAL_DEVICE_CHECK(triangle.m_VertexValueType == xiiGALValueType::Float32 || triangle.m_VertexValueType == xiiGALValueType::Float16 || triangle.m_VertexValueType == xiiGALValueType::Int32, "The Vertex Value Type specified in triangle {0} is invalid. Allowed types are xiiGALValueType::Float32, xiiGALValueType::Float16 or xiiGALValueType::Int32.", i);
      XII_GAL_DEVICE_CHECK(triangle.m_uiVertexComponentCount == 2U || triangle.m_uiVertexComponentCount == 3U, "The Vertex Component Count specified in triangle {0} is invalid. Allowed values are 2 or 3.", i);
      XII_GAL_DEVICE_CHECK(triangle.m_uiMaxVertexCount > 0U, "The Max Vertex Count specified in triangle {0} must be greater than zero.", i);
      XII_GAL_DEVICE_CHECK(triangle.m_uiMaxPrimitiveCount > 0U, "The Max Primitive Count specified in triangle {0} must be greater than zero.", i);

      if (triangle.m_IndexType == xiiGALValueType::Undefined)
      {
        XII_GAL_DEVICE_CHECK(triangle.m_uiMaxVertexCount == (triangle.m_uiMaxPrimitiveCount * 3U), "The Max Vertex Count ({0}) specified in triangle {1} must be equal to the Max Vertex Count multiplied by 3 ({2}).", triangle.m_uiMaxVertexCount, i, (triangle.m_uiMaxPrimitiveCount * 3U));
      }
      else
      {
        XII_GAL_DEVICE_CHECK(triangle.m_IndexType == xiiGALValueType::UInt32 || triangle.m_IndexType == xiiGALValueType::UInt16, "The Index Type specified in triangle {0} must be xiiGALValueType::UInt16 or xiiGALValueType::UInt32.", i);
      }
    }

    for (xiiUInt32 i = 0U; i < description.m_BoundingBoxes.GetCount(); ++i)
    {
      const xiiGALBLASBoundingBoxDescription& boundingBox = description.m_BoundingBoxes[i];

      XII_GAL_DEVICE_CHECK(!boundingBox.m_sGeometryName.IsEmpty(), "The Geometry Name in bounding box {0} must not be empty.", i);
      XII_GAL_DEVICE_CHECK(boundingBox.m_uiMaxBoxCount > 0U, "The Max Box Count in bounding box {0} must be greater than zero.", i);
    }
  }

  return CreateBottomLevelASPlatform(description);
}

xiiSharedPtr<xiiGALTopLevelAS> xiiGALDevice::CreateTopLevelAS(const xiiGALTopLevelASCreationDescription& description)
{
  VerifyMultithreadedAccess();

  if (description.m_uiCompactedSize > 0U)
  {
    XII_GAL_DEVICE_CHECK(description.m_uiMaxInstanceCount == 0U, "If a non-zero compacted size is given, the Max Instance Count must be zero.");
    XII_GAL_DEVICE_CHECK(description.m_Flags == xiiGALRayTracingBuildASFlags::None, "If a non-zero compacted size is given, the specified Flags must be xiiGALRayTracingBuildASFlags::None.");
  }
  else
  {
    XII_GAL_DEVICE_CHECK(description.m_uiMaxInstanceCount > 0U, "The max instance count must be greater than zero.");
    XII_GAL_DEVICE_CHECK(!description.m_Flags.AreAllSet(xiiGALRayTracingBuildASFlags::PreferFastTrace | xiiGALRayTracingBuildASFlags::PreferFastBuild), "xiiGALRayTracingBuildASFlags::PreferFastTrace and xiiGALRayTracingBuildASFlags::PreferFastBuild are mutually exclusive.");
  }

  return CreateTopLevelASPlatform(description);
}

xiiSharedPtr<xiiGALPipelineResourceSignature> xiiGALDevice::CreatePipelineResourceSignature(xiiGALPipelineResourceSignatureCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(description.m_uiBindingIndex < XII_GAL_MAX_RESOURCE_SIGNATURES_COUNT, "The pipeline resource signature binding index ({0}) exceeds the maximum allowed value ({1}).", description.m_uiBindingIndex, XII_GAL_MAX_RESOURCE_SIGNATURES_COUNT - 1);
  XII_GAL_DEVICE_CHECK(description.m_Resources.GetCount() <= s_uiMaxResourcesInSignature, "The pipeline resource signature resource count ({0}) exceeds the maximum allowed value ({1}).", description.m_Resources.GetCount(), s_uiMaxResourcesInSignature);

  // Ensure that shader stages do not conflict for resources with the same name.

  xiiMap<xiiTempHashedString, xiiSet<xiiGALShaderType::StorageType>> usedResourceShaderStages;
  for (xiiUInt32 i = 0; i < description.m_Resources.GetCount(); ++i)
  {
    const xiiGALPipelineResourceDescription& resource = description.m_Resources[i];

    XII_GAL_DEVICE_CHECK(!resource.m_sName.IsEmpty(), "The pipeline resource at index '{0}' requires a non-empty name.", i);
    XII_GAL_DEVICE_CHECK(!resource.m_ShaderStages.IsNoFlagSet(), "The pipeline resource at index '{0}' requires a valid shader stage, and must not be xiiGALShaderType::Unknown.", i);
    XII_GAL_DEVICE_CHECK(resource.m_uiArraySize > 0U, "The pipeline resource at index '{0}' requires a non-zero array size.", i);

    xiiSet<xiiGALShaderType::StorageType> shaderStageSet;
    if (usedResourceShaderStages.TryGetValue(resource.m_sName, shaderStageSet))
    {
      XII_GAL_DEVICE_CHECK(!shaderStageSet.Contains(resource.m_ShaderStages.GetValue()), "Multiple resources with name '{}' found with overlapping shader stages. There may be resources with the same name in different shader stages, but the stages must not overlap.");
    }
    else
    {
      xiiSet<xiiGALShaderType::StorageType> set;
      set.Insert(resource.m_ShaderStages.GetValue());

      usedResourceShaderStages.Insert(resource.m_sName, set);
    }

    if (resource.m_PipelineResourceFlags.IsSet(xiiGALPipelineResourceFlags::RuntimeArray))
    {
      XII_GAL_DEVICE_CHECK(resource.m_PipelineResourceFlags.IsSet(xiiGALPipelineResourceFlags::RuntimeArray) && m_AdapterDescription.m_Features.m_ShaderResourceRuntimeArray == xiiGALDeviceFeatureState::Enabled, "The pipeline resource at index '{0}' specifies the xiiGALPipelineResourceFlags::RuntimeArray flag, which requires the shader resource runtime array device feature.", i);
    }
    if (resource.m_ResourceType == xiiGALShaderResourceType::AccelerationStructure)
    {
      XII_GAL_DEVICE_CHECK(resource.m_ResourceType == xiiGALShaderResourceType::AccelerationStructure && m_AdapterDescription.m_Features.m_RayTracing == xiiGALDeviceFeatureState::Enabled, "The pipeline resource at index '{0}' specifies the xiiGALShaderResourceType::AccelerationStructure type, which requires ray tracing device feature.", i);
    }
    if (resource.m_ResourceType == xiiGALShaderResourceType::InputAttachment)
    {
      XII_GAL_DEVICE_CHECK(resource.m_ResourceType == xiiGALShaderResourceType::InputAttachment && resource.m_ShaderStages == xiiGALShaderType::Pixel, "The pipeline resource at index '{0}' specifies the xiiGALShaderResourceType::InputAttachment type, but its only supported in the pixel shader stage.", i);
    }

    xiiBitflags<xiiGALPipelineResourceFlags> allowedResourceFlags = xiiGALGraphicsUtilities::GetValidPipelineResourceFlags(resource.m_ResourceType);
    XII_GAL_DEVICE_CHECK(resource.m_PipelineResourceFlags.IsStrictlyAnySet(allowedResourceFlags) || resource.m_PipelineResourceFlags.IsNoFlagSet(), "The pipeline resource at index '{0}' contains flags that are not allowed for the shader resource type.", i);

    XII_GAL_DEVICE_CHECK(!(resource.m_PipelineResourceFlags.IsSet(xiiGALPipelineResourceFlags::GeneralInputAttachment) && m_Description.m_GraphicsDeviceType != xiiGALGraphicsDeviceType::Vulkan), "The pipeline resource at index '{0}' specifies the xiiGALPipelineResourceFlags::GeneralInputAttachment flag, which is only valid on a Vulkan graphics implementation.", i);
  }

  // Ensure that immutable samplers do not have conflicting shader stages.

  xiiMap<xiiTempHashedString, xiiSet<xiiGALShaderType::StorageType>> usedImmutableSamplerShaderStages;
  for (xiiUInt32 i = 0; i < description.m_ImmutableSamplers.GetCount(); ++i)
  {
    xiiGALImmutableSamplerDescription& samplerDescription = description.m_ImmutableSamplers[i];

    XII_GAL_DEVICE_CHECK(!samplerDescription.m_SamplerOrTextureName.IsEmpty(), "The immutable sampler at index '{0}' requires a non-empty name.", i);
    XII_GAL_DEVICE_CHECK(!samplerDescription.m_ShaderStages.IsNoFlagSet(), "The immutable sampler at index '{0}' requires a valid shader stage, and must not be xiiGALShaderType::Unknown.", i);

    // Use anisotropic filtering if any of Anisotropic is set.
    if (samplerDescription.m_SamplerDescription.m_MagFilter == xiiGALFilterType::Anisotropic || samplerDescription.m_SamplerDescription.m_MinFilter == xiiGALFilterType::Anisotropic || samplerDescription.m_SamplerDescription.m_MipFilter == xiiGALFilterType::Anisotropic)
    {
      if (samplerDescription.m_SamplerDescription.m_ComparisonFunction == xiiGALComparisonFunction::Never)
      {
        samplerDescription.m_SamplerDescription.m_MinFilter = xiiGALFilterType::Anisotropic;
        samplerDescription.m_SamplerDescription.m_MagFilter = xiiGALFilterType::Anisotropic;
        samplerDescription.m_SamplerDescription.m_MipFilter = xiiGALFilterType::Anisotropic;
      }
      else
      {
        samplerDescription.m_SamplerDescription.m_MinFilter = xiiGALFilterType::ComparisonAnisotropic;
        samplerDescription.m_SamplerDescription.m_MagFilter = xiiGALFilterType::ComparisonAnisotropic;
        samplerDescription.m_SamplerDescription.m_MipFilter = xiiGALFilterType::ComparisonAnisotropic;
      }
    }

    xiiSet<xiiGALShaderType::StorageType> shaderStageSet;
    if (usedImmutableSamplerShaderStages.TryGetValue(samplerDescription.m_SamplerOrTextureName, shaderStageSet))
    {
      XII_GAL_DEVICE_CHECK(!shaderStageSet.Contains(samplerDescription.m_ShaderStages.GetValue()), "Multiple immutable samplers with name '{}' found with overlapping shader stages. There may be immutable samplers with the same name in different shader stages, but the stages must not overlap.");
    }
    else
    {
      xiiSet<xiiGALShaderType::StorageType> set;
      set.Insert(samplerDescription.m_ShaderStages.GetValue());

      usedImmutableSamplerShaderStages.Insert(samplerDescription.m_SamplerOrTextureName, set);
    }
  }

  // Ensure that push constant ranges are within the device limits.
  for (const xiiGALPushConstantRange& range : description.m_PushConstantRanges)
  {
    XII_GAL_DEVICE_CHECK(range.m_ShaderStages != xiiGALShaderType::Unknown && !range.m_ShaderStages.IsNoFlagSet(), "Push constant range with offset {} and size {} has invalid shader stages. A push constant range must specify at least one valid shader stage.", range.m_uiOffset, range.m_uiSize);
    XII_GAL_DEVICE_CHECK(range.m_uiOffset < static_cast<xiiUInt64>(m_AdapterDescription.m_DeviceLimits.m_uiMaxPushConstantsSize), "Push constant range offset ({}) exceeds device limit of {} bytes.", range.m_uiOffset, m_AdapterDescription.m_DeviceLimits.m_uiMaxPushConstantsSize);
    XII_GAL_DEVICE_CHECK(range.m_uiSize <= static_cast<xiiUInt64>(m_AdapterDescription.m_DeviceLimits.m_uiMaxPushConstantsSize), "Push constant range size ({}) exceeds device limit of {} bytes.", range.m_uiSize, m_AdapterDescription.m_DeviceLimits.m_uiMaxPushConstantsSize);
    XII_GAL_DEVICE_CHECK(static_cast<xiiUInt64>(range.m_uiOffset) + static_cast<xiiUInt64>(range.m_uiSize) <= static_cast<xiiUInt64>(m_AdapterDescription.m_DeviceLimits.m_uiMaxPushConstantsSize), "Push constant range (offset {} size {}) exceeds device limit of {} bytes.", range.m_uiOffset, range.m_uiSize, m_AdapterDescription.m_DeviceLimits.m_uiMaxPushConstantsSize);
    XII_GAL_DEVICE_CHECK(range.m_uiOffset % 4U == 0U, "Push constant range offset ({}) must be a multiple of 4 bytes.", range.m_uiOffset);
    XII_GAL_DEVICE_CHECK(range.m_uiSize != 0U, "Push constant range size must be greater than zero.");
  }

  /// \todo Verify combined texture samplers, all samplers should be assigned to textures when combined texture samplers are used, all immutable samplers should be assigned to textures or samplers when combined texture samplers are used.

  // Finally, sort the resources by their ascending set index.
  description.m_Resources.Sort([](const xiiGALPipelineResourceDescription& lhs, const xiiGALPipelineResourceDescription& rhs) -> bool {
    return lhs.m_uiBindSet < rhs.m_uiBindSet;
  });

  return CreatePipelineResourceSignaturePlatform(description);
}

xiiSharedPtr<xiiGALGraphicsPipelineState> xiiGALDevice::CreateGraphicsPipelineState(const xiiGALGraphicsPipelineStateCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(description.m_PipelineType == xiiGALPipelineType::Graphics || description.m_PipelineType == xiiGALPipelineType::Mesh, "The pipeline type for a graphics pipeline must be of type xiiGALPipelineType::Graphics or xiiGALPipelineType::Mesh.");
  XII_GAL_DEVICE_CHECK(description.m_pPipelineResourceSignature != nullptr, "The pipeline resource signature is invalid. A valid pipeline resource signature is required.");
  XII_GAL_DEVICE_CHECK(description.m_GraphicsPipeline.m_pRasterizerState != nullptr, "A valid rasterizer state is required on a graphics pipeline.");
  XII_GAL_DEVICE_CHECK(description.m_GraphicsPipeline.m_uiViewportCount > 0, "The number of viewports for a graphics pipeline state must be greater than zero.");
  XII_GAL_DEVICE_CHECK(description.m_GraphicsPipeline.m_pRenderPass != nullptr, "The render pass is invalid. A valid render pass is required.");
  XII_GAL_DEVICE_CHECK(description.m_GraphicsPipeline.m_uiSubpassIndex < description.m_GraphicsPipeline.m_pRenderPass->GetDescription().m_SubPasses.GetCount(), "Subpass index ({}) exceeds the number of subpasses ({}) in render pass '{}'.", description.m_GraphicsPipeline.m_uiSubpassIndex, description.m_GraphicsPipeline.m_pRenderPass->GetDescription().m_SubPasses.GetCount(), description.m_GraphicsPipeline.m_pRenderPass->GetDebugName());

  if (description.m_GraphicsPipeline.m_ShadingRateFlags.IsAnyFlagSet())
  {
    XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_Features.m_VariableRateShading == xiiGALDeviceFeatureState::Enabled, "Shading rate flags ({}) require VariableRateShading device feature.", xiiArgEnum(description.m_GraphicsPipeline.m_ShadingRateFlags));

    if (m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::SampleMask))
    {
      const xiiUInt32 uiRequiredMask = (1U << description.m_GraphicsPipeline.m_SampleDescription.m_uiCount) - 1U;

      XII_GAL_DEVICE_CHECK(((description.m_GraphicsPipeline.m_uiSampleMask & uiRequiredMask) == uiRequiredMask), "Sample mask with zero bits is used with shading rate flags, which requires the xiiGALShadingRateCapabilityFlags::SampleMask capability.");
    }
    if (description.m_GraphicsPipeline.m_ShadingRateFlags.IsSet(xiiGALPipelineShadingRateFlags::PerPrimitive) && description.m_GraphicsPipeline.m_uiViewportCount > 1)
    {
      XII_GAL_DEVICE_CHECK(m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::PerPrimitiveWithMultipleViewports), "Multiple viewports with variable shading rate require the xiiGALShadingRateCapabilityFlags::PerPrimitiveWithMultipleViewports capability.");
    }
  }

  if (description.m_PipelineType == xiiGALPipelineType::Graphics)
  {
    XII_GAL_DEVICE_CHECK(description.m_pVertexShader != nullptr, "A graphics pipeline must contain a valid vertex shader.");
    XII_GAL_DEVICE_CHECK(description.m_pAmplificationShader == nullptr && description.m_pMeshShader == nullptr, "Mesh shaders are not supported in a graphics pipeline.");
  }
  else if (description.m_PipelineType == xiiGALPipelineType::Mesh)
  {
    XII_GAL_DEVICE_CHECK(description.m_pMeshShader != nullptr, "A mesh pipeline must contain a valid mesh shader.");
    XII_GAL_DEVICE_CHECK(description.m_pVertexShader == nullptr && description.m_pGeometryShader == nullptr && description.m_pDomainShader == nullptr && description.m_pHullShader == nullptr, "Vertex, Geometry, and Tessellation shaders are not supported in a mesh pipeline.");
    XII_GAL_DEVICE_CHECK(description.m_GraphicsPipeline.m_pInputLayout == nullptr, "An input layout is ignored in a mesh pipeline.");
    XII_GAL_DEVICE_CHECK(description.m_GraphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::TriangleList || description.m_GraphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::Undefined, "A primitive topology is ignored in a mesh pipeline. Set it to xiiGALPrimitiveTopology::Undefined or keep default value (xiiGALPrimitiveTopology::TriangleList).");
  }

  XII_GAL_DEVICE_CHECK(description.m_pVertexShader == nullptr || description.m_pVertexShader->GetDescription().m_ShaderType == xiiGALShaderType::Vertex, "The pipeline vertex shader must be of type xiiGALShaderType::Vertex.");
  XII_GAL_DEVICE_CHECK(description.m_pPixelShader == nullptr || description.m_pPixelShader->GetDescription().m_ShaderType == xiiGALShaderType::Pixel, "The pipeline vertex shader must be of type xiiGALShaderType::Pixel.");
  XII_GAL_DEVICE_CHECK(description.m_pGeometryShader == nullptr || description.m_pGeometryShader->GetDescription().m_ShaderType == xiiGALShaderType::Geometry, "The pipeline vertex shader must be of type xiiGALShaderType::Geometry.");
  XII_GAL_DEVICE_CHECK(description.m_pHullShader == nullptr || description.m_pHullShader->GetDescription().m_ShaderType == xiiGALShaderType::Hull, "The pipeline vertex shader must be of type xiiGALShaderType::Hull.");
  XII_GAL_DEVICE_CHECK(description.m_pDomainShader == nullptr || description.m_pDomainShader->GetDescription().m_ShaderType == xiiGALShaderType::Domain, "The pipeline vertex shader must be of type xiiGALShaderType::Domain.");
  XII_GAL_DEVICE_CHECK(description.m_pAmplificationShader == nullptr || description.m_pAmplificationShader->GetDescription().m_ShaderType == xiiGALShaderType::Amplification, "The pipeline vertex shader must be of type xiiGALShaderType::Amplification.");
  XII_GAL_DEVICE_CHECK(description.m_pMeshShader == nullptr || description.m_pMeshShader->GetDescription().m_ShaderType == xiiGALShaderType::Mesh, "The pipeline vertex shader must be of type xiiGALShaderType::Mesh.");

  return CreateGraphicsPipelineStatePlatform(description);
}

xiiSharedPtr<xiiGALComputePipelineState> xiiGALDevice::CreateComputePipelineState(const xiiGALComputePipelineStateCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(description.m_PipelineType == xiiGALPipelineType::Compute, "The pipeline type for a compute pipeline must be of type xiiGALPipelineType::Compute.");
  XII_GAL_DEVICE_CHECK(description.m_pPipelineResourceSignature != nullptr, "The pipeline resource signature is invalid. A valid pipeline resource signature is required.");
  XII_GAL_DEVICE_CHECK(description.m_pComputeShader != nullptr, "The compute shader for a compute pipeline state must not be null.");
  XII_GAL_DEVICE_CHECK(description.m_pComputeShader->GetDescription().m_ShaderType == xiiGALShaderType::Compute, "The shader for a compute pipeline state must be of type xiiGALShaderType::Compute.");

  return CreateComputePipelineStatePlatform(description);
}

xiiSharedPtr<xiiGALRayTracingPipelineState> xiiGALDevice::CreateRayTracingPipelineState(const xiiGALRayTracingPipelineStateCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(description.m_PipelineType == xiiGALPipelineType::RayTracing, "The pipeline type for a ray tracing pipeline must be of type xiiGALPipelineType::RayTracing.");
  XII_GAL_DEVICE_CHECK(description.m_pPipelineResourceSignature != nullptr, "The pipeline resource signature is invalid. A valid pipeline resource signature is required.");
  XII_GAL_DEVICE_CHECK((m_AdapterDescription.m_Features.m_RayTracing == xiiGALDeviceFeatureState::Enabled) && m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags.IsSet(xiiGALRayTracingCapabilityFlags::StandaloneShaders), "Standalone ray tracing shaders are not supported by the device.");

  if (m_Description.m_GraphicsDeviceType == xiiGALGraphicsDeviceType::Direct3D12)
  {
    XII_GAL_DEVICE_CHECK(!description.m_sShaderRecordName.IsEmpty() == description.m_RayTracingPipeline.m_uiShaderRecordSize > 0U, "Shader record name must not be empty if shader record size is non-zero.");
  }

  XII_GAL_DEVICE_CHECK(description.m_RayTracingPipeline.m_uiMaxRecursionDepth > m_AdapterDescription.m_RayTracingProperties.m_uiMaxRecursionDepth, "Max recursion depth ({}) exceeds device limit ({}).", description.m_RayTracingPipeline.m_uiMaxRecursionDepth, m_AdapterDescription.m_RayTracingProperties.m_uiMaxRecursionDepth);

  xiiSet<xiiStringView> groupNames(m_Allocator.GetParent());
  for (xiiUInt32 i = 0; i < description.m_GeneralShaders.GetCount(); ++i)
  {
    const xiiGALRayTracingGeneralShaderGroupDescription& group = description.m_GeneralShaders[i];

    XII_GAL_DEVICE_CHECK(!group.m_sName.IsEmpty(), "GeneralShaders[{}].sName must have a non-empty name.", i);
    XII_GAL_DEVICE_CHECK(!groupNames.Contains(group.m_sName.GetView()), "GeneralShaders[{}].sName has group name ('{}') that has already been assigned to another group. All group names must be unique.", i, group.m_sName);
    XII_GAL_DEVICE_CHECK(group.m_pShader != nullptr, "GeneralShaders[{}].pShader must not be null.", i);
    XII_GAL_DEVICE_CHECK(group.m_pShader->GetDescription().m_ShaderType.IsStrictlyAnySet(xiiGALShaderType::RayGeneration | xiiGALShaderType::RayMiss | xiiGALShaderType::Callable), "Shader type {} is not a valid type for ray tracing general shader.", xiiArgEnum(group.m_pShader->GetDescription().m_ShaderType));

    groupNames.Insert(group.m_sName);
  }

  for (xiiUInt32 i = 0; i < description.m_TriangleHitShaders.GetCount(); ++i)
  {
    const xiiGALRayTracingTriangleHitShaderGroupDescription& group = description.m_TriangleHitShaders[i];

    XII_GAL_DEVICE_CHECK(!group.m_sName.IsEmpty(), "TriangleHitShaders[{}].sName must have a non-empty name.", i);
    XII_GAL_DEVICE_CHECK(!groupNames.Contains(group.m_sName.GetView()), "TriangleHitShaders[{}].sName has group name ('{}') that has already been assigned to another group. All group names must be unique.", i, group.m_sName);
    XII_GAL_DEVICE_CHECK(group.m_pClosestHitShader != nullptr, "TriangleHitShaders[{}].pClosestHitShader must not be null.", i);
    XII_GAL_DEVICE_CHECK(group.m_pClosestHitShader->GetDescription().m_ShaderType == xiiGALShaderType::RayClosestHit, "TriangleHitShaders[{}].pClosestHitShader must be of type xiiGALShaderType::RayClosestHit.", i);
    XII_GAL_DEVICE_CHECK(group.m_pAnyHitShader == nullptr || group.m_pAnyHitShader->GetDescription().m_ShaderType == xiiGALShaderType::RayAnyHit, "TriangleHitShaders[{}].pAnyHitShader must be of type xiiGALShaderType::RayAnyHit.", i);

    groupNames.Insert(group.m_sName);
  }

  for (xiiUInt32 i = 0; i < description.m_ProceduralHitShaders.GetCount(); ++i)
  {
    const xiiGALRayTracingProceduralHitShaderGroupDescription& group = description.m_ProceduralHitShaders[i];

    XII_GAL_DEVICE_CHECK(!group.m_sName.IsEmpty(), "ProceduralHitShaders[{}].sName must have a non-empty name.", i);
    XII_GAL_DEVICE_CHECK(!groupNames.Contains(group.m_sName.GetView()), "ProceduralHitShaders[{}].sName has group name ('{}') that has already been assigned to another group. All group names must be unique.", i, group.m_sName);
    XII_GAL_DEVICE_CHECK(group.m_pIntersectionShader != nullptr, "ProceduralHitShaders[{}].pIntersectionShader must not be null.", i);
    XII_GAL_DEVICE_CHECK(group.m_pIntersectionShader->GetDescription().m_ShaderType == xiiGALShaderType::RayClosestHit, "ProceduralHitShaders[{}].pIntersectionShader must be of type xiiGALShaderType::RayClosestHit.", i);
    XII_GAL_DEVICE_CHECK(group.m_pClosestHitShader == nullptr || group.m_pClosestHitShader->GetDescription().m_ShaderType == xiiGALShaderType::RayClosestHit, "ProceduralHitShaders[{}].pClosestHitShader must be of type xiiGALShaderType::RayClosestHit.", i);
    XII_GAL_DEVICE_CHECK(group.m_pAnyHitShader == nullptr || group.m_pAnyHitShader->GetDescription().m_ShaderType == xiiGALShaderType::RayAnyHit, "ProceduralHitShaders[{}].pAnyHitShader must be of type xiiGALShaderType::RayAnyHit.", i);

    groupNames.Insert(group.m_sName);
  }

  return CreateRayTracingPipelineStatePlatform(description);
}

xiiSharedPtr<xiiGALTilePipelineState> xiiGALDevice::CreateTilePipelineState(const xiiGALTilePipelineStateCreationDescription& description)
{
  VerifyMultithreadedAccess();

  XII_GAL_DEVICE_CHECK(description.m_PipelineType == xiiGALPipelineType::Tile, "The pipeline type for a tile pipeline must be of type xiiGALPipelineType::Tile.");
  XII_GAL_DEVICE_CHECK(description.m_pPipelineResourceSignature != nullptr, "The pipeline resource signature is invalid. A valid pipeline resource signature is required.");
  XII_GAL_DEVICE_CHECK(description.m_pTileShader != nullptr, "The tile shader for a tile pipeline state must not be null.");
  XII_GAL_DEVICE_CHECK(description.m_pTileShader->GetDescription().m_ShaderType == xiiGALShaderType::Tile, "The shader for a tile pipeline state must be of type xiiGALShaderType::Tile.");

  return CreateTilePipelineStatePlatform(description);
}

void xiiGALDevice::WaitIdle()
{
  WaitIdlePlatform();
}

#undef XII_GAL_DEVICE_CHECK

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_Device);
