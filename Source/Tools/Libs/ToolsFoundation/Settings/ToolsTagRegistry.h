#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct XII_TOOLSFOUNDATION_DLL xiiToolsTag
{
  xiiToolsTag() = default;
  xiiToolsTag(const char* szCategory, const char* szName, bool bBuiltIn = false) :
    m_sCategory(szCategory), m_sName(szName), m_bBuiltInTag(bBuiltIn)
  {
  }

  xiiString m_sCategory;
  xiiString m_sName;
  bool      m_bBuiltInTag = false; ///< If set to true, this is a tag created by code that the user is not allowed to remove
};

class XII_TOOLSFOUNDATION_DLL xiiToolsTagRegistry
{
public:
  /// \brief Removes all tags that are not specified as 'built-in'
  static void Clear();

  static void      WriteToDDL(xiiStreamWriter& ref_stream);
  static xiiStatus ReadFromDDL(xiiStreamReader& ref_stream);

  static bool AddTag(const xiiToolsTag& tag);
  static bool RemoveTag(const char* szName);

  static void GetAllTags(xiiHybridArray<const xiiToolsTag*, 16>& out_tags);
  static void GetTagsByCategory(const xiiArrayPtr<xiiStringView>& categories, xiiHybridArray<const xiiToolsTag*, 16>& out_tags);

private:
  static xiiMap<xiiString, xiiToolsTag> s_NameToTags;
};
