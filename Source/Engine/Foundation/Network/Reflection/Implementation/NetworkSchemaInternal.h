/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/Reflection/NetworkAttributes.h>
#include <Foundation/Network/Reflection/NetworkTypeRegistry.h>
#include <Foundation/Threading/ConditionVariable.h>
#include <Foundation/Threading/Mutex.h>
#include <atomic>

XII_FOUNDATION_INTERNAL_HEADER

namespace xiiNetworkReflectionInternal
{
  struct CompiledSchema;
  struct CompiledValue
  {
    const xiiRTTI*                      m_pType   = nullptr;
    CompiledSchema*                     m_pNested = nullptr;
    xiiSharedPtr<xiiNetworkCustomCodec> m_pCodec;
    xiiNetworkValueKind                 m_Kind         = xiiNetworkValueKind::Invalid;
    bool                                m_bEnumeration = false;
    bool                                m_bBitflags    = false;
    xiiUInt64                           m_uiEnumMask   = 0;
    xiiDynamicArray<xiiInt64>           m_EnumValues;
  };
  struct CompiledField
  {
    const xiiAbstractProperty* m_pProperty = nullptr;
    CompiledValue              m_Value;
    xiiNetworkFieldSchema      m_Info;
  };
  struct CompiledSchema : xiiRefCountingImpl
  {
    xiiSharedPtr<xiiNetworkSchema> m_pInfo;
    xiiDynamicArray<CompiledField> m_Fields;
  };
  xiiNetworkValueKind GetValueKind(const xiiRTTI* pType);
  xiiUInt64           TypeId(const xiiRTTI* pType);
  xiiUInt64           HashBytes(xiiArrayPtr<const xiiUInt8> bytes);
  bool                IsQuantizable(xiiNetworkValueKind kind);
} // namespace xiiNetworkReflectionInternal

class xiiNetworkTypeRegistryState
{
public:
  void                OnPluginEvent(const xiiPluginEvent& event);
  void                Pause();
  void                Release();
  bool                Acquire();
  xiiNetworkDataError Build();
  xiiNetworkDataError BuildSchema(const xiiRTTI* pType, xiiNetworkReflectionInternal::CompiledSchema*& out_pSchema);
  xiiNetworkDataError CompileValue(const xiiRTTI* pType, xiiBitflags<xiiPropertyFlags> flags, xiiUInt64 uiCodec,
                                   xiiNetworkReflectionInternal::CompiledValue& out_value);

  xiiMutex                                                                                 m_MutationMutex;
  bool                                                                                     m_bPluginChanging = false;
  xiiConditionVariable                                                                     m_Drain;
  std::atomic<bool>                                                                        m_bPaused{true};
  std::atomic<xiiUInt32>                                                                   m_uiReaders{0};
  xiiHashTable<const xiiRTTI*, xiiSharedPtr<xiiNetworkReflectionInternal::CompiledSchema>> m_Schemas;
  xiiHashTable<xiiUInt64, const xiiRTTI*>                                                  m_Ids;
  xiiHashTable<const xiiRTTI*, xiiNetworkDataError>                                        m_Errors;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiNetworkCustomCodec>>                             m_Codecs;
  xiiHybridArray<const xiiRTTI*, 16>                                                       m_BuildStack;
  std::atomic<xiiUInt32>                                                                   m_uiSchemaCount{0};
  std::atomic<xiiUInt32>                                                                   m_uiFailureCount{0};
  std::atomic<xiiInt64>                                                                    m_iConstructionMicroseconds{0};
  std::atomic<xiiUInt64>                                                                   m_uiGeneration{0};
  std::atomic<xiiUInt64>                                                                   m_uiEncoded{0}, m_uiDecoded{0}, m_uiEncodeFailures{0}, m_uiDecodeFailures{0};
  std::atomic<xiiUInt64>                                                                   m_uiEncodedBytes{0}, m_uiDecodedBytes{0};
  std::atomic<xiiUInt32>                                                                   m_uiPeakEncoded{0};
};

class xiiNetworkRegistryAccess
{
public:
  static xiiNetworkTypeRegistryState& Get(const xiiNetworkTypeRegistry& registry) { return *registry.m_pState; }
};

namespace xiiNetworkReflectionInternal
{
  /// A metadata epoch permit, not a held mutex. Reentrant operations may acquire another permit.
  struct ReadScope
  {
    explicit ReadScope(xiiNetworkTypeRegistryState& state) : m_State(state), m_bAcquired(state.Acquire()) {}
    ~ReadScope()
    {
      if (m_bAcquired)
      {
        m_State.Release();
      }
    }
    ReadScope(const ReadScope&)                              = delete;
    ReadScope&                   operator=(const ReadScope&) = delete;
    xiiNetworkTypeRegistryState& m_State;
    bool                         m_bAcquired;
  };
} // namespace xiiNetworkReflectionInternal
