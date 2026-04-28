/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Utilities/SearchPatternFilter.h>

void xiiSearchPatternFilter::SetSearchText(xiiStringView sSearchText)
{
  m_sSearchText = sSearchText;
  m_Parts.Clear();

  xiiHybridArray<xiiStringView, 4> searchParts;
  m_sSearchText.Split(false, searchParts, " ");

  for (auto& searchPart : searchParts)
  {
    auto& part   = m_Parts.ExpandAndGetRef();
    part.m_sPart = searchPart;
    if (part.m_sPart.StartsWith("-"))
    {
      part.m_sPart.Shrink(1, 0);
      part.m_bExclude = true;
    }
  }
}

bool xiiSearchPatternFilter::ContainsExclusions() const
{
  for (auto& part : m_Parts)
  {
    if (part.m_bExclude)
      return true;
  }

  return false;
}

bool xiiSearchPatternFilter::PassesFilters(xiiStringView sText) const
{
  for (auto& part : m_Parts)
  {
    bool failureResult = part.m_bExclude;
    if ((sText.FindSubString_NoCase(part.m_sPart) != nullptr) == failureResult)
      return false;
  }

  return true;
}
