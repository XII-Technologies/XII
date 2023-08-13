#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALAttachmentLoadOperation, 1)
  XII_ENUM_CONSTANT(xiiGALAttachmentLoadOperation::Load),
  XII_ENUM_CONSTANT(xiiGALAttachmentLoadOperation::Clear),
  XII_ENUM_CONSTANT(xiiGALAttachmentLoadOperation::Discard),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALAttachmentStoreOperation, 1)
  XII_ENUM_CONSTANT(xiiGALAttachmentStoreOperation::Store),
  XII_ENUM_CONSTANT(xiiGALAttachmentStoreOperation::Discard),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on

xiiGALRenderPass::xiiGALRenderPass(const xiiGALRenderPassCreationDescription& creationDescription) :
  xiiGALResource<xiiGALRenderPassCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALRenderPass::~xiiGALRenderPass() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_RenderPass);
