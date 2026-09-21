/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <algorithm>

/// Base class for STL like random access iterators
template <class ARRAY, class T, bool reverse = false>
struct const_iterator_base
{
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type        = T;
  using difference_type   = std::ptrdiff_t;
  using pointer           = const T*;
  using reference         = const T&;

  const_iterator_base()
  {
    m_pArray  = nullptr;
    m_uiIndex = 0;
  }
  const_iterator_base(const ARRAY& deque, size_t uiIndex)
  {
    m_pArray  = const_cast<ARRAY*>(&deque);
    m_uiIndex = uiIndex;
  }

  XII_ALWAYS_INLINE const_iterator_base& operator++()
  {
    m_uiIndex += 1;
    return *this;
  }
  XII_ALWAYS_INLINE const_iterator_base& operator--()
  {
    m_uiIndex -= 1;
    return *this;
  }

  XII_ALWAYS_INLINE const_iterator_base operator++(xiiInt32)
  {
    m_uiIndex += 1;
    return const_iterator_base(*m_pArray, m_uiIndex - 1);
  }
  XII_ALWAYS_INLINE const_iterator_base operator--(xiiInt32)
  {
    m_uiIndex -= 1;
    return const_iterator_base(*m_pArray, m_uiIndex + 1);
  }

  XII_ALWAYS_INLINE bool operator==(const const_iterator_base& rhs) const { return m_pArray == rhs.m_pArray && m_uiIndex == rhs.m_uiIndex; }

  XII_ALWAYS_INLINE std::ptrdiff_t operator-(const const_iterator_base& rhs) const { return m_uiIndex - rhs.m_uiIndex; }

  XII_ALWAYS_INLINE const_iterator_base operator+(std::ptrdiff_t rhs) const { return const_iterator_base(*m_pArray, m_uiIndex + rhs); }
  XII_ALWAYS_INLINE const_iterator_base operator-(std::ptrdiff_t rhs) const { return const_iterator_base(*m_pArray, m_uiIndex - rhs); }

  XII_ALWAYS_INLINE void operator+=(std::ptrdiff_t rhs) { m_uiIndex += rhs; }
  XII_ALWAYS_INLINE void operator-=(std::ptrdiff_t rhs) { m_uiIndex -= rhs; }

  inline const T& operator*() const
  {
    if (reverse)
    {
      return (*m_pArray)[m_pArray->GetCount() - (xiiUInt32)m_uiIndex - 1];
    }
    else
    {
      return (*m_pArray)[(xiiUInt32)m_uiIndex];
    }
  }
  XII_ALWAYS_INLINE const T* operator->() const { return &(**this); }

  XII_ALWAYS_INLINE std::strong_ordering operator<=>(const const_iterator_base& rhs) const { return m_uiIndex <=> rhs.m_uiIndex; }

  XII_ALWAYS_INLINE const T& operator[](size_t uiIndex) const
  {
    if (reverse)
    {
      return (*m_pArray)[m_pArray->GetCount() - static_cast<xiiUInt32>(m_uiIndex + uiIndex) - 1];
    }
    else
    {
      return (*m_pArray)[static_cast<xiiUInt32>(m_uiIndex + uiIndex)];
    }
  }

protected:
  ARRAY* m_pArray;
  size_t m_uiIndex;
};

/// Non-const STL like iterators
template <class ARRAY, class T, bool reverse = false>
struct iterator_base : public const_iterator_base<ARRAY, T, reverse>
{
public:
  using pointer   = T*;
  using reference = T&;

  iterator_base() {}
  iterator_base(ARRAY& ref_deque, size_t uiIndex) :
    const_iterator_base<ARRAY, T, reverse>(ref_deque, uiIndex)
  {
  }

  XII_ALWAYS_INLINE iterator_base& operator++()
  {
    this->m_uiIndex += 1;
    return *this;
  }
  XII_ALWAYS_INLINE iterator_base& operator--()
  {
    this->m_uiIndex -= 1;
    return *this;
  }

  XII_ALWAYS_INLINE iterator_base operator++(xiiInt32)
  {
    this->m_uiIndex += 1;
    return iterator_base(*this->m_pArray, this->m_uiIndex - 1);
  }
  XII_ALWAYS_INLINE iterator_base operator--(xiiInt32)
  {
    this->m_uiIndex -= 1;
    return iterator_base(*this->m_pArray, this->m_uiIndex + 1);
  }

  using const_iterator_base<ARRAY, T, reverse>::operator+;
  using const_iterator_base<ARRAY, T, reverse>::operator-;

  XII_ALWAYS_INLINE iterator_base operator+(std::ptrdiff_t rhs) const { return iterator_base(*this->m_pArray, this->m_uiIndex + rhs); }
  XII_ALWAYS_INLINE iterator_base operator-(std::ptrdiff_t rhs) const { return iterator_base(*this->m_pArray, this->m_uiIndex - rhs); }

  inline T& operator*() const
  {
    if (reverse)
    {
      return (*this->m_pArray)[this->m_pArray->GetCount() - (xiiUInt32)this->m_uiIndex - 1];
    }
    else
    {
      return (*this->m_pArray)[(xiiUInt32)this->m_uiIndex];
    }
  }

  XII_ALWAYS_INLINE T* operator->() const { return &(**this); }

  XII_ALWAYS_INLINE T& operator[](size_t uiIndex) const
  {
    if (reverse)
    {
      return (*this->m_pArray)[this->m_pArray->GetCount() - static_cast<xiiUInt32>(this->m_uiIndex + uiIndex) - 1];
    }
    else
    {
      return (*this->m_pArray)[static_cast<xiiUInt32>(this->m_uiIndex + uiIndex)];
    }
  }
};

/// Base class for Pointer like reverse iterators
template <class T>
struct const_reverse_pointer_iterator
{
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type        = T;
  using difference_type   = std::ptrdiff_t;
  using pointer           = T*;
  using reference         = T&;

  const_reverse_pointer_iterator() { m_pPtr = nullptr; }
  const_reverse_pointer_iterator(T const* pPtr) :
    m_pPtr(const_cast<T*>(pPtr))
  {
  }

  XII_ALWAYS_INLINE const_reverse_pointer_iterator& operator++()
  {
    m_pPtr--;
    return *this;
  }
  XII_ALWAYS_INLINE const_reverse_pointer_iterator& operator--()
  {
    m_pPtr++;
    return *this;
  }

  XII_ALWAYS_INLINE const_reverse_pointer_iterator operator++(xiiInt32)
  {
    m_pPtr--;
    return const_reverse_pointer_iterator(m_pPtr + 1);
  }
  XII_ALWAYS_INLINE const_reverse_pointer_iterator operator--(xiiInt32)
  {
    m_pPtr++;
    return const_reverse_pointer_iterator(m_pPtr - 1);
  }

  XII_ALWAYS_INLINE bool operator==(const const_reverse_pointer_iterator& rhs) const { return m_pPtr == rhs.m_pPtr; }

  XII_ALWAYS_INLINE std::ptrdiff_t operator-(const const_reverse_pointer_iterator& rhs) const { return rhs.m_pPtr - m_pPtr; }

  XII_ALWAYS_INLINE const_reverse_pointer_iterator operator+(std::ptrdiff_t rhs) const { return const_reverse_pointer_iterator(m_pPtr - rhs); }
  XII_ALWAYS_INLINE const_reverse_pointer_iterator operator-(std::ptrdiff_t rhs) const { return const_reverse_pointer_iterator(m_pPtr + rhs); }

  XII_ALWAYS_INLINE void operator+=(std::ptrdiff_t rhs) { m_pPtr -= rhs; }
  XII_ALWAYS_INLINE void operator-=(std::ptrdiff_t rhs) { m_pPtr += rhs; }

  XII_ALWAYS_INLINE const T& operator*() const { return *m_pPtr; }
  XII_ALWAYS_INLINE const T* operator->() const { return m_pPtr; }

  XII_ALWAYS_INLINE std::strong_ordering operator<=>(const const_reverse_pointer_iterator& rhs) const { return rhs.m_pPtr <=> m_pPtr; }

  XII_ALWAYS_INLINE const T& operator[](std::ptrdiff_t iIndex) const { return *(m_pPtr - iIndex); }

protected:
  T* m_pPtr;
};

/// Non-Const class for Pointer like reverse iterators
template <class T>
struct reverse_pointer_iterator : public const_reverse_pointer_iterator<T>
{
public:
  using pointer   = T*;
  using reference = T&;

  reverse_pointer_iterator() {}
  reverse_pointer_iterator(T* pPtr) :
    const_reverse_pointer_iterator<T>(pPtr)
  {
  }

  XII_ALWAYS_INLINE reverse_pointer_iterator& operator++()
  {
    this->m_pPtr--;
    return *this;
  }
  XII_ALWAYS_INLINE reverse_pointer_iterator& operator--()
  {
    this->m_pPtr++;
    return *this;
  }

  XII_ALWAYS_INLINE reverse_pointer_iterator operator++(xiiInt32)
  {
    this->m_pPtr--;
    return reverse_pointer_iterator(this->m_pPtr + 1);
  }
  XII_ALWAYS_INLINE reverse_pointer_iterator operator--(xiiInt32)
  {
    this->m_pPtr++;
    return reverse_pointer_iterator(this->m_pPtr - 1);
  }

  using const_reverse_pointer_iterator<T>::operator+;
  using const_reverse_pointer_iterator<T>::operator-;

  XII_ALWAYS_INLINE reverse_pointer_iterator operator+(std::ptrdiff_t rhs) const { return reverse_pointer_iterator(this->m_pPtr - rhs); }
  XII_ALWAYS_INLINE reverse_pointer_iterator operator-(std::ptrdiff_t rhs) const { return reverse_pointer_iterator(this->m_pPtr + rhs); }

  XII_ALWAYS_INLINE T& operator*() const { return *(this->m_pPtr); }
  XII_ALWAYS_INLINE T* operator->() const { return this->m_pPtr; }
  XII_ALWAYS_INLINE T& operator[](std::ptrdiff_t iIndex) const { return *(this->m_pPtr - iIndex); }
};
