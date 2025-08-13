#pragma once

#include <GraphicsCore/Pipeline/Declarations.h>

class xiiRenderDataBatch
{
private:
  struct SortableRenderData
  {
    XII_DECLARE_POD_TYPE();

    const xiiRenderData* m_pRenderData = nullptr;
    xiiUInt64            m_uiSortingKey;
  };

public:
  XII_DECLARE_POD_TYPE();

  template <typename T>
  class Iterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;

    void Next();

    bool IsValid() const;

    void operator++();

  private:
    friend class xiiRenderDataBatch;

    Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd);

    const SortableRenderData* m_pCurrent = nullptr;
    const SortableRenderData* m_pEnd     = nullptr;
  };

  xiiUInt32 GetCount() const;

  template <typename T>
  const T* GetFirstData() const;

  template <typename T>
  Iterator<T> GetIterator(xiiUInt32 uiStartIndex = 0U, xiiUInt32 uiCount = xiiInvalidIndex) const;

private:
  friend class xiiExtractedRenderData;
  friend class xiiRenderDataBatchList;

  xiiArrayPtr<SortableRenderData> m_Data;
};

class XII_GRAPHICSCORE_DLL xiiRenderDataBatchList
{
public:
  xiiUInt32 GetBatchCount() const;

  const xiiRenderDataBatch& GetBatch(xiiUInt32 uiIndex) const;

private:
  friend class xiiExtractedRenderData;

  xiiArrayPtr<const xiiRenderDataBatch> m_Batches;
};

#include <GraphicsCore/Pipeline/Implementation/RenderDataBatch_inl.h>
