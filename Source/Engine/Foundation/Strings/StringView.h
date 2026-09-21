/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifndef XII_INCLUDING_BASICS_H
#  error "StringView.h must not be included directly, but instead include Foundation/Basics.h."
#endif

#include <Foundation/Strings/StringUtils.h>

#include <Foundation/Strings/Implementation/StringIterator.h>

#include <type_traits>

#if XII_ENABLED(XII_INTEROP_STL_STRINGS)
#  include <string_view>
#endif

/// Base class which marks a class as containing string data
struct xiiThisIsAString
{
};

class xiiStringBuilder;

/// xiiStringView represent a read-only sub-string of a larger string, as it can store a dedicated string end position.
/// It derives from xiiStringBase and thus provides a large set of functions for search and comparisons.
///
/// Attention: xiiStringView does not store string data itself. It only stores pointers into memory. For example,
/// when you get a xiiStringView to a xiiStringBuilder, the xiiStringView instance will point to the exact same memory,
/// enabling you to iterate over it (read-only).
/// That means that a xiiStringView is only valid as long as its source data is not modified. Once you make any kind
/// of modification to the source data, you should not continue using the xiiStringView to that data anymore,
/// as it might now point into invalid memory.
class XII_FOUNDATION_DLL xiiStringView : public xiiThisIsAString
{
public:
  XII_DECLARE_POD_TYPE();

  using iterator               = xiiStringIterator;
  using const_iterator         = xiiStringIterator;
  using reverse_iterator       = xiiStringReverseIterator;
  using const_reverse_iterator = xiiStringReverseIterator;

  /// Default constructor creates an invalid view.
  constexpr xiiStringView();

  /// Creates a string view starting at the given position, ending at the next '\0' terminator.
  xiiStringView(char* pStart);

  /// Creates a string view starting at the given position, ending at the next '\0' terminator.
  template <typename T>                                                                                                 // T is always const char*
  constexpr xiiStringView(T pStart, typename std::enable_if<std::is_same<T, const char*>::value, xiiInt32>::type* = 0); // [tested]

  /// Creates a string view from any class / struct which is implicitly convertible to const char *
  template <typename T>
  constexpr XII_ALWAYS_INLINE xiiStringView(const T&& str, typename std::enable_if<std::is_same<T, const char*>::value == false && std::is_convertible<T, const char*>::value, xiiInt32>::type* = 0); // [tested]

  /// Creates a string view for the range from pStart to pEnd.
  constexpr xiiStringView(const char* pStart, const char* pEnd); // [tested]

  /// Creates a string view for the range from pStart to pStart + uiLength.
  constexpr xiiStringView(const char* pStart, xiiUInt32 uiLength);

  /// Construct a string view from a string literal.
  template <size_t N>
  constexpr xiiStringView(const char (&str)[N]);

  /// Construct a string view from a fixed size buffer
  template <size_t N>
  constexpr xiiStringView(char (&str)[N]);

  /// Advances the start to the next character, unless the end of the range was reached.
  void operator++(); // [tested]

  /// Advances the start forwards by d characters. Does not move it beyond the range's end.
  void operator+=(xiiUInt32 d); // [tested]

  /// Returns the first pointed to character in Utf32 encoding.
  xiiUInt32 GetCharacter() const; // [tested]

  /// Returns true, if the current string pointed to is non empty.
  bool IsValid() const; // [tested]

  /// Returns the data as a zero-terminated string.
  ///
  /// The string will be copied to \a tempStorage and the pointer to that is returned.
  /// If you really need the raw pointer to the xiiStringView memory or are absolutely certain that the view points
  /// to a zero-terminated string, you can use GetStartPointer()
  const char* GetData(xiiStringBuilder& ref_sTempStorage) const; // [tested]

  /// Returns the number of bytes from the start position up to its end.
  ///
  /// \note Note that the element count (bytes) may be larger than the number of characters in that string, due to Utf8 encoding.
  xiiUInt32 GetElementCount() const { return m_uiElementCount; } // [tested]

  /// Allows to set the start position to a different value.
  ///
  /// Must be between the current start and end range.
  void SetStartPosition(const char* szCurPos); // [tested]

  /// Returns the start of the view range.
  /// \note Be careful to not use this and assume the view will be zero-terminated. Use GetData(xiiStringBuilder&) instead to be safe.
  const char* GetStartPointer() const { return m_pStart; } // [tested]

  /// Returns the end of the view range. This will point to the byte AFTER the last character.
  ///
  /// That means it might point to the '\0' terminator, UNLESS the view only represents a sub-string of a larger string.
  /// Accessing the value at 'GetEnd' has therefore no real use.
  const char* GetEndPointer() const { return m_pStart + m_uiElementCount; } // [tested]

  /// Returns whether the string is an empty string.
  bool IsEmpty() const; // [tested]

  /// Compares this string view with the other string view for equality.
  bool IsEqual(xiiStringView sOther) const;

  /// Compares this string view with the other string view for equality.
  bool IsEqual_NoCase(xiiStringView sOther) const;

  /// Compares up to a given number of characters of this string with the other string for equality. Case insensitive.
  bool IsEqualN(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const; // [tested]

  /// Compares up to a given number of characters of this string with the other string for equality. Case insensitive.
  bool IsEqualN_NoCase(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other one. Returns 0 for equality, -1 if this string is 'smaller', 1 otherwise.
  xiiInt32 Compare(xiiStringView sOther) const; // [tested]

  /// Compares up to a given number of characters of this string with the other one. Returns 0 for equality, -1 if this string is 'smaller',
  /// 1 otherwise.
  xiiInt32 CompareN(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other one. Returns 0 for equality, -1 if this string is 'smaller', 1 otherwise. Case insensitive.
  xiiInt32 Compare_NoCase(xiiStringView sOther) const; // [tested]

  /// Compares up to a given number of characters of this string with the other one. Returns 0 for equality, -1 if this string is 'smaller',
  /// 1 otherwise. Case insensitive.
  xiiInt32 CompareN_NoCase(xiiStringView sOther, xiiUInt32 uiCharsToCompare) const; // [tested]

  /// Returns true, if this string starts with the given string.
  bool StartsWith(xiiStringView sStartsWith) const; // [tested]

  /// Returns true, if this string starts with the given string. Case insensitive.
  bool StartsWith_NoCase(xiiStringView sStartsWith) const; // [tested]

  /// Returns true, if this string ends with the given string.
  bool EndsWith(xiiStringView sEndsWith) const; // [tested]

  /// Returns true, if this string ends with the given string. Case insensitive.
  bool EndsWith_NoCase(xiiStringView sEndsWith) const; // [tested]

  /// Computes the pointer to the n-th character in the string. This is a linear search from the start.
  const char* ComputeCharacterPosition(xiiUInt32 uiCharacterIndex) const;

  /// Returns a pointer to the first occurrence of szStringToFind, or nullptr if none was found.
  /// To find the next occurrence, use a xiiStringView which points to the next position and call FindSubString again.
  const char* FindSubString(xiiStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Returns a pointer to the first occurrence of szStringToFind, or nullptr if none was found. Case insensitive.
  /// To find the next occurrence, use a xiiStringView which points to the next position and call FindSubString again.
  const char* FindSubString_NoCase(xiiStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Returns a pointer to the last occurrence of szStringToFind, or nullptr if none was found.
  /// szStartSearchAt allows to start searching at the end of the string (if it is nullptr) or at an earlier position.
  const char* FindLastSubString(xiiStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Returns a pointer to the last occurrence of szStringToFind, or nullptr if none was found. Case insensitive.
  /// szStartSearchAt allows to start searching at the end of the string (if it is nullptr) or at an earlier position.
  const char* FindLastSubString_NoCase(xiiStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Searches for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is
  /// returned. Otherwise nullptr.
  const char* FindWholeWord(const char* szSearchFor, xiiStringUtils::XII_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Searches for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is
  /// returned. Otherwise nullptr. Ignores case.
  const char* FindWholeWord_NoCase(const char* szSearchFor, xiiStringUtils::XII_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt = nullptr) const; // [tested]


  /// Shrinks the view range by uiShrinkCharsFront characters at the front and by uiShrinkCharsBack characters at the back.
  ///
  /// Thus reduces the range of the view to a smaller sub-string.
  /// The current position is clamped to the new start of the range.
  /// The new end position is clamped to the new start of the range.
  /// If more characters are removed from the range, than it actually contains, the view range will become 'empty'
  /// and its state will be set to invalid, however no error or assert will be triggered.
  void Shrink(xiiUInt32 uiShrinkCharsFront, xiiUInt32 uiShrinkCharsBack); // [tested]

  /// Returns a sub-string that is shrunk at the start and front by the given amount of characters (not bytes!).
  xiiStringView GetShrunk(xiiUInt32 uiShrinkCharsFront, xiiUInt32 uiShrinkCharsBack = 0) const; // [tested]

  /// Returns a sub-string starting at a given character (not byte offset!) and including a number of characters (not bytes).
  ///
  /// If this is a Utf-8 string, the correct number of bytes are skipped to reach the given character.
  /// If you instead want to construct a sub-string from byte offsets, use the xiiStringView constructor that takes a start pointer like so:
  ///   xiiStringView subString(this->GetStartPointer() + byteOffset, byteCount);
  xiiStringView GetSubString(xiiUInt32 uiFirstCharacter, xiiUInt32 uiNumCharacters) const; // [tested]

  /// Identical to 'Shrink(1, 0)' in functionality, but slightly more efficient.
  void ChopAwayFirstCharacterUtf8(); // [tested]

  /// Similar to ChopAwayFirstCharacterUtf8(), but assumes that the first character is ASCII and thus exactly one byte in length.
  /// Asserts that this is the case.
  /// More efficient than ChopAwayFirstCharacterUtf8(), if it is known that the first character is ASCII.
  void ChopAwayFirstCharacterAscii(); // [tested]

  /// Removes all characters from the start and end that appear in the given strings by adjusting the begin and end of the view.
  void Trim(const char* szTrimChars = " \f\n\r\t\v"); // [tested]

  /// Removes all characters from the start and/or end that appear in the given strings by adjusting the begin and end of the view.
  void Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd); // [tested]

  /// If the string starts with the given word (case insensitive), it is removed and the function returns true.
  bool TrimWordStart(xiiStringView sWord); // [tested]

  /// If the string ends with the given word (case insensitive), it is removed and the function returns true.
  bool TrimWordEnd(xiiStringView sWord); // [tested]

  /// Fills the given container with xiiStringView's which represent each found substring.
  /// If bReturnEmptyStrings is true, even empty strings between separators are returned.
  /// Output must be a container that stores xiiStringView's and provides the functions 'Clear' and 'Append'.
  /// szSeparator1 to szSeparator6 are strings which act as separators and indicate where to split the string.
  /// This string itself will not be modified.
  template <typename Container>
  void Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 = nullptr, const char* szSeparator3 = nullptr, const char* szSeparator4 = nullptr, const char* szSeparator5 = nullptr, const char* szSeparator6 = nullptr) const; // [tested]

  /// Returns an iterator to this string, which points to the very first character.
  ///
  /// Note that this iterator will only be valid as long as this string lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  iterator GetIteratorFront() const;

  /// Returns an iterator to this string, which points to the very last character (NOT the end).
  ///
  /// Note that this iterator will only be valid as long as this string lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  reverse_iterator GetIteratorBack() const;

  // ******* Path Functions ********

  /// Checks whether the given path has any file extension
  bool HasAnyExtension() const; // [tested]

  /// Checks whether the given path ends with the given extension. szExtension may start with a '.', but doesn't have to.
  ///
  /// The check is case insensitive.
  bool HasExtension(xiiStringView sExtension) const; // [tested]

  /// Returns the file extension of the given path. Will be empty, if the path does not end with a proper extension.
  ///
  /// If bFullExtension is false, a file named "file.a.b.c" will return "c".
  /// If bFullExtension is true, a file named "file.a.b.c" will return "a.b.c".
  xiiStringView GetFileExtension(bool bFullExtension = false) const; // [tested]

  /// Returns the file name of a path, excluding the path and extension.
  ///
  /// If the path already ends with a path separator, the result will be empty.
  xiiStringView GetFileName() const; // [tested]

  /// Returns the substring that represents the file name including the file extension.
  ///
  /// Returns an empty string, if sPath already ends in a path separator, or is empty itself.
  xiiStringView GetFileNameAndExtension() const; // [tested]

  /// Returns the directory of the given file, which is the substring up to the last path separator.
  ///
  /// If the path already ends in a path separator, and thus points to a folder, instead of a file, the unchanged path is returned.
  /// "path/to/file" -> "path/to/"
  /// "path/to/folder/" -> "path/to/folder/"
  /// "filename" -> ""
  /// "/file_at_root_level" -> "/"
  xiiStringView GetFileDirectory() const; // [tested]

  /// Returns true, if the given path represents an absolute path on the current OS.
  bool IsAbsolutePath() const; // [tested]

  /// Returns true, if the given path represents a relative path on the current OS.
  bool IsRelativePath() const; // [tested]

  /// Returns true, if the given path represents a 'rooted' path. See xiiFileSystem for details.
  bool IsRootedPath() const; // [tested]

  /// Extracts the root name from a rooted path
  ///
  /// ":MyRoot" -> "MyRoot"
  /// ":MyRoot\folder" -> "MyRoot"
  /// ":\MyRoot\folder" -> "MyRoot"
  /// ":/MyRoot\folder" -> "MyRoot"
  /// Returns an empty string, if the path is not rooted.
  xiiStringView GetRootedPathRootName() const; // [tested]

#if XII_ENABLED(XII_INTEROP_STL_STRINGS)
  /// Makes the xiiStringView reference the same memory as the const std::string_view&.
  xiiStringView(const std::string_view& rhs);

  /// Makes the xiiStringView reference the same memory as the const std::string_view&.
  xiiStringView(const std::string& rhs);

  /// Returns a std::string_view to this string.
  operator std::string_view() const;

  /// Returns a std::string_view to this string.
  std::string_view GetAsStdView() const;
#endif

private:
  const char* m_pStart         = nullptr;
  xiiUInt32   m_uiElementCount = 0;
};

/// String literal suffix to create a xiiStringView.
///
/// Example:
/// "Hello World"
constexpr xiiStringView operator""_xiisv(const char* pString, std::size_t uiLength);

XII_ALWAYS_INLINE typename xiiStringView::iterator begin(xiiStringView sContainer)
{
  return typename xiiStringView::iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetStartPointer());
}

XII_ALWAYS_INLINE typename xiiStringView::const_iterator cbegin(xiiStringView sContainer)
{
  return typename xiiStringView::const_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetStartPointer());
}

XII_ALWAYS_INLINE typename xiiStringView::iterator end(xiiStringView sContainer)
{
  return typename xiiStringView::iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetEndPointer());
}

XII_ALWAYS_INLINE typename xiiStringView::const_iterator cend(xiiStringView sContainer)
{
  return typename xiiStringView::const_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetEndPointer());
}


XII_ALWAYS_INLINE typename xiiStringView::reverse_iterator rbegin(xiiStringView sContainer)
{
  return typename xiiStringView::reverse_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetEndPointer());
}

XII_ALWAYS_INLINE typename xiiStringView::const_reverse_iterator crbegin(xiiStringView sContainer)
{
  return typename xiiStringView::const_reverse_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetEndPointer());
}

XII_ALWAYS_INLINE typename xiiStringView::reverse_iterator rend(xiiStringView sContainer)
{
  return typename xiiStringView::reverse_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), nullptr);
}

XII_ALWAYS_INLINE typename xiiStringView::const_reverse_iterator crend(xiiStringView sContainer)
{
  return typename xiiStringView::const_reverse_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), nullptr);
}

#include <Foundation/Strings/Implementation/StringView_inl.h>
