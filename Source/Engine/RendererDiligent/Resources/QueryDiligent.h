#pragma once

#include <RendererFoundation/Resources/Query.h>

class xiiGALQueryDiligent : public xiiGALQuery
{
public:
  XII_ALWAYS_INLINE Diligent::IQuery* GetQuery();

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALQueryDiligent(const xiiGALQueryCreationDescription& Description);
  ~xiiGALQueryDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::IQuery* m_pQuery = nullptr;
};

#include <RendererDiligent/Resources/Implementation/QueryDiligent_inl.h>
