/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <MCPPlugin/Json.h>
#include <MCPPlugin/Registry.h>

namespace
{
  bool IsName(xiiStringView sName)
  {
    if (sName.IsEmpty() || sName.GetElementCount() > 128)
    {
      return false;
    }

    for (const char* pCharacter = sName.GetStartPointer(); pCharacter < sName.GetEndPointer(); ++pCharacter)
    {
      const char character = *pCharacter;
      if ((character < 'A' || character > 'Z') && (character < 'a' || character > 'z') && (character < '0' || character > '9') &&
          character != '-' && character != '_' && character != '.')
      {
        return false;
      }
    }
    return true;
  }

  bool IsUri(xiiStringView sUri)
  {
    if (sUri.IsEmpty() || sUri.GetElementCount() > 512 || !sUri.FindSubString(":"))
    {
      return false;
    }

    for (const char* pCharacter = sUri.GetStartPointer(); pCharacter < sUri.GetEndPointer(); ++pCharacter)
    {
      if (*pCharacter < '!' || *pCharacter > '~')
      {
        return false;
      }
    }
    return true;
  }
} // namespace

bool xiiMCPCallContext::Allows(xiiMCPPermission permission) const
{
  return (m_uiPermissions & static_cast<xiiUInt8>(permission)) != 0;
}

bool xiiMCPCallContext::HasExpired() const
{
  return m_Deadline.IsPositive() && xiiTime::Now() >= m_Deadline;
}

xiiResult xiiMCPRegistry::Register(xiiMCPCapability kind, xiiStringView sKey, xiiVariantDictionary descriptor, xiiMCPPermission permission, Callback callback)
{
  if (kind >= xiiMCPCapability::Count || !callback.IsValid())
  {
    return XII_FAILURE;
  }

  auto& entries = m_Entries[static_cast<xiiUInt8>(kind)];
  if (entries.Contains(sKey) || entries.GetCount() >= MaximumEntriesPerKind)
  {
    return XII_FAILURE;
  }

  const bool bNamed = kind == xiiMCPCapability::Tool || kind == xiiMCPCapability::Prompt;
  const bool bTemplate = kind == xiiMCPCapability::ResourceTemplate;
  if (bNamed ? !IsName(sKey) : (!IsUri(sKey) || xiiMCPJson::GetString(descriptor, "name").IsEmpty()))
  {
    return XII_FAILURE;
  }

  if (kind == xiiMCPCapability::Tool)
  {
    const auto* pSchema = xiiMCPJson::GetObject(descriptor, "inputSchema");
    if (pSchema == nullptr || xiiMCPJson::GetString(*pSchema, "type") != "object")
    {
      return XII_FAILURE;
    }
  }

  descriptor[bNamed ? "name" : bTemplate ? "uriTemplate" : "uri"] = xiiString(sKey);
  xiiStringBuilder sDescriptor;
  if (xiiMCPJson::Write(descriptor, sDescriptor, 8 * 1024).Failed())
  {
    return XII_FAILURE;
  }

  entries.Insert(xiiString(sKey), Entry{std::move(descriptor), permission, std::move(callback)});
  ++m_uiRevision;
  return XII_SUCCESS;
}

void xiiMCPRegistry::Remove(xiiMCPCapability kind, xiiStringView sKey)
{
  if (kind < xiiMCPCapability::Count && m_Entries[static_cast<xiiUInt8>(kind)].Remove(sKey))
  {
    ++m_uiRevision;
  }
}

xiiResult xiiMCPRegistry::RegisterTool(xiiStringView sName, xiiStringView sDescription, xiiStringView sInputSchema, xiiMCPPermission permission, Callback callback)
{
  xiiVariant schema;
  XII_SUCCEED_OR_RETURN(xiiMCPJson::Parse(sInputSchema, schema, 8 * 1024));
  xiiVariantDictionary descriptor;
  descriptor["description"] = xiiString(sDescription);
  descriptor["inputSchema"] = std::move(schema);
  return Register(xiiMCPCapability::Tool, sName, std::move(descriptor), permission, std::move(callback));
}

void xiiMCPRegistry::List(xiiMCPCapability kind, const xiiMCPCallContext& context, xiiVariantArray& out_entries) const
{
  out_entries.Clear();
  if (kind >= xiiMCPCapability::Count)
  {
    return;
  }

  for (auto it = m_Entries[static_cast<xiiUInt8>(kind)].GetIterator(); it.IsValid(); ++it)
  {
    if (context.Allows(it.Value().m_Permission))
    {
      out_entries.PushBack(it.Value().m_Descriptor);
    }
  }
}

const xiiMCPRegistry::Entry* xiiMCPRegistry::Find(xiiMCPCapability kind, xiiStringView sKey, const xiiMCPCallContext& context) const
{
  if (kind >= xiiMCPCapability::Count)
  {
    return nullptr;
  }

  const auto it = m_Entries[static_cast<xiiUInt8>(kind)].Find(sKey);
  return it.IsValid() && context.Allows(it.Value().m_Permission) ? &it.Value() : nullptr;
}

bool xiiMCPRegistry::Contains(xiiMCPCapability kind, xiiStringView sKey, const xiiMCPCallContext& context) const
{
  return Find(kind, sKey, context) != nullptr;
}

xiiStatus xiiMCPRegistry::Invoke(xiiMCPCapability kind, xiiStringView sKey, const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result) const
{
  if (context.HasExpired())
  {
    return xiiStatus("The request deadline expired.");
  }

  const auto* pEntry = Find(kind, sKey, context);
  if (pEntry == nullptr)
  {
    return xiiStatus("The capability is unavailable or requires additional permission.");
  }

  return pEntry->m_Callback(context, arguments, out_result);
}
