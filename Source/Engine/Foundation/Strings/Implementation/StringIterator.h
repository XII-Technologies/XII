/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifndef XII_INCLUDING_BASICS_H
#  error "StringIterator.h must not be included directly, but instead include Foundation/Basics.h."
#endif

/// STL forward iterator used by all string classes. Iterates over unicode characters.
///  The iterator starts at the first character of the string and ends at the address beyond the last character of the string.
struct xiiStringIterator
{
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type        = xiiUInt32;
  using difference_type   = std::ptrdiff_t;
  using pointer           = const char*;
  using reference         = xiiUInt32;

  XII_DECLARE_POD_TYPE();

  /// Constructs an invalid iterator.
  XII_ALWAYS_INLINE xiiStringIterator() = default; // [tested]

  /// Constructs either a begin or end iterator for the given string.
  XII_FORCE_INLINE explicit xiiStringIterator(const char* pStartPtr, const char* pEndPtr, const char* pCurPtr)
  {
    m_pStartPtr = pStartPtr;
    m_pEndPtr   = pEndPtr;
    m_pCurPtr   = pCurPtr;
  }

  /// Checks whether this iterator points to a valid element. Invalid iterators either point to m_pEndPtr or were never initialized.
  XII_ALWAYS_INLINE bool IsValid() const { return m_pCurPtr != nullptr && m_pCurPtr != m_pEndPtr; } // [tested]

  /// Returns the currently pointed to character in Utf32 encoding.
  XII_ALWAYS_INLINE xiiUInt32 GetCharacter() const { return IsValid() ? xiiUnicodeUtils::ConvertUtf8ToUtf32(m_pCurPtr) : xiiUInt32(0); } // [tested]

  /// Returns the currently pointed to character in Utf32 encoding.
  XII_ALWAYS_INLINE xiiUInt32 operator*() const { return GetCharacter(); } // [tested]

  /// Returns the address the iterator currently points to.
  XII_ALWAYS_INLINE const char* GetData() const { return m_pCurPtr; } // [tested]

  /// Checks whether the two iterators point to the same element.
  XII_ALWAYS_INLINE bool operator==(const xiiStringIterator& it2) const { return (m_pCurPtr == it2.m_pCurPtr); } // [tested]

  /// Advances the iterated to the next character, same as operator++, but returns how many bytes were consumed in the source string.
  XII_ALWAYS_INLINE xiiUInt32 Advance()
  {
    const char* pPrevElement = m_pCurPtr;

    if (m_pCurPtr < m_pEndPtr)
    {
      xiiUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();
    }

    return static_cast<xiiUInt32>(m_pCurPtr - pPrevElement);
  }

  /// Move to the next Utf8 character
  XII_ALWAYS_INLINE xiiStringIterator& operator++() // [tested]
  {
    if (m_pCurPtr < m_pEndPtr)
    {
      xiiUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();
    }

    return *this;
  }

  /// Move to the previous Utf8 character
  XII_ALWAYS_INLINE xiiStringIterator& operator--() // [tested]
  {
    if (m_pStartPtr < m_pCurPtr)
    {
      xiiUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    }

    return *this;
  }

  /// Move to the next Utf8 character
  XII_ALWAYS_INLINE xiiStringIterator operator++(xiiInt32) // [tested]
  {
    xiiStringIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  /// Move to the previous Utf8 character
  XII_ALWAYS_INLINE xiiStringIterator operator--(xiiInt32) // [tested]
  {
    xiiStringIterator tmp = *this;
    --(*this);
    return tmp;
  }

  /// Advances the iterator forwards by d characters. Does not move it beyond the range's end.
  XII_FORCE_INLINE void operator+=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      ++(*this);
      --d;
    }
    while (d < 0)
    {
      --(*this);
      ++d;
    }
  }

  /// Moves the iterator backwards by d characters. Does not move it beyond the range's start.
  XII_FORCE_INLINE void operator-=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      --(*this);
      --d;
    }
    while (d < 0)
    {
      ++(*this);
      ++d;
    }
  }

  /// Returns an iterator that is advanced forwards by d characters.
  XII_ALWAYS_INLINE xiiStringIterator operator+(difference_type d) const // [tested]
  {
    xiiStringIterator it = *this;
    it += d;
    return it;
  }

  /// Returns an iterator that is advanced backwards by d characters.
  XII_ALWAYS_INLINE xiiStringIterator operator-(difference_type d) const // [tested]
  {
    xiiStringIterator it = *this;
    it -= d;
    return it;
  }

  /// Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  void SetCurrentPosition(const char* szCurPos)
  {
    XII_ASSERT_DEV((szCurPos >= m_pStartPtr) && (szCurPos <= m_pEndPtr), "New position must still be inside the iterator's range.");

    m_pCurPtr = szCurPos;
  }

private:
  const char* m_pStartPtr = nullptr;
  const char* m_pEndPtr   = nullptr;
  const char* m_pCurPtr   = nullptr;
};


/// STL reverse iterator used by all string classes. Iterates over unicode characters.
///  The iterator starts at the last character of the string and ends at the address before the first character of the string.
struct xiiStringReverseIterator
{
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type        = xiiUInt32;
  using difference_type   = std::ptrdiff_t;
  using pointer           = const char*;
  using reference         = xiiUInt32;

  XII_DECLARE_POD_TYPE();

  /// Constructs an invalid iterator.
  XII_ALWAYS_INLINE xiiStringReverseIterator() = default; // [tested]

  /// Constructs either a rbegin or rend iterator for the given string.
  XII_FORCE_INLINE explicit xiiStringReverseIterator(const char* pStartPtr, const char* pEndPtr, const char* pCurPtr) // [tested]
  {
    m_pStartPtr = pStartPtr;
    m_pEndPtr   = pEndPtr;
    m_pCurPtr   = pCurPtr;

    if (m_pStartPtr >= m_pEndPtr)
    {
      m_pCurPtr = nullptr;
    }
    else if (m_pCurPtr == m_pEndPtr)
    {
      xiiUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    }
  }

  /// Checks whether this iterator points to a valid element.
  XII_ALWAYS_INLINE bool IsValid() const { return (m_pCurPtr != nullptr); } // [tested]

  /// Returns the currently pointed to character in Utf32 encoding.
  XII_ALWAYS_INLINE xiiUInt32 GetCharacter() const { return IsValid() ? xiiUnicodeUtils::ConvertUtf8ToUtf32(m_pCurPtr) : xiiUInt32(0); } // [tested]

  /// Returns the currently pointed to character in Utf32 encoding.
  XII_ALWAYS_INLINE xiiUInt32 operator*() const { return GetCharacter(); } // [tested]

  /// Returns the address the iterator currently points to.
  XII_ALWAYS_INLINE const char* GetData() const { return m_pCurPtr; } // [tested]

  /// Checks whether the two iterators point to the same element.
  XII_ALWAYS_INLINE bool operator==(const xiiStringReverseIterator& it2) const { return (m_pCurPtr == it2.m_pCurPtr); } // [tested]

  /// Move to the next Utf8 character
  XII_FORCE_INLINE xiiStringReverseIterator& operator++() // [tested]
  {
    if (m_pCurPtr != nullptr && m_pStartPtr < m_pCurPtr)
      xiiUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    else
      m_pCurPtr = nullptr;

    return *this;
  }

  /// Move to the previous Utf8 character
  XII_FORCE_INLINE xiiStringReverseIterator& operator--() // [tested]
  {
    if (m_pCurPtr != nullptr)
    {
      const char* szOldPos = m_pCurPtr;
      xiiUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();

      if (m_pCurPtr == m_pEndPtr)
        m_pCurPtr = szOldPos;
    }
    else
    {
      // Set back to the first character.
      m_pCurPtr = m_pStartPtr;
    }
    return *this;
  }

  /// Move to the next Utf8 character
  XII_ALWAYS_INLINE xiiStringReverseIterator operator++(xiiInt32) // [tested]
  {
    xiiStringReverseIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  /// Move to the previous Utf8 character
  XII_ALWAYS_INLINE xiiStringReverseIterator operator--(xiiInt32) // [tested]
  {
    xiiStringReverseIterator tmp = *this;
    --(*this);
    return tmp;
  }

  /// Advances the iterator forwards by d characters. Does not move it beyond the range's end.
  XII_FORCE_INLINE void operator+=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      ++(*this);
      --d;
    }
    while (d < 0)
    {
      --(*this);
      ++d;
    }
  }

  /// Moves the iterator backwards by d characters. Does not move it beyond the range's start.
  XII_FORCE_INLINE void operator-=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      --(*this);
      --d;
    }
    while (d < 0)
    {
      ++(*this);
      ++d;
    }
  }

  /// Returns an iterator that is advanced forwards by d characters.
  XII_ALWAYS_INLINE xiiStringReverseIterator operator+(difference_type d) const // [tested]
  {
    xiiStringReverseIterator it = *this;
    it += d;
    return it;
  }

  /// Returns an iterator that is advanced backwards by d characters.
  XII_ALWAYS_INLINE xiiStringReverseIterator operator-(difference_type d) const // [tested]
  {
    xiiStringReverseIterator it = *this;
    it -= d;
    return it;
  }

  /// Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  XII_FORCE_INLINE void SetCurrentPosition(const char* szCurPos)
  {
    XII_ASSERT_DEV((szCurPos == nullptr) || ((szCurPos >= m_pStartPtr) && (szCurPos < m_pEndPtr)), "New position must still be inside the iterator's range.");

    m_pCurPtr = szCurPos;
  }

private:
  const char* m_pStartPtr = nullptr;
  const char* m_pEndPtr   = nullptr;
  const char* m_pCurPtr   = nullptr;
};
