/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/ThirdParty/utf8/utf8.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/Implementation/StringBase.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

template <xiiUInt16 Size>
struct xiiHybridStringBase;

template <xiiUInt16 Size, typename AllocatorWrapper>
struct xiiHybridString;

class xiiStreamReader;
class xiiFormatString;

/// xiiStringBuilder is a class that is meant for creating and modifying strings.
///
/// It is not meant to store strings for a longer duration.
/// Each xiiStringBuilder uses a xiiHybridArray to allocate a large buffer on the stack, such that string manipulations
/// are possible without memory allocations, unless the string is too large.
/// No sharing of data happens between xiiStringBuilder instances, as it is expected that they will be modified anyway.
/// Instead all data is always copied, therefore instances should not be passed by copy.
/// All string data is stored Utf8 encoded, just as all other string classes, too.
/// That makes it difficult to modify individual characters. Instead you should prefer high-level functions
/// such as 'ReplaceSubString'. If individual characters must be modified, it might make more sense to create
/// a second xiiStringBuilder, and iterate over the first while rebuilding the desired result in the second.
/// Once a string is built and should only be stored for read access, it should be stored in a xiiString instance.
class XII_FOUNDATION_DLL xiiStringBuilder : public xiiStringBase<xiiStringBuilder>
{
public:
  /// Initializes the string to be empty. No data is allocated, but the xiiStringBuilder ALWAYS creates an array on the stack.
  xiiStringBuilder(xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// Copies the given string into this one.
  xiiStringBuilder(const xiiStringBuilder& rhs); // [tested]

  /// Moves the given string into this one.
  xiiStringBuilder(xiiStringBuilder&& rhs) noexcept;

  /// Copies the given string into this one.
  template <xiiUInt16 Size>
  xiiStringBuilder(const xiiHybridStringBase<Size>& rhs) :
    m_Data(rhs.m_Data)
  {
  }

  /// Copies the given string into this one.
  template <xiiUInt16 Size, typename A>
  xiiStringBuilder(const xiiHybridString<Size, A>& rhs) :
    m_Data(rhs.m_Data)
  {
  }


  /// Moves the given string into this one.
  template <xiiUInt16 Size>
  xiiStringBuilder(xiiHybridStringBase<Size>&& rhs) :
    m_Data(std::move(rhs.m_Data))
  {
  }

  /// Moves the given string into this one.
  template <xiiUInt16 Size, typename A>
  xiiStringBuilder(xiiHybridString<Size, A>&& rhs) :
    m_Data(std::move(rhs.m_Data))
  {
  }

  /// Constructor that appends all the given strings.
  xiiStringBuilder(xiiStringView sData1, xiiStringView sData2, xiiStringView sData3 = {}, xiiStringView sData4 = {}, xiiStringView sData5 = {}, xiiStringView sData6 = {}); // [tested]

  /// Copies the given Utf8 string into this one.
  /* implicit */ xiiStringBuilder(const char* szUTF8, xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// Copies the given wchar_t string into this one.
  /* implicit */ xiiStringBuilder(const wchar_t* pWChar, xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// Copies the given substring into this one. The xiiStringView might actually be a substring of this very string.
  /* implicit */ xiiStringBuilder(xiiStringView rhs, xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// Copies the given string into this one.
  void operator=(const xiiStringBuilder& rhs); // [tested]

  /// Moves the given string into this one.
  void operator=(xiiStringBuilder&& rhs) noexcept;

  /// Copies the given Utf8 string into this one.
  void operator=(const char* szUTF8); // [tested]

  /// Copies the given wchar_t string into this one.
  void operator=(const wchar_t* pWChar); // [tested]

  /// Copies the given substring into this one. The xiiStringView might actually be a substring of this very string.
  void operator=(xiiStringView rhs); // [tested]

  /// Copies the given string into this one.
  template <xiiUInt16 Size>
  void operator=(const xiiHybridStringBase<Size>& rhs)
  {
    m_Data = rhs.m_Data;
  }

  /// Copies the given string into this one.
  template <xiiUInt16 Size, typename A>
  void operator=(const xiiHybridString<Size, A>& rhs)
  {
    m_Data = rhs.m_Data;
  }

  /// Moves the given string into this one.
  template <xiiUInt16 Size>
  void operator=(xiiHybridStringBase<Size>&& rhs)
  {
    m_Data = std::move(rhs.m_Data);
  }

  /// Moves the given string into this one.
  template <xiiUInt16 Size, typename A>
  void operator=(xiiHybridString<Size, A>&& rhs) noexcept
  {
    m_Data = std::move(rhs.m_Data);
  }

  /// Returns the allocator that is used by this object.
  xiiAllocator* GetAllocator() const;

  /// Resets this string to be empty. Does not deallocate any previously allocated data, as it might be reused later again.
  void Clear(); // [tested]

  /// Returns a char pointer to the internal Utf8 data.
  const char* GetData() const; // [tested]

  /// Returns the number of bytes that this string takes up.
  xiiUInt32 GetElementCount() const; // [tested]

  /// Returns the number of characters of which this string consists. Might be less than GetElementCount, if it contains Utf8
  /// multi-byte characters.
  ///
  /// \note This is a slow operation, as it has to run through the entire string to count the Unicode characters.
  /// Only call this once and use the result as long as the string doesn't change. Don't call this in a loop.
  xiiUInt32 GetCharacterCount() const; // [tested]

  /// Converts all characters to upper case. Might move the string data around, so all iterators to the data will be invalid
  /// afterwards.
  void ToUpper(); // [tested]

  /// Converts all characters to lower case. Might move the string data around, so all iterators to the data will be invalid
  /// afterwards.
  void ToLower(); // [tested]

  /// Changes the single character in this string, to which the iterator currently points.
  ///
  /// The string might need to be moved around, if its encoding size changes, however the given iterator will be adjusted
  /// so that it will always stay valid.
  /// \note
  /// This can be a very costly operation (unless this string is pure ASCII).
  /// It is only provided for the few rare cases where it is more convenient and performance is not of concern.
  /// If possible, do not use this function, at all.
  void ChangeCharacter(iterator& ref_it, xiiUInt32 uiCharacter); // [tested]

  /// Sets the string to the given string.
  void Set(xiiStringView sData1); // [tested]
  /// Sets the string by concatenating all given strings.
  void Set(xiiStringView sData1, xiiStringView sData2); // [tested]
  /// Sets the string by concatenating all given strings.
  void Set(xiiStringView sData1, xiiStringView sData2, xiiStringView sData3); // [tested]
  /// Sets the string by concatenating all given strings.
  void Set(xiiStringView sData1, xiiStringView sData2, xiiStringView sData3, xiiStringView sData4); // [tested]
  /// Sets the string by concatenating all given strings.
  void Set(xiiStringView sData1, xiiStringView sData2, xiiStringView sData3, xiiStringView sData4, xiiStringView sData5, xiiStringView sData6 = {}); // [tested]

  /// Sets several path pieces. Makes sure they are always properly separated by a slash.
  void SetPath(xiiStringView sData1, xiiStringView sData2, xiiStringView sData3 = {}, xiiStringView sData4 = {});

  /// Copies the string starting at \a pStart up to \a pEnd (exclusive).
  void SetSubString_FromTo(const char* pStart, const char* pEnd);

  /// Copies the string starting at \a pStart with a length of \a uiElementCount bytes.
  void SetSubString_ElementCount(const char* pStart, xiiUInt32 uiElementCount);

  /// Copies the string starting at \a pStart with a length of \a uiCharacterCount characters.
  void SetSubString_CharacterCount(const char* pStart, xiiUInt32 uiCharacterCount);

  /// Appends a single Utf32 character.
  void Append(xiiUInt32 uiChar); // [tested]

  /// Appends all the given strings at the back of this string in one operation.
  void Append(const wchar_t* pData1, const wchar_t* pData2 = nullptr, const wchar_t* pData3 = nullptr, const wchar_t* pData4 = nullptr, const wchar_t* pData5 = nullptr, const wchar_t* pData6 = nullptr); // [tested]

  /// Appends all the given strings to the back of this string in one operation.
  void Append(xiiStringView sData1); // [tested]
  /// Appends all the given strings to the back of this string in one operation.
  void Append(xiiStringView sData1, xiiStringView sData2); // [tested]
  /// Appends all the given strings to the back of this string in one operation.
  void Append(xiiStringView sData1, xiiStringView sData2, xiiStringView sData3); // [tested]
  /// Appends all the given strings to the back of this string in one operation.
  void Append(xiiStringView sData1, xiiStringView sData2, xiiStringView sData3, xiiStringView sData4); // [tested]
  /// Appends all the given strings to the back of this string in one operation.
  void Append(xiiStringView sData1, xiiStringView sData2, xiiStringView sData3, xiiStringView sData4, xiiStringView sData5, xiiStringView sData6 = {}); // [tested]

  /// Prepends a single Utf32 character.
  void Prepend(xiiUInt32 uiChar); // [tested]

  /// Prepends all the given strings to the front of this string in one operation.
  void Prepend(const wchar_t* pData1, const wchar_t* pData2 = nullptr, const wchar_t* pData3 = nullptr, const wchar_t* pData4 = nullptr, const wchar_t* pData5 = nullptr, const wchar_t* pData6 = nullptr); // [tested]

  /// Prepends all the given strings to the front of this string in one operation.
  void Prepend(xiiStringView sData1, xiiStringView sData2 = {}, xiiStringView sData3 = {}, xiiStringView sData4 = {}, xiiStringView sData5 = {}, xiiStringView sData6 = {}); // [tested]

  /// Sets this string to the formatted string, uses printf-style formatting.
  void SetPrintf(const char* szUtf8Format, ...); // [tested]

  /// Sets this string to the formatted string, uses printf-style formatting.
  void SetPrintfArgs(const char* szUtf8Format, va_list szArgs); // [tested]

  /// Replaces this with a formatted string. Uses '{}' formatting placeholders, see xiiFormatString for details.
  void SetFormat(const xiiFormatString& string);

  /// Replaces this with a formatted string. Uses '{}' formatting placeholders, see xiiFormatString for details.
  template <typename... ARGS>
  void SetFormat(const char* szFormat, ARGS&&... args)
  {
    SetFormat(xiiFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// Appends a formatted string. Uses '{}' formatting placeholders, see xiiFormatString for details.
  void AppendFormat(const xiiFormatString& string);

  /// Appends a formatted string. Uses '{}' formatting placeholders, see xiiFormatString for details.
  template <typename... ARGS>
  void AppendFormat(const char* szFormat, ARGS&&... args)
  {
    AppendFormat(xiiFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// Prepends a formatted string. Uses '{}' formatting placeholders, see xiiFormatString for details.
  void PrependFormat(const xiiFormatString& string);

  /// Prepends a formatted string. Uses '{}' formatting placeholders, see xiiFormatString for details.
  template <typename... ARGS>
  void PrependFormat(const char* szFormat, ARGS&&... args)
  {
    PrependFormat(xiiFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// Removes the first n and last m characters from this string.
  ///
  /// This function will never reallocate data.
  /// Removing characters at the back is very cheap.
  /// Removing characters at the front needs to move data around, so can be quite costly.
  void Shrink(xiiUInt32 uiShrinkCharsFront, xiiUInt32 uiShrinkCharsBack); // [tested]

  /// Reserves uiNumElements bytes.
  void Reserve(xiiUInt32 uiNumElements); // [tested]


  /// Replaces the string that starts at szStartPos and ends at szEndPos with the string szReplaceWith.
  void ReplaceSubString(const char* szStartPos, const char* szEndPos, xiiStringView sReplaceWith); // [tested]

  /// A wrapper around ReplaceSubString. Will insert the given string at szInsertAtPos.
  void Insert(const char* szInsertAtPos, xiiStringView sTextToInsert); // [tested]

  /// A wrapper around ReplaceSubString. Will remove the substring which starts at szRemoveFromPos and ends at szRemoveToPos.
  void Remove(const char* szRemoveFromPos, const char* szRemoveToPos); // [tested]

  /// Replaces the first occurrence of szSearchFor by szReplacement. Optionally starts searching at szStartSearchAt (or the
  /// beginning).
  ///
  /// Returns the first position where szSearchFor was found, or nullptr if nothing was found (and replaced).
  const char* ReplaceFirst(xiiStringView sSearchFor, xiiStringView sReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// Case-insensitive version of ReplaceFirst.
  const char* ReplaceFirst_NoCase(xiiStringView sSearchFor, xiiStringView sReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// Replaces the last occurrence of szSearchFor by szReplacement. Optionally starts searching at szStartSearchAt (or the end).
  ///
  /// Returns the last position where szSearchFor was found, or nullptr if nothing was found (and replaced).
  const char* ReplaceLast(xiiStringView sSearchFor, xiiStringView sReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// Case-insensitive version of ReplaceLast.
  const char* ReplaceLast_NoCase(xiiStringView sSearchFor, xiiStringView sReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// Replaces all occurrences of szSearchFor by szReplacement. Returns the number of replacements.
  xiiUInt32 ReplaceAll(xiiStringView sSearchFor, xiiStringView sReplacement); // [tested]

  /// Case-insensitive version of ReplaceAll.
  xiiUInt32 ReplaceAll_NoCase(xiiStringView sSearchFor, xiiStringView sReplacement); // [tested]

  /// Replaces the first occurrence of szSearchFor by szReplaceWith, if szSearchFor was found to be a 'whole word', as indicated by
  /// the delimiter function IsDelimiterCB.
  ///
  /// Returns the start position of where the word was replaced or nullptr if nothing got replaced.
  const char* ReplaceWholeWord(const char* szSearchFor, xiiStringView sReplaceWith, xiiStringUtils::XII_CHARACTER_FILTER isDelimiterCB); // [tested]

  /// Case-insensitive version of ReplaceWholeWord.
  ///
  /// Returns the start position of where the word was replaced or nullptr if nothing got replaced.
  const char* ReplaceWholeWord_NoCase(const char* szSearchFor, xiiStringView sReplaceWith, xiiStringUtils::XII_CHARACTER_FILTER isDelimiterCB); // [tested]

  /// Replaces all occurrences of szSearchFor by szReplaceWith, if szSearchFor was found to be a 'whole word', as indicated by the
  /// delimiter function IsDelimiterCB.
  ///
  /// Returns how many words got replaced.
  xiiUInt32 ReplaceWholeWordAll(const char* szSearchFor, xiiStringView sReplaceWith, xiiStringUtils::XII_CHARACTER_FILTER isDelimiterCB); // [tested]

  /// Case-insensitive version of ReplaceWholeWordAll.
  ///
  /// Returns how many words got replaced.
  xiiUInt32 ReplaceWholeWordAll_NoCase(const char* szSearchFor, xiiStringView sReplaceWith, xiiStringUtils::XII_CHARACTER_FILTER isDelimiterCB); // [tested]

  /// Replaces the current string with the content from the stream. Reads the stream to its end.
  void ReadAll(xiiStreamReader& ref_stream);

  // ******* Path Functions ********

  /// Removes "../" where possible, replaces all path separators with /, removes double slashes.
  ///
  /// All paths use slashes on all platforms. If you need to convert a path to the OS specific representation, use
  /// 'MakePathSeparatorsNative' 'MakeCleanPath' will in rare circumstances grow the string by one character. That means it is quite safe to
  /// assume that it will not waste time on memory allocations. If it is repeatedly called on the same string, it has a minor overhead for
  /// computing the same string over and over, but no memory allocations will be done (everything is in-place).
  ///
  /// Removes all double path separators (slashes and backslashes) in a path, except if the path starts with two (back-)slashes, those are
  /// kept, as they might indicate a UNC path.
  void MakeCleanPath(); // [tested]

  /// Modifies this string to point to the parent directory.
  ///
  /// 'uiLevelsUp' can be used to go several folders upwards. It has to be at least one.
  /// If there are no more folders to go up, "../" is appended as much as needed.
  void PathParentDirectory(xiiUInt32 uiLevelsUp = 1); // [tested]

  /// Appends several path pieces. Makes sure they are always properly separated by a slash.
  void AppendPath(xiiStringView sPath1, xiiStringView sPath2 = {}, xiiStringView sPath3 = {}, xiiStringView sPath4 = {}); // [tested]

  /// Similar to Append() but the very first argument is a separator that is only appended (once) if the existing string is not empty and does
  /// not already end with the separator.
  ///
  /// This is useful when one wants to append entries that require a separator like a comma in between items. E.g. calling
  /// AppendWithSeparator(", ", "a", "b");
  /// AppendWithSeparator(", ", "c", "d");
  /// results in the string "ab, cd"
  void AppendWithSeparator(xiiStringView sSeparator, xiiStringView sText1, xiiStringView sText2 = xiiStringView(), xiiStringView sText3 = xiiStringView(), xiiStringView sText4 = xiiStringView(), xiiStringView sText5 = xiiStringView(), xiiStringView sText6 = xiiStringView());

  /// Changes the file name part of the path, keeps the extension intact (if there is any).
  void ChangeFileName(xiiStringView sNewFileName); // [tested]

  /// Changes the file name and the extension part of the path.
  void ChangeFileNameAndExtension(xiiStringView sNewFileNameWithExtension); // [tested]

  /// Only changes the file extension of the path. If there is no extension yet, one is appended (including a dot).
  ///
  /// sNewExtension may or may not start with a dot.
  /// If sNewExtension is empty, the file extension is removed, but the dot remains.
  /// E.g. "file.txt" -> "file."
  /// If you also want to remove the dot, use RemoveFileExtension() instead.
  ///
  /// If bFullExtension is false, a file named "file.a.b.c" will replace only "c".
  /// If bFullExtension is true, a file named "file.a.b.c" will replace all of "a.b.c".
  void ChangeFileExtension(xiiStringView sNewExtension, bool bFullExtension = false); // [tested]

  /// If any extension exists, it is removed, including the dot before it.
  ///
  /// If bFullExtension is false, a file named "file.a.b.c" will end up as "file.a.b"
  /// If bFullExtension is true, a file named "file.a.b.c" will end up as "file"
  void RemoveFileExtension(bool bFullExtension = false); // [tested]

  /// Converts this path into a relative path to the path with the awesome variable name 'szAbsolutePathToMakeThisRelativeTo'
  ///
  /// If the method succeeds the StringBuilder's contents are modified in place.
  xiiResult MakeRelativeTo(xiiStringView sAbsolutePathToMakeThisRelativeTo); // [tested]

  /// Cleans this path up and replaces all path separators by the OS specific separator.
  ///
  /// This can be used, if you want to present paths in the OS specific form to the user in the UI.
  /// In all other cases the internal representation uses slashes, no matter on which operating system.
  void MakePathSeparatorsNative(); // [tested]

  /// Checks whether this path is a sub-path of the given path.
  ///
  /// This function will call 'MakeCleanPath' to be able to compare both paths, thus it might modify the data of this instance.
  bool IsPathBelowFolder(const char* szPathToFolder); // [tested]

  /// Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); }

  /// Removes all characters from the start and end that appear in the given strings.
  ///
  /// The default string removes all standard whitespace characters.
  void Trim(const char* szTrimChars = " \f\n\r\t\v"); // [tested]

  /// Removes all characters from the start and/or end that appear in the given strings.
  void Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd); // [tested]

  /// Removes all characters from the start that appear in the given strings.
  ///
  /// The default string removes all standard whitespace characters.
  void TrimLeft(const char* szTrimChars = " \f\n\r\t\v");

  /// Removes all characters from the end that appear in the given strings.
  ///
  /// The default string removes all standard whitespace characters.
  void TrimRight(const char* szTrimChars = " \f\n\r\t\v");

  /// If the string starts with the given word (case insensitive), it is removed and the function returns true.
  bool TrimWordStart(xiiStringView sWord); // [tested]

  /// If the string ends with the given word (case insensitive), it is removed and the function returns true.
  bool TrimWordEnd(xiiStringView sWord); // [tested]

#if XII_ENABLED(XII_INTEROP_STL_STRINGS)
  /// Copies the given substring into this one. The xiiStringView might actually be a substring of this very string.
  /* implicit */ xiiStringBuilder(const std::string_view& rhs, xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator());

  /// Copies the given substring into this one. The xiiStringView might actually be a substring of this very string.
  /* implicit */ xiiStringBuilder(const std::string& rhs, xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator());

  /// Copies the given substring into this one. The xiiStringView might actually be a substring of this very string.
  void operator=(const std::string_view& rhs);

  /// Copies the given substring into this one. The xiiStringView might actually be a substring of this very string.
  void operator=(const std::string& rhs);
#endif

private:
  /// Will remove all double path separators (slashes and backslashes) in a path, except if the path starts with two (back-)slashes,
  /// those are kept, as they might indicate a UNC path.
  void RemoveDoubleSlashesInPath(); // [tested]

  void ChangeCharacterNonASCII(iterator& it, xiiUInt32 uiCharacter);
  void AppendTerminator();

  // needed for better copy construction
  template <xiiUInt16 T>
  friend struct xiiHybridStringBase;

  friend xiiStreamReader;

  xiiHybridArray<char, 128> m_Data;
};

#include <Foundation/Strings/Implementation/StringBuilder_inl.h>
