#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GRAPHICSVULKAN_LIB
#    define XII_GRAPHICSVULKAN_DLL XII_DECL_EXPORT
#  else
#    define XII_GRAPHICSVULKAN_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_GRAPHICSVULKAN_DLL
#endif

#include <Diligent/Common/interface/ObjectBase.hpp>
#include <Diligent/Common/interface/RefCntAutoPtr.hpp>

#include <Diligent/Graphics/GraphicsEngine/interface/Constants.h>
#include <Diligent/Graphics/GraphicsEngine/interface/GraphicsTypes.h>

#define XII_GAL_DILIGENT_REF_RELEASE(refCntObject) \
  do                                               \
  {                                                \
    if ((refCntObject) != nullptr)                 \
    {                                              \
      (refCntObject).Release();                    \
    }                                              \
  } while (0)

#define XII_GAL_DILIGENT_PTR_RELEASE(ptrObject) \
  do                                            \
  {                                             \
    if ((ptrObject) != nullptr)                 \
    {                                           \
      (ptrObject)->Release();                   \
      (ptrObject) = nullptr;                    \
    }                                           \
  } while (0)

////////// Forward Declarations //////////

namespace Diligent
{
  struct APIInfo;
  struct RenderTargetBlendDesc;
  struct BlendStateDesc;
  struct BLASTriangleDesc;
  struct BLASBoundingBoxDesc;
  struct BottomLevelASDesc;
  struct ScratchBufferSizes;
  struct IBottomLevelASDesc;
  struct BufferDesc;
  struct BufferData;
  struct SparseBufferProperties;
  struct IBuffer;
  struct BufferFormat;
  struct BufferViewDesc;
  struct IBufferView;
  struct ICommandQueue;
  struct ShaderUnpackInfo;
  struct PipelineStateUnpackInfo;
  struct RenderPassUnpackInfo;
  struct IDearchiver;
  struct ResourceSignatureUnpackInfo;
  struct StencilOpDesc;
  struct DepthStencilStateDesc;
  struct DeviceContextDesc;
  struct DrawAttribs;
  struct DrawIndexedAttribs;
  struct DrawIndirectAttribs;
  struct DrawindexedIndirectAttribs;
  struct DrawMeshAttribs;
  struct DrawMeshIndirectAttribs;
  struct DispatchComputeAttribs;
  struct DispatchComputeIndirectAttribs;
  struct DispatchTileAttribs;
  struct ResolveTextureSubresourceAttribs;
  struct Viewport;
  struct Rect;
  struct CopyTextureAttribs;
  struct SetRenderTargetAttribs;
  struct BeginRenderPassAttribs;
  struct BLASBuildTriangleData;
  struct BLASBuildBoundingBoxData;
  struct BuildBLASAttribs;
  struct InstanceMatrix;
  struct TLASBuildInstanceData;
  struct BuildTLASAttribs;
  struct CopyBLASAttribs;
  struct CopyTLASAttribs;
  struct WriteBLASCompactedSizeAttribs;
  struct WriteTLASCompactedSizeAttribs;
  struct TraceRaysAttribs;
  struct TraceRaysIndirectAttribs;
  struct UpdateIndirectRTBufferAttribs;
  struct SparseBufferMemoryBindRange;
  struct SparseBufferMemoryBindInfo;
  struct SparseTextureMemoryBindRange;
  struct SparseTextureMemoryBindInfo;
  struct BindSparseResourceMemoryAttribs;
  struct StateTransitionDesc;
  struct IDeviceContext;
  struct DeviceMemoryDesc;
  struct DeviceMemoryCreateInfo;
  struct IDeviceMemory;
  struct IDeviceObject;
  struct DearchiverCreateInfo;
  struct IEngineFactory;
  struct FenceDesc;
  struct IFence;
  struct FramebufferDesc;
  struct LayoutElement;
  struct InputLayoutDesc;
  struct ImmutableSamplerDesc;
  struct PipelineResourceDesc;
  struct PipelineResourceSignatureDesc;
  struct IPipelineResourceSignature;
  struct SampleDesc;
  struct ShaderResourceVariableDesc;
  struct PipelineResourceLayoutDesc;
  struct GraphicsPipelineDesc;
  struct RayTracingGeneralShaderGroup;
  struct RayTracingTriangleHitShaderGroup;
  struct RayTracingProceduralHitShaderGroup;
  struct RayTracingPipelineDesc;
  struct PipelineStateDesc;
  struct PipelineStateCreateInfo;
  struct GraphicsPipelineStateCreateInfo;
  struct ComputePipelineStateCreateInfo;
  struct RayTracingPipelineStateCreateInfo;
  struct TilePipelineDesc;
  struct TilePipelineStateCreateInfo;
  struct IPipelineState;
  struct PipelineStateCacheDesc;
  struct PipelineStateCacheCreateInfo;
  struct IPipelineStateCache;
  struct QueryDataOcclusion;
  struct QueryDataBinaryOcclusion;
  struct QueryDataTimestamp;
  struct QueryDataPipelineStatistics;
  struct QueryDataDuration;
  struct QueryDesc;
  struct IQuery;
  struct RasterizerStateDesc;
  struct IRenderDevice;
  struct RenderPassAttachmentDesc;
  struct AttachmentReference;
  struct ShadingRateAttachment;
  struct SubpassDesc;
  struct SubpassDependencyDesc;
  struct RenderPassDesc;
  struct ResourceMappingEntry;
  struct ResourceMappingDesc;
  struct IResourceMapping;
  struct SamplerDesc;
  struct ISampler;
  struct ShaderDesc;
  struct ShaderMacro;
  struct ShaderCreateInfo;
  struct ShaderResourceDesc;
  struct ShaderCodeVariableDesc;
  struct ShaderCodeBufferDesc;
  struct IShader;
  struct ShaderBindingTableDesc;
  struct IShaderBindingTable;
  struct IShaderResourceBinding;
  struct IShaderResourceVariable;
  struct ISwapChain;
  struct TextureDesc;
  struct TextureSubResData;
  struct TextureData;
  struct MappedTextureSubresource;
  struct SparseTextureProperties;
  struct ITexture;
  struct TextureComponentMapping;
  struct TextureViewDesc;
  struct ITextureView;
  struct TopLevelASDesc;
  struct TLASBuildInfo;
  struct TLASInstanceDesc;
  struct ITopLevelAS;

  class ICommandList;
  class IRenderPass;
  class IFramebuffer;
  class ScopedQueryHelper;
  class DurationQueryHelper;
} // namespace Diligent

class xiiGALCommandEncoderVulkan;
class xiiGALCommandListVulkan;
class xiiGALCommandQueueVulkan;
class xiiGALDeviceVulkan;
class xiiGALPassVulkan;
class xiiGALSwapChainVulkan;
class xiiGALBottomLevelASVulkan;
class xiiGALBufferVulkan;
class xiiGALBufferViewVulkan;
class xiiGALFenceVulkan;
class xiiGALFramebufferVulkan;
class xiiGALQueryVulkan;
class xiiGALRenderPassVulkan;
class xiiGALSamplerVulkan;
class xiiGALTextureVulkan;
class xiiGALTextureViewVulkan;
class xiiGALTopLevelASVulkan;
class xiiGALInputLayoutVulkan;
class xiiGALShaderVulkan;
class xiiGALBlendStateVulkan;
class xiiGALDepthStencilStateVulkan;
class xiiGALRasterizerStateVulkan;

XII_DEFINE_AS_POD_TYPE(Diligent::BufferData);
XII_DEFINE_AS_POD_TYPE(Diligent::TextureData);
XII_DEFINE_AS_POD_TYPE(Diligent::TextureSubResData);
XII_DEFINE_AS_POD_TYPE(Diligent::LayoutElement);
XII_DEFINE_AS_POD_TYPE(Diligent::RenderPassAttachmentDesc);
XII_DEFINE_AS_POD_TYPE(Diligent::AttachmentReference);
