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

  bool SplitResourceTemplate(xiiStringView sTemplate, xiiStringView& out_sPrefix, xiiStringView& out_sVariable)
  {
    const char* pVariableStart = sTemplate.FindSubString("{");
    if (pVariableStart == nullptr || pVariableStart == sTemplate.GetStartPointer() || pVariableStart[-1] != '/' || !sTemplate.EndsWith("}"))
    {
      return false;
    }
    out_sPrefix   = xiiStringView(sTemplate.GetStartPointer(), pVariableStart);
    out_sVariable = xiiStringView(pVariableStart + 1, sTemplate.GetEndPointer() - 1);
    return IsName(out_sVariable);
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

xiiMCPInvocation::~xiiMCPInvocation() = default;

xiiResult xiiMCPRegistry::Register(xiiMCPCapability kind, xiiStringView sKey, xiiVariantDictionary descriptor, xiiMCPPermission permission, Callback callback)
{
  return RegisterEntry(kind, sKey, Entry{std::move(descriptor), permission, std::move(callback), {}});
}

xiiResult xiiMCPRegistry::RegisterDeferred(xiiMCPCapability kind, xiiStringView sKey, xiiVariantDictionary descriptor, xiiMCPPermission permission, DeferredCallback callback)
{
  return RegisterEntry(kind, sKey, Entry{std::move(descriptor), permission, {}, std::move(callback)});
}

xiiResult xiiMCPRegistry::RegisterEntry(xiiMCPCapability kind, xiiStringView sKey, Entry&& entry)
{
  if (kind >= xiiMCPCapability::Count || (!entry.m_Callback.IsValid() && !entry.m_Deferred.IsValid()))
  {
    return XII_FAILURE;
  }

  auto& entries = m_Entries[static_cast<xiiUInt8>(kind)];
  if (entries.Contains(sKey) || entries.GetCount() >= MaximumEntriesPerKind)
  {
    return XII_FAILURE;
  }

  const bool bNamed     = kind == xiiMCPCapability::Tool || kind == xiiMCPCapability::Prompt;
  const bool bTemplate  = kind == xiiMCPCapability::ResourceTemplate;
  auto&      descriptor = entry.m_Descriptor;
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
  if (bTemplate)
  {
    xiiStringView sPrefix, sVariable;
    if (!SplitResourceTemplate(sKey, sPrefix, sVariable))
    {
      return XII_FAILURE;
    }
  }

  descriptor[bNamed ? "name" : bTemplate ? "uriTemplate" :
                                           "uri"] = xiiString(sKey);
  xiiVariantDictionary metadata;
  if (const auto* pMetadata = xiiMCPJson::GetObject(descriptor, "_meta"))
  {
    metadata = *pMetadata;
  }
  metadata["xii/permission"] = static_cast<xiiUInt8>(entry.m_Permission);
  descriptor["_meta"]        = metadata;
  xiiStringBuilder sDescriptor;
  if (xiiMCPJson::Write(descriptor, sDescriptor, 8 * 1024).Failed())
  {
    return XII_FAILURE;
  }

  entries.Insert(xiiString(sKey), std::move(entry));
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

xiiStatus xiiMCPRegistry::InvokeEntry(const Entry& entry, const xiiMCPCallContext& context, const xiiVariantDictionary& arguments,
                                      xiiVariantDictionary& out_result, xiiUniquePtr<xiiMCPInvocation>* pPending)
{
  if (entry.m_Callback.IsValid())
  {
    return entry.m_Callback(context, arguments, out_result);
  }
  if (pPending == nullptr)
  {
    return xiiStatus("No deferred request slot is available.");
  }

  xiiUniquePtr<xiiMCPInvocation> invocation;
  XII_SUCCEED_OR_RETURN(entry.m_Deferred(context, arguments, invocation));
  if (!invocation)
  {
    return xiiStatus("The capability did not start an invocation.");
  }
  *pPending = std::move(invocation);
  return XII_SUCCESS;
}

xiiStatus xiiMCPRegistry::Invoke(xiiMCPCapability kind, xiiStringView sKey, const xiiMCPCallContext& context, const xiiVariantDictionary& arguments,
                                 xiiVariantDictionary& out_result, xiiUniquePtr<xiiMCPInvocation>* pPending) const
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

  return InvokeEntry(*pEntry, context, arguments, out_result, pPending);
}

xiiStatus xiiMCPRegistry::ReadResource(xiiStringView sUri, const xiiMCPCallContext& context, xiiVariantDictionary& out_result,
                                       xiiUniquePtr<xiiMCPInvocation>* pPending) const
{
  if (!IsUri(sUri) || context.HasExpired())
  {
    return xiiStatus("Provide a valid resource URI before the request deadline.");
  }

  xiiVariantDictionary arguments;
  arguments["uri"] = xiiString(sUri);
  if (const auto* pEntry = Find(xiiMCPCapability::Resource, sUri, context))
  {
    return InvokeEntry(*pEntry, context, arguments, out_result, pPending);
  }

  const auto& templates = m_Entries[static_cast<xiiUInt8>(xiiMCPCapability::ResourceTemplate)];
  for (auto it = templates.GetIterator(); it.IsValid(); ++it)
  {
    xiiStringView sPrefix, sVariable;
    if (!context.Allows(it.Value().m_Permission) || !SplitResourceTemplate(it.Key(), sPrefix, sVariable) || !sUri.StartsWith(sPrefix))
    {
      continue;
    }
    const auto sValue = xiiStringView(sUri.GetStartPointer() + sPrefix.GetElementCount(), sUri.GetEndPointer());
    if (IsName(sValue) && sVariable != "uri")
    {
      arguments[sVariable] = xiiString(sValue);
      return InvokeEntry(it.Value(), context, arguments, out_result, pPending);
    }
  }
  return xiiStatus("The resource is unavailable or requires additional permission.");
}
