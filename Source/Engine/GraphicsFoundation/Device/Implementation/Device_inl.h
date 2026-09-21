/// Copyright (c) Theophilus Eriata. All Rights Reserved.

/// Used to guard xiiGALDevice functions from multi-threaded access and to verify that executing them on non-main-threads is allowed.
#define XII_GAL_DEVICE_LOCK_AND_CHECK() \
  XII_LOCK(m_Mutex);                    \
  VerifyMultithreadedAccess()

XII_ALWAYS_INLINE xiiMutex& xiiGALDevice::GetMutex() const
{
  return m_Mutex;
}

XII_ALWAYS_INLINE const xiiGALDeviceCreationDescription& xiiGALDevice::GetDescription() const
{
  return m_Description;
}

XII_ALWAYS_INLINE const xiiGALGraphicsDeviceAdapterDescription& xiiGALDevice::GetGraphicsDeviceAdapterProperties() const
{
  return m_AdapterDescription;
}

XII_ALWAYS_INLINE const xiiGALDeviceFeatures& xiiGALDevice::GetFeatures() const
{
  return m_AdapterDescription.m_Features;
}

XII_ALWAYS_INLINE const xiiGALDeviceLimits& xiiGALDevice::GetLimits() const
{
  return m_AdapterDescription.m_DeviceLimits;
}

XII_ALWAYS_INLINE xiiEnum<xiiGALGraphicsDeviceType> xiiGALDevice::GetGraphicsDeviceType() const
{
  return m_Description.m_GraphicsDeviceType;
}

// static
XII_ALWAYS_INLINE void xiiGALDevice::SetDefaultDevice(xiiSharedPtr<xiiGALDevice> pDefaultDevice)
{
  s_pDefaultDevice = pDefaultDevice;
}

// static
XII_ALWAYS_INLINE xiiSharedPtr<xiiGALDevice> xiiGALDevice::GetDefaultDevice()
{
  XII_ASSERT_DEBUG(s_pDefaultDevice != nullptr, "Default device not set.");

  return s_pDefaultDevice;
}

// static
XII_ALWAYS_INLINE bool xiiGALDevice::HasDefaultDevice()
{
  return s_pDefaultDevice != nullptr;
}

XII_ALWAYS_INLINE void xiiGALDevice::VerifyMultithreadedAccess() const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_AdapterDescription.m_Features.m_MultithreadedResourceCreation == xiiGALDeviceFeatureState::Enabled || xiiThreadUtils::IsMainThread(), "This device does not support multi-threaded resource creation, therefore this function can only be executed on the main thread.");
#endif
}
