#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/VertexDeclaration.h>

xiiGALVertexDeclaration::xiiGALVertexDeclaration(const xiiGALVertexDeclarationCreationDescription& Description) :
  xiiGALObject(Description)
{
}

xiiGALVertexDeclaration::~xiiGALVertexDeclaration() = default;


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_VertexDeclaration);
