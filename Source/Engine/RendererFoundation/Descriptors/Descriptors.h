
#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Types/RefCounted.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>
#include <Texture/Image/ImageEnums.h>

class xiiWindowBase;

struct XII_RENDERERFOUNDATION_DLL xiiShaderResourceType
{
  typedef xiiUInt8 StorageType;

  enum Enum : xiiUInt8
  {
    Unknown = 0,

    Texture1D        = 1,
    Texture1DArray   = 2,
    Texture2D        = 3,
    Texture2DArray   = 4,
    Texture2DMS      = 5,
    Texture2DMSArray = 6,
    Texture3D        = 7,
    TextureCube      = 8,
    TextureCubeArray = 9,

    UAV            = 10, ///< RW textures and buffers
    ConstantBuffer = 20, ///< Constant buffers
    GenericBuffer  = 21, ///< Read only (structured) buffers
    Sampler        = 22, ///< Separate sampler states

    Default = Unknown,
  };

  static bool IsArray(xiiShaderResourceType::Enum format);
};


/// \brief Defines a swap chain's present mode.
/// \sa xiiGALWindowSwapChainCreationDescription
struct XII_RENDERERFOUNDATION_DLL xiiGALPresentMode
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Immediate,
    VSync,

    ENUM_COUNT,

    Default = VSync
  };
};

struct xiiGALWindowSwapChainCreationDescription : public xiiHashableStruct<xiiGALWindowSwapChainCreationDescription>
{
  xiiWindowBase* m_pWindow = nullptr;

  // Describes the format that should be used for the backbuffer.
  // Note however, that different platforms may enforce restrictions on this.
  xiiGALMSAASampleCount::Enum m_SampleCount        = xiiGALMSAASampleCount::None;
  xiiGALResourceFormat::Enum  m_BackBufferFormat   = xiiGALResourceFormat::RGBAUByteNormalizedsRGB;
  xiiEnum<xiiGALPresentMode>  m_InitialPresentMode = xiiGALPresentMode::VSync;

  bool m_bDoubleBuffered   = true;
  bool m_bAllowScreenshots = false;

  // Sets the primary swapchain.
  // Note that rending backends optionally use this specification.
  bool m_bIsPrimarySwapchain = false;
};

struct xiiGALSwapChainCreationDescription : public xiiHashableStruct<xiiGALSwapChainCreationDescription>
{
  const xiiRTTI* m_pSwapChainType = nullptr;
};

struct xiiGALDeviceCreationDescription
{
  xiiGraphicsDeviceType::Enum m_GraphicsDevice = xiiGraphicsDeviceType::Undefined;

  bool m_bDebugDevice = false;
};

struct xiiGALShaderCreationDescription : public xiiHashableStruct<xiiGALShaderCreationDescription>
{
  xiiGALShaderCreationDescription();
  ~xiiGALShaderCreationDescription();

  bool HasByteCodeForStage(xiiGALShaderStage::Enum Stage) const;

  const char* m_szName = nullptr;

  xiiScopedRefPointer<xiiGALShaderByteCode> m_ByteCodes[xiiGALShaderStage::ENUM_COUNT];
};

struct xiiGALRenderTargetBlendDescription : public xiiHashableStruct<xiiGALRenderTargetBlendDescription>
{
  xiiGALBlendFactor::Enum    m_SourceBlend = xiiGALBlendFactor::One;
  xiiGALBlendFactor::Enum    m_DestBlend   = xiiGALBlendFactor::One;
  xiiGALBlendOperation::Enum m_BlendOp     = xiiGALBlendOperation::Add;

  xiiGALBlendFactor::Enum    m_SourceBlendAlpha = xiiGALBlendFactor::One;
  xiiGALBlendFactor::Enum    m_DestBlendAlpha   = xiiGALBlendFactor::One;
  xiiGALBlendOperation::Enum m_BlendOpAlpha     = xiiGALBlendOperation::Add;

  xiiGALColorWriteMask::Enum m_ColorWriteMask = xiiGALColorWriteMask::Default;

  bool m_bBlendingEnabled = false; ///< If enabled, the color will be blended into the render target. Otherwise it will overwrite the render target.
                                   ///< Set m_uiWriteMask to 0 to disable all writes to the render target.
};

struct xiiGALBlendStateCreationDescription : public xiiHashableStruct<xiiGALBlendStateCreationDescription>
{
  xiiGALRenderTargetBlendDescription m_RenderTargetBlendDescriptions[XII_GAL_MAX_RENDERTARGET_COUNT];

  bool m_bAlphaToCoverage  = false; ///< Alpha-to-coverage can only be used with MSAA render targets. Default is false.
  bool m_bIndependentBlend = false; ///< If disabled, the blend state of the first render target is used for all render targets. Otherwise each
                                    ///< render target uses a different blend state.
};

struct xiiGALStencilOpDescription : public xiiHashableStruct<xiiGALStencilOpDescription>
{
  xiiEnum<xiiGALStencilOperation> m_FailOp      = xiiGALStencilOperation::Keep;
  xiiEnum<xiiGALStencilOperation> m_DepthFailOp = xiiGALStencilOperation::Keep;
  xiiEnum<xiiGALStencilOperation> m_PassOp      = xiiGALStencilOperation::Keep;

  xiiEnum<xiiGALCompareFunc> m_StencilFunc = xiiGALCompareFunc::Always;
};

struct xiiGALDepthStencilStateCreationDescription : public xiiHashableStruct<xiiGALDepthStencilStateCreationDescription>
{
  xiiGALStencilOpDescription m_FrontFaceStencilOp;
  xiiGALStencilOpDescription m_BackFaceStencilOp;

  xiiEnum<xiiGALCompareFunc> m_DepthTestFunc = xiiGALCompareFunc::Less;

  bool m_bSeparateFrontAndBack = false; ///< If false, DX11 will use front face values for both front & back face values, GL will not call
                                        ///< gl*Separate() funcs
  bool     m_bDepthTest         = true;
  bool     m_bDepthWrite        = true;
  bool     m_bStencilTest       = false;
  xiiUInt8 m_uiStencilReadMask  = 0xFF;
  xiiUInt8 m_uiStencilWriteMask = 0xFF;
};

/// \brief Describes the settings for a new rasterizer state. See xiiGALDevice::CreateRasterizerState
struct xiiGALRasterizerStateCreationDescription : public xiiHashableStruct<xiiGALRasterizerStateCreationDescription>
{
  xiiEnum<xiiGALCullMode> m_CullMode               = xiiGALCullMode::Back; ///< Which sides of a triangle to cull. Default is xiiGALCullMode::Back
  xiiInt32                m_iDepthBias             = 0;                    ///< The pixel depth bias. Default is 0
  float                   m_fDepthBiasClamp        = 0.0f;                 ///< The pixel depth bias clamp. Default is 0
  float                   m_fSlopeScaledDepthBias  = 0.0f;                 ///< The pixel slope scaled depth bias clamp. Default is 0
  bool                    m_bWireFrame             = false;                ///< Whether triangles are rendered filled or as wireframe. Default is false
  bool                    m_bFrontCounterClockwise = false;                ///< Sets which triangle winding order defines the 'front' of a triangle. If true, the front of a triangle
                                                                           ///< is the one where the vertices appear in counter clockwise order. Default is false
  bool m_bScissorTest               = false;
  bool m_bConservativeRasterization = false; ///< Whether conservative rasterization is enabled
};

struct xiiGALSamplerStateCreationDescription : public xiiHashableStruct<xiiGALSamplerStateCreationDescription>
{
  xiiEnum<xiiGALTextureFilterMode> m_MinFilter;
  xiiEnum<xiiGALTextureFilterMode> m_MagFilter;
  xiiEnum<xiiGALTextureFilterMode> m_MipFilter;

  xiiEnum<xiiImageAddressMode> m_AddressU;
  xiiEnum<xiiImageAddressMode> m_AddressV;
  xiiEnum<xiiImageAddressMode> m_AddressW;

  xiiEnum<xiiGALCompareFunc> m_SampleCompareFunc;

  xiiColor m_BorderColor = xiiColor::Black;

  float m_fMipLodBias = 0.0f;
  float m_fMinMip     = -1.0f;
  float m_fMaxMip     = 42000.0f;

  xiiUInt32 m_uiMaxAnisotropy = 4;
};

struct xiiGALVertexAttributeSemantic
{
  enum Enum : xiiUInt8
  {
    Position,
    Normal,
    Tangent,
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    TexCoord0,
    TexCoord1,
    TexCoord2,
    TexCoord3,
    TexCoord4,
    TexCoord5,
    TexCoord6,
    TexCoord7,
    TexCoord8,
    TexCoord9,

    BiTangent,
    BoneIndices0,
    BoneIndices1,
    BoneWeights0,
    BoneWeights1,

    ENUM_COUNT
  };
};

struct xiiGALVertexAttribute
{
  xiiGALVertexAttribute() = default;

  xiiGALVertexAttribute(xiiGALVertexAttributeSemantic::Enum eSemantic, xiiGALResourceFormat::Enum eFormat, xiiUInt16 uiOffset, xiiUInt8 uiVertexBufferSlot, bool bInstanceData);

  xiiGALVertexAttributeSemantic::Enum m_eSemantic          = xiiGALVertexAttributeSemantic::Position;
  xiiGALResourceFormat::Enum          m_eFormat            = xiiGALResourceFormat::XYZFloat;
  xiiUInt16                           m_uiOffset           = 0;
  xiiUInt8                            m_uiVertexBufferSlot = 0;
  bool                                m_bInstanceData      = false;
  xiiUInt32                           m_uiStepRate         = 1;
};

struct XII_RENDERERFOUNDATION_DLL xiiGALVertexDeclarationCreationDescription : public xiiHashableStruct<xiiGALVertexDeclarationCreationDescription>
{
  xiiGALShaderHandle                        m_hShader;
  xiiStaticArray<xiiGALVertexAttribute, 16> m_VertexAttributes;
};

struct xiiGALResourceAccess
{
  XII_ALWAYS_INLINE bool IsImmutable() const { return m_bImmutable; }

  bool m_bReadBack  = false;
  bool m_bImmutable = true;
};

struct xiiGALBufferType
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Generic = 0,
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,

    ENUM_COUNT,

    Default = Generic
  };
};

struct xiiGALBufferCreationDescription : public xiiHashableStruct<xiiGALBufferCreationDescription>
{
  const char* m_szName = nullptr;

  xiiUInt32 m_uiStructSize = 0;
  xiiUInt32 m_uiTotalSize  = 0;

  xiiEnum<xiiGALBufferType> m_BufferType = xiiGALBufferType::Generic;

  bool m_bUseForIndirectArguments = false;
  bool m_bUseAsFormattedBuffer    = false;
  bool m_bUseAsStructuredBuffer   = false;
  bool m_bAllowRawViews           = false;
  bool m_bStreamOutputTarget      = false;
  bool m_bAllowShaderResourceView = false;
  bool m_bAllowUAV                = false;

  xiiGALResourceAccess m_ResourceAccess;
};

struct xiiGALTextureCreationDescription : public xiiHashableStruct<xiiGALTextureCreationDescription>
{
  void SetAsRenderTarget(
    xiiUInt32                   uiWidth,
    xiiUInt32                   uiHeight,
    xiiGALResourceFormat::Enum  format,
    xiiGALMSAASampleCount::Enum sampleCount = xiiGALMSAASampleCount::None);

  const char* m_szName = nullptr;

  xiiUInt32 m_uiWidth  = 0;
  xiiUInt32 m_uiHeight = 0;
  xiiUInt32 m_uiDepth  = 1;

  xiiUInt32 m_uiMipLevelCount = 1;

  xiiUInt32 m_uiArraySize = 1;

  xiiEnum<xiiGALResourceFormat> m_Format = xiiGALResourceFormat::Invalid;

  xiiEnum<xiiGALMSAASampleCount> m_SampleCount = xiiGALMSAASampleCount::None;

  xiiEnum<xiiGALTextureType> m_Type = xiiGALTextureType::Texture2D;

  bool m_bAllowShaderResourceView   = true;
  bool m_bAllowUAV                  = false;
  bool m_bCreateRenderTarget        = false;
  bool m_bAllowDynamicMipGeneration = false;

  xiiGALResourceAccess m_ResourceAccess;

  void* m_pExisitingNativeObject = nullptr; ///< Can be used to encapsulate existing native textures in objects usable by the GAL
};

struct xiiGALResourceViewCreationDescription : public xiiHashableStruct<xiiGALResourceViewCreationDescription>
{
  xiiGALTextureHandle m_hTexture;

  xiiGALBufferHandle m_hBuffer;

  xiiEnum<xiiGALResourceFormat> m_OverrideViewFormat = xiiGALResourceFormat::Invalid;

  // Texture only
  xiiUInt32 m_uiMostDetailedMipLevel = 0;
  xiiUInt32 m_uiMipLevelsToUse       = 0xFFFFFFFFu;

  xiiUInt32 m_uiFirstArraySlice = 0; // For cubemap array: index of first 2d slice to start with
  xiiUInt32 m_uiArraySize       = 1; // For cubemap array: number of cubemaps

  // Buffer only
  xiiUInt32 m_uiFirstElement = 0;
  xiiUInt32 m_uiNumElements  = 0;
  bool      m_bRawView       = false;
};

struct xiiGALRenderTargetViewCreationDescription : public xiiHashableStruct<xiiGALRenderTargetViewCreationDescription>
{
  xiiGALTextureHandle m_hTexture;

  xiiEnum<xiiGALResourceFormat> m_OverrideViewFormat = xiiGALResourceFormat::Invalid;

  xiiUInt32 m_uiMipLevel = 0;

  xiiUInt32 m_uiFirstSlice = 0;
  xiiUInt32 m_uiSliceCount = 1;

  bool m_bReadOnly = false; ///< Can be used for depth stencil views to create read only views (e.g. for soft particles using the native depth buffer)
};

struct xiiGALUnorderedAccessViewCreationDescription : public xiiHashableStruct<xiiGALUnorderedAccessViewCreationDescription>
{
  xiiGALTextureHandle m_hTexture;

  xiiGALBufferHandle m_hBuffer;

  xiiEnum<xiiGALResourceFormat> m_OverrideViewFormat = xiiGALResourceFormat::Invalid;

  // Texture only
  xiiUInt32 m_uiMipLevelToUse   = 0; ///< Which MipLevel is accessed with this UAV
  xiiUInt32 m_uiFirstArraySlice = 0; ///< First depth slice for 3D Textures.
  xiiUInt32 m_uiArraySize       = 1; ///< Number of depth slices for 3D textures.

  // Buffer only
  xiiUInt32 m_uiFirstElement = 0;
  xiiUInt32 m_uiNumElements  = 0;
  bool      m_bRawView       = false;
  bool      m_bAppend        = false; // Allows appending data to the end of the buffer.
};

struct xiiGALQueryType
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Occlusion,          ///< Number of samples that passed the depth and stencil test between begin and end (on a context).
    BinaryOcclusion,    ///< Acts like Occlusion. Returns true if at least one sample passed.
    Timestamp,          ///< Requests the GPU timestamp, similar to an EndQuery call.
    PipelineStatistics, ///< Gets the pipeline statistics such as the number of pixel shader invocations.
    Duration,           ///< Gets the number of high-frequency counter ticks between BeginQuery and EndQuery calls.

    Default = Occlusion

    // Note:
    // GALFence provides an implementation of "event queries".
  };
};

struct xiiGALQueryCreationDescription : public xiiHashableStruct<xiiGALQueryCreationDescription>
{
  xiiEnum<xiiGALQueryType> m_Type = xiiGALQueryType::Occlusion;

  const char* m_szName = nullptr;

  /// In case this query is used for occlusion culling (type AnySamplesPassed), this determines whether drawing should be done if the query
  /// status is still unknown.
  bool m_bDrawIfUnknown = true;
};

/// \brief Type for important GAL events.
struct xiiGALDeviceEvent
{
  enum Type
  {
    AfterInit,
    BeforeShutdown,
    BeforeBeginFrame,
    AfterBeginFrame,
    BeforeEndFrame,
    AfterEndFrame,
    // could add resource creation/destruction events, if this would be useful
  };

  Type                m_Type;
  class xiiGALDevice* m_pDevice;
};

#include <RendererFoundation/Descriptors/Implementation/Descriptors_inl.h>
