#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GRAPHICSD3D12_LIB
#    define XII_GRAPHICSD3D12_DLL XII_DECL_EXPORT
#  else
#    define XII_GRAPHICSD3D12_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_GRAPHICSD3D12_DLL
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

class xiiGALCommandEncoderD3D12;
class xiiGALCommandListD3D12;
class xiiGALCommandQueueD3D12;
class xiiGALDeviceD3D12;
class xiiGALPassD3D12;
class xiiGALSwapChainD3D12;
class xiiGALBottomLevelASD3D12;
class xiiGALBufferD3D12;
class xiiGALBufferViewD3D12;
class xiiGALFenceD3D12;
class xiiGALFramebufferD3D12;
class xiiGALQueryD3D12;
class xiiGALRenderPassD3D12;
class xiiGALSamplerD3D12;
class xiiGALTextureD3D12;
class xiiGALTextureViewD3D12;
class xiiGALTopLevelASD3D12;
class xiiGALInputLayoutD3D12;
class xiiGALShaderD3D12;
class xiiGALBlendStateD3D12;
class xiiGALDepthStencilStateD3D12;
class xiiGALRasterizerStateD3D12;

XII_DEFINE_AS_POD_TYPE(Diligent::BufferData);
XII_DEFINE_AS_POD_TYPE(Diligent::TextureData);
XII_DEFINE_AS_POD_TYPE(Diligent::TextureSubResData);
XII_DEFINE_AS_POD_TYPE(Diligent::LayoutElement);
XII_DEFINE_AS_POD_TYPE(Diligent::RenderPassAttachmentDesc);
XII_DEFINE_AS_POD_TYPE(Diligent::AttachmentReference);
