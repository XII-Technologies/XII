/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Query.h>

class XII_GRAPHICSVULKAN_DLL xiiGALQueryVulkan final : public xiiGALQuery
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALQueryVulkan, xiiGALQuery);

public:
  virtual bool GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate = true) override final;

  virtual void Invalidate() override final;

  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetQueryPoolIndex(xiiUInt32 uiQueryID) const
  {
    XII_ASSERT_DEV(uiQueryID == 0 || (m_Description.m_Type == xiiGALQueryType::Duration && uiQueryID == 1), "");

    return m_QueryPoolIndex[uiQueryID];
  }

  bool OnBeginQuery(xiiGALCommandListVulkan* pCommandListVulkan);
  bool OnEndQuery(xiiGALCommandListVulkan* pCommandListVulkan);

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALQueryVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALQueryCreationDescription& creationDescription);

  virtual ~xiiGALQueryVulkan();

  virtual xiiResult InitPlatform() override final;

private:
  bool AllocateQueries();
  void DiscardQueries();

  xiiStaticArray<xiiUInt32, 2U> m_QueryPoolIndex;

  xiiUInt64 m_uiQueryEndFenceValue = xiiInvalidIndex;

  xiiGALQueryPoolVulkan* m_pQueryPoolVulkan = nullptr;
};
