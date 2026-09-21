/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// This describes the frame buffer creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALFramebufferCreationDescription
{
  xiiSharedPtr<xiiGALRenderPass>                      m_pRenderPass;                            ///< The handle to the render pass that the frame buffer will be compatible with.
  xiiHybridArray<xiiSharedPtr<xiiGALTextureView>, 4U> m_Attachments;                            ///< An array of attachments.
  xiiSizeU32                                          m_FramebufferSize   = xiiSizeU32(0U, 0U); ///< The size of the frame buffer. The default is (0, 0).
  xiiUInt32                                           m_uiArraySliceCount = 0U;                 ///< The number of array slices in the frame buffer. The default is 0.

  XII_ALWAYS_INLINE bool operator==(const xiiGALFramebufferCreationDescription& rhs) const = default;
};

/// Interface that defines methods to manipulate a frame buffer object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALFramebuffer : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALFramebuffer, xiiGALDeviceObject);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALFramebufferCreationDescription& GetDescription() const { return m_Description; }

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALFramebuffer(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALFramebufferCreationDescription& creationDescription);

  virtual ~xiiGALFramebuffer();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiGALFramebufferCreationDescription m_Description;
};
