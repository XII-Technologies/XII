#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/MessageHandler.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>

struct xiiTypeHashTable
{
  xiiMutex                                                                                   m_Mutex;
  xiiHashTable<const char*, xiiRTTI*, xiiHashHelper<const char*>, xiiStaticAllocatorWrapper> m_Table;
};

xiiTypeHashTable* GetTypeHashTable()
{
  // Prevent static initialization hazard between first xiiRTTI instance
  // and the hash table and also make sure it is sufficiently sized before first use.
  auto CreateTable = []() -> xiiTypeHashTable* {
    xiiTypeHashTable* table = new xiiTypeHashTable();
    table->m_Table.Reserve(512);
    return table;
  };
  static xiiTypeHashTable* table = CreateTable();
  return table;
}

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiRTTI);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Reflection)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "FileSystem"
  //END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiPlugin::Events().AddEventHandler(xiiRTTI::PluginEventHandler);
    xiiRTTI::AssignPlugin("Static");
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiPlugin::Events().RemoveEventHandler(xiiRTTI::PluginEventHandler);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiRTTI::xiiRTTI(const char* szName, const xiiRTTI* pParentType, xiiUInt32 uiTypeSize, xiiUInt32 uiTypeVersion, xiiUInt32 uiVariantType, xiiBitflags<xiiTypeFlags> flags, xiiRTTIAllocator* pAllocator, xiiArrayPtr<xiiAbstractProperty*> properties, xiiArrayPtr<xiiAbstractProperty*> functions, xiiArrayPtr<xiiPropertyAttribute*> attributes, xiiArrayPtr<xiiAbstractMessageHandler*> messageHandlers, xiiArrayPtr<xiiMessageSenderInfo> messageSenders, const xiiRTTI* (*fnVerifyParent)()) :

  m_szTypeName(szName),
  m_pAllocator(pAllocator),
  m_Properties(properties),
  m_Functions(xiiMakeArrayPtr<xiiAbstractFunctionProperty*>(reinterpret_cast<xiiAbstractFunctionProperty**>(functions.GetPtr()), functions.GetCount())),
  m_Attributes(attributes),
  m_MessageHandlers(messageHandlers),

  m_MessageSenders(messageSenders),
  m_VerifyParent(fnVerifyParent)
{
  UpdateType(pParentType, uiTypeSize, uiTypeVersion, uiVariantType, flags);

  // This part is not guaranteed to always work here!
  // pParentType is (apparently) always the correct pointer to the base class BUT it is not guaranteed to have been constructed at this
  // point in time! Therefore the message handler hierarchy is initialized delayed in DispatchMessage
  //
  // However, I don't know where we could do these debug checks where they are guaranteed to be executed.
  // For now they are executed here and one might also do that in e.g. the game application
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    VerifyCorrectness();
#endif
  }

  if (m_szTypeName)
    RegisterType();
}

xiiRTTI::~xiiRTTI()
{
  if (m_szTypeName)
    UnregisterType();
}

void xiiRTTI::GatherDynamicMessageHandlers()
{
  // This cannot be done in the constructor, because the parent types are not guaranteed to be initialized at that point

  if (m_bGatheredDynamicMessageHandlers)
    return;

  m_bGatheredDynamicMessageHandlers = true;

  xiiUInt32 uiMinMsgId = xiiInvalidIndex;
  xiiUInt32 uiMaxMsgId = 0;

  const xiiRTTI* pInstance = this;
  while (pInstance != nullptr)
  {
    for (xiiUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
    {
      xiiUInt32 id = pInstance->m_MessageHandlers[i]->GetMessageId();
      uiMinMsgId   = xiiMath::Min(uiMinMsgId, id);
      uiMaxMsgId   = xiiMath::Max(uiMaxMsgId, id);
    }

    pInstance = pInstance->m_pParentType;
  }

  if (uiMinMsgId != xiiInvalidIndex)
  {
    m_uiMsgIdOffset            = uiMinMsgId;
    xiiUInt32 uiNeededCapacity = uiMaxMsgId - uiMinMsgId + 1;

    m_DynamicMessageHandlers.SetCount(uiNeededCapacity);

    pInstance = this;
    while (pInstance != nullptr)
    {
      for (xiiUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
      {
        xiiAbstractMessageHandler* pHandler = pInstance->m_MessageHandlers[i];
        xiiUInt32                  uiIndex  = pHandler->GetMessageId() - m_uiMsgIdOffset;

        // this check ensures that handlers in base classes do not override the derived handlers
        if (m_DynamicMessageHandlers[uiIndex] == nullptr)
        {
          m_DynamicMessageHandlers[uiIndex] = pHandler;
        }
      }

      pInstance = pInstance->m_pParentType;
    }
  }
}

void xiiRTTI::SetupParentHierarchy()
{
  m_ParentHierarchy.Clear();

  for (const xiiRTTI* rtti = this; rtti != nullptr; rtti = rtti->m_pParentType)
  {
    m_ParentHierarchy.PushBack(rtti);
  }
}

void xiiRTTI::VerifyCorrectness() const
{
  if (m_VerifyParent != nullptr)
  {
    XII_ASSERT_DEV(m_VerifyParent() == m_pParentType, "Type '{0}': The given parent type '{1}' does not match the actual parent type '{2}'",
                   m_szTypeName, (m_pParentType != nullptr) ? m_pParentType->GetTypeName() : "null",
                   (m_VerifyParent() != nullptr) ? m_VerifyParent()->GetTypeName() : "null");
  }

  {
    xiiSet<xiiStringView> Known;

    const xiiRTTI* pInstance = this;

    while (pInstance != nullptr)
    {
      for (xiiUInt32 i = 0; i < pInstance->m_Properties.GetCount(); ++i)
      {
        const bool bNewProperty = !Known.Find(pInstance->m_Properties[i]->GetPropertyName()).IsValid();
        Known.Insert(pInstance->m_Properties[i]->GetPropertyName());

        XII_ASSERT_DEV(bNewProperty, "{0}: The property with name '{1}' is already defined in type '{2}'.", m_szTypeName,
                       pInstance->m_Properties[i]->GetPropertyName(), pInstance->GetTypeName());
      }

      pInstance = pInstance->m_pParentType;
    }
  }

  {
    for (xiiAbstractProperty* pFunc : m_Functions)
    {
      XII_ASSERT_DEV(pFunc->GetCategory() == xiiPropertyCategory::Function, "Invalid function property '{}'", pFunc->GetPropertyName());
    }
  }
}

void xiiRTTI::VerifyCorrectnessForAllTypes()
{
  xiiRTTI* pRtti = xiiRTTI::GetFirstInstance();

  while (pRtti)
  {
    pRtti->VerifyCorrectness();
    pRtti = pRtti->GetNextInstance();
  }
}


void xiiRTTI::UpdateType(const xiiRTTI* pParentType, xiiUInt32 uiTypeSize, xiiUInt32 uiTypeVersion, xiiUInt32 uiVariantType, xiiBitflags<xiiTypeFlags> flags)
{
  m_pParentType   = pParentType;
  m_uiVariantType = uiVariantType;
  m_uiTypeSize    = uiTypeSize;
  m_uiTypeVersion = uiTypeVersion;
  m_TypeFlags     = flags;
  m_ParentHierarchy.Clear();
}

void xiiRTTI::RegisterType()
{
  m_uiTypeNameHash = xiiHashingUtils::StringHash(m_szTypeName);

  auto pTable = GetTypeHashTable();
  XII_LOCK(pTable->m_Mutex);
  pTable->m_Table.Insert(m_szTypeName, this);
}

void xiiRTTI::UnregisterType()
{
  auto pTable = GetTypeHashTable();
  XII_LOCK(pTable->m_Mutex);
  pTable->m_Table.Remove(m_szTypeName);
}

void xiiRTTI::GetAllProperties(xiiHybridArray<xiiAbstractProperty*, 32>& out_properties) const
{
  out_properties.Clear();

  if (m_pParentType)
    m_pParentType->GetAllProperties(out_properties);

  out_properties.PushBackRange(GetProperties());
}

xiiRTTI* xiiRTTI::FindTypeByName(const char* szName)
{
  xiiRTTI* pInstance = nullptr;
  {
    auto pTable = GetTypeHashTable();
    XII_LOCK(pTable->m_Mutex);
    if (pTable->m_Table.TryGetValue(szName, pInstance))
      return pInstance;
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  pInstance = xiiRTTI::GetFirstInstance();

  while (pInstance)
  {
    if (xiiStringUtils::IsEqual(pInstance->GetTypeName(), szName))
    {
      XII_REPORT_FAILURE("The hash table lookup should have already found the RTTI type '{}'", szName);
      return pInstance;
    }

    pInstance = pInstance->GetNextInstance();
  }
#endif

  return nullptr;
}

xiiRTTI* xiiRTTI::FindTypeByNameHash(xiiUInt64 uiNameHash)
{
  // TODO: actually reuse the hash table for the lookup

  xiiRTTI* pInstance = xiiRTTI::GetFirstInstance();

  while (pInstance)
  {
    if (pInstance->GetTypeNameHash() == uiNameHash)
      return pInstance;

    pInstance = pInstance->GetNextInstance();
  }

  return nullptr;
}

xiiRTTI* xiiRTTI::FindTypeByNameHash32(xiiUInt32 uiNameHash)
{
  // TODO: actually reuse the hash table for the lookup

  xiiRTTI* pInstance = xiiRTTI::GetFirstInstance();

  while (pInstance)
  {
    if (xiiHashingUtils::StringHashTo32(pInstance->GetTypeNameHash()) == uiNameHash)
      return pInstance;

    pInstance = pInstance->GetNextInstance();
  }

  return nullptr;
}

xiiAbstractProperty* xiiRTTI::FindPropertyByName(const char* szName, bool bSearchBaseTypes /* = true */) const
{
  const xiiRTTI* pInstance = this;

  do
  {
    for (xiiUInt32 p = 0; p < pInstance->m_Properties.GetCount(); ++p)
    {
      if (xiiStringUtils::IsEqual(pInstance->m_Properties[p]->GetPropertyName(), szName))
      {
        return pInstance->m_Properties[p];
      }
    }

    if (!bSearchBaseTypes)
      return nullptr;

    pInstance = pInstance->m_pParentType;
  } while (pInstance != nullptr);

  return nullptr;
}

bool xiiRTTI::DispatchMessage(void* pInstance, xiiMessage& ref_msg) const
{
  XII_ASSERT_DEBUG(m_bGatheredDynamicMessageHandlers, "Message handler table should have been gathered at this point.\n"
                                                      "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                      "you may have forgotten to instantiate an xiiPlugin object inside your plugin DLL.");

  const xiiUInt32 uiIndex = ref_msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    xiiAbstractMessageHandler* pHandler = m_DynamicMessageHandlers[uiIndex];
    if (pHandler != nullptr)
    {
      (*pHandler)(pInstance, ref_msg);
      return true;
    }
  }

  return false;
}

bool xiiRTTI::DispatchMessage(const void* pInstance, xiiMessage& ref_msg) const
{
  XII_ASSERT_DEBUG(m_bGatheredDynamicMessageHandlers, "Message handler table should have been gathered at this point.\n"
                                                      "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                      "you may have forgotten to instantiate an xiiPlugin object inside your plugin DLL.");

  const xiiUInt32 uiIndex = ref_msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    xiiAbstractMessageHandler* pHandler = m_DynamicMessageHandlers[uiIndex];
    if (pHandler != nullptr && pHandler->IsConst())
    {
      (*pHandler)(pInstance, ref_msg);
      return true;
    }
  }

  return false;
}

const xiiDynamicArray<const xiiRTTI*>& xiiRTTI::GetAllTypesDerivedFrom(
  const xiiRTTI*                   pBaseType,
  xiiDynamicArray<const xiiRTTI*>& out_derivedTypes,
  bool                             bSortByName)
{
  for (auto pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom(pBaseType))
      continue;

    out_derivedTypes.PushBack(pRtti);
  }

  if (bSortByName)
  {
    out_derivedTypes.Sort([](const xiiRTTI* p1, const xiiRTTI* p2) -> bool {
      return xiiStringUtils::Compare(p1->GetTypeName(), p2->GetTypeName()) < 0;
    });
  }

  return out_derivedTypes;
}

void xiiRTTI::AssignPlugin(const char* szPluginName)
{
  // assigns the given plugin name to every xiiRTTI instance that has no plugin assigned yet

  xiiRTTI* pInstance = xiiRTTI::GetFirstInstance();

  while (pInstance)
  {
    if (pInstance->m_szPluginName == nullptr)
    {
      pInstance->m_szPluginName = szPluginName;
      SanityCheckType(pInstance);

      pInstance->SetupParentHierarchy();
      pInstance->GatherDynamicMessageHandlers();
    }
    pInstance = pInstance->GetNextInstance();
  }
}

// Warning C4505: 'IsValidIdentifierName': unreferenced function with internal linkage has been removed
// This happens in Release builds, because the function is only used in a debug assert
#define XII_MSVC_WARNING_NUMBER 4505
#include <Foundation/Basics/Compiler/MSVC/DisableWarning_MSVC.h>

static bool IsValidIdentifierName(const char* szIdentifier)
{
  // empty strings are not valid
  if (xiiStringUtils::IsNullOrEmpty(szIdentifier))
    return false;

  // digits are not allowed as the first character
  if (szIdentifier[0] >= '0' && szIdentifier[0] <= '9')
    return false;

  for (const char* s = szIdentifier; *s != '\0'; ++s)
  {
    const char c = *s;

    if (c >= 'a' && c <= 'z')
      continue;
    if (c >= 'A' && c <= 'Z')
      continue;
    if (c >= '0' && c <= '9')
      continue;
    if (c >= '_')
      continue;
    if (c >= ':')
      continue;

    return false;
  }

  return true;
}

#include <Foundation/Basics/Compiler/MSVC/RestoreWarning_MSVC.h>

void xiiRTTI::SanityCheckType(xiiRTTI* pType)
{
  XII_ASSERT_DEV(pType->GetTypeFlags().IsSet(xiiTypeFlags::StandardType) + pType->GetTypeFlags().IsSet(xiiTypeFlags::IsEnum) +
                     pType->GetTypeFlags().IsSet(xiiTypeFlags::Bitflags) + pType->GetTypeFlags().IsSet(xiiTypeFlags::Class) ==
                   1,
                 "Types are mutually exclusive!");

  for (auto pProp : pType->m_Properties)
  {
    const xiiRTTI* pSpecificType = pProp->GetSpecificType();

    XII_ASSERT_DEBUG(IsValidIdentifierName(pProp->GetPropertyName()), "Property name is invalid: '{0}'", pProp->GetPropertyName());

#if 0
    if (!IsValidIdentifierName(pProp->GetPropertyName()))
    {
      xiiStringBuilder s;
      s.Format("RTTI: {0}\n", pProp->GetPropertyName());

      xiiLog::Print(s.GetData());
    }
#endif

    if (pProp->GetCategory() != xiiPropertyCategory::Function)
    {
      XII_ASSERT_DEV(pProp->GetFlags().IsSet(xiiPropertyFlags::StandardType) + pProp->GetFlags().IsSet(xiiPropertyFlags::IsEnum) +
                         pProp->GetFlags().IsSet(xiiPropertyFlags::Bitflags) + pProp->GetFlags().IsSet(xiiPropertyFlags::Class) <=
                       1,
                     "Types are mutually exclusive!");
    }

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Constant:
      {
        XII_ASSERT_DEV(pSpecificType->GetTypeFlags().IsSet(xiiTypeFlags::StandardType), "Only standard type constants are supported!");
      }
      break;
      case xiiPropertyCategory::Member:
      {
        XII_ASSERT_DEV(pProp->GetFlags().IsSet(xiiPropertyFlags::StandardType) == pSpecificType->GetTypeFlags().IsSet(xiiTypeFlags::StandardType),
                       "Property-Type missmatch!");
        XII_ASSERT_DEV(pProp->GetFlags().IsSet(xiiPropertyFlags::IsEnum) == pSpecificType->GetTypeFlags().IsSet(xiiTypeFlags::IsEnum),
                       "Property-Type missmatch! Use XII_BEGIN_STATIC_REFLECTED_ENUM for type and XII_ENUM_MEMBER_PROPERTY / "
                       "XII_ENUM_ACCESSOR_PROPERTY for property.");
        XII_ASSERT_DEV(pProp->GetFlags().IsSet(xiiPropertyFlags::Bitflags) == pSpecificType->GetTypeFlags().IsSet(xiiTypeFlags::Bitflags),
                       "Property-Type missmatch! Use XII_BEGIN_STATIC_REFLECTED_ENUM for type and XII_BITFLAGS_MEMBER_PROPERTY / "
                       "XII_BITFLAGS_ACCESSOR_PROPERTY for property.");
        XII_ASSERT_DEV(pProp->GetFlags().IsSet(xiiPropertyFlags::Class) == pSpecificType->GetTypeFlags().IsSet(xiiTypeFlags::Class),
                       "If xiiPropertyFlags::Class is set, the property type must be xiiTypeFlags::Class and vise versa.");
      }
      break;
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      case xiiPropertyCategory::Map:
      {
        XII_ASSERT_DEV(pProp->GetFlags().IsSet(xiiPropertyFlags::StandardType) == pSpecificType->GetTypeFlags().IsSet(xiiTypeFlags::StandardType),
                       "Property-Type missmatch!");
        XII_ASSERT_DEV(pProp->GetFlags().IsSet(xiiPropertyFlags::Class) == pSpecificType->GetTypeFlags().IsSet(xiiTypeFlags::Class),
                       "If xiiPropertyFlags::Class is set, the property type must be xiiTypeFlags::Class and vise versa.");
      }
      break;
      case xiiPropertyCategory::Function:
        XII_REPORT_FAILURE("Functions need to be put into the XII_BEGIN_FUNCTIONS / XII_END_FUNCTIONS; block.");
        break;
    }
  }
}

void xiiRTTI::PluginEventHandler(const xiiPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case xiiPluginEvent::BeforeLoading:
    {
      // before a new plugin is loaded, make sure all current xiiRTTI instances
      // are assigned to the proper plugin
      // all not-yet assigned rtti instances cannot be in any plugin, so assign them to the 'static' plugin
      AssignPlugin("Static");
    }
    break;

    case xiiPluginEvent::AfterLoadingBeforeInit:
    {
      // after we loaded a new plugin, but before it is initialized,
      // find all new rtti instances and assign them to that new plugin
      AssignPlugin(EventData.m_szPluginBinary);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      xiiRTTI::VerifyCorrectnessForAllTypes();
#endif
    }
    break;

    default:
      break;
  }
}



XII_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_RTTI);
