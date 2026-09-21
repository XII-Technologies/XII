/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/Implementation/StringBase.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

class xiiStringBuilder;
class xiiStreamReader;

/// A string class for storing and passing around strings.
///
/// This class only allows read-access to its data. It does not allow modifications.
/// To build / modify strings, use the xiiStringBuilder class.
/// xiiHybridString has an internal array to store short strings without any memory allocations, it will dynamically
/// allocate additional memory, if that cache is insufficient. Thus a hybrid string will always take up a certain amount
/// of memory, which might be of concern when it is used as a member variable, in such cases you might want to use an
/// xiiHybridString with a very small internal array (1 would basically make it into a completely dynamic string).
/// On the other hand, creating xiiHybridString instances on the stack and working locally with them, is quite fast.
/// Prefer to use the aliased string types \a xiiString, \a xiiDynamicString, \a xiiString32 etc.
/// Most strings in an application are rather short, typically shorter than 20 characters.
/// Use \a xiiString, which is an aliased xiiHybridString to use a cache size that is sufficient for more than 90%
/// of all use cases.
template <xiiUInt16 Size>
struct xiiHybridStringBase : public xiiStringBase<xiiHybridStringBase<Size>>
{
protected:
  /// Creates an empty string.
  xiiHybridStringBase(xiiAllocator* pAllocator); // [tested]

  /// Copies the data from \a rhs.
  xiiHybridStringBase(const xiiHybridStringBase& rhs, xiiAllocator* pAllocator); // [tested]

  /// Moves the data from \a rhs.
  xiiHybridStringBase(xiiHybridStringBase&& rhs, xiiAllocator* pAllocator); // [tested]

  /// Copies the data from \a rhs.
  xiiHybridStringBase(const char* rhs, xiiAllocator* pAllocator); // [tested]

  /// Copies the data from \a rhs.
  xiiHybridStringBase(const wchar_t* rhs, xiiAllocator* pAllocator); // [tested]

  /// Copies the data from \a rhs.
  xiiHybridStringBase(const xiiStringView& rhs, xiiAllocator* pAllocator); // [tested]

  /// Copies the data from \a rhs.
  xiiHybridStringBase(const xiiStringBuilder& rhs, xiiAllocator* pAllocator); // [tested]

  /// Moves the data from \a rhs.
  xiiHybridStringBase(xiiStringBuilder&& rhs, xiiAllocator* pAllocator); // [tested]

  /// Destructor.
  ~xiiHybridStringBase(); // [tested]

  /// Copies the data from \a rhs.
  void operator=(const xiiHybridStringBase& rhs); // [tested]

  /// Moves the data from \a rhs.
  void operator=(xiiHybridStringBase&& rhs); // [tested]

  /// Copies the data from \a rhs.
  void operator=(const char* rhs); // [tested]

  /// Copies the data from \a rhs.
  void operator=(const wchar_t* rhs); // [tested]

  /// Copies the data from \a rhs.
  void operator=(const xiiStringView& rhs); // [tested]

  /// Copies the data from \a rhs.
  void operator=(const xiiStringBuilder& rhs); // [tested]

  /// Moves the data from \a rhs.
  void operator=(xiiStringBuilder&& rhs); // [tested]

#if XII_ENABLED(XII_INTEROP_STL_STRINGS)
  /// Copies the data from \a rhs.
  xiiHybridStringBase(const std::string_view& rhs, xiiAllocator* pAllocator);

  /// Copies the data from \a rhs.
  xiiHybridStringBase(const std::string& rhs, xiiAllocator* pAllocator);

  /// Copies the data from \a rhs.
  void operator=(const std::string_view& rhs);

  /// Copies the data from \a rhs.
  void operator=(const std::string& rhs);
#endif

public:
  /// Resets this string to an empty string.
  ///
  /// This will not deallocate any previously allocated data, but reuse that memory.
  void Clear(); // [tested]

  /// Returns a pointer to the internal Utf8 string.
  const char* GetData() const; // [tested]

  /// Returns the amount of bytes that this string takes (excluding the '\0' terminator).
  xiiUInt32 GetElementCount() const; // [tested]

  /// Returns the number of characters in this string. Might be less than GetElementCount, if it contains Utf8
  /// multi-byte characters.
  ///
  /// \note This is a slow operation, as it has to run through the entire string to count the Unicode characters.
  /// Only call this once and use the result as long as the string doesn't change. Don't call this in a loop.
  xiiUInt32 GetCharacterCount() const; // [tested]

  /// Returns a view to a sub-string of this string, starting at character uiFirstCharacter, up until uiFirstCharacter +
  /// uiNumCharacters.
  ///
  /// Note that this view will only be valid as long as this xiiHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  xiiStringView GetSubString(xiiUInt32 uiFirstCharacter, xiiUInt32 uiNumCharacters) const; // [tested]

  /// Returns a view to the sub-string containing the first uiNumCharacters characters of this string.
  ///
  /// Note that this view will only be valid as long as this xiiHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  xiiStringView GetFirst(xiiUInt32 uiNumCharacters) const; // [tested]

  /// Returns a view to the sub-string containing the last uiNumCharacters characters of this string.
  ///
  /// Note that this view will only be valid as long as this xiiHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  xiiStringView GetLast(xiiUInt32 uiNumCharacters) const; // [tested]

  /// Replaces the current string with the content from the stream. Reads the stream to its end.
  void ReadAll(xiiStreamReader& ref_stream);

  /// Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); }

private:
  friend class xiiStringBuilder;

  xiiHybridArray<char, Size> m_Data;
};


/// \see xiiHybridStringBase
template <xiiUInt16 Size, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
struct xiiHybridString : public xiiHybridStringBase<Size>
{
public:
  xiiHybridString();
  xiiHybridString(xiiAllocator* pAllocator);

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
  void operator=(const wchar_t* pString);
  void operator=(const xiiStringView& rhs);
  void operator=(const xiiStringBuilder& rhs);
  void operator=(xiiStringBuilder&& rhs);
  void operator=(xiiHybridString<Size, AllocatorWrapper>&& rhs);
  void operator=(xiiHybridStringBase<Size>&& rhs);

#if XII_ENABLED(XII_INTEROP_STL_STRINGS)
  xiiHybridString(const std::string_view& rhs);
  xiiHybridString(const std::string& rhs);
  void operator=(const std::string_view& rhs);
  void operator=(const std::string& rhs);
#endif
};

/// String that uses the static allocator to prevent leak reports in RTTI attributes.
using xiiUntrackedString = xiiHybridString<32U, xiiStaticAllocatorWrapper>;

using xiiDynamicString = xiiHybridString<1U>;
using xiiString        = xiiHybridString<32U>;
using xiiString16      = xiiHybridString<16U>;
using xiiString24      = xiiHybridString<24U>;
using xiiString32      = xiiHybridString<32U>;
using xiiString48      = xiiHybridString<48U>;
using xiiString64      = xiiHybridString<64U>;
using xiiString128     = xiiHybridString<128U>;
using xiiString256     = xiiHybridString<256U>;

static_assert(xiiGetTypeClass<xiiString>::value == xiiTypeIsClass::value);

template <xiiUInt16 Size>
struct xiiCompareHelper<xiiHybridString<Size>>
{
  static XII_ALWAYS_INLINE bool Less(xiiStringView lhs, xiiStringView rhs)
  {
    return lhs.Compare(rhs) < 0;
  }

  static XII_ALWAYS_INLINE bool Equal(xiiStringView lhs, xiiStringView rhs)
  {
    return lhs.IsEqual(rhs);
  }
};

struct xiiCompareString_NoCase
{
  static XII_ALWAYS_INLINE bool Less(xiiStringView lhs, xiiStringView rhs)
  {
    return lhs.Compare_NoCase(rhs) < 0;
  }

  static XII_ALWAYS_INLINE bool Equal(xiiStringView lhs, xiiStringView rhs)
  {
    return lhs.IsEqual_NoCase(rhs);
  }
};

struct CompareConstChar
{
  /// Returns true if a is less than b
  static XII_ALWAYS_INLINE bool Less(const char* a, const char* b) { return xiiStringUtils::Compare(a, b) < 0; }

  /// Returns true if a is equal to b
  static XII_ALWAYS_INLINE bool Equal(const char* a, const char* b) { return xiiStringUtils::IsEqual(a, b); }
};

// For xiiFormatString
XII_FOUNDATION_DLL xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiString& sArg);
XII_FOUNDATION_DLL xiiStringView BuildString(char* szTmp, xiiUInt32 uiLength, const xiiUntrackedString& sArg);

#include <Foundation/Strings/Implementation/String_inl.h>
