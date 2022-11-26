#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Messages/EventMessage.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

namespace
{
  xiiColorGammaUB NiceColorFromFloat(float x)
  {
    return xiiColorScheme::DarkUI(x);
  }

  xiiColorGammaUB NiceColorFromString(xiiStringView s)
  {
    float x = xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(xiiHashingUtils::StringHash(s))).x();
    return NiceColorFromFloat(x);
  }

  static const xiiColor ExecutionPinColor = xiiColorScheme::DarkUI(xiiColorScheme::Gray);
} // namespace

XII_IMPLEMENT_SINGLETON(xiiVisualScriptTypeRegistry);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, VisualScript)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiVisualScriptTypeRegistry);

    xiiVisualScriptTypeRegistry::GetSingleton()->UpdateNodeTypes();
    const xiiRTTI* pBaseType = xiiVisualScriptTypeRegistry::GetSingleton()->GetNodeBaseType();

    xiiQtNodeScene::GetPinFactory().RegisterCreator(xiiGetStaticRTTI<xiiVisualScriptPin>(), [](const xiiRTTI* pRtti)->xiiQtPin* { return new xiiQtVisualScriptPin(); });
    xiiQtNodeScene::GetConnectionFactory().RegisterCreator(xiiGetStaticRTTI<xiiVisualScriptConnection>(), [](const xiiRTTI* pRtti)->xiiQtConnection* { return new xiiQtVisualScriptConnection(); });
    xiiQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const xiiRTTI* pRtti)->xiiQtNode* { return new xiiQtVisualScriptNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const xiiRTTI* pBaseType = xiiVisualScriptTypeRegistry::GetSingleton()->GetNodeBaseType();
    xiiQtNodeScene::GetNodeFactory().UnregisterCreator(pBaseType);

    xiiQtNodeScene::GetPinFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVisualScriptPin>());
    xiiQtNodeScene::GetConnectionFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVisualScriptConnection>());

    xiiVisualScriptTypeRegistry* pDummy = xiiVisualScriptTypeRegistry::GetSingleton();
    XII_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiVisualScriptTypeRegistry::xiiVisualScriptTypeRegistry() :
  m_SingletonRegistrar(this)
{
  m_pBaseType = nullptr;
  xiiPhantomRttiManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiVisualScriptTypeRegistry::PhantomTypeRegistryEventHandler, this));
}


xiiVisualScriptTypeRegistry::~xiiVisualScriptTypeRegistry()
{
  xiiPhantomRttiManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiVisualScriptTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

const xiiVisualScriptNodeDescriptor* xiiVisualScriptTypeRegistry::GetDescriptorForType(const xiiRTTI* pRtti) const
{
  auto it = m_NodeDescriptors.Find(pRtti);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

void xiiVisualScriptTypeRegistry::PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e)
{
  if (e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeAdded || e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeChanged)
  {
    UpdateNodeType(e.m_pChangedType);
  }
}

void xiiVisualScriptTypeRegistry::UpdateNodeTypes()
{
  XII_PROFILE_SCOPE("UpdateNodeTypes");

  // Base Node Type
  if (m_pBaseType == nullptr)
  {
    xiiReflectedTypeDescriptor desc;
    desc.m_sTypeName       = "xiiVisualScriptNodeBase";
    desc.m_sPluginName     = "VisualScriptTypes";
    desc.m_sParentTypeName = xiiGetStaticRTTI<xiiReflectedClass>()->GetTypeName();
    desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Abstract | xiiTypeFlags::Class;
    desc.m_uiTypeVersion   = 1;

    m_pBaseType = xiiPhantomRttiManager::RegisterType(desc);
  }

  auto& dynEnum = xiiDynamicStringEnum::CreateDynamicEnum("ComponentTypes");

  for (const xiiRTTI* pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (pRtti->IsDerivedFrom<xiiComponent>())
    {
      dynEnum.AddValidValue(pRtti->GetTypeName(), true);
    }
  }

  for (const xiiRTTI* pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    UpdateNodeType(pRtti);
  }
}

static xiiColorGammaUB PinTypeColor(xiiVisualScriptDataPinType::Enum type)
{
  float x = (float)(type - 1) / (xiiVisualScriptDataPinType::Variant - 1);
  return NiceColorFromFloat(x);
}

void xiiVisualScriptTypeRegistry::UpdateNodeType(const xiiRTTI* pRtti)
{
  if (pRtti->GetAttributeByType<xiiHiddenAttribute>() != nullptr)
    return;

  if (pRtti->IsDerivedFrom<xiiMessage>() && !pRtti->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
  {
    if (pRtti->GetAttributeByType<xiiAutoGenVisScriptMsgSender>())
    {
      CreateMessageSenderNodeType(pRtti);
    }

    if (pRtti->GetAttributeByType<xiiAutoGenVisScriptMsgHandler>())
    {
      CreateMessageHandlerNodeType(pRtti);
    }
  }

  // expose reflected functions to visual scripts
  {
    for (const xiiAbstractFunctionProperty* pFuncProp : pRtti->GetFunctions())
    {
      CreateFunctionCallNodeType(pRtti, pFuncProp);
    }
  }

  if (!pRtti->IsDerivedFrom<xiiVisualScriptNode>() || pRtti->GetTypeFlags().IsAnySet(xiiTypeFlags::Abstract))
    return;

  xiiVisualScriptNodeDescriptor nd;
  nd.m_sTypeName = pRtti->GetTypeName();

  if (const xiiCategoryAttribute* pAttr = pRtti->GetAttributeByType<xiiCategoryAttribute>())
  {
    nd.m_sCategory = pAttr->GetCategory();
  }

  nd.m_Color = NiceColorFromString(nd.m_sCategory);

  if (const xiiTitleAttribute* pAttr = pRtti->GetAttributeByType<xiiTitleAttribute>())
  {
    nd.m_sTitle = pAttr->GetTitle();
  }

  if (const xiiColorAttribute* pAttr = pRtti->GetAttributeByType<xiiColorAttribute>())
  {
    nd.m_Color = pAttr->GetColor();
  }

  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  xiiSet<xiiInt32> usedInputDataPinIDs;
  xiiSet<xiiInt32> usedOutputDataPinIDs;
  xiiSet<xiiInt32> usedInputExecPinIDs;
  xiiSet<xiiInt32> usedOutputExecPinIDs;

  for (auto prop : properties)
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName    = prop->GetPropertyName();
    pd.m_sTooltip = ""; /// \todo Use xiiTranslateTooltip

    if (const xiiVisScriptDataPinInAttribute* pAttr = prop->GetAttributeByType<xiiVisScriptDataPinInAttribute>())
    {
      pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
      pd.m_Color      = PinTypeColor(pAttr->m_DataType);
      pd.m_DataType   = pAttr->m_DataType;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_InputPins.PushBack(pd);

      if (usedInputDataPinIDs.Contains(pd.m_uiPinIndex))
        xiiLog::Error("Visual Script Node '{0}' uses the same input data pin index multiple times: '{1}'", nd.m_sTypeName, pd.m_uiPinIndex);

      usedInputDataPinIDs.Insert(pd.m_uiPinIndex);
    }

    if (const xiiVisScriptDataPinOutAttribute* pAttr = prop->GetAttributeByType<xiiVisScriptDataPinOutAttribute>())
    {
      pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
      pd.m_Color      = PinTypeColor(pAttr->m_DataType);
      pd.m_DataType   = pAttr->m_DataType;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_OutputPins.PushBack(pd);

      if (usedOutputDataPinIDs.Contains(pd.m_uiPinIndex))
        xiiLog::Error("Visual Script Node '{0}' uses the same output data pin index multiple times: '{1}'", nd.m_sTypeName, pd.m_uiPinIndex);

      usedOutputDataPinIDs.Insert(pd.m_uiPinIndex);
    }

    if (const xiiVisScriptExecPinInAttribute* pAttr = prop->GetAttributeByType<xiiVisScriptExecPinInAttribute>())
    {
      pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Execution;
      pd.m_Color      = ExecutionPinColor;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_InputPins.PushBack(pd);

      if (usedInputExecPinIDs.Contains(pd.m_uiPinIndex))
        xiiLog::Error("Visual Script Node '{0}' uses the same input exec pin index multiple times: '{1}'", nd.m_sTypeName, pd.m_uiPinIndex);

      usedInputExecPinIDs.Insert(pd.m_uiPinIndex);
    }

    if (const xiiVisScriptExecPinOutAttribute* pAttr = prop->GetAttributeByType<xiiVisScriptExecPinOutAttribute>())
    {
      pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Execution;
      pd.m_Color      = ExecutionPinColor;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_OutputPins.PushBack(pd);

      if (usedOutputExecPinIDs.Contains(pd.m_uiPinIndex))
        xiiLog::Error("Visual Script Node '{0}' uses the same output exec pin index multiple times: '{1}'", nd.m_sTypeName, pd.m_uiPinIndex);

      usedOutputExecPinIDs.Insert(pd.m_uiPinIndex);
    }

    if (prop->GetCategory() == xiiPropertyCategory::Constant)
      continue;

    {
      xiiReflectedPropertyDescriptor pd;
      pd.m_Flags    = prop->GetFlags();
      pd.m_Category = prop->GetCategory();
      pd.m_sName    = prop->GetPropertyName();
      pd.m_sType    = prop->GetSpecificType()->GetTypeName();

      for (xiiPropertyAttribute* const pAttr : prop->GetAttributes())
      {
        pd.m_Attributes.PushBack(xiiReflectionSerializer::Clone(pAttr));
      }

      nd.m_Properties.PushBack(pd);
    }
  }

  m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
}

const xiiRTTI* xiiVisualScriptTypeRegistry::GenerateTypeFromDesc(const xiiVisualScriptNodeDescriptor& nd)
{
  xiiStringBuilder temp;
  temp.Set("VisualScriptNode::", nd.m_sTypeName);

  xiiReflectedTypeDescriptor desc;
  desc.m_sTypeName       = temp;
  desc.m_sPluginName     = "VisualScriptTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Class;
  desc.m_uiTypeVersion   = 1;
  desc.m_Properties      = nd.m_Properties;

  return xiiPhantomRttiManager::RegisterType(desc);
}

void xiiVisualScriptTypeRegistry::CreateMessageSenderNodeType(const xiiRTTI* pRtti)
{
  const xiiStringBuilder tmp(pRtti->GetTypeName(), "<send>");

  xiiVisualScriptNodeDescriptor nd;
  nd.m_sTypeName = tmp;
  nd.m_Color     = NiceColorFromFloat(0.5f);
  nd.m_sCategory = "Message Senders";

  if (const xiiCategoryAttribute* pAttr = pRtti->GetAttributeByType<xiiCategoryAttribute>())
  {
    nd.m_sCategory = pAttr->GetCategory();
  }

  if (const xiiColorAttribute* pAttr = pRtti->GetAttributeByType<xiiColorAttribute>())
  {
    nd.m_Color = pAttr->GetColor();
  }

  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  // Add an input execution pin
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName      = "send";
    pd.m_sTooltip   = "When executed, the message is sent to the object or component.";
    pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Execution;
    pd.m_Color      = ExecutionPinColor;
    pd.m_uiPinIndex = 0;
    nd.m_InputPins.PushBack(pd);
  }

  // Add an output execution pin
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName      = "then";
    pd.m_sTooltip   = "";
    pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Execution;
    pd.m_Color      = ExecutionPinColor;
    pd.m_uiPinIndex = 0;
    nd.m_OutputPins.PushBack(pd);
  }

  // Add an input data pin for the target object
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName      = "Object";
    pd.m_sTooltip   = "When the object is given, the message is sent to all its components.";
    pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
    pd.m_DataType   = xiiVisualScriptDataPinType::GameObjectHandle;
    pd.m_Color      = PinTypeColor(xiiVisualScriptDataPinType::GameObjectHandle);
    pd.m_uiPinIndex = 0;
    nd.m_InputPins.PushBack(pd);
  }

  // Add an input data pin for the target component
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName      = "Component";
    pd.m_sTooltip   = "When the component is given, the message is sent directly to it.";
    pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
    pd.m_DataType   = xiiVisualScriptDataPinType::ComponentHandle;
    pd.m_Color      = PinTypeColor(xiiVisualScriptDataPinType::ComponentHandle);
    pd.m_uiPinIndex = 1;
    nd.m_InputPins.PushBack(pd);
  }

  // Add an input data pin for delay
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName      = "Delay";
    pd.m_sTooltip   = "Delay message send by the given time in seconds.";
    pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
    pd.m_DataType   = xiiVisualScriptDataPinType::Number;
    pd.m_Color      = PinTypeColor(xiiVisualScriptDataPinType::Number);
    pd.m_uiPinIndex = 2;
    nd.m_InputPins.PushBack(pd);
  }

  // Delayed Delivery Property
  {
    xiiReflectedPropertyDescriptor prd;
    prd.m_Flags    = xiiPropertyFlags::StandardType | xiiPropertyFlags::Phantom;
    prd.m_Category = xiiPropertyCategory::Member;
    prd.m_sName    = "Delay";
    prd.m_sType    = xiiGetStaticRTTI<xiiTime>()->GetTypeName();

    nd.m_Properties.PushBack(prd);
  }

  // Recursive Delivery Property
  {
    xiiReflectedPropertyDescriptor prd;
    prd.m_Flags    = xiiPropertyFlags::StandardType | xiiPropertyFlags::Phantom;
    prd.m_Category = xiiPropertyCategory::Member;
    prd.m_sName    = "Recursive";
    prd.m_sType    = xiiGetStaticRTTI<bool>()->GetTypeName();

    nd.m_Properties.PushBack(prd);
  }

  xiiInt32 iDataPinIndex = 2; // the first valid index is '3', because of the object, component and delay data pins
  for (auto prop : properties)
  {
    if (prop->GetCategory() == xiiPropertyCategory::Constant)
      continue;

    {
      xiiReflectedPropertyDescriptor prd;
      prd.m_Flags    = prop->GetFlags();
      prd.m_Category = prop->GetCategory();
      prd.m_sName    = prop->GetPropertyName();
      prd.m_sType    = prop->GetSpecificType()->GetTypeName();

      for (xiiPropertyAttribute* const pAttr : prop->GetAttributes())
      {
        prd.m_Attributes.PushBack(xiiReflectionSerializer::Clone(pAttr));
      }

      nd.m_Properties.PushBack(prd);
    }

    ++iDataPinIndex;
    auto dataPinType = xiiVisualScriptDataPinType::GetDataPinTypeForType(prop->GetSpecificType());
    if (dataPinType == xiiVisualScriptDataPinType::None)
      continue;

    xiiVisualScriptPinDescriptor pid;
    pid.m_sName      = prop->GetPropertyName();
    pid.m_sTooltip   = ""; /// \todo Use xiiTranslateTooltip
    pid.m_Color      = PinTypeColor(dataPinType);
    pid.m_DataType   = dataPinType;
    pid.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
    pid.m_uiPinIndex = iDataPinIndex;
    nd.m_InputPins.PushBack(pid);
  }

  m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
}

void xiiVisualScriptTypeRegistry::CreateMessageHandlerNodeType(const xiiRTTI* pRtti)
{
  const xiiStringBuilder tmp(pRtti->GetTypeName(), "<handle>");

  xiiVisualScriptNodeDescriptor nd;
  nd.m_sTypeName = tmp;
  nd.m_Color     = NiceColorFromFloat(0.9f);
  nd.m_sCategory = "Message Handlers";

  if (const xiiCategoryAttribute* pAttr = pRtti->GetAttributeByType<xiiCategoryAttribute>())
  {
    nd.m_sCategory = pAttr->GetCategory();
  }

  if (const xiiColorAttribute* pAttr = pRtti->GetAttributeByType<xiiColorAttribute>())
  {
    nd.m_Color = pAttr->GetColor();
  }

  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  // Add an output execution pin
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName      = "OnMsg";
    pd.m_sTooltip   = "";
    pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Execution;
    pd.m_Color      = ExecutionPinColor;
    pd.m_uiPinIndex = 0;
    nd.m_OutputPins.PushBack(pd);
  }

  xiiInt32 iDataPinIndex = -1;
  for (auto prop : properties)
  {
    if (prop->GetCategory() == xiiPropertyCategory::Constant)
      continue;

    ++iDataPinIndex;
    auto dataPinType = xiiVisualScriptDataPinType::GetDataPinTypeForType(prop->GetSpecificType());
    if (dataPinType == xiiVisualScriptDataPinType::None)
      continue;

    xiiVisualScriptPinDescriptor pid;
    pid.m_sName      = prop->GetPropertyName();
    pid.m_sTooltip   = ""; /// \todo Use xiiTranslateTooltip
    pid.m_Color      = PinTypeColor(dataPinType);
    pid.m_DataType   = dataPinType;
    pid.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
    pid.m_uiPinIndex = iDataPinIndex;
    nd.m_OutputPins.PushBack(pid);
  }

  m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
}

void xiiVisualScriptTypeRegistry::CreateFunctionCallNodeType(const xiiRTTI* pRtti, const xiiAbstractFunctionProperty* pFunction)
{
  if (pFunction->GetFunctionType() != xiiFunctionType::Member)
    return;

  const xiiScriptableFunctionAttribute* pAttr = pFunction->GetAttributeByType<xiiScriptableFunctionAttribute>();
  if (pAttr == nullptr)
    return;

  xiiStringBuilder tmp(pRtti->GetTypeName(), "::", pFunction->GetPropertyName(), "<call>");

  xiiVisualScriptNodeDescriptor nd;
  nd.m_sTypeName = tmp;

  tmp.Format("Components/{}", pRtti->GetTypeName());
  nd.m_sCategory = tmp;

  if (const xiiCategoryAttribute* pAttr = pFunction->GetAttributeByType<xiiCategoryAttribute>())
  {
    nd.m_sCategory = pAttr->GetCategory();
  }

  nd.m_Color = NiceColorFromString(nd.m_sCategory);

  if (const xiiColorAttribute* pAttr = pFunction->GetAttributeByType<xiiColorAttribute>())
  {
    nd.m_Color = pAttr->GetColor();
  }

  // Add an input execution pin
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName      = "call";
    pd.m_sTooltip   = "When executed, the message is sent to the object or component.";
    pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Execution;
    pd.m_Color      = ExecutionPinColor;
    pd.m_uiPinIndex = 0;
    nd.m_InputPins.PushBack(pd);
  }

  // Add an output execution pin
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName      = "then";
    pd.m_sTooltip   = "";
    pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Execution;
    pd.m_Color      = ExecutionPinColor;
    pd.m_uiPinIndex = 0;
    nd.m_OutputPins.PushBack(pd);
  }

  // Add an input data pin for the target object
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName      = "Object";
    pd.m_sTooltip   = "When the object is given, the function is called on the first matching component.";
    pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
    pd.m_DataType   = xiiVisualScriptDataPinType::GameObjectHandle;
    pd.m_Color      = PinTypeColor(pd.m_DataType);
    pd.m_uiPinIndex = 0;
    nd.m_InputPins.PushBack(pd);
  }

  // Add an input data pin for the target component
  {
    xiiVisualScriptPinDescriptor pd;
    pd.m_sName      = "Component";
    pd.m_sTooltip   = "When the component is given, the function is called directly on it.";
    pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
    pd.m_DataType   = xiiVisualScriptDataPinType::ComponentHandle;
    pd.m_Color      = PinTypeColor(pd.m_DataType);
    pd.m_uiPinIndex = 1;
    nd.m_InputPins.PushBack(pd);
  }

  xiiInt32 iDataPinIndexOut = 0;

  xiiStringBuilder sName;

  if (pFunction->GetReturnType() != nullptr)
  {
    auto dataPinType = xiiVisualScriptDataPinType::GetDataPinTypeForType(pFunction->GetReturnType());
    if (dataPinType != xiiVisualScriptDataPinType::None)
    {
      tmp.Set(pRtti->GetTypeName(), "::", pFunction->GetPropertyName(), "->Return");

      xiiVisualScriptPinDescriptor pd;
      pd.m_sName      = "Result";
      pd.m_sTooltip   = xiiTranslateTooltip(tmp);
      pd.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
      pd.m_DataType   = dataPinType;
      pd.m_Color      = PinTypeColor(pd.m_DataType);
      pd.m_uiPinIndex = iDataPinIndexOut; // result is always on pin 0
      nd.m_OutputPins.PushBack(pd);

      ++iDataPinIndexOut;
    }
  }

  for (xiiUInt32 argIdx = 0; argIdx < pFunction->GetArgumentCount(); ++argIdx)
  {
    if (pFunction->GetArgumentFlags(argIdx).IsAnySet(xiiPropertyFlags::StandardType) == false)
    {
      xiiLog::Error("Script function '{}' uses non-standard type for argument {}", nd.m_sTypeName, argIdx + 1);
      return;
    }

    sName = pAttr->GetArgumentName(argIdx);
    if (sName.IsEmpty())
      sName.Format("arg{}", argIdx);

    const xiiScriptableFunctionAttribute::ArgType argType = pAttr->GetArgumentType(argIdx);

    // add the inputs as properties
    if (argType != xiiScriptableFunctionAttribute::Out) // in or inout
    {
      xiiReflectedPropertyDescriptor prd;
      prd.m_Flags    = pFunction->GetArgumentFlags(argIdx);
      prd.m_Category = xiiPropertyCategory::Member;
      prd.m_sName    = sName;
      prd.m_sType    = pFunction->GetArgumentType(argIdx)->GetTypeName();

      xiiVisScriptMappingAttribute* pMappingAttr = xiiVisScriptMappingAttribute::GetStaticRTTI()->GetAllocator()->Allocate<xiiVisScriptMappingAttribute>();
      pMappingAttr->m_iMapping                   = argIdx;
      prd.m_Attributes.PushBack(pMappingAttr);

      nd.m_Properties.PushBack(prd);
    }

    auto dataPinType = xiiVisualScriptDataPinType::GetDataPinTypeForType(pFunction->GetArgumentType(argIdx));
    if (dataPinType == xiiVisualScriptDataPinType::None)
      continue;

    tmp.Set(pRtti->GetTypeName(), "::", pFunction->GetPropertyName(), "->", sName);

    xiiVisualScriptPinDescriptor pid;
    pid.m_DataType   = dataPinType;
    pid.m_sName      = sName;
    pid.m_sTooltip   = xiiTranslateTooltip(tmp);
    pid.m_PinType    = xiiVisualScriptPinDescriptor::PinType::Data;
    pid.m_Color      = PinTypeColor(pid.m_DataType);
    pid.m_uiPinIndex = 2 + argIdx; // TODO: document what m_uiPinIndex is for

    if (argType != xiiScriptableFunctionAttribute::Out) // in or inout
    {
      nd.m_InputPins.PushBack(pid);
    }

    if (argType != xiiScriptableFunctionAttribute::In /* out or inout */)
    {
      if (!pFunction->GetArgumentFlags(argIdx).IsSet(xiiPropertyFlags::Reference))
      {
        // TODO: xiiPropertyFlags::Reference is also set for const-ref parameters, should we change that ?

        xiiLog::Error("Script function '{}' argument {} is marked 'out' but is not a non-const reference value", nd.m_sTypeName, argIdx + 1);
        return;
      }

      pid.m_uiPinIndex = iDataPinIndexOut;
      nd.m_OutputPins.PushBack(pid);

      ++iDataPinIndexOut;
    }
  }

  m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
}
