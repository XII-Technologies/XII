#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderContext/RenderTargetSetup.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

struct xiiGlobalConstants;
struct xiiPassConstants;

//////////////////////////////////////////////////////////////////////////
// xiiShaderBindFlags
//////////////////////////////////////////////////////////////////////////

struct XII_GRAPHICSCORE_DLL xiiShaderBindFlags
{
  using StorageType = xiiUInt32;

  enum Enum
  {
    None                = 0,          ///< No flags causes the default shader binding behavior (all render states are applied).
    ForceRebind         = XII_BIT(0), ///< Executes shader binding (and state setting), even if the shader hasn't changed. Use this, when the same shader was previously used with custom bound states.
    NoRasterizerState   = XII_BIT(1), ///< The rasterizer state that is associated with the shader will not be bound. Use this when you intend to bind a custom rasterizer.
    NoDepthStencilState = XII_BIT(2), ///< The depth-stencil state that is associated with the shader will not be bound. Use this when you intend to bind a custom depth-stencil.
    NoBlendState        = XII_BIT(3), ///< The blend state that is associated with the shader will not be bound. Use this when you intend to bind a custom blend state.

    NoStateBinding = NoRasterizerState | NoDepthStencilState | NoBlendState,

    Default = None
  };

  struct Bits
  {
    StorageType ForceRebind : 1;
    StorageType NoRasterizerState : 1;
    StorageType NoDepthStencilState : 1;
    StorageType NoBlendState : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiShaderBindFlags);

//////////////////////////////////////////////////////////////////////////
// xiiRenderContextFlags
//////////////////////////////////////////////////////////////////////////

struct XII_GRAPHICSCORE_DLL xiiRenderContextFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None                         = 0,
    ShaderStateChanged           = XII_BIT(0),
    ConstantBufferBindingChanged = XII_BIT(1),
    TextureBindingChanged        = XII_BIT(2),
    BufferBindingChanged         = XII_BIT(3),
    TextureUAVBindingChanged     = XII_BIT(4),
    BufferUAVBindingChanged      = XII_BIT(5),
    SamplerBindingChanged        = XII_BIT(6),
    MeshBufferBindingChanged     = XII_BIT(7),
    MaterialBindingChanged       = XII_BIT(8),
    PipelineChanged              = XII_BIT(9),

    AllStatesInvalid = ShaderStateChanged | ConstantBufferBindingChanged | BufferBindingChanged | BufferUAVBindingChanged | TextureBindingChanged | TextureUAVBindingChanged | SamplerBindingChanged | MeshBufferBindingChanged | PipelineChanged,

    Default = None
  };

  struct Bits
  {
    StorageType ShaderStateChanged : 1;
    StorageType ConstantBufferBindingChanged : 1;
    StorageType BufferBindingChanged : 1;
    StorageType BufferUAVBindingChanged : 1;
    StorageType TextureBindingChanged : 1;
    StorageType TextureUAVBindingChanged : 1;
    StorageType SamplerBindingChanged : 1;
    StorageType MeshBufferBindingChanged : 1;
    StorageType MaterialBindingChanged : 1;
    StorageType PipelineChanged : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiRenderContextFlags);

//////////////////////////////////////////////////////////////////////////
// xiiDefaultSamplerFlags
//////////////////////////////////////////////////////////////////////////

struct XII_GRAPHICSCORE_DLL xiiDefaultSamplerFlags
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    PointFiltering  = 0,
    LinearFiltering = XII_BIT(0),

    Wrap  = 0,
    Clamp = XII_BIT(1)
  };

  struct Bits
  {
    StorageType LinearFiltering : 1;
    StorageType Clamp : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiDefaultSamplerFlags);

//////////////////////////////////////////////////////////////////////////
// xiiRenderContext
//////////////////////////////////////////////////////////////////////////

/// \brief Provides an interface for issuing GPU rendering and compute commands within a single rendering scope.
///
/// The render context encapsulates a command list and simplifies binding resources, setting up shaders, and issuing draw or dispatch commands.
///
/// Typical usage:
/// \code{.cpp}
/// xiiRenderContext context(pCommandList);
/// context.BeginRendering(setup, viewport);
/// context.BindShader(hShader);
/// context.BindMaterial(hMaterial);
/// context.DrawMeshBuffer();
/// context.EndRendering();
/// \endcode
class XII_GRAPHICSCORE_DLL xiiRenderContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderContext);

private:
  friend class xiiMemoryUtils;

  explicit xiiRenderContext();
  ~xiiRenderContext();

  static xiiRenderContext*                     s_pDefaultInstance;
  static xiiHybridArray<xiiRenderContext*, 2U> s_Instances;

public:
  static xiiRenderContext* GetDefaultInstance();
  static xiiRenderContext* CreateInstance();
  static void              DestroyInstance(xiiRenderContext* pRenderContext);

public:
  class GraphicsScope
  {
    XII_DISALLOW_COPY_AND_ASSIGN(GraphicsScope);

  public:
    XII_ALWAYS_INLINE ~GraphicsScope()
    {
      m_Context.EndRendering();
    }

  private:
    friend class xiiRenderContext;

    XII_ALWAYS_INLINE GraphicsScope(xiiRenderContext& context, const xiiRenderingSetup& renderingSetup, const xiiRectFloat& viewport, xiiStringView sName, bool bStereo) :
      m_Context(context)
    {
      m_Context.BeginRendering(renderingSetup, viewport, sName, bStereo);
    }

    xiiRenderContext& m_Context;
  };

  class ComputeScope
  {
    XII_DISALLOW_COPY_AND_ASSIGN(ComputeScope);

  public:
    XII_ALWAYS_INLINE ~ComputeScope()
    {
      m_Context.EndCompute();
    }

  private:
    friend class xiiRenderContext;

    XII_ALWAYS_INLINE ComputeScope(xiiRenderContext& context, xiiStringView sName) :
      m_Context(context)
    {
      m_Context.BeginCompute(sName);
    }

    xiiRenderContext& m_Context;
  };

public:
  enum class CommandListType
  {
    Graphics,
    Compute,
    Transfer,
    Unknown
  };

  template <CommandListType Type>
  class CommandListScope
  {
    XII_DISALLOW_COPY_AND_ASSIGN(CommandListScope);

  public:
    XII_ALWAYS_INLINE ~CommandListScope()
    {
      if (m_bHasDebugScope)
      {
        m_pCommandList->EndDebugGroup();
      }
      m_pCommandList->End();

      xiiGALCommandQueue* pCommandQueue = m_pDevice->GetCommandQueue(m_QueueFlags);
      XII_ASSERT_DEBUG(pCommandQueue != nullptr, "Failed to get command queue for the specified flags!");

      pCommandQueue->Submit(m_pCommandList);
    }

    XII_ALWAYS_INLINE xiiGALCommandList* operator->() { return m_pCommandList.Borrow(); }
    XII_ALWAYS_INLINE xiiSharedPtr<xiiGALCommandList>  GetCommandList() const { m_pCommandList; }
    XII_ALWAYS_INLINE static constexpr CommandListType GetType() { return Type; }

  private:
    friend class xiiRenderContext;

    XII_ALWAYS_INLINE CommandListScope(xiiStringView sName = {}) :
      m_bHasDebugGroup(!sName.IsEmpty())
    {
      if constexpr (Type == CommandListType::Graphics)
      {
        m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
      }
      else if constexpr (Type == CommandListType::Compute)
      {
        m_QueueFlags = xiiGALCommandQueueFlags::Compute;
      }
      else if constexpr (Type == CommandListType::Transfer)
      {
        m_QueueFlags = xiiGALCommandQueueFlags::Transfer;
      }
      else
      {
        m_QueueFlags = xiiGALCommandQueueFlags::None;
      }

      m_pDevice      = xiiGALDevice::GetDefaultDevice();
      m_pCommandList = m_pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      XII_ASSERT_DEBUG(m_pCommandList != nullptr, "Failed to create command list!");

      m_pCommandList->Begin();

      if (m_bHasDebugScope)
      {
        m_pCommandList->BeginDebugGroup(sName, xiiColor::White);
      }
    }

    xiiSharedPtr<xiiGALDevice>           m_pDevice;
    xiiSharedPtr<xiiGALCommandList>      m_pCommandList;
    xiiBitflags<xiiGALCommandQueueFlags> m_QueueFlags;
    bool                                 m_bHasDebugScope = false;
  };

  XII_ALWAYS_INLINE static GraphicsScope BeginRenderingScope(const xiiRenderViewContext& viewContext, const xiiRenderingSetup& renderingSetup, xiiStringView sName = {}, bool bStereoRendering = false)
  {
    return GraphicsScope(*viewContext.m_pRenderContext, renderingSetup, viewContext.m_pViewData->m_ViewPortRect, sName, bStereoRendering);
  }

  XII_ALWAYS_INLINE static ComputeScope BeginComputeScope(const xiiRenderViewContext& viewContext, xiiStringView sName = {})
  {
    return ComputeScope(*viewContext.m_pRenderContext, sName);
  }

  template <CommandListType Type>
  XII_ALWAYS_INLINE static CommandListScope<Type> BeginCommandListScope(xiiStringView sName = {})
  {
    return CommandListScope<Type>(sName);
  }

public:
  /// \brief Begins a graphics render pass with the given setup and viewport.
  ///
  /// \param renderingSetup Defines attachments and render pass config.
  /// \param viewport Defines the screen space region to render to.
  /// \param sName Optional debug label.
  /// \param bStereoRendering Whether stereo viewports are used.
  void BeginRendering(const xiiRenderingSetup& renderingSetup, const xiiRectFloat& viewport, xiiStringView sName = {}, bool bStereoRendering = false);

  /// \brief Ends the current rendering scope.
  void EndRendering();

  /// \brief Begins a compute dispatch scope.
  ///
  /// \param sName Optional debug label.
  void BeginCompute(xiiStringView sName = {});

  /// \brief Ends the current compute dispatch scope.
  void EndCompute();

  /// \brief Sets a shader permutation variable (string-based).
  void SetShaderPermutationVariable(xiiStringView sName, const xiiTempHashedString& sValue);

  /// \brief Sets a shader permutation variable (hashed key).
  void SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue);

  /// \brief Binds a constant buffer to a shader slot.
  void BindConstantBuffer(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pConstantBuffer);

  /// \brief Binds a buffer view as a shader resource.
  void BindBufferView(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBufferView> pBufferView);

  /// \brief Binds a texture view as a shader resource.
  void BindTextureView(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTextureView> pTextureView);

  /// \brief Binds a sampler state to a shader slot.
  void BindSampler(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALSampler> pSampler);

  /// \brief Binds a buffer view as an unordered access view (UAV).
  void BindBufferViewUAV(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBufferView> pBufferView);

  /// \brief Binds a texture view as an unordered access view (UAV).
  void BindTextureViewUAV(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTextureView> pTextureView);

  /// \brief Binds a 2D texture resource to a shader slot.
  void BindTexture2D(const xiiTempHashedString& sSlotName, const xiiTexture2DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);

  /// \brief Binds a 3D texture resource to a shader slot.
  void BindTexture3D(const xiiTempHashedString& sSlotName, const xiiTexture3DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);

  /// \brief Binds a cube texture resource to a shader slot.
  void BindTextureCube(const xiiTempHashedString& sSlotName, const xiiTextureCubeResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);

  /// \brief Binds a material, including shader and its parameters.
  void BindMaterial(const xiiMaterialResourceHandle& hMaterial);

  /// \brief Sets the shader for the next rendering or compute call.
  ///
  /// \param hShader - The shader to bind.
  /// \param flags   - Optional binding flags.
  void BindShader(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags = xiiShaderBindFlags::Default);

  /// \brief Sets the blend state for the graphics pipeline.
  ///
  /// \param pBlendState - A shared pointer to the blend state object to be used.
  void SetBlendState(xiiSharedPtr<xiiGALBlendState> pBlendState);

  /// \brief Sets the depth-stencil state for the graphics pipeline to control depth and stencil testing behavior..
  ///
  /// \param pDepthStencilState - A shared pointer to the depth-stencil state object to be used.
  void SetDepthStencilState(xiiSharedPtr<xiiGALDepthStencilState> pDepthStencilState);

  /// \brief Sets the rasterizer state for the graphics pipeline.
  ///
  /// \param pRasterizerState - A shared pointer to the rasterizer state object to be used.
  void SetRasterizerState(xiiSharedPtr<xiiGALRasterizerState> pRasterizerState);

  /// \brief Binds a dynamic mesh buffer for rendering.
  void BindMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hDynamicMeshBuffer);

  /// \brief Binds a static mesh buffer for rendering.
  void BindMeshBuffer(const xiiMeshBufferResourceHandle& hMeshBuffer);

  /// \brief Binds raw GPU buffers with custom vertex/index layout.
  ///
  /// Allows procedural or non-resource-backed geometry.
  void BindMeshBuffer(xiiArrayPtr<xiiSharedPtr<xiiGALBuffer>> pVertexBuffers, xiiSharedPtr<xiiGALBuffer> pIndexBuffer, const xiiInputLayoutInfo* pInputLayoutInfo, xiiEnum<xiiGALPrimitiveTopology> topology, xiiUInt32 uiPrimitiveCount);

  /// \brief Issues a draw call for the currently bound mesh buffer.
  ///
  /// \param uiPrimitiveCount Optional override for draw range.
  /// \param uiFirstPrimitive Starting primitive index.
  /// \param uiInstanceCount Number of instances to render.
  xiiResult DrawMeshBuffer(xiiUInt32 uiPrimitiveCount = 0xFFFFFFFFU, xiiUInt32 uiFirstPrimitive = 0U, xiiUInt32 uiInstanceCount = 1U);

  /// \brief Dispatches a compute shader with the given thread group counts.
  xiiResult Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY = 1U, xiiUInt32 uiThreadGroupCountZ = 1U);

  /// \brief Applies all currently bound states to the GPU context.
  ///
  /// \param bForce Forces binding even if the state hasn't changed.
  xiiResult ApplyContextStates(bool bForce = false);

public:
  /// \brief Specifies the context scope in which rendering commands can be issued.
  ///
  /// Used to distinguish whether a render context is currently being used for graphics rendering,
  /// compute work, or is inactive. This allows the renderer to enforce valid command usage based on scope.
  enum class RenderContextScope : xiiUInt8
  {
    None = 0, ///< No rendering scope is active. Commands are not currently allowed.
    Graphics, ///< Graphics pipeline scope. Allows drawing, shading, and general render operations.
    Compute   ///< Compute pipeline scope. Allows compute shader dispatch and related GPU work.
  };

  /// \brief Returns the command list in use by this render context.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALCommandList> GetCommandList() const { return m_pCommandList; }

  /// \brief Returns the current render context scope.
  [[nodiscard]] XII_ALWAYS_INLINE RenderContextScope GetRenderContextScope() const { return m_RenderContextScope; }

  /// \brief Returns the global constant buffer that is used to store this render context global state.
  [[nodiscard]] XII_ALWAYS_INLINE xiiGlobalConstants* GetGlobalConstants() const { return m_pGlobalConstants.GetPtr(); }

  /// \brief Returns the pass constant buffer that is used to store per-pass state.
  [[nodiscard]] XII_ALWAYS_INLINE xiiPassConstants* GetPassConstants() const { return m_pPassConstants.GetPtr(); }

  /// \brief Returns async shader loading. During runtime all shaders should be preloaded so this is off by default.
  [[nodiscard]] XII_ALWAYS_INLINE bool GetAllowAsyncShaderLoading() const { return m_bAllowAsyncShaderLoading; }

  /// \brief Enables or disables asynchronous shader loading.
  ///
  /// During runtime, all shaders should ideally be preloaded. Enabling async shader loading allows
  /// shaders to be loaded in the background. This is disabled by default for stability and predictability.
  ///
  /// \param bAllow - If true, asynchronous shader loading is enabled; otherwise, it is disabled.
  XII_ALWAYS_INLINE void SetAllowAsyncShaderLoading(bool bAllow) { m_bAllowAsyncShaderLoading = bAllow; }

  /// \brief Binds a buffer to the specified shader slot as a shader resource view (SRV).
  ///
  /// Internally retrieves the default shader resource view from the buffer and passes it to BindBufferView().
  ///
  /// \param sSlotName - The name of the shader slot.
  /// \param pBuffer   - The buffer to bind.
  XII_ALWAYS_INLINE void BindBuffer(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pBuffer) { BindBufferView(sSlotName, pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource)); }

  /// \brief Binds a texture to the specified shader slot as a shader resource view (SRV).
  ///
  /// Internally retrieves the default shader resource view from the texture and passes it to BindTextureView().
  ///
  /// \param sSlotName - The name of the shader slot.
  /// \param pTexture  - The texture to bind.
  XII_ALWAYS_INLINE void BindTexture(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTexture> pTexture) { BindTextureView(sSlotName, pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource)); }

  /// \brief Binds a buffer to the specified shader slot as an unordered access view (UAV).
  ///
  /// Internally retrieves the default UAV view from the buffer and passes it to BindBufferViewUAV().
  ///
  /// \param sSlotName - The name of the shader slot.
  /// \param pBuffer   - The buffer to bind as UAV.
  XII_ALWAYS_INLINE void BindBufferUAV(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pBuffer) { BindBufferViewUAV(sSlotName, pBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess)); }

  /// \brief Binds a texture to the specified shader slot as an unordered access view (UAV).
  ///
  /// Internally retrieves the default UAV view from the texture and passes it to BindTextureViewUAV().
  ///
  /// \param sSlotName - The name of the shader slot.
  /// \param pTexture  - The texture to bind as UAV.
  XII_ALWAYS_INLINE void BindTextureUAV(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTexture> pTexture) { BindTextureViewUAV(sSlotName, pTexture->GetDefaultView(xiiGALTextureViewType::UnorderedAccess)); }

  /// \brief Binds a null mesh buffer to the pipeline for procedural or indirect drawing.
  ///
  /// Useful when drawing primitives without explicit vertex/index buffers, such as fullscreen passes.
  ///
  /// \param topology         - The type of primitive topology to use.
  /// \param uiPrimitiveCount - The number of primitives to render.
  XII_ALWAYS_INLINE void BindNullMeshBuffer(xiiEnum<xiiGALPrimitiveTopology> topology, xiiUInt32 uiPrimitiveCount) { BindMeshBuffer({}, nullptr, nullptr, topology, uiPrimitiveCount); }

  void SetGlobalAndWorldTimeConstants();
  void SetGlobalAndWorldTimeConstants(xiiTime worldTime);

  /// \brief Retrieves a default sampler creation description based on the given flags.
  ///
  /// This utility function returns commonly used sampler settings such as filtering and addressing modes, derived from the specified flags.
  ///
  /// \param flags Bitflags that define the characteristics of the sampler.
  /// \return A sampler creation description that matches the requested features.
  static xiiGALSamplerCreationDescription GetDefaultSamplerDescription(xiiBitflags<xiiDefaultSamplerFlags> flags);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RendererContext);

  static void OnEngineStartup();
  static void OnEngineShutdown();
  static void GALStaticDeviceEventHandler(const xiiGALDeviceEvent& e);

  /// \brief Resets all context bindings and internal states.
  void ResetContextState();

  void SetShaderPermutationVariableInternal(const xiiHashedString& sName, const xiiHashedString& sValue);

  void BindShaderInternal(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags);

  xiiShaderPermutationResource* ApplyShaderState();
  xiiMaterialResource*          ApplyMaterialState();
  void                          ApplyConstantBufferBindings();
  void                          ApplyBufferSRVBindings();
  void                          ApplyTextureSRVBindings();
  void                          ApplyBufferUAVBindings();
  void                          ApplyTextureUAVBindings();
  void                          ApplySamplerBindings();

  void BeginInternalRenderPass();
  void BeginClearThenLoadInternalRenderPass();
  void EndInternalRenderPass();

  void PrepareGraphicsPipelineDescriptor(xiiShaderPermutationResource* pShaderPermutation);
  void PrepareComputePipelineDescriptor(xiiShaderPermutationResource* pShaderPermutation);

  void ApplyScissor();

private:
  struct RenderPassCache
  {
    XII_ALWAYS_INLINE RenderPassCache() = default;

    XII_ALWAYS_INLINE RenderPassCache(xiiSharedPtr<xiiGALRenderPass> pRenderPass) :
      m_pRenderPass(pRenderPass)
    {
    }

    xiiSharedPtr<xiiGALRenderPass> m_pRenderPass;
  };

  struct FramebufferCache
  {
    XII_ALWAYS_INLINE FramebufferCache() = default;

    xiiHybridArray<xiiSharedPtr<xiiGALFramebuffer>, 3U> m_Framebuffers;
  };

  struct ShaderVertexDeclaration
  {
    xiiSharedPtr<xiiGALShader> m_pShader;
    xiiUInt32                  m_uiInputLayoutHash;

    XII_FORCE_INLINE bool operator<(const ShaderVertexDeclaration& rhs) const { return m_uiInputLayoutHash < rhs.m_uiInputLayoutHash; }

    XII_FORCE_INLINE bool operator==(const ShaderVertexDeclaration& rhs) const { return (m_pShader == rhs.m_pShader && m_uiInputLayoutHash == rhs.m_uiInputLayoutHash); }
  };

  static xiiSharedPtr<xiiGALRenderPass> GetOrCreateRenderPass(const xiiGALRenderPassCreationDescription& description);

  static xiiSharedPtr<xiiGALFramebuffer> GetOrCreateFramebuffer(const xiiGALRenderPassCreationDescription& description, const xiiRenderingSetup& renderingSetup);

  static xiiResult BuildInputLayout(xiiSharedPtr<xiiGALShader> pVertexShader, xiiArrayPtr<xiiUInt32> pVertexBufferStrides, xiiArrayPtr<xiiEnum<xiiGALInputElementFrequency>> pInputElementFrequencies, const xiiInputLayoutInfo& declaration, const xiiInputLayoutInfo& customDeclaration, xiiSharedPtr<xiiGALInputLayout>& out_Declaration);

private:
  static xiiHashTable<xiiGALRenderPassCreationDescription, RenderPassCache, xiiGALDescriptorHash>  s_RenderPassCache;
  static xiiHashTable<xiiGALRenderPassCreationDescription, FramebufferCache, xiiGALDescriptorHash> s_FramebufferCache;
  static xiiMap<ShaderVertexDeclaration, xiiSharedPtr<xiiGALInputLayout>>                          s_InputLayouts;

private:
  xiiSharedPtr<xiiGALCommandList> m_pCommandList;

  RenderContextScope                 m_RenderContextScope       = RenderContextScope::None;
  bool                               m_bHasDebugGroup           = false;
  bool                               m_bStereoRendering         = false;
  bool                               m_bAllowAsyncShaderLoading = false;
  xiiBitflags<xiiRenderContextFlags> m_StateFlags;

  xiiRenderingSetup              m_RenderingSetup;
  bool                           m_bNeedsClear         = false;
  bool                           m_bIsRenderPassActive = false;
  xiiSharedPtr<xiiGALRenderPass> m_pActiveRenderPass;

  xiiBlobPtr<xiiGlobalConstants> m_pGlobalConstants;
  xiiSharedPtr<xiiGALBuffer>     m_pGlobalConstantsBuffer;

  xiiBlobPtr<xiiPassConstants> m_pPassConstants;
  xiiSharedPtr<xiiGALBuffer>   m_pPassConstantsBuffer;

  xiiGALGraphicsPipelineStateCreationDescription m_GraphicsPipelineDescription;
  xiiSharedPtr<xiiGALGraphicsPipelineState>      m_pGraphicsPipelineState;

  xiiGALComputePipelineStateCreationDescription m_ComputePipelineDescription;
  xiiSharedPtr<xiiGALComputePipelineState>      m_pComputePipelineState;

  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALBuffer>>      m_BoundConstantBuffers;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALBufferView>>  m_BoundBufferSRVs;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALTextureView>> m_BoundTextureSRVs;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALBufferView>>  m_BoundBufferUAVs;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALTextureView>> m_BoundTextureUAVs;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALSampler>>     m_BoundSamplers;

  xiiShaderResourceHandle                                          m_hActiveShader;
  xiiMap<xiiGALShaderType::Enum, xiiSharedPtr<xiiGALShader>>       m_ActiveGALShaders;
  xiiShaderPermutationResourceHandle                               m_hActiveShaderPermutation;
  const xiiInputLayoutInfo*                                        m_pInputLayoutInfo = nullptr;
  xiiMap<ShaderVertexDeclaration, xiiSharedPtr<xiiGALInputLayout>> m_InputLayouts;
  xiiBitflags<xiiShaderBindFlags>                                  m_ShaderBindFlags;

  xiiMaterialResourceHandle m_hNewMaterial;
  xiiMaterialResourceHandle m_hMaterial;

  xiiHybridArray<xiiSharedPtr<xiiGALBuffer>, 4U>           m_VertexBuffers;
  xiiHybridArray<xiiUInt64, 4U>                            m_VertexBufferOffsets;
  xiiHybridArray<xiiUInt32, 4U>                            m_VertexBufferStrides;
  xiiHybridArray<xiiEnum<xiiGALInputElementFrequency>, 4U> m_VertexBufferFrequencies;
  xiiInputLayoutInfo                                       m_CustomInputLayout;

  xiiSharedPtr<xiiGALBuffer> m_pIndexBuffer;
  xiiUInt64                  m_uiIndexDataOffset = 0ULL;

  xiiUInt32 m_uiMeshBufferPrimitiveCount = 0U;

  xiiHashTable<xiiHashedString, xiiHashedString> m_PermutationVariables;
};
