/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/Strings/String.h>

/// A helper class to implement a multi-part, case-insensitive search pattern filter with support for exclusions.
///
/// The search text is split into multiple parts by spaces. A text passes the filter if it contains all parts.
/// The check is always case insensitive and the order of the parts does not matter.
/// It is also possible to exclude parts by prefixing them with a minus.
/// E.g. "com mesh" would pass all texts that contain "com" and "mesh" like xiiMeshComponent.
/// "com -mesh" would pass xiiLightComponent but would fail xiiMeshComponent.
class XII_TOOLSFOUNDATION_DLL xiiSearchPatternFilter
{
public:
  /// Sets the search text and splits it into its part for faster checks.
  void SetSearchText(xiiStringView sSearchText);

  /// Returns the current search text.
  const xiiString& GetSearchText() const { return m_sSearchText; }

  /// Returns true if the search text is empty.
  bool IsEmpty() const { return m_sSearchText.IsEmpty(); }

  /// Returns true if the filter contains any exclusion patterns.
  bool ContainsExclusions() const;

  /// Determines whether the given text matches the filter patterns.
  bool PassesFilters(xiiStringView sText) const;

private:
  xiiString m_sSearchText;

  struct Part
  {
    xiiStringView m_sPart;
    bool          m_bExclude = false;
  };

  xiiHybridArray<Part, 4> m_Parts;
};
