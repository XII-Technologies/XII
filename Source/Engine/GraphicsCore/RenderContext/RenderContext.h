#include <GraphicsCore/GraphicsCoreDLL.h>

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
// xiiRenderContext
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiRenderContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderContext);

public:
  xiiRenderContext(xiiSharedPtr<xiiGALCommandList> pCommandList);
  ~xiiRenderContext();

  XII_ALWAYS_INLINE xiiGALCommandList* GetCommandList() const { return m_pCommandList.Borrow(); }

  void BeginRendering(const xiiRenderingSetup& renderingSetup, const xiiRectFloat& viewport, xiiStringView sName = {}, bool bStereoRendering = false);
  void EndRendering();

  void BeginCompute(xiiStringView sName = {});
  void EndCompute();

  void SetShaderPermutationVariable(xiiStringView sName, const xiiTempHashedString& sValue);
  void SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue);

public:
  enum class RenderContextScope : xiiUInt8
  {
    None     = 0,
    Graphics = 1,
    Compute  = 2
  };

  /// \brief Returns the current render context scope.
  [[nodiscard]] XII_ALWAYS_INLINE RenderContextScope GetRenderContextScope() const { return m_RenderContextScope; }

private:
  void SetShaderPermutationVariableInternal(const xiiHashedString& sName, const xiiHashedString& sValue);

private:
  xiiSharedPtr<xiiGALCommandList> m_pCommandList;

  RenderContextScope                 m_RenderContextScope = RenderContextScope::None;
  bool                               m_bHasDebugGroup     = false;
  xiiBitflags<xiiRenderContextFlags> m_StateFlags;

  xiiGlobalConstants         m_GlobalConstants;
  xiiSharedPtr<xiiGALBuffer> m_GlobalConstantsBuffer;

  xiiHashTable<xiiHashedString, xiiHashedString> m_PermutationVariables;
};
