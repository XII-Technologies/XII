#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/FenceD3D11.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALFenceD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALFenceD3D11::xiiGALFenceD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(pDeviceD3D11, creationDescription)
{
}

xiiGALFenceD3D11::~xiiGALFenceD3D11() = default;

xiiResult xiiGALFenceD3D11::InitPlatform()
{
  if (m_Description.m_Type != xiiGALFenceType::CpuWaitOnly)
  {
    xiiLog::Error("Only xiiGALFenceType::CpuWaitOnly is supported in Direct3D11.");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGALFenceD3D11::DeInitPlatform()
{
  if (m_uiMaxPendingQueries < 10)
  {
    xiiLog::Info("Max pending queries: {}.", m_uiMaxPendingQueries);
  }
  else
  {
    xiiLog::Info("Max pending queries ({}) is large. This may indicate that none of GetCompletedValue() or Wait() have been called.", m_uiMaxPendingQueries);
  }
  return XII_SUCCESS;
}

xiiUInt64 xiiGALFenceD3D11::GetCompletedValue()
{
  while (!m_PendingQueries.IsEmpty())
  {
    auto& queryData = m_PendingQueries.PeekFront();

    BOOL bData;
    if (queryData.m_pContextD3D11->GetData(queryData.m_pQueryD3D11, &bData, sizeof(bData), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
    {
      XII_ASSERT_DEV(bData, "");

      UpdateLastCompletedFenceValue(queryData.m_uiValue);

      XII_GAL_D3D11_RELEASE(queryData.m_pQueryD3D11);

      m_PendingQueries.PopFront();
    }
    else
    {
      break;
    }
  }
  return m_uiLastCompletedFenceValue;
}

void xiiGALFenceD3D11::Wait(xiiUInt64 uiValue, bool bFlushCommands)
{
  while (!m_PendingQueries.IsEmpty())
  {
    PendingFenceData& queryData = m_PendingQueries.PeekFront();

    if (queryData.m_uiValue > uiValue)
      break;

    BOOL bData;
    while (queryData.m_pContextD3D11->GetData(queryData.m_pQueryD3D11, &bData, sizeof(bData), bFlushCommands ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
    {
      xiiThreadUtils::Sleep(xiiTime::Microseconds(1));
    }

    XII_ASSERT_DEV(bData, "");

    UpdateLastCompletedFenceValue(queryData.m_uiValue);

    XII_GAL_D3D11_RELEASE(queryData.m_pQueryD3D11);

    m_PendingQueries.PopFront();
  }
}

void xiiGALFenceD3D11::SetDebugNamePlatform(xiiStringView sName)
{
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_FenceD3D11);
