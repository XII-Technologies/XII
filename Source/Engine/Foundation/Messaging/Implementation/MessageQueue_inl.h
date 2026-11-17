/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename MetaDataType>
xiiMessageQueueBase<MetaDataType>::xiiMessageQueueBase(xiiAllocator* pAllocator) :
  m_Queue(pAllocator)
{
}

template <typename MetaDataType>
xiiMessageQueueBase<MetaDataType>::xiiMessageQueueBase(const xiiMessageQueueBase& rhs, xiiAllocator* pAllocator) :
  m_Queue(pAllocator)
{
  m_Queue = rhs.m_Queue;
}

template <typename MetaDataType>
xiiMessageQueueBase<MetaDataType>::~xiiMessageQueueBase()
{
  Clear();
}

template <typename MetaDataType>
void xiiMessageQueueBase<MetaDataType>::operator=(const xiiMessageQueueBase& rhs)
{
  m_Queue = rhs.m_Queue;
}

template <typename MetaDataType>
XII_ALWAYS_INLINE typename xiiMessageQueueBase<MetaDataType>::Entry& xiiMessageQueueBase<MetaDataType>::operator[](xiiUInt32 uiIndex)
{
  return m_Queue[uiIndex];
}

template <typename MetaDataType>
XII_ALWAYS_INLINE const typename xiiMessageQueueBase<MetaDataType>::Entry& xiiMessageQueueBase<MetaDataType>::operator[](xiiUInt32 uiIndex) const
{
  return m_Queue[uiIndex];
}

template <typename MetaDataType>
XII_ALWAYS_INLINE xiiUInt32 xiiMessageQueueBase<MetaDataType>::GetCount() const
{
  return m_Queue.GetCount();
}

template <typename MetaDataType>
XII_ALWAYS_INLINE bool xiiMessageQueueBase<MetaDataType>::IsEmpty() const
{
  return m_Queue.IsEmpty();
}

template <typename MetaDataType>
void xiiMessageQueueBase<MetaDataType>::Clear()
{
  m_Queue.Clear();
}

template <typename MetaDataType>
XII_ALWAYS_INLINE void xiiMessageQueueBase<MetaDataType>::Reserve(xiiUInt32 uiCount)
{
  m_Queue.Reserve(uiCount);
}

template <typename MetaDataType>
XII_ALWAYS_INLINE void xiiMessageQueueBase<MetaDataType>::Compact()
{
  m_Queue.Compact();
}

template <typename MetaDataType>
void xiiMessageQueueBase<MetaDataType>::Enqueue(xiiMessage* pMessage, const MetaDataType& metaData)
{
  Entry entry;
  entry.m_pMessage = pMessage;
  entry.m_MetaData = metaData;

  {
    XII_LOCK(m_Mutex);

    m_Queue.PushBack(entry);
  }
}

template <typename MetaDataType>
bool xiiMessageQueueBase<MetaDataType>::TryDequeue(xiiMessage*& out_pMessage, MetaDataType& out_metaData)
{
  XII_LOCK(m_Mutex);

  if (!m_Queue.IsEmpty())
  {
    Entry& entry = m_Queue.PeekFront();
    out_pMessage = entry.m_pMessage;
    out_metaData = entry.m_MetaData;

    m_Queue.PopFront();
    return true;
  }

  return false;
}

template <typename MetaDataType>
bool xiiMessageQueueBase<MetaDataType>::TryPeek(xiiMessage*& out_pMessage, MetaDataType& out_metaData)
{
  XII_LOCK(m_Mutex);

  if (!m_Queue.IsEmpty())
  {
    Entry& entry = m_Queue.PeekFront();
    out_pMessage = entry.m_pMessage;
    out_metaData = entry.m_MetaData;

    return true;
  }

  return false;
}

template <typename MetaDataType>
XII_ALWAYS_INLINE typename xiiMessageQueueBase<MetaDataType>::Entry& xiiMessageQueueBase<MetaDataType>::Peek()
{
  return m_Queue.PeekFront();
}

template <typename MetaDataType>
XII_ALWAYS_INLINE void xiiMessageQueueBase<MetaDataType>::Dequeue()
{
  m_Queue.PopFront();
}

template <typename MetaDataType>
template <typename Comparer>
XII_ALWAYS_INLINE void xiiMessageQueueBase<MetaDataType>::Sort(const Comparer& comparer)
{
  m_Queue.Sort(comparer);
}

template <typename MetaDataType>
void xiiMessageQueueBase<MetaDataType>::Lock()
{
  m_Mutex.Lock();
}

template <typename MetaDataType>
void xiiMessageQueueBase<MetaDataType>::Unlock()
{
  m_Mutex.Unlock();
}


template <typename MD, typename A>
xiiMessageQueue<MD, A>::xiiMessageQueue() :
  xiiMessageQueueBase<MD>(A::GetAllocator())
{
}

template <typename MD, typename A>
xiiMessageQueue<MD, A>::xiiMessageQueue(xiiAllocator* pQueueAllocator) :
  xiiMessageQueueBase<MD>(pQueueAllocator)
{
}

template <typename MD, typename A>
xiiMessageQueue<MD, A>::xiiMessageQueue(const xiiMessageQueue<MD, A>& rhs) :
  xiiMessageQueueBase<MD>(rhs, A::GetAllocator())
{
}

template <typename MD, typename A>
xiiMessageQueue<MD, A>::xiiMessageQueue(const xiiMessageQueueBase<MD>& rhs) :
  xiiMessageQueueBase<MD>(rhs, A::GetAllocator())
{
}

template <typename MD, typename A>
void xiiMessageQueue<MD, A>::operator=(const xiiMessageQueue<MD, A>& rhs)
{
  xiiMessageQueueBase<MD>::operator=(rhs);
}

template <typename MD, typename A>
void xiiMessageQueue<MD, A>::operator=(const xiiMessageQueueBase<MD>& rhs)
{
  xiiMessageQueueBase<MD>::operator=(rhs);
}
