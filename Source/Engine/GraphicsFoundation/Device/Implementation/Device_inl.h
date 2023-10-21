
/// \brief Used to guard xiiGALDevice functions from multi-threaded access and to verify that executing them on non-main-threads is allowed.
#define XII_GAL_DEVICE_LOCK_AND_CHECK() \
  XII_LOCK(m_Mutex);                    \
  VerifyMultithreadedAccess()

XII_ALWAYS_INLINE xiiMutex& xiiGALDevice::GetMutex() const
{
  return m_Mutex;
}

template <typename IdTableType, typename ReturnType>
XII_ALWAYS_INLINE ReturnType* xiiGALDevice::Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  ReturnType* pObject = nullptr;
  IdTable.TryGetValue(hHandle, pObject);
  return pObject;
}

inline const xiiGALSwapChain* xiiGALDevice::GetSwapChain(xiiGALSwapChainHandle hSwapChain) const
{
  return Get<SwapChainTable, xiiGALSwapChain>(hSwapChain, m_SwapChains);
}

inline const xiiGALBlendState* xiiGALDevice::GetBlendState(xiiGALBlendStateHandle hBlendState) const
{
  return Get<BlendStateTable, xiiGALBlendState>(hBlendState, m_BlendStates);
}

inline const xiiGALDepthStencilState* xiiGALDevice::GetDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState) const
{
  return Get<DepthStencilStateTable, xiiGALDepthStencilState>(hDepthStencilState, m_DepthStencilStates);
}

inline const xiiGALRasterizerState* xiiGALDevice::GetRasterizerState(xiiGALRasterizerStateHandle hRasterizerState) const
{
  return Get<RasterizerStateTable, xiiGALRasterizerState>(hRasterizerState, m_RasterizerStates);
}

inline const xiiGALShader* xiiGALDevice::GetShader(xiiGALShaderHandle hShader) const
{
  return Get<ShaderTable, xiiGALShader>(hShader, m_Shaders);
}

inline const xiiGALBuffer* xiiGALDevice::GetBuffer(xiiGALBufferHandle hBuffer) const
{
  return Get<BufferTable, xiiGALBuffer>(hBuffer, m_Buffers);
}

inline const xiiGALTexture* xiiGALDevice::GetTexture(xiiGALTextureHandle hTexture) const
{
  return Get<TextureTable, xiiGALTexture>(hTexture, m_Textures);
}

inline const xiiGALBufferView* xiiGALDevice::GetBufferView(xiiGALBufferViewHandle hBufferView) const
{
  return Get<BufferViewTable, xiiGALBufferView>(hBufferView, m_BufferViews);
}

inline const xiiGALTextureView* xiiGALDevice::GetTextureView(xiiGALTextureViewHandle hTextureView) const
{
  return Get<TextureViewTable, xiiGALTextureView>(hTextureView, m_TextureViews);
}

inline const xiiGALSampler* xiiGALDevice::GetSampler(xiiGALSamplerHandle hSampler) const
{
  return Get<SamplerTable, xiiGALSampler>(hSampler, m_Samplers);
}

inline const xiiGALInputLayout* xiiGALDevice::GetInputLayout(xiiGALInputLayoutHandle hInputLayout) const
{
  return Get<InputLayoutTable, xiiGALInputLayout>(hInputLayout, m_InputLayouts);
}

inline const xiiGALQuery* xiiGALDevice::GetQuery(xiiGALQueryHandle hQuery) const
{
  return Get<QueryTable, xiiGALQuery>(hQuery, m_Queries);
}

inline const xiiGALFence* xiiGALDevice::GetFence(xiiGALFenceHandle hFence) const
{
  return Get<FenceTable, xiiGALFence>(hFence, m_Fences);
}

inline const xiiGALRenderPass* xiiGALDevice::GetRenderPass(xiiGALRenderPassHandle hRenderPass) const
{
  return Get<RenderPassTable, xiiGALRenderPass>(hRenderPass, m_RenderPasses);
}

inline const xiiGALFramebuffer* xiiGALDevice::GetFramebuffer(xiiGALFramebufferHandle hFramebuffer) const
{
  return Get<FramebufferTable, xiiGALFramebuffer>(hFramebuffer, m_Framebuffers);
}

inline const xiiGALBottomLevelAS* xiiGALDevice::GetBottomLevelAS(xiiGALBottomLevelASHandle hBottomLevelAS) const
{
  return Get<BottomLevelASTable, xiiGALBottomLevelAS>(hBottomLevelAS, m_BottomLevelAccelerationStructures);
}

inline const xiiGALTopLevelAS* xiiGALDevice::GetTopLevelAS(xiiGALTopLevelASHandle hTopLevelAS) const
{
  return Get<TopLevelASTable, xiiGALTopLevelAS>(hTopLevelAS, m_TopLevelAccelerationStructures);
}

inline const xiiGALGraphicsDeviceAdapterDescription& xiiGALDevice::GetGraphicsDeviceAdapterProperties() const
{
  return m_AdapterDescription;
}

// static
XII_ALWAYS_INLINE void xiiGALDevice::SetDefaultDevice(xiiGALDevice* pDefaultDevice)
{
  s_pDefaultDevice = pDefaultDevice;
}

// static
XII_ALWAYS_INLINE xiiGALDevice* xiiGALDevice::GetDefaultDevice()
{
  XII_ASSERT_DEBUG(s_pDefaultDevice != nullptr, "Default device not set.");

  return s_pDefaultDevice;
}

// static
XII_ALWAYS_INLINE bool xiiGALDevice::HasDefaultDevice()
{
  return s_pDefaultDevice != nullptr;
}

template <typename HandleType>
XII_FORCE_INLINE void xiiGALDevice::AddDeadObject(xiiUInt32 uiType, HandleType handle)
{
  auto& deadObject      = m_DeadObjects.ExpandAndGetRef();
  deadObject.m_uiType   = uiType;
  deadObject.m_uiHandle = handle.GetInternalID().m_Data;
}

template <typename HandleType>
void xiiGALDevice::ReviveDeadObject(xiiUInt32 uiType, HandleType handle)
{
  xiiUInt32 uiHandle = handle.GetInternalID().m_Data;

  for (xiiUInt32 i = 0; i < m_DeadObjects.GetCount(); ++i)
  {
    const auto& deadObject = m_DeadObjects[i];

    if (deadObject.m_uiType == uiType && deadObject.m_uiHandle == uiHandle)
    {
      m_DeadObjects.RemoveAtAndCopy(i);
      return;
    }
  }
}

XII_ALWAYS_INLINE void xiiGALDevice::VerifyMultithreadedAccess() const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_AdapterDescription.m_Features.m_MultithreadedResourceCreation == xiiGALDeviceFeatureState::Enabled || xiiThreadUtils::IsMainThread(),
                 "This device does not support multi-threaded resource creation, therefore this function can only be executed on the main thread.");
#endif
}
