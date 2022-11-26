#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Buffer.h>

xiiGALBuffer::xiiGALBuffer(const xiiGALBufferCreationDescription& Description) :
  xiiGALResource(Description)
{
}

xiiGALBuffer::~xiiGALBuffer() {}



XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Buffer);
