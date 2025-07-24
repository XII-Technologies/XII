#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief This class can be used to define the render targets to be used by a xiiView.
struct XII_GRAPHICSCORE_DLL xiiRenderTargets
{
  bool operator==(const xiiRenderTargets& other) const;

  xiiSharedPtr<xiiGALTextureView> m_pRTs[8];
  xiiSharedPtr<xiiGALTextureView> m_pDSTarget;
};

/// \brief Defines a structured color attachment setup with default values.
struct XII_GRAPHICSCORE_DLL xiiAttachmentSettings
{
  xiiSharedPtr<xiiGALTextureView>         m_pRenderTarget;                                         ///< Target texture view.
  xiiColor                                m_ClearColor   = xiiColor::Black;                        ///< Default clear color (black).
  xiiEnum<xiiGALAttachmentLoadOperation>  m_LoadOp       = xiiGALAttachmentLoadOperation::Load;    ///< Default load operation.
  xiiEnum<xiiGALAttachmentStoreOperation> m_StoreOp      = xiiGALAttachmentStoreOperation::Store;  ///< Default store operation.
  xiiBitflags<xiiGALResourceStateFlags>   m_SubpassState = xiiGALResourceStateFlags::RenderTarget; ///< Default subpass state.
};

/// \brief Defines a structured depth-stencil attachment setup with default values.
struct XII_GRAPHICSCORE_DLL xiiDepthStencilSettings
{
  xiiSharedPtr<xiiGALTextureView>         m_pDSTarget;                                              ///< Depth-stencil target texture view.
  float                                   m_fDepthClear    = 1.0f;                                  ///< Default depth clear value.
  xiiUInt8                                m_uiStencilClear = 0U;                                    ///< Default stencil clear value.
  xiiEnum<xiiGALAttachmentLoadOperation>  m_LoadOp         = xiiGALAttachmentLoadOperation::Load;   ///< Depth buffer load operation.
  xiiEnum<xiiGALAttachmentStoreOperation> m_StoreOp        = xiiGALAttachmentStoreOperation::Store; ///< Depth buffer store operation.
  xiiEnum<xiiGALAttachmentLoadOperation>  m_StencilLoadOp  = xiiGALAttachmentLoadOperation::Load;   ///< Stencil buffer load operation.
  xiiEnum<xiiGALAttachmentStoreOperation> m_StencilStoreOp = xiiGALAttachmentStoreOperation::Store; ///< Stencil buffer store operation.
  xiiBitflags<xiiGALResourceStateFlags>   m_SubpassState   = xiiGALResourceStateFlags::DepthWrite;  ///< Default subpass state.
};

/// xiiRenderingSetup encapsulates creating both the render pass and the framebuffer.
/// It offers a unified interface with a fluent API:
///
/// \code{.cpp}
/// xiiRenderingSetup setup;
/// setup.AddColorAttachment(myColorSettings)
///   .SetDepthStencilAttachment(myDepthSettings)
///   .AddSubPass(myCustomSubPass)
///   .AddSubPassDependency(mySubPassDependency)
///   .Build();
/// \endcode
///
/// It automatically deduces the framebuffer size from the first attachment that is added.
///
/// \sa xiiGALRenderPassCreationDescription, xiiGALFramebufferCreationDescription, xiiRenderContext::BeginRendering
class XII_GRAPHICSCORE_DLL xiiRenderingSetup
{
public:
  XII_ALWAYS_INLINE xiiRenderingSetup()
  {
    // Start with an "unspecified" framebuffer size.
    m_FramebufferDescription.m_FramebufferSize   = xiiSizeU32(0, 0);
    m_FramebufferDescription.m_uiArraySliceCount = 0;
  }

  /// \brief Adds a color attachment based on the provided settings.
  XII_FORCE_INLINE xiiRenderingSetup& AddColorAttachment(const xiiAttachmentSettings& attachmentSettings)
  {
    DeduceFramebufferSize(attachmentSettings.m_pRenderTarget);
    AddColorAttachment(attachmentSettings.m_pRenderTarget, attachmentSettings.m_LoadOp, attachmentSettings.m_StoreOp, attachmentSettings.m_SubpassState, attachmentSettings.m_ClearColor);
    return *this;
  }

  /// \brief Sets (or replaces) the depth-stencil attachment.
  XII_FORCE_INLINE xiiRenderingSetup& SetDepthStencilAttachment(const xiiDepthStencilSettings& depthStencilSettings)
  {
    DeduceFramebufferSize(depthStencilSettings.m_pDSTarget);
    AddDepthStencilAttachment(depthStencilSettings.m_pDSTarget, depthStencilSettings.m_LoadOp, depthStencilSettings.m_StoreOp, depthStencilSettings.m_StencilLoadOp, depthStencilSettings.m_StencilStoreOp, depthStencilSettings.m_SubpassState, depthStencilSettings.m_fDepthClear, depthStencilSettings.m_uiStencilClear);
    return *this;
  }

  /// \brief Adds a sub pass description directly.
  XII_FORCE_INLINE xiiRenderingSetup& AddSubPass(const xiiGALSubPassDescription& subPass)
  {
    m_RenderPassDescription.m_SubPasses.PushBack(subPass);
    return *this;
  }

  /// \brief Adds a sub pass dependency.
  XII_FORCE_INLINE xiiRenderingSetup& AddSubPassDependency(const xiiGALSubPassDependencyDescription& dependency)
  {
    m_RenderPassDescription.m_Dependencies.PushBack(dependency);
    return *this;
  }

  /// \brief Finalizes the setup. This gathers all attachments into the render pass and framebuffer creation descriptors. Optionally, if no sub passes were added,
  /// a default sub pass is generated with color attachments as render targets and the depth-stencil attachment as the depth attachment.
  void Build();

  /// \brief This is the same as creating a new instance.
  void Reset();

  /// \brief Retrieves the render pass description.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALRenderPassCreationDescription& GetRenderPassDescription() const { return m_RenderPassDescription; }

  /// \brief Retrieves the framebuffer description.
  ///
  /// \note The render pass pointer is null, and is meant to be filled externally.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALFramebufferCreationDescription& GetFramebufferDescription() const { return m_FramebufferDescription; }

  /// \brief Retrieves the render pass clear values.
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiGALOptimizedClearValue> GetClearValues() const { return m_ClearValues; }

private:
  friend class xiiRenderContext;

  // Internal structure to hold information about each attachment.
  struct Attachment
  {
    xiiSharedPtr<xiiGALTextureView>         m_pView;
    xiiEnum<xiiGALAttachmentLoadOperation>  m_LoadOp;
    xiiEnum<xiiGALAttachmentStoreOperation> m_StoreOp;
    xiiEnum<xiiGALAttachmentLoadOperation>  m_StencilLoadOp;
    xiiEnum<xiiGALAttachmentStoreOperation> m_StencilStoreOp;
    xiiGALOptimizedClearValue               m_ClearValue;
    xiiBitflags<xiiGALResourceStateFlags>   m_SubpassState;
  };

  void AddColorAttachment(const xiiSharedPtr<xiiGALTextureView>& pView, xiiEnum<xiiGALAttachmentLoadOperation> loadOp, xiiEnum<xiiGALAttachmentStoreOperation> storeOp, xiiBitflags<xiiGALResourceStateFlags> subpassState, const xiiColor& clearColor);

  void AddDepthStencilAttachment(const xiiSharedPtr<xiiGALTextureView>& pView, xiiEnum<xiiGALAttachmentLoadOperation> loadOp, xiiEnum<xiiGALAttachmentStoreOperation> storeOp, xiiEnum<xiiGALAttachmentLoadOperation> stencilLoadOp, xiiEnum<xiiGALAttachmentStoreOperation> stencilStoreOp, xiiBitflags<xiiGALResourceStateFlags> subpassState, float fDepthClear, xiiUInt8 uiStencilClear);

  /// \brief Automatically sets the framebuffer size, if not already set, by inspecting the texture view.
  void DeduceFramebufferSize(const xiiSharedPtr<xiiGALTextureView>& pView);

  xiiHybridArray<Attachment, 4U>                m_Attachments;
  xiiGALRenderPassCreationDescription           m_RenderPassDescription;
  xiiGALFramebufferCreationDescription          m_FramebufferDescription;
  xiiStaticArray<xiiGALOptimizedClearValue, 4U> m_ClearValues;
};
