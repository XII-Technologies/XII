/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <Foundation/Containers/Map.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
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

/// A provider operation driven by the owning application thread.
/// Destroying an unfinished invocation requests cancellation and releases its result.
class XII_MCPPLUGIN_DLL xiiMCPInvocation
{
public:
  virtual ~xiiMCPInvocation();

  /// Returns true when the operation is complete and consumes its status and result.
  [[nodiscard]] virtual bool Poll(xiiStatus& out_status, xiiVariantDictionary& out_result) = 0;
};

/// Provider registration, enumeration and callbacks belong to one application thread. A plugin
/// must remove its entries on that thread before unloading. Callbacks may not change registrations.
class XII_MCPPLUGIN_DLL xiiMCPRegistry final
{
public:
  using Callback         = xiiDelegate<xiiStatus(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary&)>;
  using DeferredCallback = xiiDelegate<xiiStatus(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiUniquePtr<xiiMCPInvocation>&)>;

  static constexpr xiiUInt32 MaximumEntriesPerKind = 256;

  [[nodiscard]] xiiResult Register(xiiMCPCapability kind, xiiStringView sKey, xiiVariantDictionary descriptor, xiiMCPPermission permission, Callback callback);
  [[nodiscard]] xiiResult RegisterDeferred(xiiMCPCapability kind, xiiStringView sKey, xiiVariantDictionary descriptor, xiiMCPPermission permission, DeferredCallback callback);
  [[nodiscard]] xiiResult RegisterTool(xiiStringView sName, xiiStringView sDescription, xiiStringView sInputSchema, xiiMCPPermission permission, Callback callback);
  void                    Remove(xiiMCPCapability kind, xiiStringView sKey);

  void               List(xiiMCPCapability kind, const xiiMCPCallContext& context, xiiVariantArray& out_entries) const;
  [[nodiscard]] bool Contains(xiiMCPCapability kind, xiiStringView sKey, const xiiMCPCallContext& context) const;
  /// A deferred capability transfers its invocation to pPending on success. The caller must poll or destroy it before unloading the provider.
  [[nodiscard]] xiiStatus Invoke(xiiMCPCapability kind, xiiStringView sKey, const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result,
                                 xiiUniquePtr<xiiMCPInvocation>* pPending = nullptr) const;
  [[nodiscard]] xiiStatus ReadResource(xiiStringView sUri, const xiiMCPCallContext& context, xiiVariantDictionary& out_result,
                                       xiiUniquePtr<xiiMCPInvocation>* pPending = nullptr) const;
  [[nodiscard]] xiiUInt64 GetRevision() const { return m_uiRevision; }
  /// Publish a binding-side authorization change to catalog consumers.
  void NotifyAvailabilityChanged() { ++m_uiRevision; }

private:
  struct Entry
  {
    xiiVariantDictionary m_Descriptor;
    xiiMCPPermission     m_Permission = xiiMCPPermission::Read;
    Callback             m_Callback;
    DeferredCallback     m_Deferred;
  };

  using Entries = xiiMap<xiiString, Entry>;

  [[nodiscard]] xiiResult        RegisterEntry(xiiMCPCapability kind, xiiStringView sKey, Entry&& entry);
  [[nodiscard]] static xiiStatus InvokeEntry(const Entry& entry, const xiiMCPCallContext& context, const xiiVariantDictionary& arguments,
                                             xiiVariantDictionary& out_result, xiiUniquePtr<xiiMCPInvocation>* pPending);
  [[nodiscard]] const Entry*     Find(xiiMCPCapability kind, xiiStringView sKey, const xiiMCPCallContext& context) const;

  Entries   m_Entries[static_cast<xiiUInt8>(xiiMCPCapability::Count)];
  xiiUInt64 m_uiRevision = 1;
};
