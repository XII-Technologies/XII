#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALFramebuffer, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

// clang-format on

xiiGALFramebuffer::xiiGALFramebuffer(const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALResource<xiiGALFramebufferCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALFramebuffer::~xiiGALFramebuffer() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Framebuffer);
