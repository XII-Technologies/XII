/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Rect.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TopLevelAS.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Specifies flags for configuring the behavior of a GAL command list.
///
/// This enum encapsulates flags used when creating or modifying command list behavior within the Graphics Abstraction Layer (GAL).
/// These flags determine how a command list can be recorded, submitted, and reused in a rendering pipeline.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandListFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None        = 0U,         ///< No flags set. Creates a primary command list with single-use behavior.
    Secondary   = XII_BIT(0), ///< Command list is secondary and must be executed through a primary list. Limited commands allowed.
    MultiSubmit = XII_BIT(1), ///< Command list may be submitted multiple times without re-recording.

    Default = None
  };

  struct Bits
  {
    StorageType Secondary : 1;
    StorageType MultiSubmit : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALCommandListFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALCommandListFlags);

/// This describes the pipeline state shading rate flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSetVertexBufferFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None  = 0x0U,       ///< No additional operations.
    Reset = XII_BIT(0), ///< Reset the vertex buffers outside the range of the currently set vertex buffers. All buffers previously bound to the pipeline will be unbound.

    Default = None
  };

  struct Bits
  {
    StorageType Reset : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALSetVertexBufferFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALSetVertexBufferFlags);

/// This describes the resource state transition type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALStateTransitionType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Immediate = 0U, ///< Perform the state transition immediately.
    Begin,          ///< Begin split barrier. This mode only has effect in Direct3D12 backend, and corresponds to [D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY](https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_resource_barrier_flags) flag. See https://docs.microsoft.com/en-us/windows/desktop/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#split-barriers. In other implementations, begin-split barriers are ignored.
    End,            ///< End split barrier. This mode only has effect in Direct3D12 backend, and corresponds to [D3D12_RESOURCE_BARRIER_FLAG_END_ONLY](https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_resource_barrier_flags) flag. See https://docs.microsoft.com/en-us/windows/desktop/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#split-barriers. In other backends, this mode is similar to xiiGALStateTransitionType::Immediate.

    ENUM_COUNT,

    Default = Immediate
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALStateTransitionType);

/// Resource state transition flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALStateTransitionFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None        = 0U,            ///< No state transition flags.
    UpdateState = XII_BIT(0),    ///< Indicates that the internal resource state should be updated to the new state specified by xiiGALStateTransitionDescription, and the GAL should take over the resource state management. If an application was managing the resource state manually, it is responsible for making sure that all subresources are indeed in the designated state. If not used, internal resource state will be unchanged.
                                 ///
                                 ///  \note This flag cannot be used when xiiGALStateTransitionDescription.m_TransitionType is xiiGALStateTransitionType::Begin.
    DiscardContent = XII_BIT(1), ///< If set, the contents of the resource will be discarded, when possible. This may avoid potentially expensive operations such as render target decompression or a pipeline stall when transitioning to xiiGALResourceStateFlags::Common or xiiGALResourceStateFlags::UnorderedAccess state.
    Aliasing       = XII_BIT(2), ///< Indicates state transition between aliased resources that share the same memory. Currently, it is only supported for sparse resources that were created with aliasing flag.

    Default = None
  };

  struct Bits
  {
    StorageType UpdateState : 1;
    StorageType DiscardContent : 1;
    StorageType Aliasing : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALStateTransitionFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALStateTransitionFlags);

/// This describes the resource state transition mode.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALStateTransitionMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None = 0,   ///< Perform no state transitions and no validation. Resource states are not accessed (either read or written) by the command.
    Transition, ///< Transition resources to the states required by the specific command. Resources in unknown state are ignored.
                ///
                /// \note Any method that uses this mode may alter the state of the resources it works with.
                ///       As automatic state management is not thread-safe, no other thread is allowed to read or write the state of the resources being transitioned.
                ///       If the application intends to use the same resources in other threads simultaneously, it needs to explicitly manage the states using xiiGALCommandList::TransitionResourceStates() method.
                ///
                /// \note If a resource is used in multiple threads by multiple command lists, there will be race condition accessing internal resource state. An application should use manual resource state management in this case.
    Verify,     ///< Do not transition, but verify that states are correct. No validation is performed if the state is unknown to the GAL. This mode only has effect in debug and development builds. No validation is performed in shipping builds.
                ///
                /// \note Any method that uses this mode will read the state of resources it works with. As automatic state management is not thread-safe, no other thread is allowed to alter
                ///       the state of resources being used by the command. It is safe to read these states.

    ENUM_COUNT,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALStateTransitionMode);

/// This describes the viewport. A viewport defines the rendering area within a graphical output. It specifies the position, size, and depth range of the viewport to control how the scene is displayed.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALViewport : public xiiHashableStruct<xiiGALViewport>
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiGALViewport() = default;

  XII_ALWAYS_INLINE xiiGALViewport(float fX, float fY, float fWidth, float fHeight, float fMinDepth = 0.0f, float fMaxDepth = 1.0f) :
    m_fTopLeftX(fX), m_fTopLeftY(fY), m_fWidth(fWidth), m_fHeight(fHeight), m_fMinDepth(fMinDepth), m_fMaxDepth(fMaxDepth)
  {
  }

  XII_ALWAYS_INLINE xiiGALViewport(xiiRectFloat viewport, float fMinDepth = 0.0f, float fMaxDepth = 1.0f) :
    m_fTopLeftX(viewport.x), m_fTopLeftY(viewport.y), m_fWidth(viewport.width), m_fHeight(viewport.height), m_fMinDepth(fMinDepth), m_fMaxDepth(fMaxDepth)
  {
  }

  float m_fTopLeftX = 0.0f; ///< X-coordinate of the top-left corner of the viewport.
  float m_fTopLeftY = 0.0f; ///< Y-coordinate of the top-left corner of the viewport.
  float m_fWidth    = 0.0f; ///< Width of the viewport.
  float m_fHeight   = 0.0f; ///< Height of the viewport.
  float m_fMinDepth = 0.0f; ///< Minimum depth of the viewport range. The near clipping plane's depth value. Typically set to 0.0.
  float m_fMaxDepth = 1.0f; ///< Maximum depth of the viewport range. The far clipping plane's depth value. Typically set to 1.0.
};

/// This describes the viewport.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBeginRenderPassDescription : public xiiHashableStruct<xiiGALBeginRenderPassDescription>
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiGALBeginRenderPassDescription() = default;

  XII_ALWAYS_INLINE xiiGALBeginRenderPassDescription(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer) :
    m_pRenderPass(pRenderPass), m_pFramebuffer(pFramebuffer)
  {
  }

  XII_ALWAYS_INLINE xiiGALBeginRenderPassDescription(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pClearValues) :
    m_pRenderPass(pRenderPass), m_pFramebuffer(pFramebuffer), m_ClearValues(pClearValues)
  {
  }

  xiiGALRenderPass*                             m_pRenderPass;
  xiiGALFramebuffer*                            m_pFramebuffer;
  xiiStaticArray<xiiGALOptimizedClearValue, 4U> m_ClearValues;
};

/// This describes the resource state barrier description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALStateTransitionDescription
{
  XII_DECLARE_POD_TYPE();

  xiiGALResource* m_pPreviousResource = nullptr;                                                 ///< Previous resource for aliasing transition. This member is only used for aliasing transition (xiiGALStateTransitionFlags::Aliasing flag is set), and ignored otherwise, and must point to a texture or a buffer object.
                                                                                                 ///
                                                                                                 ///  \note pPreviousResource may be null, which indicates that any sparse or normal resource could cause aliasing.
  xiiGALResource* m_pResource = nullptr;                                                         ///< Resource to transition. Can be xiiGALTexture, xiiGALBuffer, xiiGALBottomLevelAS, xiiGALTopLevelAS.
                                                                                                 ///
                                                                                                 ///  \note For aliasing transition (xiiGALStateTransitionFlags::Aliasing flag is set), pResource may be null, which indicates that any sparse or normal resource could cause aliasing.
  xiiUInt32                             m_uiFirstMipLevel   = 0U;                                ///< When transitioning a texture, first mip level of the subresource range to transition.
  xiiUInt32                             m_uiMipLevelCount   = XII_GAL_REMAINING_MIP_LEVELS;      ///< When transitioning a texture, number of mip levels of the subresource range to transition.
  xiiUInt32                             m_uiFirstArraySlice = 0U;                                ///< When transitioning a texture, first array slice of the subresource range to transition.
  xiiUInt32                             m_uiArraySliceCount = XII_GAL_REMAINING_ARRAY_SLICES;    ///< When transitioning a texture, number of array slices of the subresource range to transition.
  xiiBitflags<xiiGALResourceStateFlags> m_OldState          = xiiGALResourceStateFlags::Unknown; ///< Resource state before transition. If this value is xiiGALResourceState::Unknown, internal resource state will be used, which must be defined in this case.
                                                                                                 ///
                                                                                                 ///  \note Resource state must be compatible with the command list queue type.
  xiiBitflags<xiiGALResourceStateFlags> m_NewState = xiiGALResourceStateFlags::Unknown;          ///< Resource state after transition. This must not be xiiGALResourceState::Unknown or xiiGALResourceState::Undefined.
                                                                                                 ///
                                                                                                 ///  \note Resource state must be compatible with the command list queue type.
  xiiEnum<xiiGALStateTransitionType> m_TransitionType = xiiGALStateTransitionType::Immediate;    ///< State transition type, see xiiGALStateTransitionType.
                                                                                                 ///
                                                                                                 ///  \note When issuing UAV barrier (i.e. OldState and NewState equal xiiGALResourceState::UnorderedAccess), the transition type must be xiiGALStateTransitionType::Immediate.
  xiiBitflags<xiiGALStateTransitionFlags> m_TransitionFlags = xiiGALStateTransitionFlags::None;  ///< State transition flags, see xiiGALStateTransitionFlags.
};

/// This describes multi-sampled texture resolve command arguments.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALResolveTextureSubresourceDescription
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiGALResolveTextureSubresourceDescription() = default;

  XII_ALWAYS_INLINE xiiGALResolveTextureSubresourceDescription(xiiUInt32 uiSourceMip, xiiUInt32 uiSourceSlice, xiiUInt32 uiDestinationMipLevel, xiiUInt32 uiDestinationSlice, xiiEnum<xiiGALResourceFormat> format = xiiGALResourceFormat::Unknown, xiiEnum<xiiGALStateTransitionMode> sourceMode = xiiGALStateTransitionMode::Transition, xiiEnum<xiiGALStateTransitionMode> destinationMode = xiiGALStateTransitionMode::Transition) :
    m_uiSourceMipLevel(uiSourceMip), m_uiSourceSlice(uiSourceSlice), m_SourceTextureTransitionMode(sourceMode), m_uiDestinationMipLevel(uiDestinationMipLevel), m_uiDestinationSlice(uiDestinationSlice), m_DestinationTextureTransitionMode(destinationMode), m_Format(format)
  {
  }

  xiiUInt32                          m_uiSourceMipLevel                 = 0U;                              ///< Mip level of the source multi-sampled texture to resolve.
  xiiUInt32                          m_uiSourceSlice                    = 0U;                              ///< Array slice of the source multi-sampled texture to resolve.
  xiiEnum<xiiGALStateTransitionMode> m_SourceTextureTransitionMode      = xiiGALStateTransitionMode::None; ///< Source texture state transition mode, see xiiGALStateTransitionMode.
  xiiUInt32                          m_uiDestinationMipLevel            = 0U;                              ///< Mip level of the destination non-multi-sampled texture.
  xiiUInt32                          m_uiDestinationSlice               = 0U;                              ///< Array slice of the destination non-multi-sampled texture.
  xiiEnum<xiiGALStateTransitionMode> m_DestinationTextureTransitionMode = xiiGALStateTransitionMode::None; ///< Destination texture state transition mode, see xiiGALStateTransitionMode.
  xiiEnum<xiiGALResourceFormat>      m_Format                           = xiiGALResourceFormat::Unknown;   ///< If one or both textures are typeless, specifies the type of the typeless texture. If both texture formats are not typeless, in which case they must be identical, this member must be either xiiGALResourceFormat::Unknown, or match this format.
};

/// Describes parameters for issuing non-indexed draw calls.
///
/// Defines the vertex and instance counts, as well as starting locations, for issuing a basic GPU draw call. Used in graphics command encoding where geometry is streamed directly from vertex buffers.
///
/// \see xiiGALCommandList::Draw
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDrawDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized draw description (0 vertices, 1 instance).
  XII_ALWAYS_INLINE xiiGALDrawDescription() = default;

  /// Constructs a draw description with specified parameters.
  ///
  /// \param uiVertexCount           - Number of vertices to draw.
  /// \param uiInstanceCount         - Number of instances to render. Defaults to 1.
  /// \param uiStartVertexLocation   - Index of the first vertex to read.
  /// \param uiFirstInstanceLocation - Instance ID for the first instance.
  XII_ALWAYS_INLINE xiiGALDrawDescription(xiiUInt32 uiVertexCount, xiiUInt32 uiInstanceCount = 1U, xiiUInt32 uiStartVertexLocation = 0U, xiiUInt32 uiFirstInstanceLocation = 0U) :
    m_uiVertexCount(uiVertexCount), m_uiInstanceCount(uiInstanceCount), m_uiStartVertexLocation(uiStartVertexLocation), m_uiFirstInstanceLocation(uiFirstInstanceLocation)
  {
  }

  xiiUInt32 m_uiVertexCount           = 0U; ///< Number of vertices to process.
  xiiUInt32 m_uiInstanceCount         = 1U; ///< Number of instances to render.
  xiiUInt32 m_uiStartVertexLocation   = 0U; ///< Start vertex offset within the bound vertex buffer.
  xiiUInt32 m_uiFirstInstanceLocation = 0U; ///< First instance ID passed to vertex shader.
};

/// Describes parameters for issuing indexed draw calls.
///
/// Defines the index and instance counts, index type, and offsets required for issuing GPU draw calls using an index buffer. Used in graphics command encoding for geometry instancing and reuse.
///
/// \see xiiGALCommandList::DrawIndexed
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDrawIndexedDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized indexed draw description (0 indices, 1 instance).
  XII_ALWAYS_INLINE xiiGALDrawIndexedDescription() = default;

  /// Constructs an indexed draw description with specified parameters.
  ///
  /// \param uiIndexCount            - Number of indices to process.
  /// \param IndexType               - Type of index data (e.g., 16-bit or 32-bit unsigned int).
  /// \param uiInstanceCount         - Number of instances to render. Defaults to 1.
  /// \param uiFirstIndexLocation    - Offset into the index buffer to start reading from.
  /// \param uiBaseVertex            - Value added to each index before fetching from the vertex buffer.
  /// \param uiFirstInstanceLocation - First instance ID passed to the vertex shader.
  XII_ALWAYS_INLINE xiiGALDrawIndexedDescription(xiiUInt32 uiIndexCount, xiiEnum<xiiGALValueType> IndexType, xiiUInt32 uiInstanceCount = 1U, xiiUInt32 uiFirstIndexLocation = 0U, xiiUInt32 uiBaseVertex = 0U, xiiUInt32 uiFirstInstanceLocation = 0U) :
    m_uiIndexCount(uiIndexCount), m_IndexType(IndexType), m_uiInstanceCount(uiInstanceCount), m_uiFirstIndexLocation(uiFirstIndexLocation), m_uiBaseVertex(uiBaseVertex), m_uiFirstInstanceLocation(uiFirstInstanceLocation)
  {
  }

  xiiUInt32                m_uiIndexCount            = 0U;                         ///< Number of indices to process.
  xiiEnum<xiiGALValueType> m_IndexType               = xiiGALValueType::Undefined; ///< Type of index data.
  xiiUInt32                m_uiInstanceCount         = 1U;                         ///< Number of instances to render.
  xiiUInt32                m_uiFirstIndexLocation    = 0U;                         ///< Offset into the index buffer to start reading from.
  xiiUInt32                m_uiBaseVertex            = 0U;                         ///< Value added to each index before fetching from the vertex buffer.
  xiiUInt32                m_uiFirstInstanceLocation = 0U;                         ///< First instance ID passed to the vertex shader.
};

/// Describes parameters for issuing indirect non-indexed draw calls.
///
/// Used to issue multiple draw calls from a GPU buffer containing draw arguments. Supports optional counter buffer for dynamic draw count.
///
/// \see xiiGALCommandList::DrawIndirect
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDrawIndirectDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized indirect draw description.
  XII_ALWAYS_INLINE xiiGALDrawIndirectDescription() = default;

  /// Constructs an indirect draw description with specified parameters.
  ///
  /// \param pBuffer                      - Buffer containing draw arguments.
  /// \param uiDrawCount                  - Number of draws to execute.
  /// \param uiDrawArgumentOffset         - Byte offset to the first draw argument.
  /// \param uiDrawArgumentStride         - Stride between draw arguments in bytes.
  /// \param bufferStateTransition        - Resource state transition mode for the draw buffer.
  /// \param pCounterBuffer               - Optional buffer containing draw count.
  /// \param uiCounterOffset              - Byte offset to the draw count value.
  /// \param counterBufferStateTransition - Resource state transition mode for the counter buffer.
  XII_ALWAYS_INLINE xiiGALDrawIndirectDescription(xiiGALBuffer* pBuffer, xiiUInt32 uiDrawCount = 1U, xiiUInt64 uiDrawArgumentOffset = 0U, xiiUInt32 uiDrawArgumentStride = 16U, xiiEnum<xiiGALStateTransitionMode> bufferStateTransition = xiiGALStateTransitionMode::None, xiiGALBuffer* pCounterBuffer = nullptr, xiiUInt64 uiCounterOffset = 0U, xiiEnum<xiiGALStateTransitionMode> counterBufferStateTransition = xiiGALStateTransitionMode::None) :
    m_pBuffer(pBuffer), m_uiDrawArgumentOffset(uiDrawArgumentOffset), m_uiDrawCount(uiDrawCount), m_uiDrawArgumentStride(uiDrawArgumentStride), m_BufferStateTransition(bufferStateTransition), m_pCounterBuffer(pCounterBuffer), m_uiCounterOffset(uiCounterOffset), m_CounterBufferStateTransition(counterBufferStateTransition)
  {
  }

  xiiGALBuffer*                      m_pBuffer                      = nullptr;                         ///< Buffer containing draw arguments.
  xiiUInt64                          m_uiDrawArgumentOffset         = 0U;                              ///< Byte offset to the first draw argument.
  xiiUInt32                          m_uiDrawCount                  = 1U;                              ///< Number of draws to execute.
  xiiUInt32                          m_uiDrawArgumentStride         = 16U;                             ///< Stride between draw arguments in bytes.
  xiiEnum<xiiGALStateTransitionMode> m_BufferStateTransition        = xiiGALStateTransitionMode::None; ///< State transition mode.
  xiiGALBuffer*                      m_pCounterBuffer               = nullptr;                         ///< Optional buffer containing draw count.
  xiiUInt64                          m_uiCounterOffset              = 0U;                              ///< Byte offset to the draw count value.
  xiiEnum<xiiGALStateTransitionMode> m_CounterBufferStateTransition = xiiGALStateTransitionMode::None; ///< State transition mode.
};

/// Describes parameters for issuing indirect indexed draw calls.
///
/// Used to issue multiple indexed draw calls from a GPU buffer containing draw arguments. Supports optional counter buffer and index type specification.
///
/// \see xiiGALCommandList::DrawIndexedIndirect
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDrawIndexedIndirectDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized indexed indirect draw description.
  XII_ALWAYS_INLINE xiiGALDrawIndexedIndirectDescription() = default;

  /// Constructs an indexed indirect draw description with specified parameters.
  ///
  /// \param indexType                    - Type of index data (e.g., 16-bit or 32-bit).
  /// \param pBuffer                      - Buffer containing draw arguments.
  /// \param uiDrawCount                  - Number of draws to execute.
  /// \param uiDrawArgumentOffset         - Byte offset to the first draw argument.
  /// \param uiDrawArgumentStride         - Stride between draw arguments in bytes.
  /// \param bufferStateTransition        - Resource state transition mode for the draw buffer.
  /// \param pCounterBuffer               - Optional buffer containing draw count.
  /// \param uiCounterOffset              - Byte offset to the draw count value.
  /// \param counterBufferStateTransition - Resource state transition mode for the counter buffer.
  XII_ALWAYS_INLINE xiiGALDrawIndexedIndirectDescription(xiiEnum<xiiGALValueType> indexType, xiiGALBuffer* pBuffer, xiiUInt32 uiDrawCount = 1U, xiiUInt64 uiDrawArgumentOffset = 0U, xiiUInt32 uiDrawArgumentStride = 20U, xiiEnum<xiiGALStateTransitionMode> bufferStateTransition = xiiGALStateTransitionMode::None, xiiGALBuffer* pCounterBuffer = nullptr, xiiUInt64 uiCounterOffset = 0U, xiiEnum<xiiGALStateTransitionMode> counterBufferStateTransition = xiiGALStateTransitionMode::None) :
    m_IndexType(indexType), m_pBuffer(pBuffer), m_uiDrawArgumentOffset(uiDrawArgumentOffset), m_uiDrawCount(uiDrawCount), m_uiDrawArgumentStride(uiDrawArgumentStride), m_BufferStateTransition(bufferStateTransition), m_pCounterBuffer(pCounterBuffer), m_uiCounterOffset(uiCounterOffset), m_CounterBufferStateTransition(counterBufferStateTransition)
  {
  }

  xiiEnum<xiiGALValueType>           m_IndexType                    = xiiGALValueType::Undefined;      ///< Type of index data.
  xiiGALBuffer*                      m_pBuffer                      = nullptr;                         ///< Buffer containing draw arguments.
  xiiUInt64                          m_uiDrawArgumentOffset         = 0U;                              ///< Byte offset to the first draw argument.
  xiiUInt32                          m_uiDrawCount                  = 1U;                              ///< Number of draws to execute.
  xiiUInt32                          m_uiDrawArgumentStride         = 20U;                             ///< Stride between draw arguments in bytes.
  xiiEnum<xiiGALStateTransitionMode> m_BufferStateTransition        = xiiGALStateTransitionMode::None; ///< State transition mode.
  xiiGALBuffer*                      m_pCounterBuffer               = nullptr;                         ///< Optional buffer containing draw count.
  xiiUInt64                          m_uiCounterOffset              = 0U;                              ///< Byte offset to the draw count value.
  xiiEnum<xiiGALStateTransitionMode> m_CounterBufferStateTransition = xiiGALStateTransitionMode::None; ///< State transition mode.
};

/// Describes parameters for issuing mesh shader draw calls.
///
/// Specifies the number of workgroups to dispatch for a meshlet-driven pipeline. Used for explicit, non-indirect mesh shader draws.
///
/// \see xiiGALCommandList::DrawMesh
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDrawMeshDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized mesh draw description (1 group on X axis).
  XII_ALWAYS_INLINE xiiGALDrawMeshDescription() = default;

  /// Constructs a draw mesh description with specified thread group dimensions.
  ///
  /// \param uiThreadGroupCountX - Number of thread groups along X.
  XII_ALWAYS_INLINE explicit xiiGALDrawMeshDescription(xiiUInt32 uiThreadGroupCountX) :
    m_uiThreadGroupCountX(uiThreadGroupCountX)
  {
  }

  /// Constructs a draw mesh description with X/Y group counts.
  XII_ALWAYS_INLINE xiiGALDrawMeshDescription(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY) :
    m_uiThreadGroupCountX(uiThreadGroupCountX), m_uiThreadGroupCountY(uiThreadGroupCountY)
  {
  }

  /// Constructs a draw mesh description with full 3D group dimensions.
  XII_ALWAYS_INLINE xiiGALDrawMeshDescription(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) :
    m_uiThreadGroupCountX(uiThreadGroupCountX), m_uiThreadGroupCountY(uiThreadGroupCountY), m_uiThreadGroupCountZ(uiThreadGroupCountZ)
  {
  }

  xiiUInt32 m_uiThreadGroupCountX = 1U; ///< Mesh thread groups along X.
  xiiUInt32 m_uiThreadGroupCountY = 1U; ///< Mesh thread groups along Y.
  xiiUInt32 m_uiThreadGroupCountZ = 1U; ///< Mesh thread groups along Z.
};

/// Describes parameters for issuing indirect mesh shader draw calls.
///
/// Pulls mesh dispatch arguments from a GPU buffer, with optional draw count via counter buffer.
///
/// \see xiiGALCommandList::DrawMeshIndirect
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDrawMeshIndirectDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized indirect mesh draw description.
  XII_ALWAYS_INLINE xiiGALDrawMeshIndirectDescription() = default;

  /// Constructs an indirect mesh draw description with specified parameters.
  ///
  /// \param pBuffer                      - Buffer containing mesh dispatch arguments.
  /// \param uiCommandCount               - Number of draws to execute.
  /// \param uiDrawArgumentOffset         - Byte offset to the first mesh dispatch argument.
  /// \param bufferStateTransition        - Resource state transition mode for the draw buffer.
  /// \param pCounterBuffer               - Optional buffer containing draw count.
  /// \param uiCounterOffset              - Byte offset to the draw count value.
  /// \param counterBufferStateTransition - Resource state transition mode for the counter buffer.
  XII_ALWAYS_INLINE xiiGALDrawMeshIndirectDescription(xiiGALBuffer* pBuffer, xiiUInt32 uiCommandCount, xiiUInt64 uiDrawArgumentOffset = 0U, xiiEnum<xiiGALStateTransitionMode> bufferStateTransition = xiiGALStateTransitionMode::None, xiiGALBuffer* pCounterBuffer = nullptr, xiiUInt64 uiCounterOffset = 0U, xiiEnum<xiiGALStateTransitionMode> counterBufferStateTransition = xiiGALStateTransitionMode::None) :
    m_pBuffer(pBuffer), m_uiDrawArgumentOffset(uiDrawArgumentOffset), m_uiCommandCount(uiCommandCount), m_BufferStateTransition(bufferStateTransition), m_pCounterBuffer(pCounterBuffer), m_uiCounterOffset(uiCounterOffset), m_CounterBufferStateTransition(counterBufferStateTransition)
  {
  }

  xiiGALBuffer*                      m_pBuffer                      = nullptr;                         ///< Buffer containing mesh dispatch arguments.
  xiiUInt64                          m_uiDrawArgumentOffset         = 0U;                              ///< Byte offset to the first mesh dispatch argument.
  xiiUInt32                          m_uiCommandCount               = 1U;                              ///< Number of draws to execute.
  xiiEnum<xiiGALStateTransitionMode> m_BufferStateTransition        = xiiGALStateTransitionMode::None; ///< State transition mode.
  xiiGALBuffer*                      m_pCounterBuffer               = nullptr;                         ///< Optional buffer containing draw count.
  xiiUInt64                          m_uiCounterOffset              = 0U;                              ///< Byte offset to the draw count value.
  xiiEnum<xiiGALStateTransitionMode> m_CounterBufferStateTransition = xiiGALStateTransitionMode::None; ///< State transition mode.
};

/// Represents a single non-indexed draw entry in a multi-draw call.
///
/// Specifies the number of vertices and the start vertex offset for one draw invocation.
///
/// \see xiiGALCommandList::MultiDraw
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMultiDrawItem
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized multi-draw item (0 vertices).
  XII_ALWAYS_INLINE xiiGALMultiDrawItem() = default;

  /// Constructs a multi-draw item with specified vertex count and offset.
  ///
  /// \param uiVertexCount         - Number of vertices to draw.
  /// \param uiStartVertexLocation - Starting vertex offset in the bound vertex buffer.
  XII_ALWAYS_INLINE xiiGALMultiDrawItem(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertexLocation = 0U) :
    m_uiVertexCount(uiVertexCount), m_uiStartVertexLocation(uiStartVertexLocation)
  {
  }

  xiiUInt32 m_uiVertexCount         = 0U; ///< Number of vertices to draw.
  xiiUInt32 m_uiStartVertexLocation = 0U; ///< Starting vertex offset in the bound vertex buffer.
};

/// Describes parameters for issuing a multi-draw call with unindexed geometry.
///
/// Provides an array of draw items and instance information for batched rendering.
///
/// \see xiiGALCommandList::MultiDraw
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMultiDrawDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized multi-draw description.
  XII_ALWAYS_INLINE xiiGALMultiDrawDescription() = default;

  /// Constructs a multi-draw description with specified items and parameters.
  ///
  /// \param pDrawItems              - Pointer to draw item array.
  /// \param uiInstanceCount         - Number of instances to render. Defaults to 1.
  /// \param uiFirstInstanceLocation - Instance ID for the first instance.
  XII_ALWAYS_INLINE xiiGALMultiDrawDescription(xiiArrayPtr<const xiiGALMultiDrawItem> pDrawItems, xiiUInt32 uiInstanceCount = 1U, xiiUInt32 uiFirstInstanceLocation = 0U) :
    m_pDrawItems(pDrawItems), m_uiInstanceCount(uiInstanceCount), m_uiFirstInstanceLocation(uiFirstInstanceLocation)
  {
  }

  xiiArrayPtr<const xiiGALMultiDrawItem> m_pDrawItems;                   ///< Pointer to array of draw entries.
  xiiUInt32                              m_uiInstanceCount         = 1U; ///< Number of instances to render.
  xiiUInt32                              m_uiFirstInstanceLocation = 0U; ///< First instance ID passed to vertex shader.
};

/// Represents a single indexed draw entry in a multi-draw call.
///
/// Specifies the number of indices, the first index offset, and base vertex for one draw invocation.
///
/// \see xiiGALCommandList::MultiDrawIndexed
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMultiDrawIndexedItem
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized indexed draw item (0 indices).
  XII_ALWAYS_INLINE xiiGALMultiDrawIndexedItem() = default;

  /// Constructs a multi-draw indexed item with specified index parameters.
  ///
  /// \param uiIndexCount         - Number of indices to draw.
  /// \param uiFirstIndexLocation - Start index in the bound index buffer.
  /// \param uiBaseVertex         - Value added to each index before vertex fetch.
  XII_ALWAYS_INLINE xiiGALMultiDrawIndexedItem(xiiUInt32 uiIndexCount, xiiUInt32 uiFirstIndexLocation = 0U, xiiUInt32 uiBaseVertex = 0U) :
    m_uiIndexCount(uiIndexCount), m_uiFirstIndexLocation(uiFirstIndexLocation), m_uiBaseVertex(uiBaseVertex)
  {
  }

  xiiUInt32 m_uiIndexCount         = 0U; ///< Number of indices to draw.
  xiiUInt32 m_uiFirstIndexLocation = 0U; ///< Start index in the bound index buffer.
  xiiUInt32 m_uiBaseVertex         = 0U; ///< Value added to each index before vertex fetch.
};

/// Describes parameters for issuing a multi-draw call with indexed geometry.
///
/// Provides an array of indexed draw items and instance-level information for batched rendering.
///
/// \see xiiGALCommandList::MultiDrawIndexed
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMultiDrawIndexedDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized indexed multi-draw description.
  XII_ALWAYS_INLINE xiiGALMultiDrawIndexedDescription() = default;

  /// Constructs an indexed multi-draw description with specified items and parameters.
  ///
  /// \param pDrawItems              - Pointer to indexed draw item array.
  /// \param IndexType               - Type of index data (e.g. 16-bit or 32-bit).
  /// \param uiInstanceCount         - Number of instances to render. Defaults to 1.
  /// \param uiFirstInstanceLocation - Instance ID for the first instance.
  XII_ALWAYS_INLINE xiiGALMultiDrawIndexedDescription(xiiArrayPtr<const xiiGALMultiDrawIndexedItem> pDrawItems, xiiEnum<xiiGALValueType> IndexType, xiiUInt32 uiInstanceCount = 1U, xiiUInt32 uiFirstInstanceLocation = 0U) :
    m_pDrawItems(pDrawItems), m_IndexType(IndexType), m_uiInstanceCount(uiInstanceCount), m_uiFirstInstanceLocation(uiFirstInstanceLocation)
  {
  }

  xiiArrayPtr<const xiiGALMultiDrawIndexedItem> m_pDrawItems;                                           ///< Pointer to indexed draw entries.
  xiiEnum<xiiGALValueType>                      m_IndexType               = xiiGALValueType::Undefined; ///< Type of index data.
  xiiUInt32                                     m_uiInstanceCount         = 1U;                         ///< Number of instances to render. If more than one instances are specified, an instanced draw call will be performed.
  xiiUInt32                                     m_uiFirstInstanceLocation = 0U;                         ///< First instance ID passed to vertex shader.
};

/// Describes parameters for issuing a compute dispatch call.
///
/// Specifies the number of thread groups to launch in each dimension. Metal-specific thread group sizes may be optionally provided for backend tuning.
///
/// \see xiiGALCommandList::Dispatch
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDispatchComputeDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized compute dispatch (1 group in each dimension).
  XII_ALWAYS_INLINE xiiGALDispatchComputeDescription() = default;

  /// Constructs a compute dispatch with specified group counts.
  ///
  /// \param uiGroupCountX - Number of thread groups along X.
  /// \param uiGroupCountY - Number of thread groups along Y.
  /// \param uiGroupCountZ - Number of thread groups along Z. Defaults to 1.
  XII_ALWAYS_INLINE xiiGALDispatchComputeDescription(xiiUInt32 uiGroupCountX, xiiUInt32 uiGroupCountY, xiiUInt32 uiGroupCountZ = 1U) :
    m_uiThreadGroupCountX(uiGroupCountX), m_uiThreadGroupCountY(uiGroupCountY), m_uiThreadGroupCountZ(uiGroupCountZ)
  {
  }

  xiiUInt32 m_uiThreadGroupCountX = 1U; ///< Thread groups along X.
  xiiUInt32 m_uiThreadGroupCountY = 1U; ///< Thread groups along Y.
  xiiUInt32 m_uiThreadGroupCountZ = 1U; ///< Thread groups along Z.

  xiiUInt32 m_uiMtlThreadGroupSizeX = 0U; ///< Metal-specific override for threads per group (X).
  xiiUInt32 m_uiMtlThreadGroupSizeY = 0U; ///< Metal-specific override for threads per group (Y).
  xiiUInt32 m_uiMtlThreadGroupSizeZ = 0U; ///< Metal-specific override for threads per group (Z).
};

/// Describes parameters for issuing an indirect compute dispatch call.
///
/// Dispatch arguments are read from a GPU buffer. Metal-specific thread group sizes may be optionally provided for backend tuning.
///
/// \see xiiGALCommandList::DispatchIndirect
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDispatchComputeIndirectDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized indirect compute dispatch.
  XII_ALWAYS_INLINE xiiGALDispatchComputeIndirectDescription() = default;

  /// Constructs an indirect compute dispatch with specified parameters.
  ///
  /// \param pBuffer                  - Buffer containing dispatch arguments.
  /// \param bufferTransitionMode     - Resource state transition mode for the buffer.
  /// \param uiDispatchArgumentOffset - Byte offset to the dispatch arguments.
  XII_ALWAYS_INLINE xiiGALDispatchComputeIndirectDescription(xiiGALBuffer* pBuffer, xiiEnum<xiiGALStateTransitionMode> bufferTransitionMode, xiiUInt64 uiDispatchArgumentOffset = 0U) :
    m_pBuffer(pBuffer), m_BufferTransitionMode(bufferTransitionMode), m_uiDispatchArgumentOffset(uiDispatchArgumentOffset)
  {
  }

  xiiGALBuffer*                      m_pBuffer                  = nullptr;                         ///< Buffer containing dispatch arguments.
  xiiEnum<xiiGALStateTransitionMode> m_BufferTransitionMode     = xiiGALStateTransitionMode::None; ///< State transition mode.
  xiiUInt64                          m_uiDispatchArgumentOffset = 0U;                              ///< Byte offset to the dispatch arguments.

  xiiUInt32 m_uiMtlThreadGroupSizeX = 0U; ///< Metal-specific override for threads per group (X).
  xiiUInt32 m_uiMtlThreadGroupSizeY = 0U; ///< Metal-specific override for threads per group (Y).
  xiiUInt32 m_uiMtlThreadGroupSizeZ = 0U; ///< Metal-specific override for threads per group (Z).
};

/// Describes one shader binding table (SBT) region.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingSBTRegionDescription
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64 m_uiOffset = 0U; ///< Byte offset into the SBT buffer.
  xiiUInt64 m_uiSize   = 0U; ///< Byte size of this SBT region.
  xiiUInt64 m_uiStride = 0U; ///< Byte stride between SBT records in this region.
};

/// Describes parameters for issuing a ray tracing dispatch.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTraceRaysDescription
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiGALTraceRaysDescription() = default;

  XII_ALWAYS_INLINE xiiGALTraceRaysDescription(xiiGALBuffer* pShaderBindingTable, xiiUInt32 uiWidth, xiiUInt32 uiHeight = 1U, xiiUInt32 uiDepth = 1U, xiiEnum<xiiGALStateTransitionMode> sbtTransitionMode = xiiGALStateTransitionMode::Transition) :
    m_pShaderBindingTable(pShaderBindingTable), m_uiWidth(uiWidth), m_uiHeight(uiHeight), m_uiDepth(uiDepth), m_ShaderBindingTableTransitionMode(sbtTransitionMode)
  {
  }

  xiiGALBuffer*                        m_pShaderBindingTable = nullptr;                                      ///< SBT source buffer.
  xiiGALRayTracingSBTRegionDescription m_RayGenerationTable;                                                 ///< Ray generation table region.
  xiiGALRayTracingSBTRegionDescription m_MissTable;                                                          ///< Miss table region.
  xiiGALRayTracingSBTRegionDescription m_HitTable;                                                           ///< Hit table region.
  xiiGALRayTracingSBTRegionDescription m_CallableTable;                                                      ///< Callable table region.
  xiiUInt32                            m_uiWidth                          = 1U;                              ///< Dispatch width.
  xiiUInt32                            m_uiHeight                         = 1U;                              ///< Dispatch height.
  xiiUInt32                            m_uiDepth                          = 1U;                              ///< Dispatch depth.
  xiiEnum<xiiGALStateTransitionMode>   m_ShaderBindingTableTransitionMode = xiiGALStateTransitionMode::None; ///< SBT buffer state transition mode.
};

/// Describes parameters for issuing an indirect ray tracing dispatch.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTraceRaysIndirectDescription
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiGALTraceRaysIndirectDescription() = default;

  XII_ALWAYS_INLINE xiiGALTraceRaysIndirectDescription(xiiGALBuffer* pShaderBindingTable, xiiGALBuffer* pArgumentBuffer, xiiUInt64 uiArgumentOffset = 0U, xiiEnum<xiiGALStateTransitionMode> sbtTransitionMode = xiiGALStateTransitionMode::Transition, xiiEnum<xiiGALStateTransitionMode> argumentTransitionMode = xiiGALStateTransitionMode::Transition) :
    m_pShaderBindingTable(pShaderBindingTable), m_pArgumentBuffer(pArgumentBuffer), m_uiArgumentOffset(uiArgumentOffset), m_ShaderBindingTableTransitionMode(sbtTransitionMode), m_ArgumentBufferTransitionMode(argumentTransitionMode)
  {
  }

  xiiGALBuffer*                        m_pShaderBindingTable = nullptr;                                      ///< SBT source buffer.
  xiiGALRayTracingSBTRegionDescription m_RayGenerationTable;                                                 ///< Ray generation table region.
  xiiGALRayTracingSBTRegionDescription m_MissTable;                                                          ///< Miss table region.
  xiiGALRayTracingSBTRegionDescription m_HitTable;                                                           ///< Hit table region.
  xiiGALRayTracingSBTRegionDescription m_CallableTable;                                                      ///< Callable table region.
  xiiGALBuffer*                        m_pArgumentBuffer                  = nullptr;                         ///< Indirect dispatch arguments buffer (3 x uint32: width, height, depth).
  xiiUInt64                            m_uiArgumentOffset                 = 0U;                              ///< Byte offset into the indirect arguments buffer.
  xiiEnum<xiiGALStateTransitionMode>   m_ShaderBindingTableTransitionMode = xiiGALStateTransitionMode::None; ///< SBT buffer state transition mode.
  xiiEnum<xiiGALStateTransitionMode>   m_ArgumentBufferTransitionMode     = xiiGALStateTransitionMode::None; ///< Indirect argument buffer state transition mode.
};

/// Describes parameters for updating shader binding table records from a ray tracing pipeline.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALUpdateSBTDescription
{
  XII_DECLARE_POD_TYPE();

  xiiGALRayTracingPipelineState* m_pPipelineState      = nullptr; ///< Optional pipeline source. If null, currently bound ray tracing pipeline is used.
  xiiGALBuffer*                  m_pShaderBindingTable = nullptr; ///< Destination SBT buffer.

  xiiGALRayTracingSBTRegionDescription m_RayGenerationTable; ///< Ray generation SBT region to update.
  xiiGALRayTracingSBTRegionDescription m_MissTable;          ///< Miss SBT region to update.
  xiiGALRayTracingSBTRegionDescription m_HitTable;           ///< Hit SBT region to update.
  xiiGALRayTracingSBTRegionDescription m_CallableTable;      ///< Callable SBT region to update.

  xiiUInt32 m_uiRayGenerationShaderStartIndex = 0U; ///< Start index into ray generation shader groups.
  xiiUInt32 m_uiMissShaderStartIndex          = 0U; ///< Start index into miss shader groups.
  xiiUInt32 m_uiHitGroupStartIndex            = 0U; ///< Start index into hit shader groups.
  xiiUInt32 m_uiCallableShaderStartIndex      = 0U; ///< Start index into callable shader groups.

  xiiEnum<xiiGALStateTransitionMode> m_ShaderBindingTableTransitionMode = xiiGALStateTransitionMode::Transition; ///< SBT buffer state transition mode.
};

/// BLAS triangle build input data for one geometry description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBLASTriangleBuildDescription
{
  XII_DECLARE_POD_TYPE();

  xiiGALBuffer* m_pVertexBuffer        = nullptr; ///< Vertex buffer for this geometry.
  xiiUInt64     m_uiVertexBufferOffset = 0U;      ///< Byte offset into the vertex buffer.
  xiiUInt64     m_uiVertexStride       = 0U;      ///< Vertex stride in bytes.
  xiiGALBuffer* m_pIndexBuffer         = nullptr; ///< Optional index buffer. If null, non-indexed geometry is used.
  xiiUInt64     m_uiIndexBufferOffset  = 0U;      ///< Byte offset into the index buffer.
  xiiGALBuffer* m_pTransformBuffer     = nullptr; ///< Optional transform buffer.
  xiiUInt64     m_uiTransformOffset    = 0U;      ///< Byte offset to a 3x4 transform matrix.
  xiiUInt32     m_uiPrimitiveCount     = 0U;      ///< Primitive count for this build; if zero, the BLAS max primitive count is used.
};

/// BLAS axis-aligned bounding-box build input data for one geometry description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBLASBoundingBoxBuildDescription
{
  XII_DECLARE_POD_TYPE();

  xiiGALBuffer* m_pBoundingBoxBuffer  = nullptr; ///< Buffer containing AABB data.
  xiiUInt64     m_uiBoundingBoxOffset = 0U;      ///< Byte offset into the AABB buffer.
  xiiUInt64     m_uiBoundingBoxStride = 0U;      ///< Byte stride between AABB records.
  xiiUInt32     m_uiBoxCount          = 0U;      ///< Number of boxes for this build; if zero, the BLAS max box count is used.
};

/// Describes parameters for building or updating a bottom-level acceleration structure.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBuildBLASDescription
{
  XII_DECLARE_POD_TYPE();

  xiiGALBottomLevelAS*                                   m_pBottomLevelAS        = nullptr;                                     ///< BLAS target.
  xiiGALBuffer*                                          m_pScratchBuffer        = nullptr;                                     ///< Scratch buffer used during build.
  xiiUInt64                                              m_uiScratchBufferOffset = 0U;                                          ///< Byte offset into scratch buffer.
  xiiDynamicArray<xiiGALBLASTriangleBuildDescription>    m_Triangles;                                                           ///< Build data for triangle geometries.
  xiiDynamicArray<xiiGALBLASBoundingBoxBuildDescription> m_BoundingBoxes;                                                       ///< Build data for AABB geometries.
  xiiBitflags<xiiGALRayTracingBuildASFlags>              m_BuildFlags                  = xiiGALRayTracingBuildASFlags::None;    ///< Runtime build flags.
  bool                                                   m_bUpdate                     = false;                                 ///< Update existing AS instead of full rebuild.
  xiiEnum<xiiGALStateTransitionMode>                     m_ResourceStateTransitionMode = xiiGALStateTransitionMode::Transition; ///< State transition mode used for all referenced resources.
};

/// Describes parameters for building or updating a top-level acceleration structure.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBuildTLASDescription
{
  XII_DECLARE_POD_TYPE();

  xiiGALTopLevelAS*                         m_pTopLevelAS                 = nullptr;                               ///< TLAS target.
  xiiGALBuffer*                             m_pInstanceBuffer             = nullptr;                               ///< Instance description buffer.
  xiiUInt64                                 m_uiInstanceBufferOffset      = 0U;                                    ///< Byte offset into instance buffer.
  xiiUInt32                                 m_uiInstanceCount             = 0U;                                    ///< Number of instances.
  xiiGALBuffer*                             m_pScratchBuffer              = nullptr;                               ///< Scratch buffer used during build.
  xiiUInt64                                 m_uiScratchBufferOffset       = 0U;                                    ///< Byte offset into scratch buffer.
  xiiBitflags<xiiGALRayTracingBuildASFlags> m_BuildFlags                  = xiiGALRayTracingBuildASFlags::None;    ///< Runtime build flags.
  bool                                      m_bUpdate                     = false;                                 ///< Update existing AS instead of full rebuild.
  xiiEnum<xiiGALStateTransitionMode>        m_ResourceStateTransitionMode = xiiGALStateTransitionMode::Transition; ///< State transition mode used for all referenced resources.
};

/// AS copy mode for BLAS and TLAS copy commands.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALASCopyMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Clone = 0U, ///< Copy without changing storage format.
    Compact,    ///< Copy into compacted form.

    ENUM_COUNT,

    Default = Clone
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALASCopyMode);

/// Describes parameters for copying a BLAS.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCopyBLASDescription
{
  XII_DECLARE_POD_TYPE();

  xiiGALBottomLevelAS*               m_pSourceBottomLevelAS        = nullptr;                               ///< Source BLAS.
  xiiGALBottomLevelAS*               m_pDestinationBottomLevelAS   = nullptr;                               ///< Destination BLAS.
  xiiEnum<xiiGALASCopyMode>          m_Mode                        = xiiGALASCopyMode::Clone;               ///< Copy mode.
  xiiEnum<xiiGALStateTransitionMode> m_ResourceStateTransitionMode = xiiGALStateTransitionMode::Transition; ///< State transition mode used for both source and destination AS.
};

/// Describes parameters for copying a TLAS.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCopyTLASDescription
{
  XII_DECLARE_POD_TYPE();

  xiiGALTopLevelAS*                  m_pSourceTopLevelAS           = nullptr;                               ///< Source TLAS.
  xiiGALTopLevelAS*                  m_pDestinationTopLevelAS      = nullptr;                               ///< Destination TLAS.
  xiiEnum<xiiGALASCopyMode>          m_Mode                        = xiiGALASCopyMode::Clone;               ///< Copy mode.
  xiiEnum<xiiGALStateTransitionMode> m_ResourceStateTransitionMode = xiiGALStateTransitionMode::Transition; ///< State transition mode used for both source and destination AS.
};

/// Describes parameters for writing BLAS compacted size into a buffer.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALWriteBLASCompactedSizeDescription
{
  XII_DECLARE_POD_TYPE();

  xiiGALBottomLevelAS*               m_pBottomLevelAS              = nullptr;                               ///< Source BLAS whose compacted size is queried.
  xiiGALBuffer*                      m_pDestinationBuffer          = nullptr;                               ///< Destination buffer receiving one 64-bit compacted size.
  xiiUInt64                          m_uiDestinationBufferOffset   = 0U;                                    ///< Byte offset into destination buffer.
  xiiEnum<xiiGALStateTransitionMode> m_ResourceStateTransitionMode = xiiGALStateTransitionMode::Transition; ///< State transition mode used for source AS and destination buffer.
};

/// Describes parameters for writing TLAS compacted size into a buffer.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALWriteTLASCompactedSizeDescription
{
  XII_DECLARE_POD_TYPE();

  xiiGALTopLevelAS*                  m_pTopLevelAS                 = nullptr;                               ///< Source TLAS whose compacted size is queried.
  xiiGALBuffer*                      m_pDestinationBuffer          = nullptr;                               ///< Destination buffer receiving one 64-bit compacted size.
  xiiUInt64                          m_uiDestinationBufferOffset   = 0U;                                    ///< Byte offset into destination buffer.
  xiiEnum<xiiGALStateTransitionMode> m_ResourceStateTransitionMode = xiiGALStateTransitionMode::Transition; ///< State transition mode used for source AS and destination buffer.
};

/// Describes parameters for issuing a tile-based compute dispatch.
///
/// Used for tile shaders or compute workloads that operate on screen-space tiles.
///
/// \see xiiGALCommandList::DispatchTile
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDispatchTileDescription
{
  XII_DECLARE_POD_TYPE();

  /// Default-initialized tile dispatch (1x1 tile).
  XII_ALWAYS_INLINE xiiGALDispatchTileDescription() = default;

  /// Constructs a tile dispatch with specified tile dimensions and flags.
  ///
  /// \param uiThreadsPerTileX - Threads per tile along X.
  /// \param uiThreadsPerTileY - Threads per tile along Y.
  XII_ALWAYS_INLINE xiiGALDispatchTileDescription(xiiUInt32 uiThreadsPerTileX, xiiUInt32 uiThreadsPerTileY) :
    m_uiThreadsPerTileX(uiThreadsPerTileX), m_uiThreadsPerTileY(uiThreadsPerTileY)
  {
  }

  xiiUInt32 m_uiThreadsPerTileX = 1U; ///< Threads per tile along X.
  xiiUInt32 m_uiThreadsPerTileY = 1U; ///< Threads per tile along Y.
};

/// This describes the command list API call counters.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandListCounters
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiSetPipelineState          = 0U; ///< The total number of SetPipelineState calls.
  xiiUInt32 m_uiCommitShaderResources     = 0U; ///< The total number of CommitShaderResources calls.
  xiiUInt32 m_uiSetVertexBuffers          = 0U; ///< The total number of SetVertexBuffers calls.
  xiiUInt32 m_uiSetIndexBuffer            = 0U; ///< The total number of SetIndexBuffer calls.
  xiiUInt32 m_uiSetBlendFactors           = 0U; ///< The total number of SetBlendFactors calls.
  xiiUInt32 m_uiSetStencilRef             = 0U; ///< The total number of SetStencilRef calls.
  xiiUInt32 m_uiSetViewports              = 0U; ///< The total number of SetViewports calls.
  xiiUInt32 m_uiSetScissorRects           = 0U; ///< The total number of SetScissorRects calls.
  xiiUInt32 m_uiBeginRenderPass           = 0U; ///< The total number of BeginRenderPass calls.
  xiiUInt32 m_uiNextSubPass               = 0U; ///< The total number of NextSubPass calls.
  xiiUInt32 m_uiClearRenderTarget         = 0U; ///< The total number of ClearRenderTarget calls.
  xiiUInt32 m_uiClearDepthStencil         = 0U; ///< The total number of ClearDepthStencil calls.
  xiiUInt32 m_uiDraw                      = 0U; ///< The total number of Draw calls.
  xiiUInt32 m_uiDrawIndexed               = 0U; ///< The total number of DrawIndexed calls.
  xiiUInt32 m_uiDrawIndirect              = 0U; ///< The total number of indirect DrawIndirect calls.
  xiiUInt32 m_uiDrawIndexedIndirect       = 0U; ///< The total number of indexed indirect DrawIndexedIndirect calls.
  xiiUInt32 m_uiMultiDraw                 = 0U; ///< The total number of MultiDraw calls.
  xiiUInt32 m_uiMultiDrawIndexed          = 0U; ///< The total number of MultiDrawIndexed calls.
  xiiUInt32 m_uiDispatchCompute           = 0U; ///< The total number of DispatchCompute calls.
  xiiUInt32 m_uiDispatchComputeIndirect   = 0U; ///< The total number of DispatchComputeIndirect calls.
  xiiUInt32 m_uiDispatchTile              = 0U; ///< The total number of DispatchTile calls.
  xiiUInt32 m_uiDrawMesh                  = 0U; ///< The total number of DrawMesh calls.
  xiiUInt32 m_uiDrawMeshIndirect          = 0U; ///< The total number of DrawMeshIndirect calls.
  xiiUInt32 m_uiBuildBLAS                 = 0U; ///< The total number of BuildBLAS calls.
  xiiUInt32 m_uiBuildTLAS                 = 0U; ///< The total number of BuildTLAS calls.
  xiiUInt32 m_uiCopyBLAS                  = 0U; ///< The total number of CopyBLAS calls.
  xiiUInt32 m_uiCopyTLAS                  = 0U; ///< The total number of CopyTLAS calls.
  xiiUInt32 m_uiWriteBLASCompactedSize    = 0U; ///< The total number of WriteBLASCompactedSize calls.
  xiiUInt32 m_uiWriteTLASCompactedSize    = 0U; ///< The total number of WriteTLASCompactedSize calls.
  xiiUInt32 m_uiTraceRays                 = 0U; ///< The total number of TraceRays calls.
  xiiUInt32 m_uiTraceRaysIndirect         = 0U; ///< The total number of TraceRaysIndirect calls.
  xiiUInt32 m_uiUpdateSBT                 = 0U; ///< The total number of UpdateSBT calls.
  xiiUInt32 m_uiUpdateBuffer              = 0U; ///< The total number of UpdateBuffer calls.
  xiiUInt32 m_uiCopyBuffer                = 0U; ///< The total number of CopyBuffer calls.
  xiiUInt32 m_uiMapBuffer                 = 0U; ///< The total number of MapBuffer calls.
  xiiUInt32 m_uiUpdateTexture             = 0U; ///< The total number of UpdateTexture calls.
  xiiUInt32 m_uiCopyTexture               = 0U; ///< The total number of CopyTexture calls.
  xiiUInt32 m_uiMapTextureSubresource     = 0U; ///< The total number of MapTextureSubresource calls.
  xiiUInt32 m_uiBeginQuery                = 0U; ///< The total number of BeginQuery calls.
  xiiUInt32 m_uiGenerateMips              = 0U; ///< The total number of GenerateMips calls.
  xiiUInt32 m_uiResolveTextureSubresource = 0U; ///< The total number of ResolveTextureSubresource calls.
  xiiUInt32 m_uiBindSparseResourceMemory  = 0U; ///< The total number of BindSparseResourceMemory calls.
  xiiUInt32 m_uiSubmit                    = 0U; ///< The total number of Submit calls.

  XII_ALWAYS_INLINE void operator+=(const xiiGALCommandListCounters& rhs)
  {
    const xiiUInt32  uiCount      = sizeof(xiiGALCommandListCounters) / sizeof(xiiUInt32);
    xiiUInt32*       pDestination = reinterpret_cast<xiiUInt32*>(this);
    const xiiUInt32* pSource      = reinterpret_cast<const xiiUInt32*>(&rhs);

    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      pDestination[i] += pSource[i];
    }
  }
};

/// This describes the command list statistics.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandListStatistics
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                 m_PrimitiveCounters[xiiGALPrimitiveTopology::ENUM_COUNT] = {0U}; ///< Number of primitives drawn for each primitive topology.
  xiiGALCommandListCounters m_CommandListCounters;                                           ///< Command list counters.

  void SetStatistics();

  XII_ALWAYS_INLINE void operator+=(const xiiGALCommandListStatistics& rhs)
  {
    for (xiiUInt32 i = 0; i < xiiGALPrimitiveTopology::ENUM_COUNT; ++i)
    {
      m_PrimitiveCounters[i] += rhs.m_PrimitiveCounters[i];
    }
    m_CommandListCounters += rhs.m_CommandListCounters;
  }

  /// This returns the total number of triangles drawn.
  XII_ALWAYS_INLINE constexpr xiiUInt32 GetTotalTriangleCount() const
  {
    return m_PrimitiveCounters[xiiGALPrimitiveTopology::TriangleList] + m_PrimitiveCounters[xiiGALPrimitiveTopology::TriangleStrip] + m_PrimitiveCounters[xiiGALPrimitiveTopology::TriangleStripAdjacent];
  }

  /// This returns the total number of lines drawn.
  XII_ALWAYS_INLINE constexpr xiiUInt32 GetTotalLineCount() const
  {
    return m_PrimitiveCounters[xiiGALPrimitiveTopology::LineList] + m_PrimitiveCounters[xiiGALPrimitiveTopology::LineStrip] + m_PrimitiveCounters[xiiGALPrimitiveTopology::LineStripAdjacent];
  }

  /// This returns the total number of points drawn.
  XII_ALWAYS_INLINE constexpr xiiUInt32 GetTotalPointCount() const
  {
    return m_PrimitiveCounters[xiiGALPrimitiveTopology::PointList];
  }
};

/// Describes the parameters for creating a GAL command list.
///
/// This structure is used to configure a command list in the Graphics Abstraction Layer (GAL).
/// It defines which queue capabilities this command list targets and how it behaves in terms of submission and encoding.
///
/// The queue flags represent the functional domains this command list is allowed to access (e.g., graphics, compute, copy), which are used to validate command recording.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandListCreationDescription : public xiiHashableStruct<xiiGALCommandListCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  /// Specifies the functional capabilities required by this command list.
  ///
  /// This bitmask defines which domains the command list can operate in (such as graphics, compute, or copy), and is used to validate that recorded commands are compatible with the submission queue.
  ///
  /// For example, command lists with graphics draw calls must declare support for the Graphics flag.
  xiiBitflags<xiiGALCommandQueueFlags> m_QueueFlags = xiiGALCommandQueueFlags::None;

  /// Flags controlling command list submission and recording behavior.
  ///
  /// These flags define whether the command list is secondary, supports multiple submissions, or is immediately submitted after encoding.
  /// Use these flags to optimize command list lifetimes and submission patterns.
  xiiBitflags<xiiGALCommandListFlags> m_Flags = xiiGALCommandListFlags::None;

  /// Specifies the render pass to be used when recording this command list.
  ///
  /// The render pass defines the sequence of rendering operations and attachment formats.
  /// This must match the layout expected by the framebuffer. Required for command lists that record graphics operations within a render pass scope.
  xiiGALRenderPass* m_pRenderPass = nullptr;

  /// Specifies the framebuffer associated with the selected render pass.
  ///
  /// The framebuffer provides the actual image attachments used during rendering.
  /// It must be compatible with the render pass and is required when submitting graphics commands that depend on render targets.
  xiiGALFramebuffer* m_pFramebuffer = nullptr;

  /// Indicates the subpass within the render pass that this command list targets.
  ///
  /// Used to determine which subpass to begin encoding commands in.
  /// If multiple subpasses are defined in the render pass, this value selects the active one during recording.
  /// Must be within the bounds defined by the render pass configuration.
  xiiUInt32 m_uiSubPassIndex = 0U;
};

/// Interface that defines methods to manipulate a command list object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandList : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandList, xiiGALDeviceObject);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALCommandListCreationDescription& GetDescription() const { return m_Description; };

  /// This returns the command list statistics.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALCommandListStatistics& GetCommandListStatistics() const { return m_CommandListStatistics; };

public:
  /// Begins the command list for recording commands. This method should be called before any command is issued.
  ///
  /// \remarks This method is called automatically when using xiiGALCommandQueue::BeginCommandList to request a command list.
  void Begin();

  /// Ends the command list. This method should be called after all commands are issued.
  ///
  /// \remarks This method is called automatically when using xiiGALCommandQueue::Submit execute a command list.
  void End();

  /// Resets the command list. This method is used to clear all commands that have been recorded in the command list.
  ///
  /// \remarks This method can be called only if the command list has not yet been submitted for execution.
  void Reset();

  /// Submits a secondary command list to a primary command list for execution.
  void Submit(xiiGALCommandList* pSecondaryCommandList);

  // State functions.

  /// Sets the pipeline state object for the command list.
  ///
  /// \param pPipelineState - The handle to the pipeline state object.
  void SetPipelineState(xiiGALPipelineState* pPipelineState);

  /// Writes push constant data to the command list.
  ///
  /// \param uiOffset - Byte offset into the push constant block.
  /// \param pData    - Pointer to the source data.
  void PushConstants(xiiUInt32 uiOffset, xiiArrayPtr<xiiUInt8> pData);

  /// Sets the stencil reference value used in the stencil test.
  ///
  /// \param uiStencilRef - Stencil reference value.
  void SetStencilRef(xiiUInt32 uiStencilRef);

  /// Sets the blend factors used in the blend state.
  ///
  /// \param blendFactor - The blend factors represented by a xiiColor.
  void SetBlendFactor(const xiiColor& blendFactor);

  /// Sets the viewports used in the rasterizer stage. This defines the area of the render target to which the rasterizer will clip.
  ///
  /// \param pViewports - The array of viewports structures, describing the viewports to bind.
  void SetViewports(xiiArrayPtr<const xiiGALViewport> pViewports);

  /// Sets a single viewport for the rasterizer stage. This simplifies the process when only one viewport is needed.
  ///
  /// \param viewport - The viewport structure describing the area to bind.
  XII_ALWAYS_INLINE void SetViewport(const xiiGALViewport& viewport) { SetViewports(xiiMakeArrayPtr(&viewport, 1U)); }

  /// Sets the scissor rectangles used in the rasterizer stage. This defines the area of the render target to which the rasterizer will clip.
  ///
  /// \param pRects - The array of rectangle structures, describing the scissor rectangles to bind.
  void SetScissorRects(xiiArrayPtr<const xiiRectU32> pRects);

  /// Sets a single scissor rectangle for the rasterizer stage. This simplifies the process when only one scissor rectangle is needed.
  ///
  /// \param rect - The rectangle describing the area to bind.
  XII_ALWAYS_INLINE void SetScissorRect(const xiiRectU32& rect) { SetScissorRects(xiiMakeArrayPtr(&rect, 1U)); }

  /// Sets the index buffer for the input-assembler stage of the pipeline. This contains the indices into the vertex buffers.
  ///
  /// \param pIndexBuffer   - The handle to the index buffer object. The index buffer must be created with the xiiGALBindFlags::IndexBuffer bind flag.
  /// \param uiByteOffset   - The byte offset into the index buffer. That is, from the beginning of the buffer to the start of the index data.
  /// \param transitionMode - Resource state transition mode. Specifies whether the buffer state should be transitioned to the required state automatically.
  void SetIndexBuffer(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset = 0U, xiiEnum<xiiGALStateTransitionMode> transitionMode = xiiGALStateTransitionMode::Transition);

  /// Sets the vertex buffers for the input-assembler stage of the pipeline. This contains the vertex data.
  ///
  /// \param uiStartSlot    - The starting slot for the vertex buffers, which is the first input slot for binding. The first vertex buffer is explicitly bound to the start slot; each additional vertex buffer in the array is implicitly bound to each subsequent input slot.
  /// \param pVertexBuffers - The array of handles to the vertex buffer objects. The vertex buffers must be created with the xiiGALBindFlags::VertexBuffer bind flag.
  /// \param pByteOffsets   - The array of offset values; one offset value for each buffer in the vertex-buffer array. Each offset is the number of bytes between the first element of a vertex buffer and the first element that will be used. If this parameter is an empty array, zero offsets for all buffers will be used.
  /// \param flags          - Additional flags for setting vertex buffers. See xiiGALSetVertexBufferFlags for more information.
  /// \param transitionMode - Resource state transition mode. Specifies whether the buffer state should be transitioned to the required state automatically.
  void SetVertexBuffers(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets = {}, xiiBitflags<xiiGALSetVertexBufferFlags> flags = xiiGALSetVertexBufferFlags::None, xiiEnum<xiiGALStateTransitionMode> transitionMode = xiiGALStateTransitionMode::Transition);

  /// This is used to set the constant (uniform) buffer for a shader resource.
  ///
  /// \param bindingInformation - This describes the binding information for the shader resource, see xiiGALPipelineResourceDescription for details.
  /// \param pConstantBuffer    - The handle to the constant (uniform) buffer object to set.
  void SetConstantBuffer(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer);

  /// This is used to set the buffer view for a shader resource.
  ///
  /// \param bindingInformation - This describes the binding information for the shader resource, see xiiGALPipelineResourceDescription for details.
  /// \param pBufferView        - The handle to the buffer view object to set.
  void SetShaderResourceBufferView(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView);

  /// Updates a range in a fixed or runtime-sized buffer SRV descriptor array.
  void SetShaderResourceBufferViews(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews);

  /// This is used to set the texture view for a shader resource.
  ///
  /// \param bindingInformation - This describes the binding information for the shader resource, see xiiGALPipelineResourceDescription for details.
  /// \param pTextureView       - The handle to the texture view object to set.
  void SetShaderResourceTextureView(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView);

  /// Updates a range in a fixed or runtime-sized texture SRV descriptor array.
  void SetShaderResourceTextureViews(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews);

  /// This is used to set the buffer view for an unordered access.
  ///
  /// \param bindingInformation - This describes the binding information for the shader resource, see xiiGALPipelineResourceDescription for details.
  /// \param pBufferView        - The handle to the buffer view object to set.
  void SetUnorderedAccessBufferView(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView);

  /// Updates a range in a fixed or runtime-sized buffer UAV descriptor array.
  void SetUnorderedAccessBufferViews(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews);

  /// This is used to set the texture view for an unordered access.
  ///
  /// \param bindingInformation - This describes the binding information for the shader resource, see xiiGALPipelineResourceDescription for details.
  /// \param pTextureView       - The handle to the texture view object to set.
  void SetUnorderedAccessTextureView(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView);

  /// Updates a range in a fixed or runtime-sized texture UAV descriptor array.
  void SetUnorderedAccessTextureViews(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews);

  /// This is used to set the sampler for a sampler resource.
  ///
  /// \param bindingInformation - This describes the binding information for the sampler resource, see xiiGALPipelineResourceDescription for details.
  /// \param pSampler           - The handle to the sampler object to set.
  void SetSampler(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler);

  /// Updates a range in a fixed or runtime-sized sampler descriptor array.
  void SetSamplers(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALSampler*> pSamplers);

  /// Binds a top-level acceleration structure to a shader resource slot.
  ///
  /// \param bindingInformation - Resource binding metadata from pipeline resource signature.
  /// \param pTopLevelAS        - The top-level acceleration structure to bind.
  void SetAccelerationStructure(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTopLevelAS* pTopLevelAS);

  /// Resolves and sets a constant buffer for the given resource name.
  ///
  /// If \p shaderStages is left as `xiiGALShaderType::Unknown`, the function resolves the resource based on whatever shader stage is found. Otherwise, it attempts to find a resource description that includes all the specified shader stages.
  ///
  /// \param sResourceName   - The hashed name of the resource to resolve.
  /// \param pConstantBuffer - Shared pointer to the constant buffer to set.
  /// \param shaderStages    - Bitflags specifying applicable shader stages (defaults to unknown).
  void ResolveAndSetConstantBuffer(const xiiTempHashedString& sResourceName, xiiGALBuffer* pConstantBuffer, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  /// Resolves and sets a shader resource buffer view for the given resource name.
  ///
  /// If \p shaderStages is left as `xiiGALShaderType::Unknown`, the function resolves the resource based on whatever shader stage is found. Otherwise, it attempts to find a resource description that includes all the specified shader stages.
  ///
  /// \param sResourceName - The hashed name of the resource to resolve.
  /// \param pBufferView   - Shared pointer to the buffer view to set.
  /// \param shaderStages  - Bitflags specifying applicable shader stages (defaults to unknown).
  void ResolveAndSetShaderResourceBufferView(const xiiTempHashedString& sResourceName, xiiGALBufferView* pBufferView, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  void ResolveAndSetShaderResourceBufferViews(const xiiTempHashedString& sResourceName, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  /// Resolves and sets a shader resource texture view for the given resource name.
  ///
  /// If \p shaderStages is left as `xiiGALShaderType::Unknown`, the function resolves the resource based on whatever shader stage is found. Otherwise, it attempts to find a resource description that includes all the specified shader stages.
  ///
  /// \param sResourceName - The hashed name of the resource to resolve.
  /// \param pTextureView  - Shared pointer to the texture view to set.
  /// \param shaderStages  - Bitflags specifying applicable shader stages (defaults to unknown).
  void ResolveAndSetShaderResourceTextureView(const xiiTempHashedString& sResourceName, xiiGALTextureView* pTextureView, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  void ResolveAndSetShaderResourceTextureViews(const xiiTempHashedString& sResourceName, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  /// Resolves and sets an unordered access buffer view for the given resource name.
  ///
  /// If \p shaderStages is left as `xiiGALShaderType::Unknown`, the function resolves the resource based on whatever shader stage is found. Otherwise, it attempts to find a resource description that includes all the specified shader stages.
  ///
  /// \param sResourceName - The hashed name of the resource to resolve.
  /// \param pBufferView   - Shared pointer to the unordered access buffer view to set.
  /// \param shaderStages  - Bitflags specifying applicable shader stages (defaults to unknown).
  void ResolveAndSetUnorderedAccessBufferView(const xiiTempHashedString& sResourceName, xiiGALBufferView* pBufferView, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  void ResolveAndSetUnorderedAccessBufferViews(const xiiTempHashedString& sResourceName, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  /// Resolves and sets an unordered access texture view for the given resource name.
  ///
  /// If \p shaderStages is left as `xiiGALShaderType::Unknown`, the function resolves the resource based on whatever shader stage is found. Otherwise, it attempts to find a resource description that includes all the specified shader stages.
  ///
  /// \param sResourceName - The hashed name of the resource to resolve.
  /// \param pTextureView  - Shared pointer to the unordered access texture view to set.
  /// \param shaderStages  - Bitflags specifying applicable shader stages (defaults to unknown).
  void ResolveAndSetUnorderedAccessTextureView(const xiiTempHashedString& sResourceName, xiiGALTextureView* pTextureView, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  void ResolveAndSetUnorderedAccessTextureViews(const xiiTempHashedString& sResourceName, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  /// Resolves and sets a sampler for the given resource name.
  ///
  /// If \p shaderStages is left as `xiiGALShaderType::Unknown`, the function resolves the resource based on whatever shader stage is found. Otherwise, it attempts to find a resource description that includes all the specified shader stages.
  ///
  /// \param sResourceName - The hashed name of the resource to resolve.
  /// \param pSampler      - Shared pointer to the sampler to set.
  /// \param shaderStages  - Bitflags specifying applicable shader stages (defaults to unknown).
  void ResolveAndSetSampler(const xiiTempHashedString& sResourceName, xiiGALSampler* pSampler, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  void ResolveAndSetSamplers(const xiiTempHashedString& sResourceName, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALSampler*> pSamplers, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  /// Resolves and sets a top-level acceleration structure for the given resource name.
  ///
  /// If \p shaderStages is left as `xiiGALShaderType::Unknown`, the function resolves the resource based on whatever shader stage is found. Otherwise, it attempts to find a resource description that includes all the specified shader stages.
  ///
  /// \param sResourceName - The hashed name of the resource to resolve.
  /// \param pTopLevelAS   - Shared pointer to the top-level acceleration structure to set.
  /// \param shaderStages  - Bitflags specifying applicable shader stages (defaults to unknown).
  void ResolveAndSetAccelerationStructure(const xiiTempHashedString& sResourceName, xiiGALTopLevelAS* pTopLevelAS, xiiBitflags<xiiGALShaderType> shaderStages = xiiGALShaderType::Unknown);

  /// This commits the pipeline shader resources to the GPU, and ensures that all necessary state transitions are performed.
  ///
  /// \param mode - The state transition mode. The default is xiiGALStateTransitionMode::Transition.
  xiiResult CommitShaderResources(xiiEnum<xiiGALStateTransitionMode> mode = xiiGALStateTransitionMode::Transition);

  /// This clears the specified render target view to the specified color.
  ///
  /// \param pRenderTargetView - The handle to the render target view object. The view must be a xiiGALTextureViewType::RenderTarget.
  /// \param clearColor        - The color to which to clear the render target view.
  void ClearRenderTargetView(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor);

  /// This clears the specified depth stencil view to the specified depth and stencil values.
  ///
  /// \param pDepthStencilView - The handle to the depth stencil view object. The view must be a xiiGALTextureViewType::DepthStencil.
  /// \param bClearDepth       - Whether to clear the depth portion of the buffer.
  /// \param bClearStencil     - Whether to clear the stencil portion of the buffer.
  /// \param fDepthClear       - The value to which to clear the depth portion of the buffer with.
  /// \param uiStencilClear    - The value to which to clear the stencil portion of the buffer with.
  void ClearDepthStencilView(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear);

  /// This begins a render pass, which contains a collection of attachments, subpasses, and dependencies between the subpasses, and describes how the attachments are used over the course of the subpasses.
  ///
  /// \param beginRenderPass - The description of the render pass. See xiiGALBeginRenderPassDescription for more information.
  void BeginRenderPass(const xiiGALBeginRenderPassDescription& beginRenderPass);

  /// This transitions to the next subpass of the begun render pass.
  void NextSubpass();

  /// This ends a render pass that has already begun.
  void EndRenderPass();

  // Draw functions.

  /// Issues a non-indexed draw call using the specified parameters.
  ///
  /// Executes a basic draw using vertex buffers without indexing.
  ///
  /// \param description - Vertex and instance counts, and vertex offsets.
  ///
  /// \see xiiGALDrawDescription
  void Draw(const xiiGALDrawDescription& description);

  /// Issues an indexed draw call using the specified parameters.
  ///
  /// Uses an index buffer to reference geometry vertices and enables instancing.
  ///
  /// \param description - Index type, counts, and offset information.
  ///
  /// \see xiiGALDrawIndexedDescription
  void DrawIndexed(const xiiGALDrawIndexedDescription& description);

  /// Issues an indirect draw call based on arguments stored in a GPU buffer.
  ///
  /// Supports multi-draw and optional counter buffer for dynamic draw count.
  ///
  /// \param description - Buffer handles, offsets, draw count, and flags.
  ///
  /// \see xiiGALDrawIndirectDescription
  void DrawIndirect(const xiiGALDrawIndirectDescription& description);

  /// Issues an indexed indirect draw call based on arguments stored in a GPU buffer.
  ///
  /// Uses index data and indirect arguments pulled from a structured GPU buffer.
  ///
  /// \param description - Index type, buffer handles, offsets, and draw count.
  ///
  /// \see xiiGALDrawIndexedIndirectDescription
  void DrawIndexedIndirect(const xiiGALDrawIndexedIndirectDescription& description);

  /// Issues a draw call that dispatches GPU mesh shaders directly.
  ///
  /// Typically used when mesh shading pipelines are active and task amplification is desired.
  ///
  /// \param description - Thread group dimensions and flags.
  ///
  /// \see xiiGALDrawMeshDescription
  void DrawMesh(const xiiGALDrawMeshDescription& description);

  /// Issues an indirect mesh shader draw using arguments stored in a GPU buffer.
  ///
  /// Supports GPU-driven workflows for meshlets with optional counter buffer.
  ///
  /// \param description - Buffer handles, offsets, draw count, and flags.
  ///
  /// \see xiiGALDrawMeshIndirectDescription
  void DrawMeshIndirect(const xiiGALDrawMeshIndirectDescription& description);

  /// Executes a batch of non-indexed draw calls using an array of parameters.
  ///
  /// Enables multi-draw submission without index buffers, with per-draw configurations.
  ///
  /// \param description - Array of draw items and instance parameters.
  ///
  /// \see xiiGALMultiDrawDescription
  void MultiDraw(const xiiGALMultiDrawDescription& description);

  /// Executes a batch of indexed draw calls using an array of parameters.
  ///
  /// Supports per-item base vertex and index range offsets across draws.
  ///
  /// \param description - Array of indexed draw items and instance parameters.
  ///
  /// \see xiiGALMultiDrawIndexedDescription
  void MultiDrawIndexed(const xiiGALMultiDrawIndexedDescription& description);

  // Dispatch functions.

  /// Dispatches a compute workload using the specified thread group dimensions.
  ///
  /// Synchronously encodes compute workload dimensions per axis.
  ///
  /// \param description - Thread group count and Metal overrides (if any).
  ///
  /// \see xiiGALDispatchComputeDescription
  void DispatchCompute(const xiiGALDispatchComputeDescription& description);

  /// Dispatches a compute workload using arguments stored in a GPU buffer.
  ///
  /// Enables GPU-controlled compute invocation for async workloads or culling passes.
  ///
  /// \param description - Buffer containing dispatch dimensions and optional Metal overrides.
  ///
  /// \see xiiGALDispatchComputeIndirectDescription
  void DispatchComputeIndirect(const xiiGALDispatchComputeIndirectDescription& description);

  /// Dispatches rays using the currently bound ray tracing pipeline.
  void TraceRays(const xiiGALTraceRaysDescription& description);

  /// Dispatches rays indirectly using dimensions sourced from a GPU buffer.
  void TraceRaysIndirect(const xiiGALTraceRaysIndirectDescription& description);

  /// Updates SBT records from ray tracing shader group handles.
  void UpdateSBT(const xiiGALUpdateSBTDescription& description);

  /// Builds or updates a BLAS.
  void BuildBLAS(const xiiGALBuildBLASDescription& description);

  /// Builds or updates a TLAS.
  void BuildTLAS(const xiiGALBuildTLASDescription& description);

  /// Copies a BLAS (clone or compact).
  void CopyBLAS(const xiiGALCopyBLASDescription& description);

  /// Copies a TLAS (clone or compact).
  void CopyTLAS(const xiiGALCopyTLASDescription& description);

  /// Writes BLAS compacted size into a destination buffer.
  void WriteBLASCompactedSize(const xiiGALWriteBLASCompactedSizeDescription& description);

  /// Writes TLAS compacted size into a destination buffer.
  void WriteTLASCompactedSize(const xiiGALWriteTLASCompactedSizeDescription& description);

  // Query functions.

  /// Begins a query.
  ///
  /// \param pQuery - The handle to the query object.
  void BeginQuery(xiiGALQuery* pQuery);

  /// Ends a query.
  ///
  /// \param pQuery - The handle to the query object.
  void EndQuery(xiiGALQuery* pQuery);

  // Buffer methods.

  /// Updates a buffer.
  ///
  /// \param pBuffer             - The handle to the buffer object.
  /// \param uiDestinationOffset - Byte offset into the buffer where the update should start.
  /// \param pSourceData         - Pointer to the source data.
  void UpdateBuffer(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData);

  /// Copies the entire contents of the source buffer to the destination buffer.
  ///
  /// \param pSourceBuffer      - The handle to the source buffer object.
  /// \param pDestinationBuffer - The handle to the destination buffer object.
  void CopyBuffer(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer);

  /// Copies a region from the source buffer to the destination buffer.
  ///
  /// \param pSourceBuffer       - The handle to the source buffer object.
  /// \param uiSourceOffset      - Byte offset into the source buffer where the copy should start.
  /// \param pDestinationBuffer  - The handle to the destination buffer object.
  /// \param uiDestinationOffset - Byte offset into the destination buffer where the copy should start.
  /// \param uiSize              - Size in bytes of the region to copy.
  void CopyBufferRegion(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize);

  /// Maps a buffer into the CPU's address space.
  ///
  /// \param pBuffer     - The handle to the buffer object.
  /// \param mapType     - Specifies the CPU's access pattern for the map operation. See xiiGALMapType for details.
  /// \param mapFlags    - Flags specifying how the buffer should be mapped. See xiiGALMapFlags for details.
  /// \param pMappedData - Pointer to the mapped data.
  xiiResult MapBuffer(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData);

  /// Unmaps a buffer from the CPU's address space.
  ///
  /// \param pBuffer - The handle to the buffer object.
  /// \param mapType - Specifies the CPU's access pattern for the map operation. See xiiGALMapType for details.
  xiiResult UnmapBuffer(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType);

  // Texture methods.

  /// Updates a texture.
  ///
  /// \param pTexture            - The handle to the texture object.
  /// \param textureMiplevelData - Specifies the subresource to update. See xiiGALTextureMipLevelData for details.
  /// \param textureBox          - Specifies the region within the subresource to update.
  /// \param subresourceData     - Specifies the new data. See xiiGALTextureSubResourceData for details.
  void UpdateTexture(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData);

  /// Copies the entire contents of the source texture to the destination texture.
  ///
  /// \param pSourceTexture      - The handle to the source texture object.
  /// \param pDestinationTexture - The handle to the destination texture object.
  void CopyTexture(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture);

  /// Copies a region from the source texture to the destination texture.
  ///
  /// \param pSourceTexture          - The handle to the source texture object.
  /// \param sourceMipLevelData      - Specifies the subresource in the source texture. See xiiGALTextureMipLevelData for details.
  /// \param box                     - Specifies the region within the source subresource to copy.
  /// \param pDestinationTexture     - The handle to the destination texture object.
  /// \param destinationMipLevelData - Specifies the subresource in the destination texture. See xiiGALTextureMipLevelData for details.
  /// \param vDestinationPoint       - Specifies the point within the destination subresource where the region should be copied to.
  void CopyTextureRegion(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint);

  /// Resolves a multisampled source texture into a non-multisampled destination texture.
  ///
  /// This operation performs a resolve from a multi-sampled texture (typically used for anti-aliasing) into a non-multi-sampled texture, commonly used for presenting or further processing.
  ///
  /// \param pSourceTexture      - Handle to the multi-sampled source texture.
  /// \param pDestinationTexture - Handle to the destination texture which must not be multi-sampled.
  /// \param description         - Structure that specifies the source and destination subresources, including mip levels, array slices, resource formats, and texture state transitions. See xiiGALResolveTextureSubresourceDescription for details.
  void ResolveTextureSubResource(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture, const xiiGALResolveTextureSubresourceDescription& description);

  /// Generates mipmap levels for a texture.
  ///
  /// \param pTextureView - The handle to the texture view object. The texture view must be of type xiiGALTextureViewType::ShaderResource.
  ///
  /// \remarks This method must only be called on a shader resource view. The texture must be created with xiiGALMiscTextureFlags::GenerateMips.
  void GenerateMips(xiiGALTextureView* pTextureView);

  /// Maps a texture subresource into the address space of the command list.
  ///
  /// \param pTexture            - The handle to the texture object. This is the texture that contains the subresource to map.
  /// \param textureMipLevelData - Specifies the subresource to map. This is the mipmap level of the texture to map.
  /// \param mapType             - Specifies the CPU's read and write access to a resource.
  /// \param mapFlags            - Specifies the behavior of the map operation.
  /// \param pTextureBox         - Specifies the region of the resource to map. If this parameter is null, the entire resource is mapped.
  /// \param mappedData          - Receives information about the resource data when the function returns.
  xiiResult MapTextureSubresource(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData);

  /// Unmaps a texture subresource from the address space of the command list.
  ///
  /// \param pTexture            - The handle to the texture object.
  /// \param textureMipLevelData - Specifies the subresource to unmap.
  xiiResult UnmapTextureSubresource(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData);

  // Shading rate methods.

  /// Sets the fragment shading rate for subsequent draw calls.
  ///
  /// Configures the base shading rate and combiner logic used to resolve per-primitive and screen-space (texture-based) shading rates. This allows dynamic control over rendering performance and visual fidelity by adjusting the number of pixels shaded per fragment.
  ///
  /// \param baseRateFlags          - The default shading rate to apply if no overrides are present (e.g., 1x1, 2x2).
  /// \param primitiveCombinerFlags - The combiner logic used when both base and primitive rates are specified.
  /// \param textureCombinerFlags   - The combiner logic used when both primitive and texture rates are applied.
  ///
  /// \note Requires graphics backend support for Variable Rate Shading (VRS), see xiiGALDeviceFeatures.
  ///
  /// \see xiiGALShadingRateFlags, xiiGALShadingRateCombinerFlags
  void SetShadingRate(xiiBitflags<xiiGALShadingRateFlags> baseRateFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> primitiveCombinerFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> textureCombinerFlags);

  // Resource methods.

  /// Transitions the resource states.
  ///
  /// \param pResourceBarriers - Pointer to the array of resource barriers.
  ///
  /// \remarks When both old and new states are xiiGALResourceState::UnorderedAccess, the GAL executes UAV barrier on the resource. The barrier makes sure that all UAV accesses (reads or writes) are complete before any future UAV accesses (read or write) can begin.\n
  ///
  ///          There are two main usage scenarios for this method:
  ///          1. An application knows specifics of resource state transitions not available to the GAL. For example, only single mip level needs to be transitioned.
  ///          2. An application manages resource states in multiple threads in parallel.
  ///
  ///          The method always reads the states of all resources to transition. If the state of a resource is managed by multiple threads in parallel, the resource must first be transitioned to unknown state (xiiGALResourceState::Unknown) to disable automatic state management in the GAL.
  ///
  ///          When xiiGALStateTransitionFlags::UpdateState is set, the method may update the state of the corresponding resource which is not thread safe. No other threads should read or write the state of that resource.
  ///
  /// \note  Resource states for shader access (e.g. xiiGALResourceState::ConstantBuffer, xiiGALResourceState::UnorderedAccess, xiiGALResourceState::ShaderResource) may map to different native state depending on what command queue type is used (see xiiGALCommandListCreationDescription).
  ///        To synchronize write access in compute shader in a compute queue with a pixel shader read in graphics queue, an application should call TransitionResourceStates() in graphics queue.
  ///        Using TransitionResourceStates() with NewState = xiiGALResourceState::ShaderResource will not invalidate cache in graphics shaders and may cause undefined behaviour.
  void TransitionResourceStates(xiiArrayPtr<xiiGALStateTransitionDescription> pResourceBarriers);

  // Fence functions.

  /// Tells the GPU to set a fence to a specified value after all previous work has completed.
  ///
  /// \param pFence  - The fence to signal.
  /// \param uiValue - The value to set the fence to. This value must be greater than the previously signalled value on the same fence.
  ///
  /// \note The fence will be signalled when the command list is submitted. If an application needs to wait for the fence in a loop, it must submit the command list after signalling the fence.
  void EnqueueSignal(xiiGALFence* pFence, xiiUInt64 uiValue);

  /// Waits until the specified fence reaches or exceeds the specified value, on the device.
  ///
  /// \param pFence  - The fence to wait. The fence must be created with type xiiGALFenceType::General.
  /// \param uiValue - The value that the command list is waiting for the fence to reach.
  ///
  /// \note If NativeFence feature is not enabled (see xiiGALDeviceFeatures), then uiValue must be less than or equal to the last signalled or pending value. uiValue becomes pending when the command list is submitted. Waiting for a value that is greater than any pending value will cause a deadlock.
  ///
  /// \note If NativeFence feature is enabled (see xiiGALDeviceFeatures), then waiting for a value that is greater than any pending value will cause a GPU stall.
  void DeviceWaitForFence(xiiGALFence* pFence, xiiUInt64 uiValue);

  // Debug functions.

  /// Begins a new debug group with a specified name and color.
  ///
  /// \param sName - The name of the debug group.
  /// \param color - The color associated with the debug group.
  void BeginDebugGroup(xiiStringView sName, const xiiColor& color = xiiColor::White);

  /// Ends the current debug group.
  void EndDebugGroup();

  /// Inserts a debug label into the command list.
  ///
  /// \param sName - The name of the debug label.
  /// \param color - The color associated with the debug label.
  void InsertDebugLabel(xiiStringView sName, const xiiColor& color = xiiColor::White);

  /// Invalidates the current state of the command list. It is typically called when the command list is reset or when the pipeline state is changed.
  void InvalidateState();

public:
  /// Enum class representing the state of a command list recording.
  enum class RecordingState : xiiUInt8
  {
    Reset = 0U, ///< The command list has been reset and is ready to be recorded again.
    Recording,  ///< The command list is currently being recorded.
    Ended,      ///< The recording of the command list has ended.

    ENUM_COUNT,
  };

  XII_ALWAYS_INLINE void AssertRenderingThread() const { XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "This function may only be executed on the main thread."); };

  /// This returns the command list recording state.
  [[nodiscard]] XII_ALWAYS_INLINE RecordingState GetRecordingState() const { return m_RecordingState; };

  /// This returns the command list statistics.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALCommandListStatistics& GetStatistics() const { return m_CommandListStatistics; };

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALCommandList(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandList();

  virtual xiiResult InitPlatform() = 0;

  void ValidateTextureRegion(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiMipLevel, xiiUInt32 uiSlice, const xiiBoundingBoxU32& box) const;
  void ValidateTextureUpdateRegion(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiMipLevel, xiiUInt32 uiSlice, const xiiBoundingBoxU32& destinationBox, const xiiGALTextureSubResourceData& subresourceData) const;

  bool VerifyResourceState(xiiBitflags<xiiGALResourceStateFlags> stateFlags, xiiBitflags<xiiGALCommandQueueFlags> queueFlags, const char* szParameterName) const;
  bool VerifyResourceStates(xiiBitflags<xiiGALResourceStateFlags> stateFlags, bool bIsTexture) const;

  void VerifyBufferState(const xiiGALBuffer* pBuffer, xiiBitflags<xiiGALResourceStateFlags> requiredState, const char* szOperationName) const;
  void VerifyTextureState(const xiiGALTexture* pTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState, const char* szOperationName) const;
  void VerifyBottomLevelASState(const xiiGALBottomLevelAS* pBottomLevelAS, xiiBitflags<xiiGALResourceStateFlags> requiredState, const char* szOperationName) const;
  void VerifyTopLevelASState(const xiiGALTopLevelAS* pTopLevelAS, xiiBitflags<xiiGALResourceStateFlags> requiredState, const char* szOperationName) const;

  struct VertexStreamDescription
  {
    XII_DECLARE_POD_TYPE();

    xiiGALBuffer* m_pBuffer;         ///< Buffer for the vertex stream.
    xiiUInt64     m_uiOffset = 0ULL; ///< The offset in bytes.
  };

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  // These functions need to be implemented by a graphics API abstraction.
protected:
  virtual void BeginPlatform() = 0;
  virtual void EndPlatform()   = 0;
  virtual void ResetPlatform() = 0;

  virtual void SubmitPlatform(xiiGALCommandList* pSecondaryCommandList) = 0;

  virtual void SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)                = 0;
  virtual void PushConstantsPlatform(xiiUInt32 uiOffset, xiiArrayPtr<const xiiUInt8> pData) = 0;

  virtual void SetStencilRefPlatform(xiiUInt32 uiStencilRef)       = 0;
  virtual void SetBlendFactorPlatform(const xiiColor& blendFactor) = 0;

  virtual void SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports) = 0;
  virtual void SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects)      = 0;

  virtual void SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset, xiiEnum<xiiGALStateTransitionMode> transitionMode)                                                                          = 0;
  virtual void SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<VertexStreamDescription> pVertexStreams, xiiBitflags<xiiGALSetVertexBufferFlags> flags, xiiEnum<xiiGALStateTransitionMode> transitionMode) = 0;

  virtual void      SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer)               = 0;
  virtual void      SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)     = 0;
  virtual void      SetShaderResourceBufferViewsPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews) = 0;
  virtual void      SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)  = 0;
  virtual void      SetShaderResourceTextureViewsPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews) = 0;
  virtual void      SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)    = 0;
  virtual void      SetUnorderedAccessBufferViewsPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews) = 0;
  virtual void      SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) = 0;
  virtual void      SetUnorderedAccessTextureViewsPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews) = 0;
  virtual void      SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler)                            = 0;
  virtual void      SetSamplersPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALSampler*> pSamplers) = 0;
  virtual void      SetAccelerationStructurePlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTopLevelAS* pTopLevelAS)        = 0;
  virtual xiiResult CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode)                                                              = 0;

  virtual void ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)                                                       = 0;
  virtual void ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) = 0;

  virtual void BeginRenderPassPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues) = 0;
  virtual void NextSubpassPlatform()                                                                                                                                       = 0;
  virtual void EndRenderPassPlatform()                                                                                                                                     = 0;

  virtual void DrawPlatform(const xiiGALDrawDescription& description)                               = 0;
  virtual void DrawIndexedPlatform(const xiiGALDrawIndexedDescription& description)                 = 0;
  virtual void DrawIndirectPlatform(const xiiGALDrawIndirectDescription& description)               = 0;
  virtual void DrawIndexedIndirectPlatform(const xiiGALDrawIndexedIndirectDescription& description) = 0;
  virtual void DrawMeshPlatform(const xiiGALDrawMeshDescription& description)                       = 0;
  virtual void DrawMeshIndirectPlatform(const xiiGALDrawMeshIndirectDescription& description)       = 0;
  virtual void MultiDrawPlatform(const xiiGALMultiDrawDescription& description)                     = 0;
  virtual void MultiDrawIndexedPlatform(const xiiGALMultiDrawIndexedDescription& description)       = 0;

  virtual void DispatchComputePlatform(const xiiGALDispatchComputeDescription& description)                 = 0;
  virtual void DispatchComputeIndirectPlatform(const xiiGALDispatchComputeIndirectDescription& description) = 0;
  virtual void TraceRaysPlatform(const xiiGALTraceRaysDescription& description)                             = 0;
  virtual void TraceRaysIndirectPlatform(const xiiGALTraceRaysIndirectDescription& description)             = 0;
  virtual void UpdateSBTPlatform(const xiiGALUpdateSBTDescription& description)                             = 0;
  virtual void BuildBLASPlatform(const xiiGALBuildBLASDescription& description)                             = 0;
  virtual void BuildTLASPlatform(const xiiGALBuildTLASDescription& description)                             = 0;
  virtual void CopyBLASPlatform(const xiiGALCopyBLASDescription& description)                               = 0;
  virtual void CopyTLASPlatform(const xiiGALCopyTLASDescription& description)                               = 0;
  virtual void WriteBLASCompactedSizePlatform(const xiiGALWriteBLASCompactedSizeDescription& description)   = 0;
  virtual void WriteTLASCompactedSizePlatform(const xiiGALWriteTLASCompactedSizeDescription& description)   = 0;

  virtual void BeginQueryPlatform(xiiGALQuery* pQuery) = 0;
  virtual void EndQueryPlatform(xiiGALQuery* pQuery)   = 0;

  virtual void      UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)                                                = 0;
  virtual void      CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)                                                                                  = 0;
  virtual void      CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize) = 0;
  virtual xiiResult MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)                                 = 0;
  virtual xiiResult UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)                                                                                         = 0;

  virtual void      UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)                                                                                 = 0;
  virtual void      CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)                                                                                                                                                                                         = 0;
  virtual void      CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint) = 0;
  virtual void      ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture, const xiiGALResolveTextureSubresourceDescription& description)                                                                                                            = 0;
  virtual void      GenerateMipsPlatform(xiiGALTextureView* pTextureView)                                                                                                                                                                                                                          = 0;
  virtual xiiResult MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)                        = 0;
  virtual xiiResult UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)                                                                                                                                                                        = 0;

  virtual void SetShadingRatePlatform(xiiBitflags<xiiGALShadingRateFlags> baseRateFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> primitiveCombinerFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> textureCombinerFlags) = 0;

  virtual void TransitionResourceStatesPlatform(xiiArrayPtr<xiiGALStateTransitionDescription> pResourceBarriers) = 0;

  virtual void EnqueueSignalPlatform(xiiGALFence* pFence, xiiUInt64 uiValue)      = 0;
  virtual void DeviceWaitForFencePlatform(xiiGALFence* pFence, xiiUInt64 uiValue) = 0;

  virtual void BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)  = 0;
  virtual void EndDebugGroupPlatform()                                              = 0;
  virtual void InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color) = 0;

  virtual void InvalidateStatePlatform() = 0;

  /// \endcond

protected:
  static constexpr xiiUInt32 s_uiDrawMeshIndirectCommandStride = sizeof(xiiUInt32) * 3; // Vulkan: 8 bytes (task count, first task), D3D12: 12 bytes (x, y, z dimension).

  xiiGALCommandListCreationDescription m_Description;

  RecordingState m_RecordingState = RecordingState::Reset;
  const bool     m_bNativeMultiDrawSupported;

  xiiGALPipelineState*             m_pPipelineState             = nullptr;
  xiiGALPipelineResourceSignature* m_pPipelineResourceSignature = nullptr;
  xiiStaticArray<xiiUInt8, 256U>   m_PushConstantStaging;

  xiiHybridArray<VertexStreamDescription, 2U> m_VertexStreams;

  xiiGALBuffer* m_pIndexBuffer      = nullptr;
  xiiUInt64     m_uiIndexDataOffset = 0ULL;

  xiiGALRenderPass*  m_pRenderPass  = nullptr;
  xiiGALFramebuffer* m_pFramebuffer = nullptr;

  xiiColor  m_BlendFactors = xiiColor::Black;
  xiiUInt32 m_uiStencilRef = 0U;

  xiiHybridArray<xiiGALViewport, 2U> m_Viewports;
  xiiHybridArray<xiiRectU32, 2U>     m_ScissorRects;

private:
  xiiGALCommandListStatistics m_CommandListStatistics;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiUInt32 m_uiDebugGroupCount = 0;

  xiiMap<xiiGALBuffer*, xiiEnum<xiiGALMapType>> m_MappedBuffers;
#endif
};
