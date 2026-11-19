/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <Foundation/Containers/Map.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Variant.h>

enum class xiiMCPCapability : xiiUInt8
{
  Tool,
  Resource,
  ResourceTemplate,
  Prompt,
  Count
};

enum class xiiMCPPermission : xiiUInt8
{
  Read           = 1,
  Capture        = 2,
  Edit           = 4,
  Execute        = 8,
  Administration = 16
};

struct XII_MCPPLUGIN_DLL xiiMCPCallContext
{
  xiiUInt8             m_uiPermissions = static_cast<xiiUInt8>(xiiMCPPermission::Read);
  xiiTime              m_Deadline;
  const xiiAtomicBool* m_pCanceled = nullptr; ///< Borrowed for the duration of local invocation only.
  xiiStringView        m_sIdentity;

  [[nodiscard]] bool Allows(xiiMCPPermission permission) const;
  [[nodiscard]] bool HasExpired() const;
};

/// Provider registration, enumeration and callbacks belong to one application thread. A plugin
/// must remove its entries on that thread before unloading. Callbacks may not change registrations.
class XII_MCPPLUGIN_DLL xiiMCPRegistry final
{
public:
  using Callback = xiiDelegate<xiiStatus(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary&)>;

  static constexpr xiiUInt32 MaximumEntriesPerKind = 256;

  [[nodiscard]] xiiResult Register(xiiMCPCapability kind, xiiStringView sKey, xiiVariantDictionary descriptor, xiiMCPPermission permission, Callback callback);
  [[nodiscard]] xiiResult RegisterTool(xiiStringView sName, xiiStringView sDescription, xiiStringView sInputSchema, xiiMCPPermission permission, Callback callback);
  void                    Remove(xiiMCPCapability kind, xiiStringView sKey);

  void                    List(xiiMCPCapability kind, const xiiMCPCallContext& context, xiiVariantArray& out_entries) const;
  [[nodiscard]] bool      Contains(xiiMCPCapability kind, xiiStringView sKey, const xiiMCPCallContext& context) const;
  [[nodiscard]] xiiStatus Invoke(xiiMCPCapability kind, xiiStringView sKey, const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result) const;
  [[nodiscard]] xiiStatus ReadResource(xiiStringView sUri, const xiiMCPCallContext& context, xiiVariantDictionary& out_result) const;
  [[nodiscard]] xiiUInt64 GetRevision() const { return m_uiRevision; }
  /// Publish a binding-side authorization change to catalog consumers.
  void NotifyAvailabilityChanged() { ++m_uiRevision; }

private:
  struct Entry
  {
    xiiVariantDictionary m_Descriptor;
    xiiMCPPermission     m_Permission = xiiMCPPermission::Read;
    Callback             m_Callback;
  };

  using Entries = xiiMap<xiiString, Entry>;

  [[nodiscard]] const Entry* Find(xiiMCPCapability kind, xiiStringView sKey, const xiiMCPCallContext& context) const;

  Entries   m_Entries[static_cast<xiiUInt8>(xiiMCPCapability::Count)];
  xiiUInt64 m_uiRevision = 1;
};
