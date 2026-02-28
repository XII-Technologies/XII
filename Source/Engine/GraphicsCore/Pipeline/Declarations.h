#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/Declarations.h>

class xiiCamera;
class xiiExtractedRenderData;
class xiiExtractor;
class xiiView;
class xiiRenderer;
class xiiRenderData;
class xiiRenderDataManager;
class xiiRenderDataBatch;
class xiiRenderPipeline;
class xiiRenderPipelinePassBase;
class xiiGraphicsPipelinePass;
class xiiComputePipelinePass;
class xiiCopyPipelinePass;
class xiiPresentPipelinePass;
class xiiUtilityPipelinePass;
class xiiDebugRendererContext;

struct xiiRenderPipelineNodePin;
struct xiiRenderPipelineNodeInputBufferPin;
struct xiiRenderPipelineNodeInputColourAttachmentPin;
struct xiiRenderPipelineNodeInputDepthAttachmentPin;
struct xiiRenderPipelineNodeInputSamplerPin;
struct xiiRenderPipelineNodeInputAccelerationStructurePin;
struct xiiRenderPipelineNodeOutputBufferPin;
struct xiiRenderPipelineNodeOutputColourAttachmentPin;
struct xiiRenderPipelineNodeOutputDepthAttachmentPin;
struct xiiRenderPipelineNodeOutputSamplerPin;
struct xiiRenderPipelineNodeOutputAccelerationStructurePin;
struct xiiRenderPipelineNodePassThroughBufferPin;
struct xiiRenderPipelineNodePassThroughColourAttachmentPin;
struct xiiRenderPipelineNodePassThroughDepthAttachmentPin;
struct xiiRenderPipelineNodePassThroughSamplerPin;
struct xiiRenderPipelineNodePassThroughAccelerationStructurePin;
struct xiiRenderPipelineNodeInputBufferProviderPin;
struct xiiRenderPipelineNodeInputColourAttachmentProviderPin;
struct xiiRenderPipelineNodeInputDepthAttachmentProviderPin;
struct xiiRenderPipelineNodeInputSamplerProviderPin;
struct xiiRenderPipelineNodeInputAccelerationStructureProviderPin;
struct xiiRenderPipelineNodeOutputBufferProviderPin;
struct xiiRenderPipelineNodeOutputColourAttachmentProviderPin;
struct xiiRenderPipelineNodeOutputDepthAttachmentProviderPin;
struct xiiRenderPipelineNodeOutputSamplerProviderPin;
struct xiiRenderPipelineNodeOutputAccelerationStructureProviderPin;
struct xiiRenderPipelinePassConnection;
struct xiiViewData;

namespace xiiInternal
{
  struct RenderDataCache;

  struct RenderDataCacheEntry
  {
    XII_DECLARE_POD_TYPE();

    const xiiRenderData* m_pRenderData      = nullptr;
    xiiUInt16            m_uiCategory       = 0;
    xiiUInt16            m_uiComponentIndex = 0;
    xiiUInt16            m_uiPartIndex      = 0;

    XII_ALWAYS_INLINE bool operator==(const RenderDataCacheEntry& other) const { return m_pRenderData == other.m_pRenderData && m_uiCategory == other.m_uiCategory && m_uiComponentIndex == other.m_uiComponentIndex && m_uiPartIndex == other.m_uiPartIndex; }

    /// \brief Cache entries need to be sorted by component index and then by part index.
    XII_ALWAYS_INLINE bool operator<(const RenderDataCacheEntry& other) const
    {
      if (m_uiComponentIndex == other.m_uiComponentIndex)
        return m_uiPartIndex < other.m_uiPartIndex;

      return m_uiComponentIndex < other.m_uiComponentIndex;
    }
  };
} // namespace xiiInternal

/// \brief Describes the format of pixel data used for textures and attachments in the rendering pipeline.
///
/// The source format determines how color, depth, or compressed texture data is stored and interpreted during rendering.
/// It includes both uncompressed and block-compressed formats, floating-point precision options, and sRGB or linear variants.
///
/// These formats are typically used when specifying render target formats, texture asset loading, or framebuffer attachment configurations.
struct XII_GRAPHICSCORE_DLL xiiSourceFormat
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Color4Channel8BitNormalized_sRGB = 0U, ///< 8-bit RGBA, normalized, stored in sRGB color space (default).
    Color4Channel8BitNormalized,           ///< 8-bit RGBA, normalized, stored in linear color space.
    Color2Channel16BitFloat,               ///< 16-bit float RG format.
    Color4Channel16BitFloat,               ///< 16-bit float RGBA format.
    Color2Channel32BitFloat,               ///< 32-bit float RG format.
    Color3Channel32BitFloat,               ///< 32-bit float RGB format.
    Color4Channel32BitFloat,               ///< 32-bit float RGBA format.
    Color3Channel11_11_10BitFloat,         ///< Packed 11-11-10 bit RGB float format.
    Depth16Bit,                            ///< 16-bit depth-only format.
    Depth24BitStencil8Bit,                 ///< Combined 24-bit depth and 8-bit stencil format.
    Depth32BitFloat,                       ///< 32-bit float depth-only format.
    BC1_RGB_DXT1,                          ///< Block-compressed DXT1 format for RGB (no alpha).
    BC2_RGBA_DXT3,                         ///< Block-compressed DXT3 format for RGBA.
    BC3_RGBA_DXT5,                         ///< Block-compressed DXT5 format for RGBA.
    BC4_R_Grey_DXT5A,                      ///< Block-compressed DXT5A format for single-channel grayscale.
    BC5_RG_Grey_DXT5A,                     ///< Block-compressed DXT5A format for dual-channel grayscale.
    BC6H_RGB_Float,                        ///< High dynamic range RGB compression (BC6H).
    BC7_RGBA,                              ///< High-quality block compression for RGBA (BC7).

    ENUM_COUNT,

    Default = Color4Channel8BitNormalized_sRGB
  };

  /// \brief Converts a xiiSourceFormat into its corresponding xiiGALResourceFormat for low-level use.
  ///
  /// \param format             - The source format to translate.
  /// \param bFlipColorChannels - Whether to flip color channel order during conversion (optional).
  ///
  /// \return The corresponding GAL format enum usable in GPU resource creation.
  static xiiGALResourceFormat::Enum GetGALResourceFormat(xiiSourceFormat::Enum format, bool bFlipColorChannels = false);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSourceFormat);

/// \brief Defines the shading quality levels used in rendering operations.
///
/// This enumeration allows rendering systems or materials to selectively enable or disable visual features depending on the desired quality level.
/// Lower settings may omit expensive effects (e.g., shadows, complex lighting), while higher levels offer more realistic and detailed shading.
struct XII_GRAPHICSCORE_DLL xiiShadingQualityLevel
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Low = 0U, ///< Minimal shading, suitable for previews or constrained hardware.
    Medium,   ///< Standard shading with balanced performance and fidelity.
    High,     ///< Enhanced shading with advanced lighting or material features.
    Ultra,    ///< Maximum visual fidelity, which may include ray tracing or physically-based effects.

    ENUM_COUNT,

    Default = Medium
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiShadingQualityLevel);

struct XII_GRAPHICSCORE_DLL xiiRenderViewContext
{
  const xiiCamera*   m_pCamera   = nullptr;
  const xiiViewData* m_pViewData = nullptr;

  const xiiDebugRendererContext* m_pWorldDebugContext = nullptr;
  const xiiDebugRendererContext* m_pViewDebugContext  = nullptr;
};

using xiiViewId = xiiGenericId<24, 8>;

class xiiViewHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiViewHandle, xiiViewId);

  friend class xiiRenderWorld;
};

/// \brief HashHelper implementation so view handles can be used as key in a hashtable.
template <>
struct xiiHashHelper<xiiViewHandle>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiViewHandle value) { return value.GetInternalID().m_Data * 2654435761U; }

  XII_ALWAYS_INLINE static bool Equal(xiiViewHandle a, xiiViewHandle b) { return a == b; }
};

/// \brief Usage hint of a camera/view.
struct XII_GRAPHICSCORE_DLL xiiCameraUsageHint
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None,         ///< No hint, camera may not be used, at all.
    MainView,     ///< The main camera from which the scene gets rendered. There should only be one camera with this hint.
    EditorView,   ///< The editor view shall be rendered from this camera.
    RenderTarget, ///< The camera is used to render to a render target.
    Culling,      ///< This camera should be used for culling only. Usually culling is done from the main view, but with a dedicated culling camera, one can debug the culling system.
    Shadow,       ///< This camera is used for rendering shadow maps.
    Reflection,   ///< This camera is used for rendering reflections.
    Thumbnail,    ///< This camera should be used for rendering a scene thumbnail when exporting from the editor.

    ENUM_COUNT,

    Default = None,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiCameraUsageHint);
