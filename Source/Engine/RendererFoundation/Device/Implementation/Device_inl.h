
/// \brief Used to guard xiiGALDevice functions from multi-threaded access and to verify that executing them on non-main-threads is allowed
#define XII_GALDEVICE_LOCK_AND_CHECK() \
  XII_LOCK(m_Mutex);                   \
  VerifyMultithreadedAccess()

XII_ALWAYS_INLINE const xiiGALDeviceCreationDescription* xiiGALDevice::GetDescription() const
{
  return &m_Description;
}

XII_ALWAYS_INLINE xiiResult xiiGALDevice::GetTimestampResult(xiiGALTimestampHandle hTimestamp, xiiTime& ref_result)
{
  return GetTimestampResultPlatform(hTimestamp, ref_result);
}

XII_ALWAYS_INLINE xiiGALTimestampHandle xiiGALDevice::GetTimestamp()
{
  return GetTimestampPlatform();
}

template <typename IdTableType, typename ReturnType>
XII_ALWAYS_INLINE ReturnType* xiiGALDevice::Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  ReturnType* pObject = nullptr;
  IdTable.TryGetValue(hHandle, pObject);
  return pObject;
}

inline const xiiGALSwapChain* xiiGALDevice::GetSwapChain(xiiGALSwapChainHandle hSwapChain) const
{
  return Get<SwapChainTable, xiiGALSwapChain>(hSwapChain, m_SwapChains);
}

inline const xiiGALShader* xiiGALDevice::GetShader(xiiGALShaderHandle hShader) const
{
  return Get<ShaderTable, xiiGALShader>(hShader, m_Shaders);
}

inline const xiiGALTexture* xiiGALDevice::GetTexture(xiiGALTextureHandle hTexture) const
{
  return Get<TextureTable, xiiGALTexture>(hTexture, m_Textures);
}

inline const xiiGALBuffer* xiiGALDevice::GetBuffer(xiiGALBufferHandle hBuffer) const
{
  return Get<BufferTable, xiiGALBuffer>(hBuffer, m_Buffers);
}

inline const xiiGALDepthStencilState* xiiGALDevice::GetDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState) const
{
  return Get<DepthStencilStateTable, xiiGALDepthStencilState>(hDepthStencilState, m_DepthStencilStates);
}

inline const xiiGALBlendState* xiiGALDevice::GetBlendState(xiiGALBlendStateHandle hBlendState) const
{
  return Get<BlendStateTable, xiiGALBlendState>(hBlendState, m_BlendStates);
}

inline const xiiGALRasterizerState* xiiGALDevice::GetRasterizerState(xiiGALRasterizerStateHandle hRasterizerState) const
{
  return Get<RasterizerStateTable, xiiGALRasterizerState>(hRasterizerState, m_RasterizerStates);
}

inline const xiiGALVertexDeclaration* xiiGALDevice::GetVertexDeclaration(xiiGALVertexDeclarationHandle hVertexDeclaration) const
{
  return Get<VertexDeclarationTable, xiiGALVertexDeclaration>(hVertexDeclaration, m_VertexDeclarations);
}

inline const xiiGALSamplerState* xiiGALDevice::GetSamplerState(xiiGALSamplerStateHandle hSamplerState) const
{
  return Get<SamplerStateTable, xiiGALSamplerState>(hSamplerState, m_SamplerStates);
}

inline const xiiGALResourceView* xiiGALDevice::GetResourceView(xiiGALResourceViewHandle hResourceView) const
{
  return Get<ResourceViewTable, xiiGALResourceView>(hResourceView, m_ResourceViews);
}

inline const xiiGALRenderTargetView* xiiGALDevice::GetRenderTargetView(xiiGALRenderTargetViewHandle hRenderTargetView) const
{
  return Get<RenderTargetViewTable, xiiGALRenderTargetView>(hRenderTargetView, m_RenderTargetViews);
}

inline const xiiGALUnorderedAccessView* xiiGALDevice::GetUnorderedAccessView(xiiGALUnorderedAccessViewHandle hUnorderedAccessView) const
{
  return Get<UnorderedAccessViewTable, xiiGALUnorderedAccessView>(hUnorderedAccessView, m_UnorderedAccessViews);
}

inline const xiiGALQuery* xiiGALDevice::GetQuery(xiiGALQueryHandle hQuery) const
{
  return Get<QueryTable, xiiGALQuery>(hQuery, m_Queries);
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
  XII_ASSERT_DEV(m_Capabilities.m_bMultithreadedResourceCreation || xiiThreadUtils::IsMainThread(),
                 "This device does not support multi-threaded resource creation, therefore this function can only be executed on the main thread.");
#endif
}
