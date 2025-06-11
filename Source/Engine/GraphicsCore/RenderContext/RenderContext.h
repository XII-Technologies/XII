#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/RenderContext/RenderTargetSetup.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

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
    TextureBindingChanged        = XII_BIT(1),
    UAVBindingChanged            = XII_BIT(2),
    SamplerBindingChanged        = XII_BIT(3),
    BufferBindingChanged         = XII_BIT(4),
    ConstantBufferBindingChanged = XII_BIT(5),
    MeshBufferBindingChanged     = XII_BIT(6),
    MaterialBindingChanged       = XII_BIT(7),
    PipelineChanged              = XII_BIT(7),

    AllStatesInvalid = ShaderStateChanged | TextureBindingChanged | UAVBindingChanged | SamplerBindingChanged | BufferBindingChanged | ConstantBufferBindingChanged | MeshBufferBindingChanged | PipelineChanged,

    Default = None
  };

  struct Bits
  {
    StorageType ShaderStateChanged : 1;
    StorageType TextureBindingChanged : 1;
    StorageType UAVBindingChanged : 1;
    StorageType SamplerBindingChanged : 1;
    StorageType BufferBindingChanged : 1;
    StorageType ConstantBufferBindingChanged : 1;
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

public:
  /// \brief Initializes the render context with a command list.
  explicit xiiRenderContext(xiiSharedPtr<xiiGALCommandList> pCommandList);

  /// \brief Cleans up stateful bindings.
  ~xiiRenderContext();

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
  void BindSampler(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALSampler> pSamplerSate);

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

  /// \brief Binds a dynamic mesh buffer for rendering.
  void BindMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hDynamicMeshBuffer);

  /// \brief Binds a static mesh buffer for rendering.
  void BindMeshBuffer(const xiiMeshBufferResourceHandle& hMeshBuffer);

  /// \brief Binds raw GPU buffers with custom vertex/index layout.
  ///
  /// Allows procedural or non-resource-backed geometry.
  void BindMeshBuffer(xiiSharedPtr<xiiGALBuffer> pVertexBuffer, xiiSharedPtr<xiiGALBuffer> pIndexBuffer, const xiiInputLayoutInfo* pInputLayoutInfo, xiiEnum<xiiGALPrimitiveTopology> topology, xiiUInt32 uiPrimitiveCount, xiiSharedPtr<xiiGALBuffer> pVertexBuffer2 = {}, xiiSharedPtr<xiiGALBuffer> pVertexBuffer3 = {}, xiiSharedPtr<xiiGALBuffer> pVertexBuffer4 = {});

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

  /// \brief Resets all context bindings and internal states.
  void ResetContextState();

public:
  /// \brief Specifies the context scope in which rendering commands can be issued.
  ///
  /// Used to distinguish whether a render context is currently being used for graphics rendering,
  /// compute work, or is inactive. This allows the renderer to enforce valid command usage based on scope.
  enum class RenderContextScope : xiiUInt8
  {
    None     = 0, ///< No rendering scope is active. Commands are not currently allowed.
    Graphics = 1, ///< Graphics pipeline scope. Allows drawing, shading, and general render operations.
    Compute  = 2  ///< Compute pipeline scope. Allows compute shader dispatch and related GPU work.
  };

  /// \brief Returns the command list in use by this render context.
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALCommandList* GetCommandList() const { return m_pCommandList.Borrow(); }

  /// \brief Returns the current render context scope.
  [[nodiscard]] XII_ALWAYS_INLINE RenderContextScope GetRenderContextScope() const { return m_RenderContextScope; }

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
  XII_ALWAYS_INLINE void BindNullMeshBuffer(xiiEnum<xiiGALPrimitiveTopology> topology, xiiUInt32 uiPrimitiveCount) { BindMeshBuffer(nullptr, nullptr, nullptr, topology, uiPrimitiveCount); }

  /// \brief Retrieves a default sampler creation description based on the given flags.
  ///
  /// This utility function returns commonly used sampler settings such as filtering and addressing modes, derived from the specified flags.
  ///
  /// \param flags Bitflags that define the characteristics of the sampler.
  /// \return A sampler creation description that matches the requested features.
  static xiiGALSamplerCreationDescription GetDefaultSamplerDescription(xiiBitflags<xiiDefaultSamplerFlags> flags);

private:
  void SetShaderPermutationVariableInternal(const xiiHashedString& sName, const xiiHashedString& sValue);

private:
  xiiSharedPtr<xiiGALCommandList> m_pCommandList;

  RenderContextScope                 m_RenderContextScope       = RenderContextScope::None;
  bool                               m_bHasDebugGroup           = false;
  bool                               m_bStereoRendering         = false;
  bool                               m_bAllowAsyncShaderLoading = false;
  xiiBitflags<xiiRenderContextFlags> m_StateFlags;

  xiiBlobPtr<xiiGlobalConstants> m_GlobalConstants;
  xiiSharedPtr<xiiGALBuffer>     m_GlobalConstantsBuffer;

  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALBuffer>>      m_BoundConstantBuffers;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALBufferView>>  m_BoundBufferSRVs;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALTextureView>> m_BoundTextureSRVs;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALBufferView>>  m_BoundBufferUAVs;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALTextureView>> m_BoundTextureUAVs;

  xiiHashTable<xiiHashedString, xiiHashedString> m_PermutationVariables;
};
