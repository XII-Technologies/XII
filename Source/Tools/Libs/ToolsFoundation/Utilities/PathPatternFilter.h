/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/Strings/String.h>

/// Describes a single path pattern for filtering file paths.
///
/// A path pattern is something like "*.jpg", "SubFolder/*" or "*/temp/*".
/// It may start or end with a * indicating that it matches paths that start with, end with, or contain the pattern.
/// If no * is present, the pattern has to match exactly.
struct XII_TOOLSFOUNDATION_DLL xiiPathPattern
{
  enum MatchType : xiiUInt8
  {
    Exact,
    StartsWith,
    EndsWith,
    Contains
  };

  MatchType m_MatchType = MatchType::Exact;
  xiiString m_sString;

  /// Sets up the pattern from the given text. Whitespace is trimmed.
  void Configure(const xiiStringView sText);

  /// Returns true if the given text matches this path pattern.
  bool Matches(const xiiStringView sText) const;
};

/// A collection of xiiPathPatterns for include/exclude filtering of file paths.
struct XII_TOOLSFOUNDATION_DLL xiiPathPatternFilter
{
  xiiDynamicArray<xiiPathPattern> m_ExcludePatterns;
  xiiDynamicArray<xiiPathPattern> m_IncludePatterns;

  /// Reads all patterns from the given file.
  ///
  /// The file is parsed with a xiiPreprocessor, so may contain #include statements and such.
  /// Custom preprocessor definitions can be provided.
  ///
  /// After preprocessing, every line represents a single pattern.
  /// Lines that contain '[INCLUDE]' or '[EXCLUDE]' are special and change whether the
  /// following lines are considered as include patterns or exclude patterns.
  xiiResult ReadConfigFile(xiiStringView sFile, const xiiDynamicArray<xiiString>& preprocessorDefines);

  /// Adds a pattern as either an include or exclude filter.
  void AddFilter(xiiStringView sText, bool bIncludeFilter);

  /// Determines whether the given text matches the filter patterns.
  ///
  /// Include patterns take precedence over exclude patterns.
  /// If the text matches any include pattern, it passes the filter.
  /// Otherwise, if it matches any exclude pattern, it does not pass the filter.
  /// Otherwise, if it doesn't match any include or exclude pattern, it passes the filter, since it isn't explicitely ruled out.
  bool PassesFilters(xiiStringView sText, xiiStringBuilder* pMatchingFilter = nullptr) const;
};
