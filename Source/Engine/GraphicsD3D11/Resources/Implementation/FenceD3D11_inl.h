
XII_ALWAYS_INLINE void xiiGALFenceD3D11::Signal(xiiUInt64 uiValue)
{
  XII_REPORT_FAILURE("Signal() is not supported on the Direct3D 11 device.");
}

XII_ALWAYS_INLINE void xiiGALFenceD3D11::Wait(xiiUInt64 uiValue)
{
  Wait(uiValue, true);
}

XII_ALWAYS_INLINE void xiiGALFenceD3D11::AddPendingQuery(ID3D11DeviceContext* pContext, ID3D11Query* pQuery, xiiUInt64 uiValue)
{
  m_PendingQueries.PushBack(PendingFenceData{pContext, pQuery, uiValue});

  m_uiMaxPendingQueries = xiiMath::Max(m_uiMaxPendingQueries, m_PendingQueries.GetCount());
}
