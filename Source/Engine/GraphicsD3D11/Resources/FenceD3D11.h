#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/Fence.h>

struct ID3D11Fence;

class XII_GRAPHICSD3D11_DLL xiiGALFenceD3D11 final : public xiiGALFence
{
public:
  virtual xiiUInt64 GetCompletedValue() override final;

  virtual void Signal(xiiUInt64 uiValue) override final;

  virtual void Wait(xiiUInt64 uiValue) override final;

  void AddPendingQuery(ID3D11DeviceContext* pContext, ID3D11Query* pQuery, xiiUInt64 uiValue);

  void Wait(xiiUInt64 uiValue, bool bFlushCommands);

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALFenceD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALFenceCreationDescription& creationDescription);

  virtual ~xiiGALFenceD3D11();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  struct PendingFenceData
  {
    PendingFenceData(ID3D11DeviceContext* pContext, ID3D11Query* pQuery, xiiUInt64 uiValue) :
      m_pContextD3D11(pContext), m_pQueryD3D11(pQuery), m_uiValue(uiValue)
    {
    }

    ID3D11DeviceContext* m_pContextD3D11 = nullptr;
    ID3D11Query*         m_pQueryD3D11   = nullptr;
    const xiiUInt64      m_uiValue;
  };

  xiiDeque<PendingFenceData> m_PendingQueries;

  xiiUInt32 m_uiMaxPendingQueries = 0;
};

#include <GraphicsD3D11/Resources/Implementation/FenceD3D11_inl.h>
