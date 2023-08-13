#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

xiiGALFramebuffer::xiiGALFramebuffer(const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALResource<xiiGALFramebufferCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALFramebuffer::~xiiGALFramebuffer() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Framebuffer);
