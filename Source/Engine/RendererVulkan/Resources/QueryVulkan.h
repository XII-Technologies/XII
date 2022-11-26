#pragma once

#include <RendererFoundation/Resources/Query.h>

class xiiGALQueryVulkan : public xiiGALQuery
{
public:
  XII_ALWAYS_INLINE xiiUInt32 GetID() const;
  XII_ALWAYS_INLINE vk::QueryPool GetPool() const { return nullptr; } // TODO

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALQueryVulkan(const xiiGALQueryCreationDescription& Description);
  ~xiiGALQueryVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  xiiUInt32 m_uiID;
};

#include <RendererVulkan/Resources/Implementation/QueryVulkan_inl.h>
