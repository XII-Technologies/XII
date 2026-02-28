#pragma once

#include <GraphicsCore/Pipeline/Declarations.h>

/// \brief Represents a batch of render data that can be rendered together.
///
/// Render data is grouped into batches to minimize state changes during rendering.
/// Each batch contains render data of the same type, sorted by a sorting key.
/// Provides iterator access to iterate through the typed render data.
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

  /// \brief Iterator for traversing typed render data within a batch.
  template <typename T>
  class Iterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;

    /// \brief Advances to the next element.
    void Next();

    /// \brief Returns true if the iterator points to a valid element.
    bool IsValid() const;

    void operator++();

  private:
    friend class xiiRenderDataBatch;

    Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd);

    const SortableRenderData* m_pCurrent;
    const SortableRenderData* m_pEnd;
  };

  xiiUInt32 GetDataCount() const;

  template <typename T>
  const T* GetFirstData() const;

  template <typename T>
  Iterator<T> GetIterator(xiiUInt32 uiStartIndex = 0, xiiUInt32 uiCount = xiiInvalidIndex) const;

  xiiSharedPtr<xiiGALBuffer> GetDataOffsetsBuffer() const;
  xiiUInt32                  GetFirstDataOffsetIndex() const;
  xiiUInt32                  GetInstanceCount() const;

private:
  friend class xiiExtractedRenderData;
  friend class xiiRenderDataBatchList;

  xiiArrayPtr<SortableRenderData> m_Data;

  xiiSharedPtr<xiiGALBuffer> m_pDataOffsetsBuffer;
  xiiUInt32                  m_uiFirstDataOffsetIndex = 0;
  xiiUInt32                  m_uiInstanceCount        = 0;
};

/// \brief Contains a list of render data batches for a specific render category.
///
/// Used to access all batches that need to be rendered for a particular category.
class xiiRenderDataBatchList
{
public:
  xiiUInt32 GetBatchCount() const;

  const xiiRenderDataBatch& GetBatch(xiiUInt32 uiIndex) const;

private:
  friend class xiiExtractedRenderData;

  xiiArrayPtr<const xiiRenderDataBatch> m_Batches;
};

#include <GraphicsCore/Pipeline/Implementation/RenderDataBatch_inl.h>
