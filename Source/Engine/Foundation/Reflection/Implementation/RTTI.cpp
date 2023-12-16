#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/MessageHandler.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>

struct xiiTypeData
{
  xiiMutex                                                                               m_Mutex;
  xiiHashTable<xiiUInt64, xiiRTTI*, xiiHashHelper<xiiUInt64>, xiiStaticAllocatorWrapper> m_TypeNameHashToType;
  xiiDynamicArray<xiiRTTI*>                                                              m_AllTypes;

  bool m_bIsIterating = false;
};

xiiTypeData* GetTypeData()
{
  // Prevent static initialization hazard between first xiiRTTI instance
  // and type data and also make sure it is sufficiently sized before first use.
  auto CreateData = []() -> xiiTypeData* {
    xiiTypeData* pData = new xiiTypeData();
    pData->m_TypeNameHashToType.Reserve(512);
    pData->m_AllTypes.Reserve(512);
    return pData;
  };
  static xiiTypeData* pData = CreateData();
  return pData;
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Reflection)

  // BEGIN_SUBSYSTEM_DEPENDENCIES
  //   "FileSystem"
  // END_SUBSYSTEM_DEPENDENCIES

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

xiiRTTI::xiiRTTI(xiiStringView sName, const xiiRTTI* pParentType, xiiUInt32 uiTypeSize, xiiUInt32 uiTypeVersion, xiiUInt8 uiVariantType, xiiBitflags<xiiTypeFlags> flags, xiiRTTIAllocator* pAllocator, xiiArrayPtr<const xiiAbstractProperty*> properties, xiiArrayPtr<const xiiAbstractFunctionProperty*> functions, xiiArrayPtr<const xiiPropertyAttribute*> attributes, xiiArrayPtr<xiiAbstractMessageHandler*> messageHandlers, xiiArrayPtr<xiiMessageSenderInfo> messageSenders, const xiiRTTI* (*fnVerifyParent)()) :
  m_sTypeName(sName), m_pAllocator(pAllocator), m_Properties(properties), m_Functions(functions), m_Attributes(attributes), m_MessageHandlers(messageHandlers), m_MessageSenders(messageSenders), m_VerifyParent(fnVerifyParent)
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

  if (!m_sTypeName.IsEmpty())
  {
    RegisterType();
  }
}

xiiRTTI::~xiiRTTI()
{
  if (!m_sTypeName.IsEmpty())
  {
    UnregisterType();
  }
}

void xiiRTTI::GatherDynamicMessageHandlers()
{
  // This cannot be done in the constructor, because the parent types are not guaranteed to be initialized at that point

  if (m_uiMsgIdOffset != xiiSmallInvalidIndex)
    return;

  m_uiMsgIdOffset = 0;

  xiiUInt16 uiMinMsgId = xiiSmallInvalidIndex;
  xiiUInt16 uiMaxMsgId = 0;

  const xiiRTTI* pInstance = this;
  while (pInstance != nullptr)
  {
    for (xiiUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
    {
      xiiUInt16 id = pInstance->m_MessageHandlers[i]->GetMessageId();
      uiMinMsgId   = xiiMath::Min(uiMinMsgId, id);
      uiMaxMsgId   = xiiMath::Max(uiMaxMsgId, id);
    }

    pInstance = pInstance->m_pParentType;
  }

  if (uiMinMsgId != xiiSmallInvalidIndex)
  {
    m_uiMsgIdOffset            = uiMinMsgId;
    xiiUInt16 uiNeededCapacity = uiMaxMsgId - uiMinMsgId + 1;

    m_DynamicMessageHandlers.SetCount(uiNeededCapacity);

    pInstance = this;
    while (pInstance != nullptr)
    {
      for (xiiUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
      {
        xiiAbstractMessageHandler* pHandler = pInstance->m_MessageHandlers[i];
        xiiUInt16                  uiIndex  = pHandler->GetMessageId() - m_uiMsgIdOffset;

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
                   m_sTypeName, (m_pParentType != nullptr) ? m_pParentType->GetTypeName() : "null",
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

        XII_ASSERT_DEV(bNewProperty, "{0}: The property with name '{1}' is already defined in type '{2}'.", m_sTypeName, pInstance->m_Properties[i]->GetPropertyName(), pInstance->GetTypeName());
      }

      pInstance = pInstance->m_pParentType;
    }
  }

  {
    for (const xiiAbstractProperty* pFunc : m_Functions)
    {
      XII_ASSERT_DEV(pFunc->GetCategory() == xiiPropertyCategory::Function, "Invalid function property '{}'", pFunc->GetPropertyName());
    }
  }
}

void xiiRTTI::VerifyCorrectnessForAllTypes()
{
  xiiRTTI::ForEachType([](const xiiRTTI* pRtti) { pRtti->VerifyCorrectness(); });
}

void xiiRTTI::UpdateType(const xiiRTTI* pParentType, xiiUInt32 uiTypeSize, xiiUInt32 uiTypeVersion, xiiUInt8 uiVariantType, xiiBitflags<xiiTypeFlags> flags)
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
  m_uiTypeNameHash = xiiHashingUtils::StringHash(m_sTypeName);

  auto pData = GetTypeData();
  XII_LOCK(pData->m_Mutex);
  pData->m_TypeNameHashToType.Insert(m_uiTypeNameHash, this);

  m_uiTypeIndex = pData->m_AllTypes.GetCount();
  pData->m_AllTypes.PushBack(this);
}

void xiiRTTI::UnregisterType()
{
  auto pData = GetTypeData();
  XII_LOCK(pData->m_Mutex);
  pData->m_TypeNameHashToType.Remove(m_uiTypeNameHash);

  XII_ASSERT_DEV(pData->m_bIsIterating == false, "Unregistering types while iterating over types might cause unexpected behavior");

  pData->m_AllTypes.RemoveAtAndSwap(m_uiTypeIndex);
  if (m_uiTypeIndex != pData->m_AllTypes.GetCount())
  {
    pData->m_AllTypes[m_uiTypeIndex]->m_uiTypeIndex = m_uiTypeIndex;
  }
}

void xiiRTTI::GetAllProperties(xiiDynamicArray<const xiiAbstractProperty*>& out_properties) const
{
  out_properties.Clear();

  if (m_pParentType)
    m_pParentType->GetAllProperties(out_properties);

  out_properties.PushBackRange(GetProperties());
}

const xiiRTTI* xiiRTTI::FindTypeByName(xiiStringView sName)
{
  xiiUInt64 uiNameHash = xiiHashingUtils::StringHash(sName);

  auto pData = GetTypeData();
  XII_LOCK(pData->m_Mutex);

  xiiRTTI* pType = nullptr;
  pData->m_TypeNameHashToType.TryGetValue(uiNameHash, pType);
  return pType;
}

const xiiRTTI* xiiRTTI::FindTypeByNameHash(xiiUInt64 uiNameHash)
{
  auto pData = GetTypeData();
  XII_LOCK(pData->m_Mutex);

  xiiRTTI* pType = nullptr;
  pData->m_TypeNameHashToType.TryGetValue(uiNameHash, pType);
  return pType;
}

const xiiRTTI* xiiRTTI::FindTypeByNameHash32(xiiUInt32 uiNameHash)
{
  return FindTypeIf([=](const xiiRTTI* pRtti) { return (xiiHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()) == uiNameHash); });
}

const xiiAbstractProperty* xiiRTTI::FindPropertyByName(xiiStringView sName, bool bSearchBaseTypes /* = true */) const
{
  const xiiRTTI* pInstance = this;

  do
  {
    for (xiiUInt32 p = 0; p < pInstance->m_Properties.GetCount(); ++p)
    {
      if (pInstance->m_Properties[p]->GetPropertyName() == sName)
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

const xiiRTTI* xiiRTTI::FindTypeIf(PredicateFunc func)
{
  auto pData = GetTypeData();
  XII_LOCK(pData->m_Mutex);

  for (const xiiRTTI* pRtti : pData->m_AllTypes)
  {
    if (func(pRtti))
    {
      return pRtti;
    }
  }

  return nullptr;
}

bool xiiRTTI::DispatchMessage(void* pInstance, xiiMessage& ref_msg) const
{
  XII_ASSERT_DEBUG(m_uiMsgIdOffset != xiiSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                            "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                            "you may have forgotten to instantiate a xiiPlugin object inside your plugin DLL.");

  const xiiUInt32 uiIndex = ref_msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    xiiAbstractMessageHandler* pHandler = m_DynamicMessageHandlers.GetData()[uiIndex];
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
  XII_ASSERT_DEBUG(m_uiMsgIdOffset != xiiSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                            "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                            "you may have forgotten to instantiate a xiiPlugin object inside your plugin DLL.");

  const xiiUInt32 uiIndex = ref_msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    xiiAbstractMessageHandler* pHandler = m_DynamicMessageHandlers.GetData()[uiIndex];
    if (pHandler != nullptr && pHandler->IsConst())
    {
      (*pHandler)(pInstance, ref_msg);
      return true;
    }
  }

  return false;
}

void xiiRTTI::ForEachType(VisitorFunc func, xiiBitflags<ForEachOptions> options /*= ForEachOptions::Default*/)
{
  auto pData = GetTypeData();
  XII_LOCK(pData->m_Mutex);

  pData->m_bIsIterating = true;
  // Cannot use ranged based for loop here since we might add new types while iterating and the m_AllTypes array might re-allocate.
  for (xiiUInt32 i = 0; i < pData->m_AllTypes.GetCount(); ++i)
  {
    auto pRtti = pData->m_AllTypes.GetData()[i];

    if (options.IsSet(ForEachOptions::ExcludeNonAllocatable) && (pRtti->GetAllocator() == nullptr || pRtti->GetAllocator()->CanAllocate() == false))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeAbstract) && pRtti->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
      continue;

    func(pRtti);
  }
  pData->m_bIsIterating = false;
}

void xiiRTTI::ForEachDerivedType(const xiiRTTI* pBaseType, VisitorFunc func, xiiBitflags<ForEachOptions> options /*= ForEachOptions::Default*/)
{
  auto pData = GetTypeData();
  XII_LOCK(pData->m_Mutex);

  pData->m_bIsIterating = true;
  // Can't use ranged based for loop here since we might add new types while iterating and the m_AllTypes array might re-allocate.
  for (xiiUInt32 i = 0; i < pData->m_AllTypes.GetCount(); ++i)
  {
    auto pRtti = pData->m_AllTypes.GetData()[i];

    if (!pRtti->IsDerivedFrom(pBaseType))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeNonAllocatable) && (pRtti->GetAllocator() == nullptr || pRtti->GetAllocator()->CanAllocate() == false))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeAbstract) && pRtti->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
      continue;

    func(pRtti);
  }
  pData->m_bIsIterating = false;
}

void xiiRTTI::AssignPlugin(xiiStringView sPluginName)
{
  // Assigns the given plugin name to every xiiRTTI instance that has no plugin assigned yet

  auto pData = GetTypeData();
  XII_LOCK(pData->m_Mutex);

  for (xiiRTTI* pRtti : pData->m_AllTypes)
  {
    if (pRtti->m_sPluginName.IsEmpty())
    {
      pRtti->m_sPluginName = sPluginName;
      SanityCheckType(pRtti);

      pRtti->SetupParentHierarchy();
      pRtti->GatherDynamicMessageHandlers();
    }
  }
}

// Warning C4505: 'IsValidIdentifierName': unreferenced function with internal linkage has been removed
// This happens in Release builds, because the function is only used in a debug assert
XII_WARNING_PUSH()
XII_WARNING_DISABLE_MSVC(4505)

static bool IsValidIdentifierName(xiiStringView sIdentifier)
{
  // empty strings are not valid
  if (sIdentifier.IsEmpty())
    return false;

  // digits are not allowed as the first character
  xiiUInt32 uiChar = sIdentifier.GetCharacter();
  if (uiChar >= '0' && uiChar <= '9')
    return false;

  for (auto it = sIdentifier.GetIteratorFront(); it.IsValid(); ++it)
  {
    const xiiUInt32 c = it.GetCharacter();

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

XII_WARNING_POP()

void xiiRTTI::SanityCheckType(xiiRTTI* pType)
{
  XII_ASSERT_DEV(pType->GetTypeFlags().IsSet(xiiTypeFlags::StandardType) + pType->GetTypeFlags().IsSet(xiiTypeFlags::IsEnum) + pType->GetTypeFlags().IsSet(xiiTypeFlags::Bitflags) + pType->GetTypeFlags().IsSet(xiiTypeFlags::Class) == 1,
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
      XII_ASSERT_DEV(pProp->GetFlags().IsSet(xiiPropertyFlags::StandardType) + pProp->GetFlags().IsSet(xiiPropertyFlags::IsEnum) + pProp->GetFlags().IsSet(xiiPropertyFlags::Bitflags) + pProp->GetFlags().IsSet(xiiPropertyFlags::Class) <= 1,
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
      AssignPlugin(EventData.m_sPluginBinary);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      xiiRTTI::VerifyCorrectnessForAllTypes();
#endif
    }
    break;

    default:
      break;
  }
}

xiiRTTIAllocator::~xiiRTTIAllocator() = default;

XII_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_RTTI);
