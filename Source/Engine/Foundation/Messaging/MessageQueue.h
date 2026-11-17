/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Messaging/Message.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// A FIFO of borrowed message pointers and metadata, with optional sorting.
///
/// The caller owns message storage. Copying the queue copies entries and their pointers.
/// Erasing entries or destroying the queue releases only queue storage.
///
/// Enqueue(), TryDequeue() and TryPeek() acquire the queue mutex. Use xiiLock for compound operations
/// and other access to a shared queue. Hold it for as long as a borrowed entry reference is needed.
/// Concurrent access to the messages themselves requires separate synchronization.
/// \see xiiMessage
template <typename MetaDataType>
class xiiMessageQueueBase
{
public:
  struct Entry
  {
    XII_DECLARE_POD_TYPE();

    xiiMessage*       m_pMessage;
    MetaDataType      m_MetaData;
    mutable xiiUInt64 m_uiMessageHash = 0;
  };

protected:
  /// No memory is allocated during construction.
  xiiMessageQueueBase(xiiAllocator* pAllocator); // [tested]

  /// No memory is allocated during construction.
  xiiMessageQueueBase(const xiiMessageQueueBase& rhs, xiiAllocator* pAllocator);

  /// Destructor.
  ~xiiMessageQueueBase(); // [tested]

  /// Assignment operator.
  void operator=(const xiiMessageQueueBase& rhs);

public:
  /// Returns the element at the given index. Not thread safe.
  Entry& operator[](xiiUInt32 uiIndex); // [tested]

  /// Returns the element at the given index. Not thread safe.
  const Entry& operator[](xiiUInt32 uiIndex) const; // [tested]

  /// Returns the number of active elements in the queue.
  xiiUInt32 GetCount() const;

  /// Returns true, if the queue does not contain any elements.
  bool IsEmpty() const;

  /// Removes all entries and retains the queue capacity.
  void Clear();

  /// Expands the queue so it can at least store the given capacity.
  void Reserve(xiiUInt32 uiCount);

  /// Tries to compact the array to avoid wasting memory.The resulting capacity is at least 'GetCount' (no elements get removed).
  void Compact();

  /// Enqueues the given message and meta-data. This method is thread safe.
  void Enqueue(xiiMessage* pMessage, const MetaDataType& metaData); // [tested]

  /// Dequeues the first element if the queue is not empty and returns true. Returns false if the queue is empty. This method is thread safe.
  bool TryDequeue(xiiMessage*& out_pMessage, MetaDataType& out_metaData); // [tested]

  /// Copies the front pointer and metadata if present. The caller must ensure that the pointed-to
  /// message stays alive after this method releases the queue lock.
  bool TryPeek(xiiMessage*& out_pMessage, MetaDataType& out_metaData); // [tested]

  /// Returns the first element in the queue. Not thread safe.
  Entry& Peek();

  /// Removes the first element from the queue. Not thread safe.
  void Dequeue();

  /// Sort with explicit comparer. Not thread safe.
  template <typename Comparer>
  void Sort(const Comparer& comparer); // [tested]

  /// Acquires an exclusive lock on the queue. Do not use this method directly but use xiiLock instead.
  void Lock(); // [tested]

  /// Releases a lock that has been previously acquired. Do not use this method directly but use xiiLock instead.
  void Unlock(); // [tested]

private:
  xiiDeque<Entry, xiiNullAllocatorWrapper> m_Queue;
  xiiMutex                                 m_Mutex;
};

/// \see xiiMessageQueueBase
template <typename MetaDataType, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
class xiiMessageQueue : public xiiMessageQueueBase<MetaDataType>
{
public:
  xiiMessageQueue();
  xiiMessageQueue(xiiAllocator* pAllocator);

  xiiMessageQueue(const xiiMessageQueue<MetaDataType, AllocatorWrapper>& rhs);
  xiiMessageQueue(const xiiMessageQueueBase<MetaDataType>& rhs);

  void operator=(const xiiMessageQueue<MetaDataType, AllocatorWrapper>& rhs);
  void operator=(const xiiMessageQueueBase<MetaDataType>& rhs);
};

#include <Foundation/Messaging/Implementation/MessageQueue_inl.h>
