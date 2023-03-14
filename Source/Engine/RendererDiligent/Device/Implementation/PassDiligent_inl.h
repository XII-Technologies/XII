
XII_ALWAYS_INLINE Diligent::IRenderPass* xiiGALPassDiligent::GetRenderPass(const xiiGALRenderingSetup& renderingSetup)
{
  return *m_RenderPasses.GetValue(renderingSetup);
}

XII_ALWAYS_INLINE Diligent::IFramebuffer* xiiGALPassDiligent::GetFramebuffer(const xiiGALRenderingSetup& renderingSetup)
{
  return *m_Framebuffers.GetValue(renderingSetup);
}
