#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALFramebuffer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALFramebuffer::xiiGALFramebuffer(const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALDeviceObject(), m_Description(creationDescription)
{
}

xiiGALFramebuffer::~xiiGALFramebuffer() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Framebuffer);
