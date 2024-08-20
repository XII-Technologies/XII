
XII_ALWAYS_INLINE constexpr bool xiiGALDeviceFeatures::operator==(const xiiGALDeviceFeatures& rhs) const
{
  return m_SeparablePrograms == rhs.m_SeparablePrograms &&
    m_ShaderResourceQueries == rhs.m_ShaderResourceQueries &&
    m_WireframeFill == rhs.m_WireframeFill &&
    m_MultithreadedResourceCreation == rhs.m_MultithreadedResourceCreation &&
    m_ComputeShaders == rhs.m_ComputeShaders &&
    m_GeometryShaders == rhs.m_GeometryShaders &&
    m_Tessellation == rhs.m_Tessellation &&
    m_MeshShaders == rhs.m_MeshShaders &&
    m_RayTracing == rhs.m_RayTracing &&
    m_BindlessResources == rhs.m_BindlessResources &&
    m_OcclusionQueries == rhs.m_OcclusionQueries &&
    m_BinaryOcclusionQueries == rhs.m_BinaryOcclusionQueries &&
    m_TimestampQueries == rhs.m_TimestampQueries &&
    m_PipelineStatisticsQueries == rhs.m_PipelineStatisticsQueries &&
    m_DurationQueries == rhs.m_DurationQueries &&
    m_DepthBiasClamp == rhs.m_DepthBiasClamp &&
    m_DepthClamp == rhs.m_DepthClamp &&
    m_IndependentBlend == rhs.m_IndependentBlend &&
    m_DualSourceBlend == rhs.m_DualSourceBlend &&
    m_MultiViewport == rhs.m_MultiViewport &&
    m_TextureCompressionBC == rhs.m_TextureCompressionBC &&
    m_VertexPipelineUAVWritesAndAtomics == rhs.m_VertexPipelineUAVWritesAndAtomics &&
    m_PixelUAVWritesAndAtomics == rhs.m_PixelUAVWritesAndAtomics &&
    m_TextureUAVExtendedFormats == rhs.m_TextureUAVExtendedFormats &&
    m_ShaderFloat16 == rhs.m_ShaderFloat16 &&
    m_ResourceBuffer16BitAccess == rhs.m_ResourceBuffer16BitAccess &&
    m_UniformBuffer16BitAccess == rhs.m_UniformBuffer16BitAccess &&
    m_ShaderInputOutput16 == rhs.m_ShaderInputOutput16 &&
    m_ShaderInt8 == rhs.m_ShaderInt8 &&
    m_ResourceBuffer8BitAccess == rhs.m_ResourceBuffer8BitAccess &&
    m_UniformBuffer8BitAccess == rhs.m_UniformBuffer8BitAccess &&
    m_ShaderResourceRuntimeArray == rhs.m_ShaderResourceRuntimeArray &&
    m_WaveOperation == rhs.m_WaveOperation &&
    m_InstanceDataStepRate == rhs.m_InstanceDataStepRate &&
    m_NativeFence == rhs.m_NativeFence &&
    m_TileShaders == rhs.m_TileShaders &&
    m_TransferQueueTimestampQueries == rhs.m_TransferQueueTimestampQueries &&
    m_VariableRateShading == rhs.m_VariableRateShading &&
    m_SparseResources == rhs.m_SparseResources &&
    m_SubpassFramebufferFetch == rhs.m_SubpassFramebufferFetch &&
    m_TextureComponentSwizzle == rhs.m_TextureComponentSwizzle &&
    m_NativeMultiDraw == rhs.m_NativeMultiDraw &&
    m_AsynchronousShaderCompilation == rhs.m_AsynchronousShaderCompilation;
}

XII_ALWAYS_INLINE constexpr bool xiiGALDepthStencilClearValue::operator==(const xiiGALDepthStencilClearValue& rhs) const
{
  return m_fDepth == rhs.m_fDepth && m_uiStencil == rhs.m_uiStencil;
}

XII_ALWAYS_INLINE constexpr bool xiiGALOptimizedClearValue::operator==(const xiiGALOptimizedClearValue& rhs) const
{
  return m_TextureFormat == rhs.m_TextureFormat && m_ClearColor == rhs.m_ClearColor && m_DepthStencil == rhs.m_DepthStencil;
}

XII_ALWAYS_INLINE constexpr bool xiiGALDisplayModeDescription::operator==(const xiiGALDisplayModeDescription& rhs) const
{
  return m_Resolution == rhs.m_Resolution &&
    m_TextureFormat == rhs.m_TextureFormat &&
    m_uiRefreshRateNumerator == rhs.m_uiRefreshRateNumerator &&
    m_uiRefreshRateDenominator == rhs.m_uiRefreshRateDenominator &&
    m_ScalingMode == rhs.m_ScalingMode &&
    m_ScanLineOrder == rhs.m_ScanLineOrder;
}

XII_ALWAYS_INLINE constexpr bool xiiGALSwapChainCreationDescription::operator==(const xiiGALSwapChainCreationDescription& rhs) const
{
  return m_pWindow == rhs.m_pWindow &&
    m_Resolution == rhs.m_Resolution &&
    m_ColorBufferFormat == rhs.m_ColorBufferFormat &&
    m_Usage == rhs.m_Usage &&
    m_PreTransform == rhs.m_PreTransform &&
    m_uiBufferCount == rhs.m_uiBufferCount &&
    m_fDefaultDepthValue == rhs.m_fDefaultDepthValue &&
    m_uiDefaultStencilValue == rhs.m_uiDefaultStencilValue &&
    m_bIsPrimary == rhs.m_bIsPrimary;
}

XII_ALWAYS_INLINE constexpr bool xiiGALFullScreenModeDescription::operator==(const xiiGALFullScreenModeDescription& rhs) const
{
  return m_bIsFullScreen == rhs.m_bIsFullScreen &&
    m_uiRefreshRateNumerator == rhs.m_uiRefreshRateNumerator &&
    m_uiRefreshRateDenominator == rhs.m_uiRefreshRateDenominator &&
    m_ScalingMode == rhs.m_ScalingMode &&
    m_ScanLineOrder == rhs.m_ScanLineOrder;
}

XII_ALWAYS_INLINE constexpr bool xiiGALTextureProperties::operator==(const xiiGALTextureProperties& rhs) const
{
  return m_uiMaxTexture1DDimension == rhs.m_uiMaxTexture1DDimension &&
    m_uiMaxTexture1DArraySlices == rhs.m_uiMaxTexture1DArraySlices &&
    m_uiMaxTexture2DDimension == rhs.m_uiMaxTexture2DDimension &&
    m_uiMaxTexture2DArraySlices == rhs.m_uiMaxTexture2DArraySlices &&
    m_uiMaxTexture3DDimension == rhs.m_uiMaxTexture3DDimension &&
    m_uiMaxTextureCubeDimension == rhs.m_uiMaxTextureCubeDimension &&
    m_bTexture2DMSSupported == rhs.m_bTexture2DMSSupported &&
    m_bTexture2DMSArraySupported == rhs.m_bTexture2DMSArraySupported &&
    m_bTextureViewSupported == rhs.m_bTextureViewSupported &&
    m_bCubeMapArraysSupported == rhs.m_bCubeMapArraysSupported &&
    m_bTextureView2DOn3DSupported == rhs.m_bTextureView2DOn3DSupported;
}

XII_ALWAYS_INLINE constexpr bool xiiGALSamplerProperties::operator==(const xiiGALSamplerProperties& rhs) const
{
  return m_bBorderSamplingModeSupported == rhs.m_bBorderSamplingModeSupported && m_uiMaxAnisotropy == rhs.m_uiMaxAnisotropy && m_bLODBiasSupported == rhs.m_bLODBiasSupported;
}

XII_ALWAYS_INLINE constexpr bool xiiGALWaveOperationProperties::operator==(const xiiGALWaveOperationProperties& rhs) const
{
  return m_uiMinSize == rhs.m_uiMinSize && m_uiMaxSize == rhs.m_uiMaxSize && m_SupportedShaderStages == rhs.m_SupportedShaderStages && m_WaveFeatures == rhs.m_WaveFeatures;
}

XII_ALWAYS_INLINE constexpr bool xiiGALBufferProperties::operator==(const xiiGALBufferProperties& rhs) const
{
  return m_uiConstantBufferAlignment == rhs.m_uiConstantBufferAlignment && m_uiStructuredBufferOffsetAlignment == rhs.m_uiStructuredBufferOffsetAlignment;
}

XII_ALWAYS_INLINE constexpr bool xiiGALRayTracingProperties::operator==(const xiiGALRayTracingProperties& rhs) const
{
  return m_uiMaxRecursionDepth == rhs.m_uiMaxRecursionDepth &&
    m_uiMaxRayGenThreads == rhs.m_uiMaxRayGenThreads &&
    m_uiMaxInstancesPerTLAS == rhs.m_uiMaxInstancesPerTLAS &&
    m_uiMaxPrimitivesPerBLAS == rhs.m_uiMaxPrimitivesPerBLAS &&
    m_uiMaxGeometriesPerBLAS == rhs.m_uiMaxGeometriesPerBLAS &&
    m_uiVertexBufferAlignment == rhs.m_uiVertexBufferAlignment &&
    m_uiIndexBufferAlignment == rhs.m_uiIndexBufferAlignment &&
    m_uiTransformBufferAlignment == rhs.m_uiTransformBufferAlignment &&
    m_uiBoxBufferAlignment == rhs.m_uiBoxBufferAlignment &&
    m_uiScratchBufferAlignment == rhs.m_uiScratchBufferAlignment &&
    m_uiInstanceBufferAlignment == rhs.m_uiInstanceBufferAlignment &&
    m_CapabilityFlags == rhs.m_CapabilityFlags &&
    m_uiShaderGroupHandleSize == rhs.m_uiShaderGroupHandleSize &&
    m_uiMaxShaderRecordStride == rhs.m_uiMaxShaderRecordStride &&
    m_uiShaderGroupBaseAlignment == rhs.m_uiShaderGroupBaseAlignment;
}

XII_ALWAYS_INLINE constexpr bool xiiGALMeshShaderProperties::operator==(const xiiGALMeshShaderProperties& rhs) const
{
  return m_uiMaxThreadGroupCountX == rhs.m_uiMaxThreadGroupCountX && m_uiMaxThreadGroupCountY == rhs.m_uiMaxThreadGroupCountY && m_uiMaxThreadGroupCountZ == rhs.m_uiMaxThreadGroupCountZ && m_uiMaxThreadGroupTotalCount == rhs.m_uiMaxThreadGroupTotalCount;
}

XII_ALWAYS_INLINE constexpr bool xiiGALComputeShaderProperties::operator==(const xiiGALComputeShaderProperties& rhs) const
{
  return m_uiSharedMemorySize == rhs.m_uiSharedMemorySize &&
    m_uiMaxThreadGroupInvocations == rhs.m_uiMaxThreadGroupInvocations &&
    m_uiMaxThreadGroupSizeX == rhs.m_uiMaxThreadGroupSizeX &&
    m_uiMaxThreadGroupSizeY == rhs.m_uiMaxThreadGroupSizeY &&
    m_uiMaxThreadGroupSizeZ == rhs.m_uiMaxThreadGroupSizeZ &&
    m_uiMaxThreadGroupCountX == rhs.m_uiMaxThreadGroupCountX &&
    m_uiMaxThreadGroupCountY == rhs.m_uiMaxThreadGroupCountY &&
    m_uiMaxThreadGroupCountZ == rhs.m_uiMaxThreadGroupCountZ;
}

XII_ALWAYS_INLINE constexpr bool xiiGALNormalizedDeviceCoordinates::operator==(const xiiGALNormalizedDeviceCoordinates& rhs) const
{
  return m_fMinZ == rhs.m_fMinZ && m_fZToDepthScale == rhs.m_fZToDepthScale && m_fYToVScale == rhs.m_fYToVScale;
}

XII_ALWAYS_INLINE constexpr float xiiGALNormalizedDeviceCoordinates::GetZtoDepthBias() const
{
  return -m_fMinZ * m_fZToDepthScale;
}

XII_ALWAYS_INLINE constexpr bool xiiGALDeviceCreationDescription::operator==(const xiiGALDeviceCreationDescription& rhs) const
{
  return m_GraphicsDeviceType == rhs.m_GraphicsDeviceType &&
    m_AdapterType == rhs.m_AdapterType &&
    m_ValidationLevel == rhs.m_ValidationLevel &&
    m_uiAdapterID == rhs.m_uiAdapterID &&
    m_DeviceFeatures == rhs.m_DeviceFeatures &&
    m_DeviceNormalizedCoordinates == rhs.m_DeviceNormalizedCoordinates;
}

XII_ALWAYS_INLINE constexpr bool xiiGALDeviceMemoryProperties::operator==(const xiiGALDeviceMemoryProperties& rhs) const
{
  return m_uiLocalMemory == rhs.m_uiLocalMemory &&
    m_uiHostVisibleMemory == rhs.m_uiHostVisibleMemory &&
    m_uiUnifiedMemory == rhs.m_uiUnifiedMemory &&
    m_uiMaxMemoryAllocation == rhs.m_uiMaxMemoryAllocation &&
    m_UnifiedMemoryCPUAccessFlags == rhs.m_UnifiedMemoryCPUAccessFlags &&
    m_MemorylessTextureBindFlags == rhs.m_MemorylessTextureBindFlags;
}

XII_ALWAYS_INLINE constexpr bool xiiGALShadingRateMode::operator==(const xiiGALShadingRateMode& rhs) const
{
  return m_ShadingRate == rhs.m_ShadingRate && m_SampleBits == rhs.m_SampleBits;
}

XII_ALWAYS_INLINE bool xiiGALShadingRateProperties::operator==(const xiiGALShadingRateProperties& rhs) const
{
  if (m_Modes.GetCount() != rhs.m_Modes.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < m_Modes.GetCount(); ++i)
  {
    if (m_Modes[i] != rhs.m_Modes[i])
      return false;
  }

  return m_CapabilityFlags == rhs.m_CapabilityFlags &&
    m_CombinerFlags == rhs.m_CombinerFlags &&
    m_Format == rhs.m_Format &&
    m_TextureAccess == rhs.m_TextureAccess &&
    m_BindFlags == rhs.m_BindFlags &&
    m_MinTileSize == rhs.m_MinTileSize &&
    m_MaxTileSize == rhs.m_MaxTileSize &&
    m_uiMaxSubSampledArraySlices == rhs.m_uiMaxSubSampledArraySlices;
}

XII_ALWAYS_INLINE constexpr bool xiiGALDrawCommandProperties::operator==(const xiiGALDrawCommandProperties& rhs) const
{
  return m_CapabilityFlags == rhs.m_CapabilityFlags && m_uiMaxIndexValue == rhs.m_uiMaxIndexValue && m_uiMaxDrawIndirectCount == rhs.m_uiMaxDrawIndirectCount;
}

XII_ALWAYS_INLINE constexpr bool xiiGALSparseResourceProperties::operator==(const xiiGALSparseResourceProperties& rhs) const
{
  return m_uiAddressSpaceSize == rhs.m_uiAddressSpaceSize &&
    m_uiResourceSpaceSize == rhs.m_uiResourceSpaceSize &&
    m_CapabilityFlags == rhs.m_CapabilityFlags &&
    m_uiStandardBlockSize == rhs.m_uiStandardBlockSize &&
    m_BindFlags == rhs.m_BindFlags;
}

XII_ALWAYS_INLINE constexpr bool xiiGALCommandQueueProperties::operator==(const xiiGALCommandQueueProperties& rhs) const
{
  return m_Type == rhs.m_Type && m_uiMaxDeviceContexts == rhs.m_uiMaxDeviceContexts && m_TextureCopyGranularity[0] == rhs.m_TextureCopyGranularity[0] && m_TextureCopyGranularity[1] == rhs.m_TextureCopyGranularity[1] && m_TextureCopyGranularity[2] == rhs.m_TextureCopyGranularity[2];
}

XII_ALWAYS_INLINE bool xiiGALGraphicsDeviceAdapterDescription::operator==(const xiiGALGraphicsDeviceAdapterDescription& rhs) const
{
  if (m_CommandQueueProperties.GetCount() != rhs.m_CommandQueueProperties.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < m_CommandQueueProperties.GetCount(); ++i)
  {
    if (m_CommandQueueProperties[i] != rhs.m_CommandQueueProperties[i])
      return false;
  }

  return m_sAdapterName == rhs.m_sAdapterName &&
    m_Type == rhs.m_Type &&
    m_Vendor == rhs.m_Vendor &&
    m_uiVendorID == rhs.m_uiVendorID &&
    m_uiDeviceID == rhs.m_uiDeviceID &&
    m_uiVideoOutputCount == rhs.m_uiVideoOutputCount &&
    m_MemoryProperties == rhs.m_MemoryProperties &&
    m_RayTracingProperties == rhs.m_RayTracingProperties &&
    m_WaveOperationProperties == rhs.m_WaveOperationProperties &&
    m_BufferProperties == rhs.m_BufferProperties &&
    m_TextureProperties == rhs.m_TextureProperties &&
    m_SamplerProperties == rhs.m_SamplerProperties &&
    m_MeshShaderProperties == rhs.m_MeshShaderProperties &&
    m_ShadingRateProperties == rhs.m_ShadingRateProperties &&
    m_ComputeShaderProperties == rhs.m_ComputeShaderProperties &&
    m_DrawCommandProperties == rhs.m_DrawCommandProperties &&
    m_SparseResourceProperties == rhs.m_SparseResourceProperties &&
    m_Features == rhs.m_Features;
}

XII_ALWAYS_INLINE constexpr bool xiiGALDeviceEvent::operator==(const xiiGALDeviceEvent& rhs) const
{
  return m_pDevice == rhs.m_pDevice && m_Type == rhs.m_Type;
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALTextureFormatDescription::GetElementSize() const
{
  return m_uiComponentSize * (m_ComponentType != xiiGALTextureFormatComponentType::Compressed ? m_uiComponentCount : 1);
}

XII_ALWAYS_INLINE constexpr bool xiiGALTextureFormatDescription::operator==(const xiiGALTextureFormatDescription& rhs) const
{
  return m_Format == rhs.m_Format &&
    m_uiComponentSize == rhs.m_uiComponentSize &&
    m_uiComponentCount == rhs.m_uiComponentCount &&
    m_ComponentType == rhs.m_ComponentType &&
    m_bIsTypeless == rhs.m_bIsTypeless &&
    m_uiBlockWidth == rhs.m_uiBlockWidth &&
    m_uiBlockHeight == rhs.m_uiBlockHeight;
}
