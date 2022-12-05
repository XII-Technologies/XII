#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Buffer.h>

xiiGALBuffer::xiiGALBuffer(const xiiGALBufferCreationDescription& Description) :
  xiiGALResource(Description)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(Description.m_szName);
#endif
}

xiiGALBuffer::~xiiGALBuffer() {}



XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Buffer);
