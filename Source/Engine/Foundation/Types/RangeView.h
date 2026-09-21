/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/Delegate.h>

/// This class uses delegates to define a range of values that can be enumerated using a forward iterator.
///
/// Can be used to create a contiguous view to elements of a certain type without the need for them to actually
/// exist in the same space or format. Think of IEnumerable in c# using composition via xiiDelegate instead of derivation.
/// ValueType defines the value type we are iterating over and IteratorType is the internal key to identify an element.
/// An example that creates a RangeView of strings that are stored in a linear array of structs.
/// \code{.cpp}
/// auto range = xiiRangeView<const char*, xiiUInt32>(
///   [this]()-> xiiUInt32 { return 0; },
///   [this]()-> xiiUInt32 { return array.GetCount(); },
///   [this](xiiUInt32& it) { ++it; },
///   [this](const xiiUInt32& it)-> const char* { return array[it].m_String; });
///
/// for (const char* szValue : range)
/// {
/// }
/// \endcode
template <typename ValueType, typename IteratorType>
class xiiRangeView
{
public:
  using BeginCallback = xiiDelegate<IteratorType()>;
  using EndCallback   = xiiDelegate<IteratorType()>;
  using NextCallback  = xiiDelegate<void(IteratorType&)>;
  using ValueCallback = xiiDelegate<ValueType(const IteratorType&)>;

  /// Initializes the xiiRangeView with the delegates used to enumerate the range.
  XII_ALWAYS_INLINE xiiRangeView(BeginCallback begin, EndCallback end, NextCallback next, ValueCallback value);

  /// Const iterator, don't use directly, use ranged based for loops or call begin() end().
  struct ConstIterator
  {
    XII_DECLARE_POD_TYPE();

    using iterator_category = std::forward_iterator_tag;
    using value_type        = ConstIterator;
    using pointer           = ConstIterator*;
    using reference         = ConstIterator&;

    XII_ALWAYS_INLINE           ConstIterator(const ConstIterator& rhs) = default;
    XII_FORCE_INLINE void       Next();
    XII_FORCE_INLINE ValueType  Value() const;
    XII_ALWAYS_INLINE ValueType operator*() const { return Value(); }
    XII_ALWAYS_INLINE void      operator++() { Next(); }
    XII_FORCE_INLINE bool       operator==(const typename xiiRangeView<ValueType, IteratorType>::ConstIterator& it2) const;

  protected:
    XII_FORCE_INLINE explicit ConstIterator(const xiiRangeView<ValueType, IteratorType>* view, IteratorType pos);

    friend class xiiRangeView<ValueType, IteratorType>;
    const xiiRangeView<ValueType, IteratorType>* m_pView = nullptr;
    IteratorType                                 m_Pos;
  };

  /// Iterator, don't use directly, use ranged based for loops or call begin() end().
  struct Iterator : public ConstIterator
  {
    XII_DECLARE_POD_TYPE();

    using iterator_category = std::forward_iterator_tag;
    using value_type        = Iterator;
    using pointer           = Iterator*;
    using reference         = Iterator&;

    using ConstIterator::Value;
    XII_ALWAYS_INLINE           Iterator(const Iterator& rhs) = default;
    XII_FORCE_INLINE ValueType  Value();
    XII_ALWAYS_INLINE ValueType operator*() { return Value(); }

  protected:
    XII_FORCE_INLINE explicit Iterator(const xiiRangeView<ValueType, IteratorType>* view, IteratorType pos);
  };

  Iterator      begin() { return Iterator(this, m_Begin()); }
  Iterator      end() { return Iterator(this, m_End()); }
  ConstIterator begin() const { return ConstIterator(this, m_Begin()); }
  ConstIterator end() const { return ConstIterator(this, m_End()); }
  ConstIterator cbegin() const { return ConstIterator(this, m_Begin()); }
  ConstIterator cend() const { return ConstIterator(this, m_End()); }

private:
  friend struct Iterator;
  friend struct ConstIterator;

  BeginCallback m_Begin;
  EndCallback   m_End;
  NextCallback  m_Next;
  ValueCallback m_Value;
};

template <typename V, typename I>
typename xiiRangeView<V, I>::Iterator begin(xiiRangeView<V, I>& ref_container)
{
  return ref_container.begin();
}

template <typename V, typename I>
typename xiiRangeView<V, I>::ConstIterator begin(const xiiRangeView<V, I>& container)
{
  return container.cbegin();
}

template <typename V, typename I>
typename xiiRangeView<V, I>::ConstIterator cbegin(const xiiRangeView<V, I>& container)
{
  return container.cbegin();
}

template <typename V, typename I>
typename xiiRangeView<V, I>::Iterator end(xiiRangeView<V, I>& ref_container)
{
  return ref_container.end();
}

template <typename V, typename I>
typename xiiRangeView<V, I>::ConstIterator end(const xiiRangeView<V, I>& container)
{
  return container.cend();
}

template <typename V, typename I>
typename xiiRangeView<V, I>::ConstIterator cend(const xiiRangeView<V, I>& container)
{
  return container.cend();
}

#include <Foundation/Types/Implementation/RangeView_inl.h>
