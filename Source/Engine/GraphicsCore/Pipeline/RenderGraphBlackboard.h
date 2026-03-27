#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/Variant.h>

/// \brief Thread-safe per-frame key-value store for inter-pass data exchange in the render graph.
///
/// Passes and external systems write typed values during graph setup or before Execute(). During
/// Execute(), all parallel pass threads may safely read concurrently via TryGet() and Contains().
///
/// Threading contract:
///   - Writes (Set, Remove, Clear) are NOT thread-safe and must complete before Execute().
///   - Reads (TryGet, Contains) ARE thread-safe and may be called concurrently from multiple threads.
///
/// \note Values are stored as xiiVariant. Type mismatches on TryGet return false without asserting.
class XII_GRAPHICSCORE_DLL xiiRenderGraphBlackboard
{
public:
  /// \brief Stores a typed value under the given key. Replaces any previously stored value.
  ///        Not thread-safe — must be called before Execute().
  template <typename T>
  XII_ALWAYS_INLINE void Set(xiiHashedString sKey, const T& value)
  {
    m_Entries.Insert(sKey, xiiVariant(value));
  }

  /// \brief Stores a typed value by move. Not thread-safe — must be called before Execute().
  template <typename T>
  XII_ALWAYS_INLINE void Set(xiiHashedString sKey, T&& value)
  {
    m_Entries.Insert(sKey, xiiVariant(std::forward<T>(value)));
  }

  /// \brief Attempts to retrieve and cast a value to type T.
  ///        Thread-safe — safe to call from parallel execute threads.
  ///
  /// \returns True if the key exists and the stored value is of type T, false otherwise.
  template <typename T>
  [[nodiscard]] bool TryGet(xiiHashedString sKey, T& out_value) const
  {
    XII_LOCK(m_ReadMutex);

    xiiVariant value;
    if (!m_Entries.TryGetValue(sKey, value))
      return false;

    if (!value.IsA<T>())
      return false;

    out_value = value.Get<T>();
    return true;
  }

  /// \brief Returns a typed reference to the stored value.
  ///        Thread-safe — safe to call from parallel execute threads.
  ///
  /// \note Asserts if the key does not exist or the type does not match.
  template <typename T>
  [[nodiscard]] const T& GetRef(xiiHashedString sKey) const
  {
    XII_LOCK(m_ReadMutex);

    xiiVariant value;
    XII_VERIFY(m_Entries.TryGetValue(sKey, value), "Blackboard key '{}' does not exist.", sKey.GetView());
    XII_ASSERT_DEV(value.IsA<T>(), "Blackboard key '{}' exists but type does not match.", sKey.GetView());
    return value.Get<T>();
  }

  /// \brief Returns true if the given key is present.
  ///        Thread-safe — safe to call from parallel execute threads.
  [[nodiscard]] bool Contains(xiiHashedString sKey) const
  {
    XII_LOCK(m_ReadMutex);
    return m_Entries.Contains(sKey);
  }

  /// \brief Removes the entry for the given key. Not thread-safe.
  void Remove(xiiHashedString sKey)
  {
    m_Entries.Remove(sKey);
  }

  /// \brief Clears all entries. Not thread-safe.
  void Clear()
  {
    m_Entries.Clear();
  }

private:
  xiiHashTable<xiiHashedString, xiiVariant> m_Entries;
  mutable xiiMutex                          m_ReadMutex;
};
