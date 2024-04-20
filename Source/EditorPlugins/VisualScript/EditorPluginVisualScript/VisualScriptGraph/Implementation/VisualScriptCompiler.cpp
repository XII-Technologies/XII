#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/World/World.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
  xiiResult ExtractPropertyName(xiiStringView sPinName, xiiStringView& out_sPropertyName, xiiUInt32* out_pArrayIndex = nullptr)
  {
    const char* szBracket = sPinName.FindSubString("[");
    if (szBracket == nullptr)
      return XII_FAILURE;

    out_sPropertyName = xiiStringView(sPinName.GetStartPointer(), szBracket);

    if (out_pArrayIndex != nullptr)
    {
      return xiiConversionUtils::StringToUInt(szBracket + 1, *out_pArrayIndex);
    }

    return XII_SUCCESS;
  }

  void MakeSubfunctionName(const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject, xiiStringBuilder& out_sName)
  {
    xiiVariant sNameProperty = pObject->GetTypeAccessor().GetValue("Name");
    xiiUInt32  uiHash        = xiiHashHelper<xiiUuid>::Hash(pObject->GetGuid());

    out_sName.SetFormat("{}_{}_{}", pEntryObject != nullptr ? xiiVisualScriptNodeManager::GetNiceFunctionName(pEntryObject) : "", sNameProperty, xiiArgU(uiHash, 8, true, 16));
  }

  xiiVisualScriptDataType::Enum FinalizeDataType(xiiVisualScriptDataType::Enum dataType)
  {
    xiiVisualScriptDataType::Enum result = dataType;
    if (result == xiiVisualScriptDataType::EnumValue)
      result = xiiVisualScriptDataType::Int64;

    return result;
  }

  using FillUserDataFunction = xiiResult (*)(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject);

  static xiiResult FillUserData_CoroutineMode(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("CoroutineMode");
    return XII_SUCCESS;
  }

  static xiiResult FillUserData_ReflectedPropertyOrFunction(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc->m_pTargetType != nullptr)
      inout_astNode.m_sTargetTypeName.Assign(pNodeDesc->m_pTargetType->GetTypeName());

    xiiVariantArray propertyNames;
    for (auto& pProp : pNodeDesc->m_TargetProperties)
    {
      xiiHashedString sPropertyName;
      sPropertyName.Assign(pProp->GetPropertyName());
      propertyNames.PushBack(sPropertyName);
    }

    inout_astNode.m_Value = propertyNames;

    return XII_SUCCESS;
  }

  static xiiResult FillUserData_DynamicReflectedProperty(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    auto pTargetType     = xiiVisualScriptTypeDeduction::GetReflectedType(pObject);
    auto pTargetProperty = xiiVisualScriptTypeDeduction::GetReflectedProperty(pObject);
    if (pTargetType == nullptr || pTargetProperty == nullptr)
      return XII_FAILURE;

    inout_astNode.m_sTargetTypeName.Assign(pTargetType->GetTypeName());

    xiiVariantArray propertyNames;
    {
      xiiHashedString sPropertyName;
      sPropertyName.Assign(pTargetProperty->GetPropertyName());
      propertyNames.PushBack(sPropertyName);
    }

    inout_astNode.m_Value = propertyNames;

    return XII_SUCCESS;
  }

  static xiiResult FillUserData_ConstantValue(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value            = pObject->GetTypeAccessor().GetValue("Value");
    inout_astNode.m_DeductedDataType = xiiVisualScriptDataType::FromVariantType(inout_astNode.m_Value.GetType());
    return XII_SUCCESS;
  }

  static xiiResult FillUserData_VariableName(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Name");

    xiiStringView sName = inout_astNode.m_Value.Get<xiiString>().GetView();

    xiiVariant defaultValue;
    if (static_cast<const xiiVisualScriptNodeManager*>(pObject->GetDocumentObjectManager())->GetVariableDefaultValue(xiiTempHashedString(sName), defaultValue).Failed())
    {
      xiiLog::Error("Invalid variable named '{}'", sName);
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  static xiiResult FillUserData_Switch(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    inout_astNode.m_DeductedDataType = xiiVisualScriptDataType::Int64;

    xiiVariantArray casesVarArray;

    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc->m_pTargetType != nullptr)
    {
      xiiHybridArray<xiiReflectionUtils::EnumKeyValuePair, 16> enumKeysAndValues;
      xiiReflectionUtils::GetEnumKeysAndValues(pNodeDesc->m_pTargetType, enumKeysAndValues, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);
      for (auto& keyAndValue : enumKeysAndValues)
      {
        casesVarArray.PushBack(keyAndValue.m_iValue);
      }
    }
    else
    {
      xiiVariant casesVar = pObject->GetTypeAccessor().GetValue("Cases");
      casesVarArray       = casesVar.Get<xiiVariantArray>();
      for (auto& caseVar : casesVarArray)
      {
        if (caseVar.IsA<xiiString>())
        {
          inout_astNode.m_DeductedDataType = xiiVisualScriptDataType::HashedString;
          caseVar                          = xiiTempHashedString(caseVar.Get<xiiString>()).GetHash();
        }
        else if (caseVar.IsA<xiiHashedString>())
        {
          inout_astNode.m_DeductedDataType = xiiVisualScriptDataType::HashedString;
          caseVar                          = caseVar.Get<xiiHashedString>().GetHash();
        }
      }
    }

    inout_astNode.m_Value = casesVarArray;
    return XII_SUCCESS;
  }

  static xiiResult FillUserData_Builtin_Compare(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Operator");
    return XII_SUCCESS;
  }

  static xiiResult FillUserData_Builtin_TryGetComponentOfBaseType(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    auto           typeName = pObject->GetTypeAccessor().GetValue("TypeName");
    const xiiRTTI* pType    = xiiRTTI::FindTypeByName(typeName.Get<xiiString>());
    if (pType == nullptr)
    {
      xiiLog::Error("Invalid type '{}' for GameObject::TryGetComponentOfBaseType node.", typeName);
      return XII_FAILURE;
    }

    inout_astNode.m_sTargetTypeName.Assign(pType->GetTypeName());
    return XII_SUCCESS;
  }

  static xiiResult FillUserData_Builtin_StartCoroutine(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    XII_SUCCEED_OR_RETURN(FillUserData_CoroutineMode(inout_astNode, pCompiler, pObject, pEntryObject));

    auto                                          pManager = static_cast<const xiiVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());
    xiiHybridArray<const xiiVisualScriptPin*, 16> pins;
    pManager->GetOutputExecutionPins(pObject, pins);

    const xiiUInt32 uiCoroutineBodyIndex = 1;
    auto            connections          = pManager->GetConnections(*pins[uiCoroutineBodyIndex]);
    if (connections.IsEmpty() == false)
    {
      xiiStringBuilder sFunctionName;
      MakeSubfunctionName(pObject, pEntryObject, sFunctionName);

      xiiStringBuilder sFullName;
      sFullName.Set(pCompiler->GetCompiledModule().m_sScriptClassName, "::", sFunctionName, "<Coroutine>");

      inout_astNode.m_sTargetTypeName.Assign(sFullName);

      return pCompiler->AddFunction(sFunctionName, connections[0]->GetTargetPin().GetParent(), pObject);
    }

    return XII_SUCCESS;
  }

  static FillUserDataFunction s_TypeToFillUserDataFunctions[] = {
    nullptr,                                   // Invalid,
    &FillUserData_CoroutineMode,               // EntryCall,
    &FillUserData_CoroutineMode,               // EntryCall_Coroutine,
    &FillUserData_ReflectedPropertyOrFunction, // MessageHandler,
    &FillUserData_ReflectedPropertyOrFunction, // MessageHandler_Coroutine,
    &FillUserData_ReflectedPropertyOrFunction, // ReflectedFunction,
    &FillUserData_DynamicReflectedProperty,    // GetReflectedProperty,
    &FillUserData_DynamicReflectedProperty,    // SetReflectedProperty,
    &FillUserData_ReflectedPropertyOrFunction, // InplaceCoroutine,
    nullptr,                                   // GetOwner,
    &FillUserData_ReflectedPropertyOrFunction, // SendMessage,

    nullptr, // FirstBuiltin,

    &FillUserData_ConstantValue, // Builtin_Constant,
    &FillUserData_VariableName,  // Builtin_GetVariable,
    &FillUserData_VariableName,  // Builtin_SetVariable,
    &FillUserData_VariableName,  // Builtin_IncVariable,
    &FillUserData_VariableName,  // Builtin_DecVariable,

    nullptr,              // Builtin_Branch,
    &FillUserData_Switch, // Builtin_Switch,
    nullptr,              // Builtin_WhileLoop,
    nullptr,              // Builtin_ForLoop,
    nullptr,              // Builtin_ForEachLoop,
    nullptr,              // Builtin_ReverseForEachLoop,
    nullptr,              // Builtin_Break,
    nullptr,              // Builtin_Jump,

    nullptr,                       // Builtin_And,
    nullptr,                       // Builtin_Or,
    nullptr,                       // Builtin_Not,
    &FillUserData_Builtin_Compare, // Builtin_Compare,
    &FillUserData_Builtin_Compare, // Builtin_CompareExec,
    nullptr,                       // Builtin_IsValid,
    nullptr,                       // Builtin_Select,

    nullptr, // Builtin_Add,
    nullptr, // Builtin_Subtract,
    nullptr, // Builtin_Multiply,
    nullptr, // Builtin_Divide,
    nullptr, // Builtin_Expression,

    nullptr, // Builtin_ToBool,
    nullptr, // Builtin_ToByte,
    nullptr, // Builtin_ToInt,
    nullptr, // Builtin_ToInt64,
    nullptr, // Builtin_ToFloat,
    nullptr, // Builtin_ToDouble,
    nullptr, // Builtin_ToString,
    nullptr, // Builtin_String_Format,
    nullptr, // Builtin_ToHashedString,
    nullptr, // Builtin_ToVariant,
    nullptr, // Builtin_Variant_ConvertTo,

    nullptr, // Builtin_MakeArray
    nullptr, // Builtin_Array_GetElement,
    nullptr, // Builtin_Array_SetElement,
    nullptr, // Builtin_Array_GetCount,
    nullptr, // Builtin_Array_IsEmpty,
    nullptr, // Builtin_Array_Clear,
    nullptr, // Builtin_Array_Contains,
    nullptr, // Builtin_Array_IndexOf,
    nullptr, // Builtin_Array_Insert,
    nullptr, // Builtin_Array_PushBack,
    nullptr, // Builtin_Array_Remove,
    nullptr, // Builtin_Array_RemoveAt,

    &FillUserData_Builtin_TryGetComponentOfBaseType, // Builtin_TryGetComponentOfBaseType

    &FillUserData_Builtin_StartCoroutine, // Builtin_StartCoroutine,
    nullptr,                              // Builtin_StopCoroutine,
    nullptr,                              // Builtin_StopAllCoroutines,
    nullptr,                              // Builtin_WaitForAll,
    nullptr,                              // Builtin_WaitForAny,
    nullptr,                              // Builtin_Yield,

    nullptr, // LastBuiltin,
  };

  static_assert(XII_ARRAY_SIZE(s_TypeToFillUserDataFunctions) == xiiVisualScriptNodeDescription::Type::Count);

  xiiResult FillUserData(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    if (pObject == nullptr)
      return XII_SUCCESS;

    auto nodeType = inout_astNode.m_Type;
    XII_ASSERT_DEBUG(nodeType >= 0 && nodeType < XII_ARRAY_SIZE(s_TypeToFillUserDataFunctions), "Out of bounds access");
    auto func = s_TypeToFillUserDataFunctions[nodeType];

    if (func != nullptr)
    {
      XII_SUCCEED_OR_RETURN(func(inout_astNode, pCompiler, pObject, pEntryObject));
    }

    return XII_SUCCESS;
  }

} // namespace

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptCompiler::CompiledModule::CompiledModule() :
  m_ConstantDataStorage(xiiSharedPtr<xiiVisualScriptDataDescription>(&m_ConstantDataDesc, nullptr))
{
  // Prevent the data desc from being deleted by fake shared ptr above
  m_ConstantDataDesc.AddRef();
}

xiiResult xiiVisualScriptCompiler::CompiledModule::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_ASSERT_DEV(m_sScriptClassName.IsEmpty() == false, "Invalid script class name");

  xiiStringDeduplicationWriteContext stringDedup(inout_stream);

  xiiChunkStreamWriter chunk(stringDedup.Begin());
  chunk.BeginStream(1);

  {
    chunk.BeginChunk("Header", 1);
    chunk << m_sBaseClassName;
    chunk << m_sScriptClassName;
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("FunctionGraphs", 1);
    chunk << m_Functions.GetCount();

    for (auto& function : m_Functions)
    {
      chunk << function.m_sName;
      chunk << function.m_Type;
      chunk << function.m_CoroutineCreationMode;

      XII_SUCCEED_OR_RETURN(xiiVisualScriptGraphDescription::Serialize(function.m_NodeDescriptions, function.m_LocalDataDesc, chunk));
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("ConstantData", 1);
    XII_SUCCEED_OR_RETURN(m_ConstantDataDesc.Serialize(chunk));
    XII_SUCCEED_OR_RETURN(m_ConstantDataStorage.Serialize(chunk));
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("InstanceData", 1);
    XII_SUCCEED_OR_RETURN(m_InstanceDataDesc.Serialize(chunk));
    XII_SUCCEED_OR_RETURN(chunk.WriteHashTable(m_InstanceDataMapping.m_Content));
    chunk.EndChunk();
  }

  chunk.EndStream();

  return stringDedup.End();
}

//////////////////////////////////////////////////////////////////////////

// static
xiiUInt32 xiiVisualScriptCompiler::ConnectionHasher::Hash(const Connection& c)
{
  xiiUInt32 uiHashes[] = {
    xiiHashHelper<void*>::Hash(c.m_pPrev),
    xiiHashHelper<void*>::Hash(c.m_pCurrent),
    xiiHashHelper<xiiUInt32>::Hash(c.m_Type),
    xiiHashHelper<xiiUInt32>::Hash(c.m_uiPrevPinIndex),
  };
  return xiiHashingUtils::xxHash32(uiHashes, sizeof(uiHashes));
}

// static
bool xiiVisualScriptCompiler::ConnectionHasher::Equal(const Connection& a, const Connection& b)
{
  return a.m_pPrev == b.m_pPrev &&
    a.m_pCurrent == b.m_pCurrent &&
    a.m_Type == b.m_Type &&
    a.m_uiPrevPinIndex == b.m_uiPrevPinIndex;
}

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptCompiler::xiiVisualScriptCompiler()  = default;
xiiVisualScriptCompiler::~xiiVisualScriptCompiler() = default;

void xiiVisualScriptCompiler::InitModule(xiiStringView sBaseClassName, xiiStringView sScriptClassName)
{
  m_Module.m_sBaseClassName   = sBaseClassName;
  m_Module.m_sScriptClassName = sScriptClassName;
}

xiiResult xiiVisualScriptCompiler::AddFunction(xiiStringView sName, const xiiDocumentObject* pEntryObject, const xiiDocumentObject* pParentObject)
{
  if (m_pManager == nullptr)
  {
    m_pManager = static_cast<const xiiVisualScriptNodeManager*>(pEntryObject->GetDocumentObjectManager());
  }
  XII_ASSERT_DEV(m_pManager == pEntryObject->GetDocumentObjectManager(), "Can't add functions from different document");

  for (auto& existingFunction : m_Module.m_Functions)
  {
    if (existingFunction.m_sName == sName)
    {
      xiiLog::Error("A function named '{}' already exists. Function names need to unique.", sName);
      return XII_FAILURE;
    }
  }

  AstNode* pEntryAstNode = BuildAST(pEntryObject);
  if (pEntryAstNode == nullptr)
    return XII_FAILURE;

  auto& function   = m_Module.m_Functions.ExpandAndGetRef();
  function.m_sName = sName;
  function.m_Type  = pEntryAstNode->m_Type;

  {
    auto pObjectWithCoroutineMode = pParentObject != nullptr ? pParentObject : pEntryObject;
    auto mode                     = pObjectWithCoroutineMode->GetTypeAccessor().GetValue("CoroutineMode");
    if (mode.IsA<xiiInt64>())
    {
      function.m_CoroutineCreationMode = static_cast<xiiScriptCoroutineCreationMode::Enum>(mode.Get<xiiInt64>());
    }
    else
    {
      function.m_CoroutineCreationMode = xiiScriptCoroutineCreationMode::AllowOverlap;
    }
  }

  m_EntryAstNodes.PushBack(pEntryAstNode);
  XII_ASSERT_DEBUG(m_Module.m_Functions.GetCount() == m_EntryAstNodes.GetCount(), "");

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::Compile(xiiStringView sDebugAstOutputPath)
{
  for (xiiUInt32 i = 0; i < m_Module.m_Functions.GetCount(); ++i)
  {
    auto&    function      = m_Module.m_Functions[i];
    AstNode* pEntryAstNode = m_EntryAstNodes[i];

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_00");

    XII_SUCCEED_OR_RETURN(ReplaceUnsupportedNodes(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_01_Replaced");

    XII_SUCCEED_OR_RETURN(InlineConstants(pEntryAstNode));
    XII_SUCCEED_OR_RETURN(InsertTypeConversions(pEntryAstNode));
    XII_SUCCEED_OR_RETURN(InlineVariables(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_02_TypeConv");

    XII_SUCCEED_OR_RETURN(BuildDataExecutions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_03_FlattenedExec");

    XII_SUCCEED_OR_RETURN(FillDataOutputConnections(pEntryAstNode));
    XII_SUCCEED_OR_RETURN(AssignLocalVariables(pEntryAstNode, function.m_LocalDataDesc));
    XII_SUCCEED_OR_RETURN(BuildNodeDescriptions(pEntryAstNode, function.m_NodeDescriptions));

    DumpGraph(function.m_NodeDescriptions, sDebugAstOutputPath, function.m_sName, "_Graph");

    m_PinIdToDataDesc.Clear();
  }

  XII_SUCCEED_OR_RETURN(FinalizeDataOffsets());
  XII_SUCCEED_OR_RETURN(FinalizeConstantData());

  return XII_SUCCESS;
}

xiiUInt32 xiiVisualScriptCompiler::GetPinId(const xiiVisualScriptPin* pPin)
{
  xiiUInt32 uiId = 0;
  if (pPin != nullptr && m_PinToId.TryGetValue(pPin, uiId))
    return uiId;

  uiId = m_uiNextPinId++;
  if (pPin != nullptr)
  {
    m_PinToId.Insert(pPin, uiId);
  }
  return uiId;
}

xiiVisualScriptCompiler::DataOutput& xiiVisualScriptCompiler::GetDataOutput(const DataInput& dataInput)
{
  if (dataInput.m_uiSourcePinIndex < dataInput.m_pSourceNode->m_Outputs.GetCount())
  {
    return dataInput.m_pSourceNode->m_Outputs[dataInput.m_uiSourcePinIndex];
  }

  XII_ASSERT_DEBUG(false, "This code should be never reached");
  static DataOutput dummy;
  return dummy;
}

xiiVisualScriptCompiler::AstNode& xiiVisualScriptCompiler::CreateAstNode(xiiVisualScriptNodeDescription::Type::Enum type, xiiVisualScriptDataType::Enum deductedDataType, bool bImplicitExecution)
{
  auto& node                = m_AstNodes.ExpandAndGetRef();
  node.m_Type               = type;
  node.m_DeductedDataType   = deductedDataType;
  node.m_bImplicitExecution = bImplicitExecution;
  return node;
}

void xiiVisualScriptCompiler::AddDataInput(AstNode& node, AstNode* pSourceNode, xiiUInt8 uiSourcePinIndex, xiiVisualScriptDataType::Enum dataType)
{
  auto& dataInput              = node.m_Inputs.ExpandAndGetRef();
  dataInput.m_pSourceNode      = pSourceNode;
  dataInput.m_uiId             = GetPinId(nullptr);
  dataInput.m_uiSourcePinIndex = uiSourcePinIndex;
  dataInput.m_DataType         = dataType;
}

void xiiVisualScriptCompiler::AddDataOutput(AstNode& node, xiiVisualScriptDataType::Enum dataType)
{
  auto& dataOutput      = node.m_Outputs.ExpandAndGetRef();
  dataOutput.m_uiId     = GetPinId(nullptr);
  dataOutput.m_DataType = dataType;
}

xiiVisualScriptCompiler::DefaultInput xiiVisualScriptCompiler::GetDefaultPointerInput(const xiiRTTI* pDataType)
{
  DefaultInput defaultInput;
  if (m_DefaultInputs.TryGetValue(pDataType, defaultInput) == false)
  {
    if (pDataType == xiiGetStaticRTTI<xiiGameObject>() || pDataType == xiiGetStaticRTTI<xiiGameObjectHandle>())
    {
      auto& getOwnerNode = CreateAstNode(xiiVisualScriptNodeDescription::Type::GetScriptOwner, true);
      AddDataOutput(getOwnerNode, xiiVisualScriptDataType::TypedPointer);
      AddDataOutput(getOwnerNode, xiiVisualScriptDataType::GameObject);

      defaultInput.m_pSourceNode      = &getOwnerNode;
      defaultInput.m_uiSourcePinIndex = 1;
      m_DefaultInputs.Insert(pDataType, defaultInput);
    }
    else if (pDataType == xiiGetStaticRTTI<xiiWorld>())
    {
      auto& getOwnerNode = CreateAstNode(xiiVisualScriptNodeDescription::Type::GetScriptOwner, true);
      AddDataOutput(getOwnerNode, xiiVisualScriptDataType::TypedPointer);

      defaultInput.m_pSourceNode      = &getOwnerNode;
      defaultInput.m_uiSourcePinIndex = 0;
      m_DefaultInputs.Insert(pDataType, defaultInput);
    }
  }

  return defaultInput;
}

xiiVisualScriptCompiler::AstNode* xiiVisualScriptCompiler::CreateConstantNode(const xiiVariant& value)
{
  xiiVisualScriptDataType::Enum valueDataType = xiiVisualScriptDataType::FromVariantType(value.GetType());

  auto& constantNode   = CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Constant, valueDataType, true);
  constantNode.m_Value = value;

  AddDataOutput(constantNode, valueDataType);

  return &constantNode;
}

xiiVisualScriptCompiler::AstNode* xiiVisualScriptCompiler::CreateJumpNode(AstNode* pTargetNode)
{
  auto& jumpNode   = CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Jump);
  jumpNode.m_Value = xiiUInt64(*reinterpret_cast<size_t*>(&pTargetNode));

  return &jumpNode;
}

xiiVisualScriptCompiler::DataOffset xiiVisualScriptCompiler::GetInstanceDataOffset(xiiHashedString sName, xiiVisualScriptDataType::Enum dataType)
{
  xiiVisualScriptInstanceData instanceData;
  if (m_Module.m_InstanceDataMapping.m_Content.TryGetValue(sName, instanceData) == false)
  {
    XII_ASSERT_DEBUG(dataType < xiiVisualScriptDataType::Count, "Invalid data type");
    auto& offsetAndCount                     = m_Module.m_InstanceDataDesc.m_PerTypeInfo[dataType];
    instanceData.m_DataOffset.m_uiByteOffset = offsetAndCount.m_uiCount;
    instanceData.m_DataOffset.m_uiType       = dataType;
    instanceData.m_DataOffset.m_uiSource     = DataOffset::Source::Instance;
    ++offsetAndCount.m_uiCount;

    m_pManager->GetVariableDefaultValue(sName, instanceData.m_DefaultValue).AssertSuccess();

    m_Module.m_InstanceDataMapping.m_Content.Insert(sName, instanceData);
  }

  return instanceData.m_DataOffset;
}

xiiVisualScriptCompiler::AstNode* xiiVisualScriptCompiler::BuildAST(const xiiDocumentObject* pEntryObject)
{
  m_DefaultInputs.Clear();

  xiiHashTable<const xiiDocumentObject*, AstNode*> objectToAstNode;
  xiiHybridArray<const xiiVisualScriptPin*, 16>    pins;

  auto CreateAstNodeFromObject = [&](const xiiDocumentObject* pObject) -> AstNode* {
    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    XII_ASSERT_DEV(pNodeDesc != nullptr, "Invalid node type");

    auto& astNode = CreateAstNode(pNodeDesc->m_Type, FinalizeDataType(GetDeductedType(pObject)), pNodeDesc->m_bImplicitExecution);
    if (FillUserData(astNode, this, pObject, pEntryObject).Failed())
      return nullptr;

    objectToAstNode.Insert(pObject, &astNode);

    return &astNode;
  };

  AstNode* pEntryAstNode = CreateAstNodeFromObject(pEntryObject);
  if (pEntryAstNode == nullptr)
    return nullptr;

  if (xiiVisualScriptNodeDescription::Type::IsEntry(pEntryAstNode->m_Type) == false)
  {
    auto& astNode = CreateAstNode(xiiVisualScriptNodeDescription::Type::EntryCall);
    astNode.m_Next.PushBack(pEntryAstNode);

    pEntryAstNode = &astNode;
  }

  xiiHybridArray<const xiiDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pEntryObject);

  while (nodeStack.IsEmpty() == false)
  {
    const xiiDocumentObject* pObject = nodeStack.PeekBack();
    nodeStack.PopBack();

    AstNode* pAstNode = nullptr;
    XII_VERIFY(objectToAstNode.TryGetValue(pObject, pAstNode), "Implementation error");

    if (xiiVisualScriptNodeDescription::Type::MakesOuterCoroutine(pAstNode->m_Type))
    {
      MarkAsCoroutine(pEntryAstNode);
    }

    m_pManager->GetInputDataPins(pObject, pins);
    xiiUInt32 uiNextInputPinIndex = 0;

    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    for (auto& pinDesc : pNodeDesc->m_InputPins)
    {
      if (pinDesc.IsExecutionPin())
        continue;

      AstNode* pAstNodeToAddInput = pAstNode;
      bool     bArrayInput        = false;
      if (pNodeDesc->m_Type != xiiVisualScriptNodeDescription::Type::Builtin_MakeArray && pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
      {
        const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(pinDesc.m_sDynamicPinProperty);
        if (pProp == nullptr)
          return nullptr;

        if (pProp->GetCategory() == xiiPropertyCategory::Array)
        {
          auto pMakeArrayAstNode = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_MakeArray, true);
          AddDataOutput(*pMakeArrayAstNode, xiiVisualScriptDataType::Array);

          AddDataInput(*pAstNode, pMakeArrayAstNode, 0, xiiVisualScriptDataType::Array);

          pAstNodeToAddInput = pMakeArrayAstNode;
          bArrayInput        = true;
        }
      }

      while (uiNextInputPinIndex < pins.GetCount())
      {
        auto pPin = pins[uiNextInputPinIndex];

        xiiStringView sPropertyName = pPin->GetName();
        xiiUInt32     uiArrayIndex  = 0;
        ExtractPropertyName(sPropertyName, sPropertyName, &uiArrayIndex).IgnoreResult();

        if (pinDesc.m_sName.GetView() != sPropertyName)
          break;

        auto connections = m_pManager->GetConnections(*pPin);
        if (pPin->IsRequired() && connections.IsEmpty())
        {
          xiiLog::Error("Required input '{}' for '{}' is not connected", pPin->GetName(), GetNiceTypeName(pObject));
          return nullptr;
        }

        xiiVisualScriptDataType::Enum targetDataType = pPin->GetResolvedScriptDataType();
        if (targetDataType == xiiVisualScriptDataType::Invalid)
        {
          xiiLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pObject), pPin->GetName());
          return nullptr;
        }

        auto& dataInput      = pAstNodeToAddInput->m_Inputs.ExpandAndGetRef();
        dataInput.m_uiId     = GetPinId(pPin);
        dataInput.m_DataType = bArrayInput ? xiiVisualScriptDataType::Variant : FinalizeDataType(targetDataType);

        if (connections.IsEmpty())
        {
          if (xiiVisualScriptDataType::IsPointer(dataInput.m_DataType))
          {
            auto defaultInput = GetDefaultPointerInput(pPin->GetDataType());
            if (defaultInput.m_pSourceNode != nullptr)
            {
              dataInput.m_pSourceNode      = defaultInput.m_pSourceNode;
              dataInput.m_uiSourcePinIndex = defaultInput.m_uiSourcePinIndex;
            }
          }
          else
          {
            xiiStringBuilder sTmp;
            const char*      szPropertyName = sPropertyName.GetData(sTmp);

            xiiVariant value = pObject->GetTypeAccessor().GetValue(szPropertyName);
            if (value.IsValid() && pPin->HasDynamicPinProperty())
            {
              XII_ASSERT_DEBUG(value.IsA<xiiVariantArray>(), "Implementation error");
              value = value.Get<xiiVariantArray>()[uiArrayIndex];
            }

            xiiVisualScriptDataType::Enum valueDataType = xiiVisualScriptDataType::FromVariantType(value.GetType());
            if (dataInput.m_DataType != xiiVisualScriptDataType::Variant)
            {
              value = value.ConvertTo(xiiVisualScriptDataType::GetVariantType(dataInput.m_DataType));
              if (value.IsValid() == false)
              {
                xiiLog::Error("Failed to convert '{}.{}' of type '{}' to '{}'.", GetNiceTypeName(pObject), pPin->GetName(), xiiVisualScriptDataType::GetName(valueDataType), xiiVisualScriptDataType::GetName(dataInput.m_DataType));
                return nullptr;
              }
            }

            dataInput.m_pSourceNode      = CreateConstantNode(value);
            dataInput.m_uiSourcePinIndex = 0;
          }
        }
        else
        {
          auto&                    sourcePin     = static_cast<const xiiVisualScriptPin&>(connections[0]->GetSourcePin());
          const xiiDocumentObject* pSourceObject = sourcePin.GetParent();

          AstNode* pSourceAstNode;
          if (objectToAstNode.TryGetValue(pSourceObject, pSourceAstNode) == false)
          {
            pSourceAstNode = CreateAstNodeFromObject(pSourceObject);
            if (pSourceAstNode == nullptr)
              return nullptr;

            nodeStack.PushBack(pSourceObject);
          }

          xiiVisualScriptDataType::Enum sourceDataType = sourcePin.GetResolvedScriptDataType();
          if (sourceDataType == xiiVisualScriptDataType::Invalid)
          {
            xiiLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pSourceObject), sourcePin.GetName());
            return nullptr;
          }

          if (sourcePin.CanConvertTo(*pPin) == false)
          {
            xiiLog::Error("Can't implicitly convert pin '{}.{}' of type '{}' connected to pin '{}.{}' of type '{}'", GetNiceTypeName(pSourceObject), sourcePin.GetName(), sourcePin.GetDataTypeName(), GetNiceTypeName(pObject), pPin->GetName(), pPin->GetDataTypeName());
            return nullptr;
          }

          dataInput.m_pSourceNode      = pSourceAstNode;
          dataInput.m_uiSourcePinIndex = sourcePin.GetDataPinIndex();
        }

        ++uiNextInputPinIndex;
      }
    }

    m_pManager->GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& dataOutput      = pAstNode->m_Outputs.ExpandAndGetRef();
      dataOutput.m_uiId     = GetPinId(pPin);
      dataOutput.m_DataType = FinalizeDataType(pPin->GetResolvedScriptDataType());
    }

    m_pManager->GetOutputExecutionPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto connections = m_pManager->GetConnections(*pPin);
      if (connections.IsEmpty() || pPin->SplitExecution())
      {
        pAstNode->m_Next.PushBack(nullptr);
        continue;
      }

      XII_ASSERT_DEV(connections.GetCount() == 1, "Output execution pins should only have one connection");
      const xiiDocumentObject* pNextNode = connections[0]->GetTargetPin().GetParent();

      AstNode* pNextAstNode;
      if (objectToAstNode.TryGetValue(pNextNode, pNextAstNode) == false)
      {
        pNextAstNode = CreateAstNodeFromObject(pNextNode);
        if (pNextAstNode == nullptr)
          return nullptr;

        nodeStack.PushBack(pNextNode);
      }

      pAstNode->m_Next.PushBack(pNextAstNode);
    }
  }

  return pEntryAstNode;
}

void xiiVisualScriptCompiler::MarkAsCoroutine(AstNode* pEntryAstNode)
{
  switch (pEntryAstNode->m_Type)
  {
    case xiiVisualScriptNodeDescription::Type::EntryCall:
      pEntryAstNode->m_Type = xiiVisualScriptNodeDescription::Type::EntryCall_Coroutine;
      break;
    case xiiVisualScriptNodeDescription::Type::MessageHandler:
      pEntryAstNode->m_Type = xiiVisualScriptNodeDescription::Type::MessageHandler_Coroutine;
      break;
    case xiiVisualScriptNodeDescription::Type::EntryCall_Coroutine:
    case xiiVisualScriptNodeDescription::Type::MessageHandler_Coroutine:
      // Already a coroutine
      break;
      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

xiiResult xiiVisualScriptCompiler::ReplaceUnsupportedNodes(AstNode* pEntryAstNode)
{
  XII_SUCCEED_OR_RETURN(TraverseExecutionConnections(pEntryAstNode,
                                                     [&](Connection& connection) {
                                                       AstNode* pNode = connection.m_pCurrent;

                                                       if (xiiVisualScriptNodeDescription::Type::IsLoop(pNode->m_Type))
                                                       {
                                                         if (ReplaceLoop(connection).Failed())
                                                           return VisitorResult::Error;
                                                       }

                                                       return VisitorResult::Continue;
                                                     }));

  return TraverseExecutionConnections(pEntryAstNode,
                                      [&](Connection& connection) {
                                        AstNode* pNode = connection.m_pCurrent;

                                        if (pNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_CompareExec)
                                        {
                                          const auto& dataInputA = pNode->m_Inputs[0];
                                          const auto& dataInputB = pNode->m_Inputs[1];

                                          auto& compareNode   = CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Compare, pNode->m_DeductedDataType, false);
                                          compareNode.m_Value = pNode->m_Value;
                                          AddDataInput(compareNode, dataInputA.m_pSourceNode, dataInputA.m_uiSourcePinIndex, dataInputA.m_DataType);
                                          AddDataInput(compareNode, dataInputB.m_pSourceNode, dataInputB.m_uiSourcePinIndex, dataInputB.m_DataType);
                                          AddDataOutput(compareNode, xiiVisualScriptDataType::Bool);

                                          auto& branchNode = CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Branch);
                                          AddDataInput(branchNode, &compareNode, 0, xiiVisualScriptDataType::Bool);
                                          branchNode.m_Next.PushBack(pNode->m_Next[0]);
                                          branchNode.m_Next.PushBack(pNode->m_Next[1]);

                                          compareNode.m_Next.PushBack(&branchNode);
                                          connection.m_pPrev->m_Next[connection.m_uiPrevPinIndex] = &compareNode;
                                          connection.m_pCurrent                                   = &branchNode;
                                        }

                                        return VisitorResult::Continue;
                                      });
}

xiiResult xiiVisualScriptCompiler::ReplaceLoop(Connection& connection)
{
  AstNode* pLoopInitStart      = nullptr;
  AstNode* pLoopInitEnd        = nullptr;
  AstNode* pLoopConditionStart = nullptr;
  AstNode* pLoopConditionEnd   = nullptr;
  AstNode* pLoopIncrementStart = nullptr;
  AstNode* pLoopIncrementEnd   = nullptr;

  AstNode* pLoopElement = nullptr;
  AstNode* pLoopIndex   = nullptr;

  AstNode* pLoopNode      = connection.m_pCurrent;
  AstNode* pLoopBody      = pLoopNode->m_Next[0];
  AstNode* pLoopCompleted = pLoopNode->m_Next[1];
  auto     loopType       = pLoopNode->m_Type;

  if (loopType == xiiVisualScriptNodeDescription::Type::Builtin_WhileLoop)
  {
    pLoopConditionEnd                       = pLoopNode->m_Inputs[0].m_pSourceNode;
    pLoopConditionEnd->m_bImplicitExecution = false;

    XII_SUCCEED_OR_RETURN(InlineConstants(pLoopConditionEnd));
    XII_SUCCEED_OR_RETURN(InsertTypeConversions(pLoopConditionEnd));
    XII_SUCCEED_OR_RETURN(InlineVariables(pLoopConditionEnd));

    xiiHybridArray<AstNode*, 64> nodeStack;
    XII_SUCCEED_OR_RETURN(BuildDataStack(pLoopConditionEnd, nodeStack));

    if (nodeStack.IsEmpty())
    {
      pLoopConditionStart = pLoopConditionEnd;
    }
    else
    {
      for (auto pDataNode : nodeStack)
      {
        pDataNode->m_bImplicitExecution = false;
      }

      pLoopConditionStart = nodeStack.PeekBack();

      AstNode* pLastDataNode = nodeStack[0];
      pLastDataNode->m_Next.PushBack(pLoopConditionEnd);
    }
  }
  else if (loopType == xiiVisualScriptNodeDescription::Type::Builtin_ForLoop)
  {
    auto& firstIndexInput = pLoopNode->m_Inputs[0];
    auto& lastIndexInput  = pLoopNode->m_Inputs[1];

    // Loop Init
    {
      pLoopInitStart = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_ToInt, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopInitStart, firstIndexInput.m_pSourceNode, firstIndexInput.m_uiSourcePinIndex, firstIndexInput.m_DataType);
      AddDataOutput(*pLoopInitStart, xiiVisualScriptDataType::Int);

      pLoopInitEnd = pLoopInitStart;

      pLoopIndex = pLoopInitStart;
    }

    // Loop Condition
    {
      pLoopConditionStart          = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Compare, xiiVisualScriptDataType::Int);
      pLoopConditionStart->m_Value = xiiInt64(xiiComparisonOperator::LessEqual);
      AddDataInput(*pLoopConditionStart, pLoopInitStart, 0, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionStart, lastIndexInput.m_pSourceNode, lastIndexInput.m_uiSourcePinIndex, lastIndexInput.m_DataType);
      AddDataOutput(*pLoopConditionStart, xiiVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }

    // Loop Increment
    {
      pLoopIncrementStart = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Add, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, pLoopIndex, 0, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, CreateConstantNode(1), 0, xiiVisualScriptDataType::Int);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput      = pLoopIncrementStart->m_Outputs.ExpandAndGetRef();
      dataOutput.m_uiId     = pLoopIndex->m_Outputs[0].m_uiId;
      dataOutput.m_DataType = xiiVisualScriptDataType::Int;

      pLoopIncrementEnd = pLoopIncrementStart;
    }
  }
  else if (loopType == xiiVisualScriptNodeDescription::Type::Builtin_ForEachLoop ||
           loopType == xiiVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop)
  {
    const bool isReverse  = (loopType == xiiVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop);
    auto&      arrayInput = pLoopNode->m_Inputs[0];

    // Loop Init
    if (isReverse)
    {
      pLoopInitStart = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      AddDataInput(*pLoopInitStart, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);
      AddDataOutput(*pLoopInitStart, xiiVisualScriptDataType::Int);

      pLoopInitEnd = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Subtract, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopInitEnd, pLoopInitStart, 0, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopInitEnd, CreateConstantNode(1), 0, xiiVisualScriptDataType::Int);
      AddDataOutput(*pLoopInitEnd, xiiVisualScriptDataType::Int);

      pLoopInitStart->m_Next.PushBack(pLoopInitEnd);

      pLoopIndex = pLoopInitEnd;
    }
    else
    {
      pLoopInitStart = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_ToInt, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopInitStart, CreateConstantNode(0), 0, xiiVisualScriptDataType::Int);
      AddDataOutput(*pLoopInitStart, xiiVisualScriptDataType::Int);

      pLoopInitEnd = pLoopInitStart;

      pLoopIndex = pLoopInitStart;
    }

    // Loop Condition
    if (isReverse)
    {
      pLoopConditionStart          = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Compare, xiiVisualScriptDataType::Int);
      pLoopConditionStart->m_Value = xiiInt64(xiiComparisonOperator::GreaterEqual);
      AddDataInput(*pLoopConditionStart, pLoopIndex, 0, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionStart, CreateConstantNode(0), 0, xiiVisualScriptDataType::Int);
      AddDataOutput(*pLoopConditionStart, xiiVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }
    else
    {
      pLoopConditionStart = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      AddDataInput(*pLoopConditionStart, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);
      AddDataOutput(*pLoopConditionStart, xiiVisualScriptDataType::Int);

      pLoopConditionEnd          = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Compare, xiiVisualScriptDataType::Int);
      pLoopConditionEnd->m_Value = xiiInt64(xiiComparisonOperator::Less);
      AddDataInput(*pLoopConditionEnd, pLoopIndex, 0, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionEnd, pLoopConditionStart, 0, xiiVisualScriptDataType::Int);
      AddDataOutput(*pLoopConditionEnd, xiiVisualScriptDataType::Bool);

      pLoopConditionStart->m_Next.PushBack(pLoopConditionEnd);
    }

    // Loop Increment
    {
      auto incType = isReverse ? xiiVisualScriptNodeDescription::Type::Builtin_Subtract : xiiVisualScriptNodeDescription::Type::Builtin_Add;

      pLoopIncrementStart = &CreateAstNode(incType, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, pLoopIndex, 0, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, CreateConstantNode(1), 0, xiiVisualScriptDataType::Int);

      // Dummy input that is not used at runtime but prevents the array from being re-used across the loop's lifetime
      AddDataInput(*pLoopIncrementStart, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput      = pLoopIncrementStart->m_Outputs.ExpandAndGetRef();
      dataOutput.m_uiId     = pLoopIndex->m_Outputs[0].m_uiId;
      dataOutput.m_DataType = xiiVisualScriptDataType::Int;

      pLoopIncrementEnd = pLoopIncrementStart;
    }

    pLoopElement = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Array_GetElement, xiiVisualScriptDataType::Invalid, true);
    AddDataInput(*pLoopElement, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);
    AddDataInput(*pLoopElement, pLoopIndex, 0, xiiVisualScriptDataType::Int);
    AddDataOutput(*pLoopElement, xiiVisualScriptDataType::Variant);
  }
  else
  {
    XII_ASSERT_NOT_IMPLEMENTED;
  }

  pLoopNode->m_Inputs.Clear();
  pLoopNode->m_Next.Clear();

  {
    auto& branchNode = CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Branch);
    AddDataInput(branchNode, pLoopConditionEnd, 0, xiiVisualScriptDataType::Bool);

    if (pLoopConditionStart->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_Constant)
    {
      pLoopConditionStart = &branchNode;
    }
    else
    {
      pLoopConditionEnd->m_bImplicitExecution = false;
      pLoopConditionEnd->m_Next.PushBack(&branchNode);
    }

    branchNode.m_Next.PushBack(pLoopBody);      // True -> LoopBody
    branchNode.m_Next.PushBack(pLoopCompleted); // False -> Completed
    pLoopConditionEnd = &branchNode;
  }

  if (pLoopInitStart != nullptr)
  {
    connection.m_pPrev->m_Next[connection.m_uiPrevPinIndex] = pLoopInitStart;
    pLoopInitEnd->m_Next.PushBack(pLoopConditionStart);
  }
  else
  {
    connection.m_pPrev->m_Next[connection.m_uiPrevPinIndex] = pLoopConditionStart;
  }

  AstNode* pJumpNode = CreateJumpNode(pLoopConditionStart);
  if (pLoopIncrementStart != nullptr)
  {
    pLoopIncrementEnd->m_Next.PushBack(pJumpNode);
    pJumpNode = pLoopIncrementStart;
  }

  XII_SUCCEED_OR_RETURN(TraverseAllConnections(pLoopBody,
                                               [&](Connection& connection) {
                                                 if (connection.m_pPrev == nullptr)
                                                 {
                                                   connection.m_pPrev          = pLoopConditionEnd;
                                                   connection.m_uiPrevPinIndex = 0;
                                                 }

                                                 if (xiiVisualScriptNodeDescription::Type::IsLoop(connection.m_pCurrent->m_Type))
                                                 {
                                                   if (ReplaceLoop(connection).Failed())
                                                     return VisitorResult::Error;
                                                 }

                                                 if (connection.m_Type == ConnectionType::Data && connection.m_pCurrent->m_bImplicitExecution == false)
                                                   return VisitorResult::Skip;

                                                 AstNode* pNode = connection.m_pCurrent;

                                                 if (pNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_Break)
                                                 {
                                                   connection.m_pPrev->m_Next[connection.m_uiPrevPinIndex] = pLoopCompleted;
                                                   return VisitorResult::Continue;
                                                 }

                                                 for (auto& pNext : pNode->m_Next)
                                                 {
                                                   if (pNext == nullptr)
                                                   {
                                                     pNext = pJumpNode;
                                                   }
                                                 }

                                                 for (auto& dataInput : pNode->m_Inputs)
                                                 {
                                                   if (loopType == xiiVisualScriptNodeDescription::Type::Builtin_ForEachLoop ||
                                                       loopType == xiiVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop)
                                                   {
                                                     if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 0)
                                                     {
                                                       dataInput.m_pSourceNode = pLoopElement;
                                                     }
                                                     else if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 1)
                                                     {
                                                       dataInput.m_pSourceNode      = pLoopIndex;
                                                       dataInput.m_uiSourcePinIndex = 0;
                                                     }
                                                   }
                                                   else
                                                   {
                                                     if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 0)
                                                     {
                                                       dataInput.m_pSourceNode = pLoopIndex;
                                                     }
                                                   }
                                                 }

                                                 return VisitorResult::Continue;
                                               }));

  connection.m_pPrev          = pLoopConditionEnd;
  connection.m_pCurrent       = pLoopCompleted;
  connection.m_uiPrevPinIndex = 1;

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::InsertTypeConversions(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
                                [&](const Connection& connection) {
                                  if (connection.m_Type == ConnectionType::Data)
                                  {
                                    auto& dataInput  = connection.m_pPrev->m_Inputs[connection.m_uiPrevPinIndex];
                                    auto& dataOutput = GetDataOutput(dataInput);

                                    if (dataOutput.m_DataType != dataInput.m_DataType)
                                    {
                                      auto nodeType = xiiVisualScriptNodeDescription::Type::GetConversionType(dataInput.m_DataType);

                                      auto& astNode = CreateAstNode(nodeType, dataOutput.m_DataType, true);
                                      AddDataInput(astNode, dataInput.m_pSourceNode, dataInput.m_uiSourcePinIndex, dataOutput.m_DataType);
                                      AddDataOutput(astNode, dataInput.m_DataType);

                                      dataInput.m_pSourceNode      = &astNode;
                                      dataInput.m_uiSourcePinIndex = 0;
                                    }
                                  }

                                  return VisitorResult::Continue;
                                });
}

xiiResult xiiVisualScriptCompiler::InlineConstants(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
                                [&](const Connection& connection) {
                                  auto pCurrentNode = connection.m_pCurrent;
                                  for (auto& dataInput : pCurrentNode->m_Inputs)
                                  {
                                    if (m_PinIdToDataDesc.Contains(dataInput.m_uiId))
                                      continue;

                                    auto pSourceNode = dataInput.m_pSourceNode;
                                    if (pSourceNode == nullptr)
                                      continue;

                                    if (pSourceNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_Constant)
                                    {
                                      auto dataType = pSourceNode->m_DeductedDataType;

                                      xiiUInt32 uiIndex = xiiInvalidIndex;
                                      if (m_ConstantDataToIndex.TryGetValue(pSourceNode->m_Value, uiIndex) == false)
                                      {
                                        auto& offsetAndCount = m_Module.m_ConstantDataDesc.m_PerTypeInfo[dataType];
                                        uiIndex              = offsetAndCount.m_uiCount;
                                        ++offsetAndCount.m_uiCount;

                                        m_ConstantDataToIndex.Insert(pSourceNode->m_Value, uiIndex);
                                      }

                                      DataDesc dataDesc;
                                      dataDesc.m_DataOffset = DataOffset(uiIndex, dataType, DataOffset::Source::Constant);
                                      m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);

                                      dataInput.m_pSourceNode      = nullptr;
                                      dataInput.m_uiSourcePinIndex = 0;
                                    }
                                  }

                                  return VisitorResult::Continue;
                                });
}

xiiResult xiiVisualScriptCompiler::InlineVariables(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
                                [&](const Connection& connection) {
                                  auto pCurrentNode = connection.m_pCurrent;
                                  for (auto& dataInput : pCurrentNode->m_Inputs)
                                  {
                                    if (m_PinIdToDataDesc.Contains(dataInput.m_uiId))
                                      continue;

                                    auto pSourceNode = dataInput.m_pSourceNode;
                                    if (pSourceNode == nullptr)
                                      continue;

                                    if (pSourceNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_GetVariable)
                                    {
                                      auto& dataOutput = pSourceNode->m_Outputs[0];

                                      xiiHashedString sName;
                                      sName.Assign(pSourceNode->m_Value.Get<xiiString>());

                                      DataDesc dataDesc;
                                      dataDesc.m_DataOffset = GetInstanceDataOffset(sName, dataOutput.m_DataType);
                                      m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);

                                      dataInput.m_pSourceNode      = nullptr;
                                      dataInput.m_uiSourcePinIndex = 0;
                                    }
                                  }

                                  if (pCurrentNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_SetVariable ||
                                      pCurrentNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_IncVariable ||
                                      pCurrentNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_DecVariable)
                                  {
                                    xiiHashedString sName;
                                    sName.Assign(pCurrentNode->m_Value.Get<xiiString>());

                                    DataDesc dataDesc;
                                    dataDesc.m_DataOffset = GetInstanceDataOffset(sName, pCurrentNode->m_DeductedDataType);

                                    if (pCurrentNode->m_Type != xiiVisualScriptNodeDescription::Type::Builtin_SetVariable)
                                    {
                                      if (pCurrentNode->m_Inputs.IsEmpty())
                                      {
                                        AddDataInput(*pCurrentNode, nullptr, 0, pCurrentNode->m_DeductedDataType);
                                      }

                                      m_PinIdToDataDesc.Insert(pCurrentNode->m_Inputs[0].m_uiId, dataDesc);
                                    }

                                    {
                                      if (pCurrentNode->m_Outputs.IsEmpty())
                                      {
                                        AddDataOutput(*pCurrentNode, pCurrentNode->m_DeductedDataType);
                                      }

                                      m_PinIdToDataDesc.Insert(pCurrentNode->m_Outputs[0].m_uiId, dataDesc);
                                    }
                                  }

                                  return VisitorResult::Continue;
                                });
}

xiiResult xiiVisualScriptCompiler::BuildDataStack(AstNode* pEntryAstNode, xiiDynamicArray<AstNode*>& out_Stack)
{
  xiiHashSet<const AstNode*> visitedNodes;
  out_Stack.Clear();

  XII_SUCCEED_OR_RETURN(TraverseDataConnections(
    pEntryAstNode,
    [&](const Connection& connection) {
      if (connection.m_pCurrent->m_bImplicitExecution == false)
        return VisitorResult::Skip;

      if (visitedNodes.Insert(connection.m_pCurrent))
      {
        // If the node was already visited, remove it again so it is moved to the top of the stack
        out_Stack.RemoveAndCopy(connection.m_pCurrent);
      }

      out_Stack.PushBack(connection.m_pCurrent);

      return VisitorResult::Continue;
    },
    false));

  // Make unique
  xiiHashTable<AstNode*, AstNode*> oldToNewNodes;
  for (xiiUInt32 i = out_Stack.GetCount(); i > 0; --i)
  {
    auto& pDataNode = out_Stack[i - 1];

    if (pDataNode->m_Next.IsEmpty())
    {
      // remap inputs to new nodes
      for (auto& dataInput : pDataNode->m_Inputs)
      {
        AstNode* pNewNode = nullptr;
        if (oldToNewNodes.TryGetValue(dataInput.m_pSourceNode, pNewNode))
        {
          dataInput.m_pSourceNode = pNewNode;
        }
      }
    }
    else
    {
      auto& newDataNode             = CreateAstNode(pDataNode->m_Type, pDataNode->m_DeductedDataType, pDataNode->m_bImplicitExecution);
      newDataNode.m_sTargetTypeName = pDataNode->m_sTargetTypeName;
      newDataNode.m_Value           = pDataNode->m_Value;

      for (auto& dataInput : pDataNode->m_Inputs)
      {
        AstNode* pSourceNode = dataInput.m_pSourceNode;
        if (oldToNewNodes.TryGetValue(dataInput.m_pSourceNode, pSourceNode) == false)
        {
          XII_ASSERT_DEBUG(dataInput.m_pSourceNode == nullptr || dataInput.m_pSourceNode->m_bImplicitExecution == false, "");
        }

        AddDataInput(newDataNode, pSourceNode, dataInput.m_uiSourcePinIndex, dataInput.m_DataType);

        DataDesc dataDesc;
        if (m_PinIdToDataDesc.TryGetValue(dataInput.m_uiId, dataDesc))
        {
          m_PinIdToDataDesc.Insert(newDataNode.m_Inputs.PeekBack().m_uiId, dataDesc);
        }
      }

      for (auto& dataOutput : pDataNode->m_Outputs)
      {
        AddDataOutput(newDataNode, dataOutput.m_DataType);

        DataDesc dataDesc;
        if (m_PinIdToDataDesc.TryGetValue(dataOutput.m_uiId, dataDesc))
        {
          m_PinIdToDataDesc.Insert(newDataNode.m_Outputs.PeekBack().m_uiId, dataDesc);
        }
      }

      oldToNewNodes.Insert(pDataNode, &newDataNode);
      pDataNode = &newDataNode;
    }
  }

  // Connect next execution
  if (out_Stack.GetCount() > 1)
  {
    AstNode* pLastDataNode = out_Stack.PeekBack();
    for (xiiUInt32 i = out_Stack.GetCount() - 1; i > 0; --i)
    {
      auto& pDataNode = out_Stack[i - 1];
      pLastDataNode->m_Next.PushBack(pDataNode);
      pLastDataNode = pDataNode;
    }
  }

  // Remap inputs
  for (auto& dataInput : pEntryAstNode->m_Inputs)
  {
    if (dataInput.m_pSourceNode != nullptr)
    {
      oldToNewNodes.TryGetValue(dataInput.m_pSourceNode, dataInput.m_pSourceNode);
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::BuildDataExecutions(AstNode* pEntryAstNode)
{
  xiiHybridArray<Connection, 64> allExecConnections;

  XII_SUCCEED_OR_RETURN(TraverseExecutionConnections(pEntryAstNode,
                                                     [&](const Connection& connection) {
                                                       allExecConnections.PushBack(connection);
                                                       return VisitorResult::Continue;
                                                     }));

  xiiHybridArray<AstNode*, 64>     nodeStack;
  xiiHashTable<AstNode*, AstNode*> nodeToFirstDataNode;

  for (const auto& connection : allExecConnections)
  {
    AstNode* pFirstDataNode = nullptr;
    if (nodeToFirstDataNode.TryGetValue(connection.m_pCurrent, pFirstDataNode) == false)
    {
      if (BuildDataStack(connection.m_pCurrent, nodeStack).Failed())
        return XII_FAILURE;

      if (nodeStack.IsEmpty() == false)
      {
        pFirstDataNode = nodeStack.PeekBack();

        AstNode* pLastDataNode = nodeStack[0];
        pLastDataNode->m_Next.PushBack(connection.m_pCurrent);
      }
    }

    if (pFirstDataNode != nullptr)
    {
      connection.m_pPrev->m_Next[connection.m_uiPrevPinIndex] = pFirstDataNode;
    }
    nodeToFirstDataNode.Insert(connection.m_pCurrent, pFirstDataNode);
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::FillDataOutputConnections(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
                                [&](const Connection& connection) {
                                  if (connection.m_Type == ConnectionType::Data)
                                  {
                                    auto& dataInput  = connection.m_pPrev->m_Inputs[connection.m_uiPrevPinIndex];
                                    auto& dataOutput = GetDataOutput(dataInput);

                                    XII_ASSERT_DEBUG(dataInput.m_pSourceNode == connection.m_pCurrent, "");
                                    if (dataOutput.m_TargetNodes.Contains(connection.m_pPrev) == false)
                                    {
                                      dataOutput.m_TargetNodes.PushBack(connection.m_pPrev);
                                    }
                                  }

                                  return VisitorResult::Continue;
                                });
}

xiiResult xiiVisualScriptCompiler::AssignLocalVariables(AstNode* pEntryAstNode, xiiVisualScriptDataDescription& inout_localDataDesc)
{
  xiiDynamicArray<DataOffset> freeDataOffsets;

  return TraverseExecutionConnections(pEntryAstNode,
                                      [&](const Connection& connection) {
                                        // Outputs first so we don't end up using the same data as input and output
                                        for (auto& dataOutput : connection.m_pCurrent->m_Outputs)
                                        {
                                          if (m_PinIdToDataDesc.Contains(dataOutput.m_uiId))
                                            continue;

                                          if (dataOutput.m_TargetNodes.IsEmpty() == false)
                                          {
                                            DataOffset dataOffset;
                                            dataOffset.m_uiType = dataOutput.m_DataType;

                                            for (xiiUInt32 i = 0; i < freeDataOffsets.GetCount(); ++i)
                                            {
                                              auto freeDataOffset = freeDataOffsets[i];
                                              if (freeDataOffset.m_uiType == dataOffset.m_uiType)
                                              {
                                                dataOffset = freeDataOffset;
                                                freeDataOffsets.RemoveAtAndSwap(i);
                                                break;
                                              }
                                            }

                                            if (dataOffset.IsValid() == false)
                                            {
                                              XII_ASSERT_DEBUG(dataOffset.GetType() < xiiVisualScriptDataType::Count, "Invalid data type");
                                              auto& offsetAndCount      = inout_localDataDesc.m_PerTypeInfo[dataOffset.m_uiType];
                                              dataOffset.m_uiByteOffset = offsetAndCount.m_uiCount;
                                              ++offsetAndCount.m_uiCount;
                                            }

                                            DataDesc dataDesc;
                                            dataDesc.m_DataOffset     = dataOffset;
                                            dataDesc.m_uiUsageCounter = dataOutput.m_TargetNodes.GetCount();
                                            m_PinIdToDataDesc.Insert(dataOutput.m_uiId, dataDesc);
                                          }
                                        }

                                        for (auto& dataInput : connection.m_pCurrent->m_Inputs)
                                        {
                                          if (m_PinIdToDataDesc.Contains(dataInput.m_uiId) || dataInput.m_pSourceNode == nullptr)
                                            continue;

                                          auto&     dataOutput = GetDataOutput(dataInput);
                                          DataDesc* pDataDesc  = nullptr;
                                          m_PinIdToDataDesc.TryGetValue(dataOutput.m_uiId, pDataDesc);
                                          if (pDataDesc == nullptr)
                                          {
                                            xiiLog::Error("Internal Compiler Error: Local variable for output id {} is not yet assigned.", dataOutput.m_uiId);
                                            return VisitorResult::Error;
                                          }

                                          --pDataDesc->m_uiUsageCounter;
                                          if (pDataDesc->m_uiUsageCounter == 0 && pDataDesc->m_DataOffset.IsLocal())
                                          {
                                            freeDataOffsets.PushBack(pDataDesc->m_DataOffset);
                                          }

                                          // Make a copy first because Insert() might re-allocate and the pointer might point to dead memory afterwards.
                                          DataDesc dataDesc = *pDataDesc;
                                          m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);
                                        }

                                        return VisitorResult::Continue;
                                      });
}

xiiResult xiiVisualScriptCompiler::BuildNodeDescriptions(AstNode* pEntryAstNode, xiiDynamicArray<xiiVisualScriptNodeDescription>& out_NodeDescriptions)
{
  xiiHashTable<const AstNode*, xiiUInt32> astNodeToNodeDescIndices;
  out_NodeDescriptions.Clear();

  auto CreateNodeDesc = [&](const AstNode& astNode, xiiUInt32& out_uiNodeDescIndex) -> xiiResult {
    out_uiNodeDescIndex = out_NodeDescriptions.GetCount();

    auto& nodeDesc              = out_NodeDescriptions.ExpandAndGetRef();
    nodeDesc.m_Type             = astNode.m_Type;
    nodeDesc.m_DeductedDataType = astNode.m_DeductedDataType;
    nodeDesc.m_sTargetTypeName  = astNode.m_sTargetTypeName;
    nodeDesc.m_Value            = astNode.m_Value;

    for (auto& dataInput : astNode.m_Inputs)
    {
      DataDesc dataDesc;
      m_PinIdToDataDesc.TryGetValue(dataInput.m_uiId, dataDesc);
      nodeDesc.m_InputDataOffsets.PushBack(dataDesc.m_DataOffset);
    }

    for (auto& dataOutput : astNode.m_Outputs)
    {
      DataDesc dataDesc;
      m_PinIdToDataDesc.TryGetValue(dataOutput.m_uiId, dataDesc);
      nodeDesc.m_OutputDataOffsets.PushBack(dataDesc.m_DataOffset);
    }

    astNodeToNodeDescIndices.Insert(&astNode, out_uiNodeDescIndex);
    return XII_SUCCESS;
  };

  xiiUInt32 uiNodeDescIndex = 0;
  XII_SUCCEED_OR_RETURN(CreateNodeDesc(*pEntryAstNode, uiNodeDescIndex));

  return TraverseExecutionConnections(pEntryAstNode,
                                      [&](const Connection& connection) {
                                        xiiUInt32 uiCurrentIndex = 0;
                                        if (astNodeToNodeDescIndices.TryGetValue(connection.m_pCurrent, uiCurrentIndex) == false)
                                        {
                                          return VisitorResult::Skip;
                                        }

                                        auto pNodeDesc = &out_NodeDescriptions[uiCurrentIndex];
                                        if (pNodeDesc->m_ExecutionIndices.GetCount() == connection.m_pCurrent->m_Next.GetCount())
                                        {
                                          return VisitorResult::Continue;
                                        }

                                        for (auto pNextAstNode : connection.m_pCurrent->m_Next)
                                        {
                                          if (pNextAstNode == nullptr)
                                          {
                                            pNodeDesc->m_ExecutionIndices.PushBack(static_cast<xiiUInt16>(xiiInvalidIndex));
                                          }
                                          else if (pNextAstNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_Jump)
                                          {
                                            xiiUInt64 uiPtr          = pNextAstNode->m_Value.Get<xiiUInt64>();
                                            AstNode*  pTargetAstNode = *reinterpret_cast<AstNode**>(&uiPtr);

                                            xiiUInt32 uiNextIndex = 0;
                                            if (astNodeToNodeDescIndices.TryGetValue(pTargetAstNode, uiNextIndex) == false)
                                              return VisitorResult::Error;

                                            pNodeDesc->m_ExecutionIndices.PushBack(uiNextIndex);
                                          }
                                          else
                                          {
                                            xiiUInt32 uiNextIndex = 0;
                                            if (astNodeToNodeDescIndices.TryGetValue(pNextAstNode, uiNextIndex) == false)
                                            {
                                              if (CreateNodeDesc(*pNextAstNode, uiNextIndex).Failed())
                                                return VisitorResult::Error;

                                              // array might have been resized, fetch node desc again
                                              pNodeDesc = &out_NodeDescriptions[uiCurrentIndex];
                                            }

                                            pNodeDesc->m_ExecutionIndices.PushBack(uiNextIndex);
                                          }
                                        }

                                        return VisitorResult::Continue;
                                      });
}

xiiResult xiiVisualScriptCompiler::TraverseExecutionConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate /*= true*/)
{
  m_ReportedConnections.Clear();
  xiiHybridArray<AstNode*, 64> nodeStack;

  {
    Connection connection = {nullptr, pEntryAstNode, ConnectionType::Execution, xiiInvalidIndex};
    auto       res        = func(connection);
    if (res == VisitorResult::Skip || res == VisitorResult::Stop)
      return XII_SUCCESS;
    if (res == VisitorResult::Error)
      return XII_FAILURE;

    if (connection.m_pCurrent != nullptr)
    {
      nodeStack.PushBack(connection.m_pCurrent);
    }
  }

  while (nodeStack.IsEmpty() == false)
  {
    AstNode* pCurrentAstNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    for (xiiUInt32 i = 0; i < pCurrentAstNode->m_Next.GetCount(); ++i)
    {
      auto pNextAstNode = pCurrentAstNode->m_Next[i];
      XII_ASSERT_DEBUG(pNextAstNode != pCurrentAstNode, "");

      if (pNextAstNode == nullptr)
        continue;

      Connection connection = {pCurrentAstNode, pNextAstNode, ConnectionType::Execution, i};
      if (bDeduplicate && m_ReportedConnections.Insert(connection))
        continue;

      auto res = func(connection);
      if (res == VisitorResult::Skip)
        continue;
      if (res == VisitorResult::Stop)
        return XII_SUCCESS;
      if (res == VisitorResult::Error)
        return XII_FAILURE;

      if (connection.m_pCurrent != nullptr)
      {
        nodeStack.PushBack(connection.m_pCurrent);
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::TraverseDataConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate /*= true*/, bool bClearReportedConnections /*= true*/)
{
  if (bClearReportedConnections)
  {
    m_ReportedConnections.Clear();
  }

  xiiHybridArray<AstNode*, 64> nodeStack;

  nodeStack.PushBack(pEntryAstNode);

  while (nodeStack.IsEmpty() == false)
  {
    AstNode* pCurrentAstNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    for (xiiUInt32 i = 0; i < pCurrentAstNode->m_Inputs.GetCount(); ++i)
    {
      auto& dataInput = pCurrentAstNode->m_Inputs[i];

      if (dataInput.m_pSourceNode == nullptr)
        continue;

      Connection connection = {pCurrentAstNode, dataInput.m_pSourceNode, ConnectionType::Data, i};
      if (bDeduplicate && m_ReportedConnections.Insert(connection))
        continue;

      auto res = func(connection);
      if (res == VisitorResult::Skip)
        continue;
      if (res == VisitorResult::Stop)
        return XII_SUCCESS;
      if (res == VisitorResult::Error)
        return XII_FAILURE;

      if (connection.m_pCurrent != nullptr)
      {
        nodeStack.PushBack(connection.m_pCurrent);
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::TraverseAllConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate /*= true*/)
{
  return TraverseExecutionConnections(
    pEntryAstNode,
    [&](Connection& connection) {
      auto res = func(connection);
      if (res != VisitorResult::Continue)
        return res;

      if (TraverseDataConnections(connection.m_pCurrent, func, bDeduplicate, false).Failed())
        return VisitorResult::Error;

      return VisitorResult::Continue;
    },
    bDeduplicate);
}

xiiResult xiiVisualScriptCompiler::FinalizeDataOffsets()
{
  m_Module.m_InstanceDataDesc.CalculatePerTypeStartOffsets();
  m_Module.m_ConstantDataDesc.CalculatePerTypeStartOffsets();

  auto GetDataDesc = [this](const CompiledFunction& function, DataOffset dataOffset) -> const xiiVisualScriptDataDescription* {
    switch (dataOffset.GetSource())
    {
      case DataOffset::Source::Local:
        return &function.m_LocalDataDesc;
      case DataOffset::Source::Instance:
        return &m_Module.m_InstanceDataDesc;
      case DataOffset::Source::Constant:
        return &m_Module.m_ConstantDataDesc;
        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return nullptr;
  };

  for (auto& function : m_Module.m_Functions)
  {
    function.m_LocalDataDesc.CalculatePerTypeStartOffsets();

    for (auto& nodeDesc : function.m_NodeDescriptions)
    {
      for (auto& dataOffset : nodeDesc.m_InputDataOffsets)
      {
        dataOffset = GetDataDesc(function, dataOffset)->GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
      }

      for (auto& dataOffset : nodeDesc.m_OutputDataOffsets)
      {
        XII_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Cannot write to constant data");
        dataOffset = GetDataDesc(function, dataOffset)->GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
      }
    }
  }

  for (auto& it : m_Module.m_InstanceDataMapping.m_Content)
  {
    auto& dataOffset = it.Value().m_DataOffset;
    dataOffset       = m_Module.m_InstanceDataDesc.GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::FinalizeConstantData()
{
  m_Module.m_ConstantDataStorage.AllocateStorage();

  for (auto& it : m_ConstantDataToIndex)
  {
    const xiiVariant& value   = it.Key();
    xiiUInt32         uiIndex = it.Value();

    auto scriptDataType = xiiVisualScriptDataType::FromVariantType(value.GetType());
    if (scriptDataType == xiiVisualScriptDataType::Invalid)
    {
      scriptDataType = xiiVisualScriptDataType::Variant;
    }

    auto dataOffset = m_Module.m_ConstantDataDesc.GetOffset(scriptDataType, uiIndex, DataOffset::Source::Constant);

    m_Module.m_ConstantDataStorage.SetDataFromVariant(dataOffset, value, 0);
  }

  return XII_SUCCESS;
}

void xiiVisualScriptCompiler::DumpAST(AstNode* pEntryAstNode, xiiStringView sOutputPath, xiiStringView sFunctionName, xiiStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  xiiDGMLGraph dgmlGraph;
  {
    xiiHashTable<const AstNode*, xiiUInt32> nodeCache;
    xiiHashTable<xiiUInt64, xiiString>      connectionCache;
    xiiStringBuilder                        sb;

    TraverseAllConnections(pEntryAstNode,
                           [&](const Connection& connection) {
                             AstNode* pAstNode = connection.m_pCurrent;

                             xiiUInt32 uiGraphNode = 0;
                             if (nodeCache.TryGetValue(pAstNode, uiGraphNode) == false)
                             {
                               const char* szTypeName = xiiVisualScriptNodeDescription::Type::GetName(pAstNode->m_Type);
                               sb                     = szTypeName;
                               if (pAstNode->m_sTargetTypeName.IsEmpty() == false)
                               {
                                 sb.Append("\n", pAstNode->m_sTargetTypeName);
                               }
                               if (pAstNode->m_DeductedDataType != xiiVisualScriptDataType::Invalid)
                               {
                                 sb.Append("\nDataType: ", xiiVisualScriptDataType::GetName(pAstNode->m_DeductedDataType));
                               }
                               sb.AppendFormat("\nImplicitExec: {}", pAstNode->m_bImplicitExecution);
                               if (pAstNode->m_Value.IsValid())
                               {
                                 sb.AppendFormat("\nValue: {}", pAstNode->m_Value);
                               }

                               float colorX = xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(xiiHashingUtils::StringHash(szTypeName))).x();

                               xiiDGMLGraph::NodeDesc nd;
                               nd.m_Color  = xiiColorScheme::LightUI(colorX);
                               uiGraphNode = dgmlGraph.AddNode(sb, &nd);
                               nodeCache.Insert(pAstNode, uiGraphNode);
                             }

                             if (connection.m_pPrev != nullptr)
                             {
                               xiiUInt32 uiPrevGraphNode = 0;
                               XII_VERIFY(nodeCache.TryGetValue(connection.m_pPrev, uiPrevGraphNode), "");

                               if (connection.m_Type == ConnectionType::Execution)
                               {
                                 xiiUInt64  uiConnectionKey = uiPrevGraphNode | xiiUInt64(uiGraphNode) << 32;
                                 xiiString& sLabel          = connectionCache[uiConnectionKey];

                                 xiiStringBuilder sb = sLabel;
                                 if (sb.IsEmpty() == false)
                                 {
                                   sb.Append(" + ");
                                 }
                                 sb.Append("Exec");
                                 sLabel = sb;
                               }
                               else
                               {
                                 xiiUInt64  uiConnectionKey = uiGraphNode | xiiUInt64(uiPrevGraphNode) << 32;
                                 xiiString& sLabel          = connectionCache[uiConnectionKey];

                                 auto& dataInput  = connection.m_pPrev->m_Inputs[connection.m_uiPrevPinIndex];
                                 auto& dataOutput = GetDataOutput(dataInput);

                                 xiiStringBuilder sb = sLabel;
                                 if (sb.IsEmpty() == false)
                                 {
                                   sb.Append(" + ");
                                 }
                                 sb.AppendFormat("o{}:{} (id: {})->i{}:{} (id: {})", dataInput.m_uiSourcePinIndex, xiiVisualScriptDataType::GetName(dataOutput.m_DataType), dataOutput.m_uiId, connection.m_uiPrevPinIndex, xiiVisualScriptDataType::GetName(dataInput.m_DataType), dataInput.m_uiId);
                                 sLabel = sb;
                               }
                             }

                             return VisitorResult::Continue;
                           })
      .IgnoreResult();

    for (auto& it : connectionCache)
    {
      xiiUInt32 uiSource = it.Key() & 0xFFFFFFFF;
      xiiUInt32 uiTarget = it.Key() >> 32;

      dgmlGraph.AddConnection(uiSource, uiTarget, it.Value());
    }
  }

  xiiStringView    sExt = sOutputPath.GetFileExtension();
  xiiStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), "_", sFunctionName, sSuffix);
  sFullPath.Append(".", sExt);

  xiiDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    xiiLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    xiiLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}

void xiiVisualScriptCompiler::DumpGraph(xiiArrayPtr<const xiiVisualScriptNodeDescription> nodeDescriptions, xiiStringView sOutputPath, xiiStringView sFunctionName, xiiStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  xiiDGMLGraph dgmlGraph;
  {
    xiiStringBuilder sTmp;
    for (auto& nodeDesc : nodeDescriptions)
    {
      xiiStringView sTypeName = xiiVisualScriptNodeDescription::Type::GetName(nodeDesc.m_Type);
      sTmp                    = sTypeName;

      nodeDesc.AppendUserDataName(sTmp);

      for (auto& dataOffset : nodeDesc.m_InputDataOffsets)
      {
        sTmp.AppendFormat("\n Input {} {}[{}]", DataOffset::Source::GetName(dataOffset.GetSource()), xiiVisualScriptDataType::GetName(dataOffset.GetType()), dataOffset.m_uiByteOffset);

        if (dataOffset.GetSource() == DataOffset::Source::Constant)
        {
          for (auto& it : m_ConstantDataToIndex)
          {
            auto scriptDataType = xiiVisualScriptDataType::FromVariantType(it.Key().GetType());
            if (scriptDataType == dataOffset.GetType() && it.Value() == dataOffset.m_uiByteOffset)
            {
              sTmp.AppendFormat(" ({})", it.Key());
              break;
            }
          }
        }
      }

      for (auto& dataOffset : nodeDesc.m_OutputDataOffsets)
      {
        sTmp.AppendFormat("\n Output {} {}[{}]", DataOffset::Source::GetName(dataOffset.GetSource()), xiiVisualScriptDataType::GetName(dataOffset.GetType()), dataOffset.m_uiByteOffset);
      }

      float colorX = xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(xiiHashingUtils::StringHash(sTypeName))).x();

      xiiDGMLGraph::NodeDesc nd;
      nd.m_Color = xiiColorScheme::LightUI(colorX);

      dgmlGraph.AddNode(sTmp, &nd);
    }

    for (xiiUInt32 i = 0; i < nodeDescriptions.GetCount(); ++i)
    {
      for (auto uiNextIndex : nodeDescriptions[i].m_ExecutionIndices)
      {
        if (uiNextIndex == xiiSmallInvalidIndex)
          continue;

        dgmlGraph.AddConnection(i, uiNextIndex);
      }
    }
  }

  xiiStringView    sExt = sOutputPath.GetFileExtension();
  xiiStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), "_", sFunctionName, sSuffix);
  sFullPath.Append(".", sExt);

  xiiDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    xiiLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    xiiLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}
