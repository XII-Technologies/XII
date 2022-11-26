#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/Implementation/StringBase.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

class xiiStringBuilder;
class xiiStreamReader;

/// \brief A string class for storing and passing around strings.
///
/// This class only allows read-access to its data. It does not allow modifications.
/// To build / modify strings, use the xiiStringBuilder class.
/// xiiHybridString has an internal array to store short strings without any memory allocations, it will dynamically
/// allocate additional memory, if that cache is insufficient. Thus a hybrid string will always take up a certain amount
/// of memory, which might be of concern when it is used as a member variable, in such cases you might want to use an
/// xiiHybridString with a very small internal array (1 would basically make it into a completely dynamic string).
/// On the other hand, creating xiiHybridString instances on the stack and working locally with them, is quite fast.
/// Prefer to use the typedef'd string types \a xiiString, \a xiiDynamicString, \a xiiString32 etc.
/// Most strings in an application are rather short, typically shorter than 20 characters.
/// Use \a xiiString, which is a typedef'd xiiHybridString to use a cache size that is sufficient for more than 90%
/// of all use cases.
template <xiiUInt16 Size>
struct xiiHybridStringBase : public xiiStringBase<xiiHybridStringBase<Size>>
{
protected:
  /// \brief Creates an empty string.
  xiiHybridStringBase(xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  xiiHybridStringBase(const xiiHybridStringBase& rhs, xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Moves the data from \a rhs.
  xiiHybridStringBase(xiiHybridStringBase&& rhs, xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  xiiHybridStringBase(const char* rhs, xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  xiiHybridStringBase(const wchar_t* rhs, xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  xiiHybridStringBase(const xiiStringView& rhs, xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  xiiHybridStringBase(const xiiStringBuilder& rhs, xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Moves the data from \a rhs.
  xiiHybridStringBase(xiiStringBuilder&& rhs, xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Destructor.
  ~xiiHybridStringBase(); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const xiiHybridStringBase& rhs); // [tested]

  /// \brief Moves the data from \a rhs.
  void operator=(xiiHybridStringBase&& rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const char* rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const wchar_t* rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const xiiStringView& rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const xiiStringBuilder& rhs); // [tested]

  /// \brief Moves the data from \a rhs.
  void operator=(xiiStringBuilder&& rhs); // [tested]

public:
  /// \brief Resets this string to an empty string.
  ///
  /// This will not deallocate any previously allocated data, but reuse that memory.
  void Clear(); // [tested]

  /// \brief Returns a pointer to the internal Utf8 string.
  const char* GetData() const; // [tested]

  /// \brief Returns the amount of bytes that this string takes (excluding the '\0' terminator).
  xiiUInt32 GetElementCount() const; // [tested]

  /// \brief Returns the number of characters in this string.
  xiiUInt32 GetCharacterCount() const; // [tested]

  /// \brief Returns a view to a sub-string of this string, starting at character uiFirstCharacter, up until uiFirstCharacter +
  /// uiNumCharacters.
  ///
  /// Note that this view will only be valid as long as this xiiHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  xiiStringView GetSubString(xiiUInt32 uiFirstCharacter, xiiUInt32 uiNumCharacters) const; // [tested]

  /// \brief Returns a view to the sub-string containing the first uiNumCharacters characters of this string.
  ///
  /// Note that this view will only be valid as long as this xiiHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  xiiStringView GetFirst(xiiUInt32 uiNumCharacters) const; // [tested]

  /// \brief Returns a view to the sub-string containing the last uiNumCharacters characters of this string.
  ///
  /// Note that this view will only be valid as long as this xiiHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  xiiStringView GetLast(xiiUInt32 uiNumCharacters) const; // [tested]

  /// \brief Replaces the current string with the content from the stream. Reads the stream to its end.
  void ReadAll(xiiStreamReader& Stream);

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); }

private:
  friend class xiiStringBuilder;

  xiiHybridArray<char, Size> m_Data;
  xiiUInt32                  m_uiCharacterCount = 0;
};


/// \brief \see xiiHybridStringBase
template <xiiUInt16 Size, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
struct xiiHybridString : public xiiHybridStringBase<Size>
{
public:
  XII_DECLARE_MEM_RELOCATABLE_TYPE();

  xiiHybridString();
  xiiHybridString(xiiAllocatorBase* pAllocator);

  xiiHybridString(const xiiHybridString<Size, AllocatorWrapper>& other);
  xiiHybridString(const xiiHybridStringBase<Size>& other);
  xiiHybridString(const char* rhs);
  xiiHybridString(const wchar_t* rhs);
  xiiHybridString(const xiiStringView& rhs);
  xiiHybridString(const xiiStringBuilder& rhs);
  xiiHybridString(xiiStringBuilder&& rhs);

  xiiHybridString(xiiHybridString<Size, AllocatorWrapper>&& other);
  xiiHybridString(xiiHybridStringBase<Size>&& other);


  void operator=(const xiiHybridString<Size, AllocatorWrapper>& rhs);
  void operator=(const xiiHybridStringBase<Size>& rhs);
  void operator=(const char* szString);
  void operator=(const wchar_t* szString);
  void operator=(const xiiStringView& rhs);
  void operator=(const xiiStringBuilder& rhs);
  void operator=(xiiStringBuilder&& rhs);

  void operator=(xiiHybridString<Size, AllocatorWrapper>&& rhs);
  void operator=(xiiHybridStringBase<Size>&& rhs);
};

using xiiDynamicString = xiiHybridString<1>;
/// \brief String that uses the static allocator to prevent leak reports in RTTI attributes.
using xiiUntrackedString = xiiHybridString<32, xiiStaticAllocatorWrapper>;
using xiiString          = xiiHybridString<32>;
using xiiString16        = xiiHybridString<16>;
using xiiString24        = xiiHybridString<24>;
using xiiString32        = xiiHybridString<32>;
using xiiString48        = xiiHybridString<48>;
using xiiString64        = xiiHybridString<64>;
using xiiString128       = xiiHybridString<128>;
using xiiString256       = xiiHybridString<256>;

XII_CHECK_AT_COMPILETIME_MSG(xiiGetTypeClass<xiiString>::value == 2, "string is not memory relocatable");

template <xiiUInt16 Size>
struct xiiCompareHelper<xiiHybridString<Size>>
{
  XII_ALWAYS_INLINE bool Less(xiiStringView lhs, xiiStringView rhs) const
  {
    return lhs.Compare(rhs) < 0;
  }

  XII_ALWAYS_INLINE bool Equal(xiiStringView lhs, xiiStringView rhs) const
  {
    return lhs.IsEqual(rhs);
  }
};

struct xiiCompareString_NoCase
{
  XII_ALWAYS_INLINE bool Less(xiiStringView lhs, xiiStringView rhs) const
  {
    return lhs.Compare_NoCase(rhs) < 0;
  }

  XII_ALWAYS_INLINE bool Equal(xiiStringView lhs, xiiStringView rhs) const
  {
    return lhs.IsEqual_NoCase(rhs);
  }
};

struct CompareConstChar
{
  /// \brief Returns true if a is less than b
  XII_ALWAYS_INLINE bool Less(const char* a, const char* b) const { return xiiStringUtils::Compare(a, b) < 0; }

  /// \brief Returns true if a is equal to b
  XII_ALWAYS_INLINE bool Equal(const char* a, const char* b) const { return xiiStringUtils::IsEqual(a, b); }
};

// For xiiFormatString
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiString& arg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* tmp, xiiUInt32 uiLength, const xiiUntrackedString& arg);

#include <Foundation/Strings/Implementation/String_inl.h>
