/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Query.h>

class xiiGALCommandListD3D12;
class xiiGALQueryPoolD3D12;

class XII_GRAPHICSD3D12_DLL xiiGALQueryD3D12 final : public xiiGALQuery
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALQueryD3D12, xiiGALQuery);

public:
  virtual bool GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate = true) override final;

  virtual void Invalidate() override final;

  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetQueryPoolIndex(xiiUInt32 uiQueryID) const
  {
    XII_ASSERT_DEV(uiQueryID == 0U || (m_Description.m_Type == xiiGALQueryType::Duration && uiQueryID == 1U), "");

    return m_QueryPoolIndices[uiQueryID];
  }

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueryPoolD3D12* GetQueryPoolD3D12() const { return m_pQueryPoolD3D12; }

  bool OnBeginQuery(xiiGALCommandListD3D12* pCommandListD3D12);
  bool OnEndQuery(xiiGALCommandListD3D12* pCommandListD3D12);

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALQueryD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALQueryCreationDescription& creationDescription);

  virtual ~xiiGALQueryD3D12();

  virtual xiiResult InitPlatform() override final;

private:
  bool AllocateQueries();
  void DiscardQueries();

  [[nodiscard]] bool ReadbackQueryData(xiiUInt32 uiQueryID, void* pDestinationData, xiiUInt32 uiDataSize) const;

  xiiStaticArray<xiiUInt32, 2U>        m_QueryPoolIndices;
  xiiUInt64                            m_uiQueryEndFenceValue = xiiInvalidIndex;
  xiiBitflags<xiiGALCommandQueueFlags> m_QueryQueueFlags      = xiiGALCommandQueueFlags::None;

  xiiGALQueryPoolD3D12* m_pQueryPoolD3D12 = nullptr;
};
