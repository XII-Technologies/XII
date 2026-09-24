/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/Variant.h>

/// Thread-safe per-frame key-value store for inter-pass data exchange in the render graph.
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
  /// Stores a typed value under the given key. Replaces any previously stored value. Not thread-safe, must be called before Execute().
  template <typename T>
  void Set(xiiStringView sKey, const T& value);

  /// Stores a typed value by move. Not thread-safe, must be called before Execute().
  template <typename T>
  void Set(xiiStringView sKey, T&& value);

  /// Stores data that survives ClearFrame() and is shared by all frames of this graph.
  template <typename T>
  void SetGraph(xiiStringView sKey, const T& value);

  /// Attempts to retrieve and cast a value to type T. Thread-safe, safe to call from parallel execute threads.
  ///
  /// \returns True if the key exists and the stored value is of type T, false otherwise.
  template <typename T>
  [[nodiscard]] bool TryGet(xiiStringView sKey, T& out_value) const;

  /// Looks in frame scope first, then graph scope.
  template <typename T>
  [[nodiscard]] bool TryGetScoped(xiiStringView sKey, T& out_value) const;

  /// Returns a typed reference to the stored value. Thread-safe, safe to call from parallel execute threads.
  template <typename T>
  [[nodiscard]] const T& GetRef(xiiStringView sKey) const;

  /// Returns true if the given key is present. Thread-safe, safe to call from parallel execute threads.
  [[nodiscard]] bool Contains(xiiStringView sKey) const;

  /// Removes the entry for the given key. Not thread-safe.
  void Remove(xiiStringView sKey);

  /// Clears all entries. Not thread-safe.
  void Clear();

  /// Clears only per-frame entries. Graph-scoped entries remain available.
  void ClearFrame();

  /// Clears only graph-scoped entries.
  void ClearGraph();

private:
  xiiHashTable<xiiHashedString, xiiVariant> m_FrameEntries;
  xiiHashTable<xiiHashedString, xiiVariant> m_GraphEntries;
  mutable xiiMutex                          m_ReadMutex;
};

#include <GraphicsCore/Pipeline/Implementation/RenderGraphBlackboard_inl.h>
