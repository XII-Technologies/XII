#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class xiiRenderDataBatch
{
private:
  struct SortableRenderData
  {
    XII_DECLARE_POD_TYPE();

    const xiiRenderData* m_pRenderData;
    xiiUInt64            m_uiSortingKey;
  };

public:
  XII_DECLARE_POD_TYPE();

  /// \brief This function should return true if the given render data should be filtered and not rendered.
  typedef xiiDelegate<bool(const xiiRenderData*)> Filter;

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

    Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd, Filter filter);

    Filter                    m_Filter;
    const SortableRenderData* m_pCurrent;
    const SortableRenderData* m_pEnd;
  };

  xiiUInt32 GetCount() const;

  template <typename T>
  const T* GetFirstData() const;

  template <typename T>
  Iterator<T> GetIterator(xiiUInt32 uiStartIndex = 0, xiiUInt32 uiCount = xiiInvalidIndex) const;

private:
  friend class xiiExtractedRenderData;
  friend class xiiRenderDataBatchList;

  Filter                          m_Filter;
  xiiArrayPtr<SortableRenderData> m_Data;
};

class xiiRenderDataBatchList
{
public:
  xiiUInt32 GetBatchCount() const;

  xiiRenderDataBatch GetBatch(xiiUInt32 uiIndex) const;

private:
  friend class xiiExtractedRenderData;

  xiiRenderDataBatch::Filter            m_Filter;
  xiiArrayPtr<const xiiRenderDataBatch> m_Batches;
};

#include <RendererCore/Pipeline/Implementation/RenderDataBatch_inl.h>
