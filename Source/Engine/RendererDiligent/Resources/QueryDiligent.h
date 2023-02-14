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

  Diligent::RefCntAutoPtr<Diligent::IQuery> m_pQuery;
};

#include <RendererDiligent/Resources/Implementation/QueryDiligent_inl.h>
