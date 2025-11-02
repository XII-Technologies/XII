#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/World/World.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
  void MakeSubfunctionName(const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject, xiiStringBuilder& out_sName)
  {
    xiiVariant sNameProperty = pObject->GetTypeAccessor().GetValue("Name");
    xiiUInt32  uiHash        = xiiHashHelper<xiiUuid>::Hash(pObject->GetGuid());

    out_sName.SetFormat("{}_{}_{}", pEntryObject != nullptr ? xiiVisualScriptNodeManager::GetNiceFunctionName(pEntryObject) : "", sNameProperty, xiiArgU(uiHash, 8, true, 16));
  }

  xiiVisualScriptDataType::Enum FinalizeDataType(xiiVisualScriptDataType::Enum dataType)
  {
    xiiVisualScriptDataType::Enum result = dataType;
    if (result == xiiVisualScriptDataType::EnumValue || result == xiiVisualScriptDataType::BitflagValue)
      result = xiiVisualScriptDataType::Int64;

    if (result == xiiVisualScriptDataType::Resource)
      result = xiiVisualScriptDataType::String;

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

  static xiiResult FillUserData_VariableName(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Name");

    xiiStringView sName = inout_astNode.m_Value.Get<xiiString>().GetView();

    xiiVisualScriptVariable v;
    if (static_cast<const xiiVisualScriptNodeManager*>(pObject->GetDocumentObjectManager())->GetVariable(xiiTempHashedString(sName), v).Failed())
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

  static xiiResult FillUserData_Builtin_Expression(xiiVisualScriptCompiler::AstNode& inout_astNode, xiiVisualScriptCompiler* pCompiler, const xiiDocumentObject* pObject, const xiiDocumentObject* pEntryObject)
  {
    auto pManager = static_cast<const xiiVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

    xiiHybridArray<const xiiVisualScriptPin*, 16> pins;

    xiiHybridArray<xiiExpression::StreamDesc, 8> inputs;
    pManager->GetInputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& input = inputs.ExpandAndGetRef();
      input.m_sName.Assign(pPin->GetName());
      input.m_DataType = xiiVisualScriptDataType::GetStreamDataType(pPin->GetResolvedScriptDataType());
    }

    xiiHybridArray<xiiExpression::StreamDesc, 8> outputs;
    pManager->GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& output = outputs.ExpandAndGetRef();
      output.m_sName.Assign(pPin->GetName());
      output.m_DataType = xiiVisualScriptDataType::GetStreamDataType(pPin->GetResolvedScriptDataType());
    }

    xiiString sExpressionSource = pObject->GetTypeAccessor().GetValue("Expression").Get<xiiString>();

    xiiExpressionParser          parser;
    xiiExpressionParser::Options options = {};
    xiiExpressionAST             ast;
    XII_SUCCEED_OR_RETURN(parser.Parse(sExpressionSource, inputs, outputs, options, ast));

    xiiExpressionCompiler compiler;
    xiiExpressionByteCode byteCode;
    XII_SUCCEED_OR_RETURN(compiler.Compile(ast, byteCode));

    inout_astNode.m_Value = byteCode;

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

    nullptr,                    // Builtin_Constant,
    &FillUserData_VariableName, // Builtin_GetVariable,
    &FillUserData_VariableName, // Builtin_SetVariable,
    &FillUserData_VariableName, // Builtin_IncVariable,
    &FillUserData_VariableName, // Builtin_DecVariable,
    nullptr,                    // Builtin_TempVariable,

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

    nullptr,                          // Builtin_Add,
    nullptr,                          // Builtin_Subtract,
    nullptr,                          // Builtin_Multiply,
    nullptr,                          // Builtin_Divide,
    &FillUserData_Builtin_Expression, // Builtin_Expression,

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
    nullptr, // Builtin_Array_PushBackRange,
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

xiiVisualScriptCompiler::CompiledModule::CompiledModule() : m_ConstantDataStorage(xiiSharedPtr<xiiVisualScriptDataDescription>(&m_ConstantDataDesc, nullptr))
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

  {
    chunk.BeginChunk("FunctionGraphs", 1);
    chunk << m_Functions.GetCount();

    // Reverse order so functions that are added later (e.g. coroutines) are loaded first
    for (xiiUInt32 i = m_Functions.GetCount(); i-- > 0;)
    {
      auto& function = m_Functions[i];

      chunk << function.m_sName;
      chunk << function.m_Type;
      chunk << function.m_CoroutineCreationMode;

      XII_SUCCEED_OR_RETURN(xiiVisualScriptGraphDescription::Serialize(function.m_NodeDescriptions, function.m_LocalDataDesc, chunk));
    }

    chunk.EndChunk();
  }

  chunk.EndStream();

  return stringDedup.End();
}

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptCompiler::xiiVisualScriptCompiler(xiiVisualScriptNodeManager& ref_nodeManager) : m_NodeManager(ref_nodeManager)
{
}

xiiVisualScriptCompiler::~xiiVisualScriptCompiler() = default;

void xiiVisualScriptCompiler::InitModule(xiiStringView sBaseClassName, xiiStringView sScriptClassName)
{
  m_Module.m_sBaseClassName   = sBaseClassName;
  m_Module.m_sScriptClassName = sScriptClassName;
}

xiiResult xiiVisualScriptCompiler::AddFunction(xiiStringView sName, const xiiDocumentObject* pEntryObject, const xiiDocumentObject* pParentObject)
{
  XII_ASSERT_DEV(&m_NodeManager == pEntryObject->GetDocumentObjectManager(), "Can't add functions from different document");

  for (auto& existingFunction : m_Module.m_Functions)
  {
    if (existingFunction.m_sName == sName)
    {
      xiiLog::Error("A function named '{}' already exists. Function names need to unique.", sName);
      return XII_FAILURE;
    }
  }

  auto& function   = m_Module.m_Functions.ExpandAndGetRef();
  function.m_sName = sName;

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

  m_EntryObjects.PushBack(pEntryObject);
  XII_ASSERT_DEBUG(m_Module.m_Functions.GetCount() == m_EntryObjects.GetCount(), "");

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::Compile(xiiStringView sDebugAstOutputPath)
{
  XII_SUCCEED_OR_RETURN(BuildInstanceDataMapping());

  for (xiiUInt32 i = 0; i < m_Module.m_Functions.GetCount(); ++i)
  {
    auto&                    function     = m_Module.m_Functions[i];
    const xiiDocumentObject* pEntryObject = m_EntryObjects[i];

    AstNode* pEntryAstNode = BuildExecutionFlow(pEntryObject);
    if (pEntryAstNode == nullptr)
    {
      xiiLog::Error("Failed to build execution flow for function '{}'", function.m_sName);
      return XII_FAILURE;
    }

    function.m_Type = pEntryAstNode->m_Type;

    XII_SUCCEED_OR_RETURN(BuildDataExecutions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_00");

    XII_SUCCEED_OR_RETURN(InsertTypeConversions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_01_TypeConv");

    XII_SUCCEED_OR_RETURN(AssignInstanceVariables(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_02_InstanceVarsAssigned");

    XII_SUCCEED_OR_RETURN(ReplaceUnsupportedNodes(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_03_Replaced");

    XII_SUCCEED_OR_RETURN(CopyOutputsToInputs(pEntryAstNode));
    XII_SUCCEED_OR_RETURN(AssignLocalVariables(pEntryAstNode, function.m_LocalDataDesc));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_04_LocalVarsAssigned");

    XII_SUCCEED_OR_RETURN(BuildNodeDescriptions(pEntryAstNode, function.m_NodeDescriptions));

    DumpGraph(function.m_NodeDescriptions, sDebugAstOutputPath, function.m_sName, "_Graph");

    m_CompilationState.Clear();
  }

  XII_SUCCEED_OR_RETURN(FinalizeConstantData());

  return XII_SUCCESS;
}

// Ast node creation
//////////////////////////////////////////////////////////////////////////

xiiVisualScriptCompiler::AstNode& xiiVisualScriptCompiler::CreateAstNode(xiiVisualScriptNodeDescription::Type::Enum type, xiiVisualScriptDataType::Enum deductedDataType, bool bImplicitExecution /*= false*/)
{
  auto& node                = m_AstNodes.ExpandAndGetRef();
  node.m_Type               = type;
  node.m_DeductedDataType   = deductedDataType;
  node.m_bImplicitExecution = bImplicitExecution;
  return node;
}

xiiVisualScriptCompiler::AstNode& xiiVisualScriptCompiler::CreateJumpNode(AstNode* pTargetNode)
{
  auto& jumpNode   = CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Jump);
  jumpNode.m_Value = xiiUInt64(*reinterpret_cast<size_t*>(&pTargetNode));

  return jumpNode;
}

xiiVisualScriptCompiler::AstNode* xiiVisualScriptCompiler::CreateAstNodeFromObject(const xiiDocumentObject* pObject, const xiiVisualScriptNodeRegistry::NodeDesc* pNodeDesc, const xiiDocumentObject* pEntryObject, bool bImplicitOnly /*= false*/)
{
  if (pNodeDesc == nullptr)
  {
    pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  }
  XII_ASSERT_DEV(pNodeDesc != nullptr && pNodeDesc->m_Type != xiiVisualScriptNodeDescription::Type::GetScriptOwner, "Invalid node type");

  if (bImplicitOnly && !pNodeDesc->m_bImplicitExecution)
    return nullptr;

  auto& astNode     = CreateAstNode(pNodeDesc->m_Type, FinalizeDataType(m_NodeManager.GetDeductedType(pObject)), pNodeDesc->m_bImplicitExecution);
  astNode.m_pObject = pObject;

  if (FillUserData(astNode, this, pObject, pEntryObject).Failed())
    return nullptr;

  if (pNodeDesc->m_bImplicitExecution)
  {
    m_CompilationState.m_DataObjectToAstNode.Insert(pObject, &astNode);
  }
  else
  {
    m_CompilationState.m_ExecObjectToAstNode.Insert(pObject, &astNode);
  }

  return &astNode;
}

xiiVisualScriptCompiler::DataInput xiiVisualScriptCompiler::GetOrCreateDefaultPointerNode(const AstNode& node, const xiiRTTI* pRtti)
{
  if (pRtti == nullptr)
  {
    pRtti = xiiRTTI::FindTypeByNameHash(node.m_sTargetTypeName.GetHash());
  }

  const bool bIsGameObject = pRtti == xiiGetStaticRTTI<xiiGameObject>() || pRtti == xiiGetStaticRTTI<xiiGameObjectHandle>();
  const bool bIsWorld      = pRtti == xiiGetStaticRTTI<xiiWorld>();

  if (bIsWorld || bIsGameObject)
  {
    DataInput dataInput;
    dataInput.m_pSourceNode      = m_CompilationState.m_pGetScriptOwnerNode;
    dataInput.m_uiSourcePinIndex = bIsGameObject ? 1 : 0;
    dataInput.m_DataOffset       = m_CompilationState.m_pGetScriptOwnerNode->m_DataOutputs[dataInput.m_uiSourcePinIndex].m_DataOffset;
    return dataInput;
  }

  return DataInput();
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

// Pins, inputs and outputs
//////////////////////////////////////////////////////////////////////////

void xiiVisualScriptCompiler::AddConstantDataInput(AstNode& node, const xiiVariant& value)
{
  xiiVisualScriptDataType::Enum dataType = xiiVisualScriptDataType::FromVariantType(value.GetType());

  xiiUInt32 uiIndex = xiiInvalidIndex;
  if (m_Module.m_ConstantDataToIndex.TryGetValue(value, uiIndex) == false)
  {
    auto& offsetAndCount = m_Module.m_ConstantDataDesc.m_PerTypeInfo[dataType];
    uiIndex              = offsetAndCount.m_uiCount;
    ++offsetAndCount.m_uiCount;

    m_Module.m_ConstantDataToIndex.Insert(value, uiIndex);
  }

  auto& dataInput              = node.m_DataInputs.ExpandAndGetRef();
  dataInput.m_pSourceNode      = nullptr;
  dataInput.m_uiSourcePinIndex = 0;
  dataInput.m_DataOffset       = DataOffset(uiIndex, dataType, DataOffset::Source::Constant);
}

xiiResult xiiVisualScriptCompiler::AddConstantDataInput(AstNode& node, const xiiDocumentObject* pObject, const xiiVisualScriptPin* pPin, xiiVisualScriptDataType::Enum dataType)
{
  xiiStringView sPropertyName = pPin->HasDynamicPinProperty() ? pPin->GetDynamicPinProperty() : pPin->GetName();

  xiiVariant value = pObject->GetTypeAccessor().GetValue(sPropertyName);
  if (value.IsValid() && pPin->HasDynamicPinProperty())
  {
    XII_ASSERT_DEBUG(value.IsA<xiiVariantArray>(), "Implementation error");
    value = value.Get<xiiVariantArray>()[pPin->GetElementIndex()];
  }

  if (value.IsA<xiiUuid>())
  {
    value = 0;
  }

  xiiVisualScriptDataType::Enum valueDataType = xiiVisualScriptDataType::FromVariantType(value.GetType());
  if (dataType != xiiVisualScriptDataType::Variant)
  {
    value = value.ConvertTo(xiiVisualScriptDataType::GetVariantType(dataType));
    if (value.IsValid() == false)
    {
      xiiLog::Error("Failed to convert '{}.{}' of type '{}' to '{}'.", GetNiceTypeName(pObject), pPin->GetName(), xiiVisualScriptDataType::GetName(valueDataType), xiiVisualScriptDataType::GetName(dataType));
      return XII_FAILURE;
    }
  }

  AddConstantDataInput(node, value);

  return XII_SUCCESS;
}

void xiiVisualScriptCompiler::AddDataInput(AstNode& node, AstNode* pSourceNode, xiiUInt32 uiSourcePinIndex, xiiVisualScriptDataType::Enum dataType)
{
  auto& dataInput                 = node.m_DataInputs.ExpandAndGetRef();
  dataInput.m_pSourceNode         = pSourceNode;
  dataInput.m_uiSourcePinIndex    = uiSourcePinIndex;
  dataInput.m_DataOffset.m_uiType = dataType;
}

void xiiVisualScriptCompiler::AddDataOutput(AstNode& node, xiiVisualScriptDataType::Enum dataType)
{
  auto& dataOutput        = node.m_DataOutputs.ExpandAndGetRef();
  dataOutput.m_DataOffset = DataOffset(m_CompilationState.m_uiNextLocalVarId++, dataType, DataOffset::Source::Local);
}

xiiVisualScriptCompiler::DataOutput& xiiVisualScriptCompiler::GetDataOutputFromInput(const DataInput& dataInput)
{
  if (dataInput.m_uiSourcePinIndex < dataInput.m_pSourceNode->m_DataOutputs.GetCount())
  {
    return dataInput.m_pSourceNode->m_DataOutputs[dataInput.m_uiSourcePinIndex];
  }

  XII_ASSERT_DEBUG(false, "This code should be never reached");
  static DataOutput dummy;
  return dummy;
}

void xiiVisualScriptCompiler::ConnectExecution(AstNode& sourceNode, AstNode& targetNode, xiiUInt32 uiSourcePinIndex /*= xiiInvalidIndex*/)
{
  if (uiSourcePinIndex == xiiInvalidIndex)
  {
    // New connection
    ExecInput execInput;
    execInput.m_pSourceNode      = &sourceNode;
    execInput.m_uiSourcePinIndex = sourceNode.m_ExecOutputs.GetCount();

    targetNode.m_ExecInputs.PushBack(execInput);

    ExecOutput execOutput;
    execOutput.m_pTargetNode = &targetNode;

    sourceNode.m_ExecOutputs.PushBack(execOutput);
  }
  else
  {
    ExecInput execInput;
    execInput.m_pSourceNode      = &sourceNode;
    execInput.m_uiSourcePinIndex = uiSourcePinIndex;

    targetNode.m_ExecInputs.PushBack(execInput);

    sourceNode.m_ExecOutputs.EnsureCount(uiSourcePinIndex + 1);
    auto& execOutput         = sourceNode.m_ExecOutputs[uiSourcePinIndex];
    execOutput.m_pTargetNode = &targetNode;
  }
}

void xiiVisualScriptCompiler::DisconnectExecution(AstNode& sourceNode, AstNode& targetNode, xiiUInt32 uiSourcePinIndex)
{
  ExecInput execInputToRemove;
  execInputToRemove.m_pSourceNode      = &sourceNode;
  execInputToRemove.m_uiSourcePinIndex = uiSourcePinIndex;
  XII_VERIFY(targetNode.m_ExecInputs.RemoveAndCopy(execInputToRemove), "");

  sourceNode.m_ExecOutputs[uiSourcePinIndex].m_pTargetNode = nullptr;
}

void xiiVisualScriptCompiler::ExecuteBefore(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode)
{
  for (auto& execInput : node.m_ExecInputs)
  {
    ConnectExecution(*execInput.m_pSourceNode, firstNewNode, execInput.m_uiSourcePinIndex);
  }
  node.m_ExecInputs.Clear();

  ConnectExecution(lastNewNode, node);
}

void xiiVisualScriptCompiler::ExecuteAfter(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode)
{
  XII_ASSERT_DEV(node.m_ExecOutputs.GetCount() == 1, "This function only works for nodes with a single exec output pin");
  AstNode* pNodeAfter = node.m_ExecOutputs[0].m_pTargetNode;

  ConnectExecution(node, firstNewNode, 0);

  pNodeAfter->m_ExecInputs.Clear();
  ConnectExecution(lastNewNode, *pNodeAfter);
}

void xiiVisualScriptCompiler::ReplaceExecution(AstNode& oldNode, AstNode& newNode)
{
  XII_ASSERT_DEBUG(newNode.m_ExecInputs.IsEmpty() && newNode.m_ExecOutputs.IsEmpty(), "New node must not have any connections yet");

  for (auto& execInput : oldNode.m_ExecInputs)
  {
    ConnectExecution(*execInput.m_pSourceNode, newNode, execInput.m_uiSourcePinIndex);
  }
  oldNode.m_ExecInputs.Clear();

  for (xiiUInt32 i = 0; i < oldNode.m_ExecOutputs.GetCount(); ++i)
  {
    AstNode* pTargetNode = oldNode.m_ExecOutputs[i].m_pTargetNode;
    if (pTargetNode != nullptr)
    {
      DisconnectExecution(oldNode, *pTargetNode, i);
      ConnectExecution(newNode, *pTargetNode, i);
    }
    else
    {
      newNode.m_ExecOutputs.PushBack({nullptr});
    }
  }
  oldNode.m_ExecOutputs.Clear();
}

xiiVisualScriptCompiler::DataOffset xiiVisualScriptCompiler::GetInstanceDataOffset(xiiHashedString sName, xiiVisualScriptDataType::Enum dataType)
{
  xiiVisualScriptInstanceData instanceData;
  if (m_Module.m_InstanceDataMapping.m_Content.TryGetValue(sName, instanceData) == false)
  {
    xiiLog::Error("Invalid variable named '{}'", sName);
    return DataOffset();
  }

  XII_ASSERT_DEBUG(instanceData.m_DataOffset.m_uiType == dataType, "Data type mismatch");
  return instanceData.m_DataOffset;
}

// Compilation steps
//////////////////////////////////////////////////////////////////////////

xiiResult xiiVisualScriptCompiler::BuildInstanceDataMapping()
{
  xiiHybridArray<xiiVisualScriptVariable, 16> variables;
  m_NodeManager.GetAllVariables(variables);

  for (auto& variable : variables)
  {
    auto dataType = variable.m_TypeDecl.GetDataType();

    XII_ASSERT_DEV(dataType == xiiVisualScriptDataType::Variant ||
                     ((dataType == xiiVisualScriptDataType::GameObject || dataType == xiiVisualScriptDataType::Component || dataType == xiiVisualScriptDataType::TypedPointer) && variable.m_DefaultValue.IsValid() == false) ||
                     dataType == xiiVisualScriptDataType::FromVariantType(variable.m_DefaultValue.GetType()),
                   "Data type mismatch");

    auto& offsetAndCount = m_Module.m_InstanceDataDesc.m_PerTypeInfo[dataType];

    xiiVisualScriptInstanceData instanceData;
    instanceData.m_DataOffset.m_uiByteOffset = offsetAndCount.m_uiCount;
    instanceData.m_DataOffset.m_uiType       = dataType;
    instanceData.m_DataOffset.m_uiSource     = DataOffset::Source::Instance;
    instanceData.m_DefaultValue              = variable.m_DefaultValue;

    ++offsetAndCount.m_uiCount;

    m_Module.m_InstanceDataMapping.m_Content.Insert(variable.m_sName, instanceData);
  }

  return XII_SUCCESS;
}

xiiVisualScriptCompiler::AstNode* xiiVisualScriptCompiler::BuildExecutionFlow(const xiiDocumentObject* pEntryObject)
{
  AstNode* pEntryAstNode = CreateAstNodeFromObject(pEntryObject, nullptr, pEntryObject);
  if (pEntryAstNode == nullptr)
    return nullptr;

  if (xiiVisualScriptNodeDescription::Type::IsEntry(pEntryAstNode->m_Type) == false)
  {
    auto& astNode = CreateAstNode(xiiVisualScriptNodeDescription::Type::EntryCall);
    ConnectExecution(astNode, *pEntryAstNode);

    pEntryAstNode = &astNode;
  }

  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;
  xiiHybridArray<const xiiDocumentObject*, 64>  objectStack;
  objectStack.PushBack(pEntryObject);

  while (objectStack.IsEmpty() == false)
  {
    const xiiDocumentObject* pObject = objectStack.PeekBack();
    objectStack.PopBack();

    AstNode* pAstNode = nullptr;
    XII_VERIFY(m_CompilationState.m_ExecObjectToAstNode.TryGetValue(pObject, pAstNode), "Implementation error");

    if (xiiVisualScriptNodeDescription::Type::MakesOuterCoroutine(pAstNode->m_Type))
    {
      MarkAsCoroutine(pEntryAstNode);
    }

    m_NodeManager.GetOutputExecutionPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto connections = m_NodeManager.GetConnections(*pPin);
      if (connections.IsEmpty() || pPin->SplitExecution())
      {
        pAstNode->m_ExecOutputs.PushBack({nullptr});
        continue;
      }

      XII_ASSERT_DEV(connections.GetCount() == 1, "Output execution pins should only have one connection");
      const xiiDocumentObject* pNextObject = connections[0]->GetTargetPin().GetParent();

      AstNode* pNextAstNode;
      if (m_CompilationState.m_ExecObjectToAstNode.TryGetValue(pNextObject, pNextAstNode) == false)
      {
        pNextAstNode = CreateAstNodeFromObject(pNextObject, nullptr, pEntryObject);
        if (pNextAstNode == nullptr)
          return nullptr;

        objectStack.PushBack(pNextObject);
      }

      ConnectExecution(*pAstNode, *pNextAstNode);
    }
  }

  // Always add a GetScriptOwner node directly after the entry node
  {
    XII_ASSERT_DEBUG(m_CompilationState.m_pGetScriptOwnerNode == nullptr, "");

    auto& getScriptOwnerNode = CreateAstNode(xiiVisualScriptNodeDescription::Type::GetScriptOwner);
    AddDataOutput(getScriptOwnerNode, xiiVisualScriptDataType::TypedPointer);
    AddDataOutput(getScriptOwnerNode, xiiVisualScriptDataType::GameObject);
    AddDataOutput(getScriptOwnerNode, xiiVisualScriptDataType::Component);

    m_CompilationState.m_pGetScriptOwnerNode = &getScriptOwnerNode;

    ExecuteAfter(*pEntryAstNode, getScriptOwnerNode, getScriptOwnerNode);
  }

  return pEntryAstNode;
}

xiiResult xiiVisualScriptCompiler::BuildDataStack(AstNode* pEntryAstNode, AstNode*& out_pFirstDataNode, AstNode*& out_pLastDataNode)
{
  if (pEntryAstNode->m_pObject == nullptr)
    return XII_SUCCESS;

  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;

  struct ObjectContext
  {
    const xiiDocumentObject*                     m_pObject   = nullptr;
    const xiiVisualScriptNodeRegistry::NodeDesc* m_pNodeDesc = nullptr;
  };

  xiiHybridArray<ObjectContext, 32> objectStack;
  objectStack.PushBack({pEntryAstNode->m_pObject, xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pEntryAstNode->m_pObject->GetType())});

  // Start from scratch for each data stack
  m_CompilationState.m_DataObjectToAstNode.Clear();
  m_CompilationState.m_DataObjectToAstNode.Insert(pEntryAstNode->m_pObject, pEntryAstNode);

  xiiHybridArray<AstNode*, 32> dataStack;

  while (objectStack.IsEmpty() == false)
  {
    auto&                                        objCtx    = objectStack.PeekBack();
    const xiiDocumentObject*                     pObject   = objCtx.m_pObject;
    const xiiVisualScriptNodeRegistry::NodeDesc* pNodeDesc = objCtx.m_pNodeDesc;
    objectStack.PopBack();

    AstNode* pAstNode = nullptr;
    XII_VERIFY(m_CompilationState.m_DataObjectToAstNode.TryGetValue(pObject, pAstNode), "Implementation error");

    if (pAstNode != pEntryAstNode)
    {
      dataStack.PushBack(pAstNode);
    }

    m_NodeManager.GetInputDataPins(pObject, pins);
    xiiUInt32 uiNextInputPinIndex = 0;

    for (auto& pinDesc : pNodeDesc->m_InputPins)
    {
      if (pinDesc.IsExecutionPin())
        continue;

      AstNode* pAstNodeToAddInput = pAstNode;
      bool     bArrayInput        = false;
      if (pinDesc.m_bReplaceWithArray && pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
      {
        const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(pinDesc.m_sDynamicPinProperty);
        if (pProp == nullptr)
          return XII_FAILURE;

        if (pProp->GetCategory() == xiiPropertyCategory::Array)
        {
          auto pMakeArrayAstNode       = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_MakeArray, true);
          pMakeArrayAstNode->m_pObject = pObject;
          AddDataOutput(*pMakeArrayAstNode, xiiVisualScriptDataType::Array);

          AddDataInput(*pAstNode, pMakeArrayAstNode, 0, xiiVisualScriptDataType::Array);

          dataStack.PushBack(pMakeArrayAstNode);

          pAstNodeToAddInput = pMakeArrayAstNode;
          bArrayInput        = true;
        }
      }

      while (uiNextInputPinIndex < pins.GetCount())
      {
        auto pPin = pins[uiNextInputPinIndex];

        if (pPin->GetDynamicPinProperty() != pinDesc.m_sDynamicPinProperty)
          break;

        xiiVisualScriptDataType::Enum targetDataType = pPin->GetResolvedScriptDataType();
        if (targetDataType == xiiVisualScriptDataType::Invalid)
        {
          xiiLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pObject), pPin->GetName());
          return XII_FAILURE;
        }

        auto connections = m_NodeManager.GetConnections(*pPin);
        if (pPin->IsRequired() && targetDataType != xiiVisualScriptDataType::GameObject && connections.IsEmpty())
        {
          xiiLog::Error("Required input '{}' for '{}' is not connected", pPin->GetName(), GetNiceTypeName(pObject));
          return XII_FAILURE;
        }

        auto dataInputType = bArrayInput ? xiiVisualScriptDataType::Variant : FinalizeDataType(targetDataType);

        if (connections.IsEmpty())
        {
          if (xiiVisualScriptDataType::IsPointer(dataInputType))
          {
            auto defaultInput = GetOrCreateDefaultPointerNode(*pAstNode, pPin->GetDataType());
            AddDataInput(*pAstNodeToAddInput, defaultInput.m_pSourceNode, defaultInput.m_uiSourcePinIndex, defaultInput.m_DataOffset.GetType());

            if (defaultInput.m_pSourceNode != nullptr)
            {
              if (defaultInput.m_pSourceNode->m_ExecInputs.IsEmpty())
              {
                dataStack.RemoveAndCopy(defaultInput.m_pSourceNode);
                dataStack.PushBack(defaultInput.m_pSourceNode);
              }
            }
          }
          else
          {
            XII_SUCCEED_OR_RETURN(AddConstantDataInput(*pAstNodeToAddInput, pObject, pPin, dataInputType));
          }
        }
        else
        {
          auto&                    sourcePin       = static_cast<const xiiVisualScriptPin&>(connections[0]->GetSourcePin());
          const xiiDocumentObject* pSourceObject   = sourcePin.GetParent();
          auto                     pSourceNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pSourceObject->GetType());

          if (pSourceNodeDesc->m_Type == xiiVisualScriptNodeDescription::Type::GetScriptOwner)
          {
            AstNode* pSourceAstNode = m_CompilationState.m_pGetScriptOwnerNode;
            AddDataInput(*pAstNodeToAddInput, pSourceAstNode, sourcePin.GetDataPinIndex(), dataInputType);
          }
          else if (pSourceNodeDesc->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_Constant)
          {
            xiiVariant value = pSourceObject->GetTypeAccessor().GetValue("Value");
            AddConstantDataInput(*pAstNodeToAddInput, value);
          }
          else
          {
            AstNode* pSourceAstNode;
            if (m_CompilationState.m_DataObjectToAstNode.TryGetValue(pSourceObject, pSourceAstNode) == false)
            {
              pSourceAstNode = CreateAstNodeFromObject(pSourceObject, pSourceNodeDesc, nullptr, true);
              if (pSourceAstNode != nullptr)
              {
                objectStack.PushBack({pSourceObject, pSourceNodeDesc});
              }
              else if (pSourceAstNode == nullptr)
              {
                if (m_CompilationState.m_ExecObjectToAstNode.TryGetValue(pSourceObject, pSourceAstNode) == false)
                {
                  xiiLog::Error("The source node '{}' is not executed in the current function.", GetNiceTypeName(pSourceObject));
                  return XII_FAILURE;
                }
              }
            }

            xiiVisualScriptDataType::Enum sourceDataType = sourcePin.GetResolvedScriptDataType();
            if (sourceDataType == xiiVisualScriptDataType::Invalid)
            {
              xiiLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pSourceObject), sourcePin.GetName());
              return XII_FAILURE;
            }

            if (sourcePin.CanConvertTo(*pPin) == false)
            {
              xiiLog::Error("Can't implicitly convert pin '{}.{}' of type '{}' connected to pin '{}.{}' of type '{}'", GetNiceTypeName(pSourceObject), sourcePin.GetName(), sourcePin.GetDataTypeName(), GetNiceTypeName(pObject), pPin->GetName(), pPin->GetDataTypeName());
              return XII_FAILURE;
            }

            AddDataInput(*pAstNodeToAddInput, pSourceAstNode, sourcePin.GetDataPinIndex(), dataInputType);
          }
        }

        ++uiNextInputPinIndex;
      }
    }

    m_NodeManager.GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      AddDataOutput(*pAstNode, FinalizeDataType(pPin->GetResolvedScriptDataType()));
    }
  }

  // Traverse in topological order and connect executions
  xiiHybridArray<AstNode*, 32> sortedDataStack;

  while (dataStack.IsEmpty() == false)
  {
    // Find next node with all data dependencies already in the sorted stack
    AstNode* nextNode = nullptr;
    for (xiiUInt32 i = dataStack.GetCount(); i-- > 0;)
    {
      AstNode*   nextNodeCandiate = dataStack[i];
      const bool bValidCandidate  = [&]() {
        m_NodeManager.GetInputDataPins(nextNodeCandiate->m_pObject, pins);
        for (auto pPin : pins)
        {
          auto connections = m_NodeManager.GetConnections(*pPin);
          for (auto pConnection : connections)
          {
            const xiiDocumentObject* pSourceObject = pConnection->GetSourcePin().GetParent();

            AstNode* pSourceAstNode = nullptr;
            if (!m_CompilationState.m_DataObjectToAstNode.TryGetValue(pSourceObject, pSourceAstNode))
            {
              // Not part of the data stack so we can ignore it
              continue;
            }

            if (!sortedDataStack.Contains(pSourceAstNode))
            {
              return false;
            }
          }
        }

        return true;
      }();

      if (bValidCandidate)
      {
        nextNode = nextNodeCandiate;
        dataStack.RemoveAtAndCopy(i);
        break;
      }
    }

    if (nextNode == nullptr)
    {
      XII_REPORT_FAILURE("Data connection corrupted or loop detected");
      return XII_FAILURE;
    }

    if (sortedDataStack.IsEmpty() == false)
    {
      ConnectExecution(*sortedDataStack.PeekBack(), *nextNode);
    }
    sortedDataStack.PushBack(nextNode);
  }

  if (sortedDataStack.IsEmpty())
  {
    out_pFirstDataNode = nullptr;
    out_pLastDataNode  = nullptr;
  }
  else
  {
    out_pFirstDataNode = sortedDataStack[0];
    out_pLastDataNode  = sortedDataStack.PeekBack();
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::BuildDataExecutions(AstNode* pEntryAstNode)
{
  return TraverseAstDepthFirst(pEntryAstNode,
                               [&](AstNode*& pAstNode) {
                                 if (pAstNode->m_bImplicitExecution)
                                   return VisitorResult::Continue;

                                 AstNode* pFirstDataNode = nullptr;
                                 AstNode* pLastDataNode  = nullptr;
                                 if (BuildDataStack(pAstNode, pFirstDataNode, pLastDataNode).Failed())
                                   return VisitorResult::Error;

                                 if (pFirstDataNode == nullptr || pLastDataNode == nullptr)
                                   return VisitorResult::Continue;

                                 // Connect data stack
                                 ExecuteBefore(*pAstNode, *pFirstDataNode, *pLastDataNode);

                                 return VisitorResult::Continue;
                               });
}

xiiResult xiiVisualScriptCompiler::InsertTypeConversions(AstNode* pEntryAstNode)
{
  auto InsertBuiltinTypeConversion = [&](AstNode& node, DataInput& dataInput, xiiVisualScriptDataType::Enum inputDataType, xiiVisualScriptDataType::Enum outputDataType) -> AstNode& {
    auto nodeType = xiiVisualScriptNodeDescription::Type::GetConversionType(inputDataType);

    auto& conversionNode = CreateAstNode(nodeType, outputDataType, true);
    AddDataInput(conversionNode, dataInput.m_pSourceNode, dataInput.m_uiSourcePinIndex, outputDataType);
    AddDataOutput(conversionNode, inputDataType);

    dataInput.m_pSourceNode      = &conversionNode;
    dataInput.m_uiSourcePinIndex = 0;

    ExecuteBefore(node, conversionNode, conversionNode);

    return conversionNode;
  };

  return TraverseAstDepthFirst(pEntryAstNode,
                               [&](AstNode*& pAstNode) {
                                 for (auto& dataInput : pAstNode->m_DataInputs)
                                 {
                                   if (dataInput.IsConnected() == false)
                                     continue;

                                   auto& dataOutput = GetDataOutputFromInput(dataInput);

                                   auto inputDataType  = dataInput.m_DataOffset.GetType();
                                   auto outputDataType = dataOutput.m_DataOffset.GetType();

                                   if (dataOutput.m_DataOffset.GetType() != inputDataType)
                                   {
                                     if (xiiVisualScriptDataType::IsNumberOrBool(outputDataType) && inputDataType == xiiVisualScriptDataType::Vector3)
                                     {
                                       AstNode*  pSourceNode      = dataInput.m_pSourceNode;
                                       xiiUInt32 uiSourcePinIndex = dataInput.m_uiSourcePinIndex;
                                       if (outputDataType != xiiVisualScriptDataType::Float)
                                       {
                                         auto& conversionNode = InsertBuiltinTypeConversion(*pAstNode, dataInput, xiiVisualScriptDataType::Float, outputDataType);
                                         pSourceNode          = &conversionNode;
                                         uiSourcePinIndex     = 0;
                                       }

                                       auto& makeVec3Node = CreateAstNode(xiiVisualScriptNodeDescription::Type::ReflectedFunction, inputDataType, true);
                                       makeVec3Node.m_sTargetTypeName.Assign("xiiVec3");

                                       xiiVariantArray a;
                                       a.PushBack(xiiMakeHashedString("Make"));
                                       makeVec3Node.m_Value = a;

                                       AddDataInput(makeVec3Node, pSourceNode, uiSourcePinIndex, xiiVisualScriptDataType::Float);
                                       AddDataInput(makeVec3Node, pSourceNode, uiSourcePinIndex, xiiVisualScriptDataType::Float);
                                       AddDataInput(makeVec3Node, pSourceNode, uiSourcePinIndex, xiiVisualScriptDataType::Float);
                                       AddDataOutput(makeVec3Node, xiiVisualScriptDataType::Vector3);

                                       dataInput.m_pSourceNode      = &makeVec3Node;
                                       dataInput.m_uiSourcePinIndex = 0;

                                       ExecuteBefore(*pAstNode, makeVec3Node, makeVec3Node);
                                     }
                                     else if (outputDataType == xiiVisualScriptDataType::Vector3 && inputDataType == xiiVisualScriptDataType::Transform)
                                     {
                                       auto& makeTransformNode = CreateAstNode(xiiVisualScriptNodeDescription::Type::ReflectedFunction, inputDataType, true);
                                       makeTransformNode.m_sTargetTypeName.Assign("xiiTransform");

                                       xiiVariantArray a;
                                       a.PushBack(xiiMakeHashedString("Make"));
                                       makeTransformNode.m_Value = a;

                                       AddDataInput(makeTransformNode, dataInput.m_pSourceNode, dataInput.m_uiSourcePinIndex, xiiVisualScriptDataType::Vector3);
                                       AddConstantDataInput(makeTransformNode, xiiQuat::MakeIdentity());
                                       AddConstantDataInput(makeTransformNode, xiiVec3(1));
                                       AddDataOutput(makeTransformNode, xiiVisualScriptDataType::Transform);

                                       dataInput.m_pSourceNode      = &makeTransformNode;
                                       dataInput.m_uiSourcePinIndex = 0;

                                       ExecuteBefore(*pAstNode, makeTransformNode, makeTransformNode);
                                     }
                                     else
                                     {
                                       InsertBuiltinTypeConversion(*pAstNode, dataInput, inputDataType, outputDataType);
                                     }
                                   }
                                 }

                                 return VisitorResult::Continue;
                               });
}

xiiResult xiiVisualScriptCompiler::ReplaceLoop(AstNode* pLoopNode)
{
  AstNode* pLoopInitStart = nullptr;
  AstNode* pLoopInitEnd   = nullptr;

  AstNode* pLoopConditionStart = nullptr;
  AstNode* pLoopConditionEnd   = nullptr;

  DataInput conditionDataInput;
  conditionDataInput.m_uiSourcePinIndex    = 0;
  conditionDataInput.m_DataOffset.m_uiType = xiiVisualScriptDataType::Bool;

  AstNode* pLoopIncrement = nullptr;

  AstNode* pLoopElement = nullptr;
  AstNode* pLoopIndex   = nullptr;

  AstNode* pLoopBody      = pLoopNode->m_ExecOutputs[0].m_pTargetNode;
  AstNode* pLoopCompleted = pLoopNode->m_ExecOutputs[1].m_pTargetNode;

  auto loopType = pLoopNode->m_Type;

  if (loopType == xiiVisualScriptNodeDescription::Type::Builtin_WhileLoop)
  {
    conditionDataInput = pLoopNode->m_DataInputs[0];

    if (conditionDataInput.m_pSourceNode != nullptr)
    {
      pLoopConditionEnd = conditionDataInput.m_pSourceNode;

      pLoopConditionStart = conditionDataInput.m_pSourceNode;
      while (pLoopConditionStart->m_ExecInputs.GetCount() == 1 && pLoopConditionStart->m_ExecInputs[0].m_pSourceNode->m_bImplicitExecution)
      {
        pLoopConditionStart = pLoopConditionStart->m_ExecInputs[0].m_pSourceNode;
      }
    }
  }
  else if (loopType == xiiVisualScriptNodeDescription::Type::Builtin_ForLoop)
  {
    auto& firstIndexInput = pLoopNode->m_DataInputs[0];
    auto& lastIndexInput  = pLoopNode->m_DataInputs[1];

    // Loop Init
    {
      pLoopInitStart = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_ToInt, xiiVisualScriptDataType::Int);
      pLoopInitStart->m_DataInputs.PushBack(firstIndexInput);
      AddDataOutput(*pLoopInitStart, xiiVisualScriptDataType::Int);

      pLoopInitEnd = pLoopInitStart;
      pLoopIndex   = pLoopInitStart;
    }

    // Loop Condition
    {
      pLoopConditionStart          = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Compare, xiiVisualScriptDataType::Int);
      pLoopConditionStart->m_Value = xiiInt64(xiiComparisonOperator::LessEqual);
      AddDataInput(*pLoopConditionStart, pLoopInitStart, 0, xiiVisualScriptDataType::Int);
      pLoopConditionStart->m_DataInputs.PushBack(lastIndexInput);
      AddDataOutput(*pLoopConditionStart, xiiVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }

    // Loop Increment
    {
      pLoopIncrement = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Add, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrement, pLoopIndex, 0, xiiVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopIncrement, 1);

      // Dummy input that is not used at runtime but prevents the lastIndexInput from being re-used across the loop's lifetime
      pLoopIncrement->m_DataInputs.PushBack(lastIndexInput);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput        = pLoopIncrement->m_DataOutputs.ExpandAndGetRef();
      dataOutput.m_DataOffset = pLoopIndex->m_DataOutputs[0].m_DataOffset;
    }
  }
  else if (loopType == xiiVisualScriptNodeDescription::Type::Builtin_ForEachLoop ||
           loopType == xiiVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop)
  {
    const bool isReverse  = (loopType == xiiVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop);
    auto&      arrayInput = pLoopNode->m_DataInputs[0];

    // Loop Init
    if (isReverse)
    {
      pLoopInitStart = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      pLoopInitStart->m_DataInputs.PushBack(arrayInput);
      AddDataOutput(*pLoopInitStart, xiiVisualScriptDataType::Int);

      pLoopInitEnd = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Subtract, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopInitEnd, pLoopInitStart, 0, xiiVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopInitEnd, 1);
      AddDataOutput(*pLoopInitEnd, xiiVisualScriptDataType::Int);

      ConnectExecution(*pLoopInitStart, *pLoopInitEnd);

      pLoopIndex = pLoopInitEnd;
    }
    else
    {
      pLoopInitStart = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_ToInt, xiiVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopInitStart, 0);
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
      AddConstantDataInput(*pLoopConditionStart, 0);
      AddDataOutput(*pLoopConditionStart, xiiVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }
    else
    {
      pLoopConditionStart = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      pLoopConditionStart->m_DataInputs.PushBack(arrayInput);
      AddDataOutput(*pLoopConditionStart, xiiVisualScriptDataType::Int);

      pLoopConditionEnd          = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Compare, xiiVisualScriptDataType::Int);
      pLoopConditionEnd->m_Value = xiiInt64(xiiComparisonOperator::Less);
      AddDataInput(*pLoopConditionEnd, pLoopIndex, 0, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionEnd, pLoopConditionStart, 0, xiiVisualScriptDataType::Int);
      AddDataOutput(*pLoopConditionEnd, xiiVisualScriptDataType::Bool);

      ConnectExecution(*pLoopConditionStart, *pLoopConditionEnd);
    }

    // Loop Increment
    {
      auto incType = isReverse ? xiiVisualScriptNodeDescription::Type::Builtin_Subtract : xiiVisualScriptNodeDescription::Type::Builtin_Add;

      pLoopIncrement = &CreateAstNode(incType, xiiVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrement, pLoopIndex, 0, xiiVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopIncrement, 1);

      // Dummy input that is not used at runtime but prevents the array from being re-used across the loop's lifetime
      pLoopIncrement->m_DataInputs.PushBack(arrayInput);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput        = pLoopIncrement->m_DataOutputs.ExpandAndGetRef();
      dataOutput.m_DataOffset = pLoopIndex->m_DataOutputs[0].m_DataOffset;
    }

    pLoopElement = &CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Array_GetElement, xiiVisualScriptDataType::Invalid, true);
    pLoopElement->m_DataInputs.PushBack(arrayInput);
    AddDataInput(*pLoopElement, pLoopIndex, 0, xiiVisualScriptDataType::Int);
    AddDataOutput(*pLoopElement, xiiVisualScriptDataType::Variant);
  }
  else
  {
    XII_ASSERT_NOT_IMPLEMENTED;
  }

  {
    if (conditionDataInput.m_pSourceNode == nullptr && conditionDataInput.m_DataOffset.IsLocal())
    {
      conditionDataInput.m_pSourceNode = pLoopConditionEnd;
    }

    auto& branchNode = CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Branch);
    branchNode.m_DataInputs.PushBack(conditionDataInput);

    ReplaceExecution(*pLoopNode, branchNode);

    if (pLoopConditionStart == nullptr)
    {
      pLoopConditionStart = &branchNode;
    }
    else if (pLoopConditionEnd->m_ExecOutputs.IsEmpty())
    {
      ExecuteBefore(branchNode, *pLoopConditionStart, *pLoopConditionEnd);
    }

    if (pLoopInitStart != nullptr)
    {
      ExecuteBefore(*pLoopConditionStart, *pLoopInitStart, *pLoopInitEnd);
    }

    if (pLoopElement != nullptr && pLoopBody != nullptr)
    {
      ExecuteBefore(*pLoopBody, *pLoopElement, *pLoopElement);
    }
  }

  AstNode* pJumpNode = &CreateJumpNode(pLoopConditionStart);
  if (pLoopIncrement != nullptr)
  {
    ConnectExecution(*pLoopIncrement, *pJumpNode);
    pJumpNode = pLoopIncrement;
  }

  xiiHybridArray<AstNode*, 8> nodesConnectedToBreak;

  if (TraverseAstDepthFirst(pLoopBody,
                            [&](AstNode*& pAstNode) {
                              XII_ASSERT_DEV(xiiVisualScriptNodeDescription::Type::IsLoop(pAstNode->m_Type) == false, "Nested Loops should have been resolved already");

                              for (xiiUInt32 i = 0; i < pAstNode->m_ExecOutputs.GetCount(); ++i)
                              {
                                auto& execOutput = pAstNode->m_ExecOutputs[i];
                                if (execOutput.m_pTargetNode == nullptr)
                                {
                                  ConnectExecution(*pAstNode, *pJumpNode, i);
                                }
                                else if (execOutput.m_pTargetNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_Break)
                                {
                                  // handle breaks after this traversal, otherwise we could end up traversing to nodes outside the loop
                                  nodesConnectedToBreak.PushBack(pAstNode);
                                }
                              }

                              for (auto& dataInput : pAstNode->m_DataInputs)
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
                            })
        .Failed())
  {
    return XII_FAILURE;
  }

  // Go through all loop body nodes and check if they have data connections to nodes outside the loop.
  // If so add the data input to the jump node to prevent register re-use inside the loop.
  m_CompilationState.m_VisitedNodes.Insert(pLoopBody);
  for (auto pAstNode : m_CompilationState.m_VisitedNodes)
  {
    if (pAstNode == pLoopIncrement)
      continue;

    for (auto& dataInput : pAstNode->m_DataInputs)
    {
      auto pSourceNode = dataInput.m_pSourceNode;
      if (pSourceNode == nullptr || pSourceNode == pLoopIndex || pSourceNode == pLoopElement)
        continue;

      if (!m_CompilationState.m_VisitedNodes.Contains(dataInput.m_pSourceNode) && pJumpNode->m_DataInputs.Contains(dataInput) == false)
      {
        pJumpNode->m_DataInputs.PushBack(dataInput);
      }
    }
  }

  // Remove break nodes from the execution flow and connect them to the loop completed node
  for (auto pAstNode : nodesConnectedToBreak)
  {
    for (xiiUInt32 i = 0; i < pAstNode->m_ExecOutputs.GetCount(); ++i)
    {
      auto& execOutput = pAstNode->m_ExecOutputs[i];
      if (execOutput.m_pTargetNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_Break)
      {
        DisconnectExecution(*pAstNode, *execOutput.m_pTargetNode, i);
        if (pLoopCompleted != nullptr)
        {
          ConnectExecution(*pAstNode, *pLoopCompleted, i);
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::ReplaceUnsupportedNodes(AstNode* pEntryAstNode)
{
  xiiHybridArray<AstNode*, 64> unsupportedNodes;

  if (TraverseAstDepthFirst(pEntryAstNode,
                            [&](AstNode*& pAstNode) {
                              if (xiiVisualScriptNodeDescription::Type::IsLoop(pAstNode->m_Type) || pAstNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_CompareExec)
                              {
                                XII_ASSERT_DEBUG(unsupportedNodes.Contains(pAstNode) == false, "");
                                unsupportedNodes.PushBack(pAstNode);
                              }

                              return VisitorResult::Continue;
                            })
        .Failed())
  {
    return XII_FAILURE;
  }

  // Replace unsupported nodes backwards so that we replace inner loops first, order doesn't matter for the other nodes
  for (xiiUInt32 i = unsupportedNodes.GetCount(); i-- > 0;)
  {
    AstNode* pAstNode = unsupportedNodes[i];

    if (pAstNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_CompareExec)
    {
      auto& compareNode        = CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Compare, pAstNode->m_DeductedDataType, true);
      compareNode.m_Value      = pAstNode->m_Value;
      compareNode.m_DataInputs = pAstNode->m_DataInputs;
      AddDataOutput(compareNode, xiiVisualScriptDataType::Bool);

      auto& branchNode = CreateAstNode(xiiVisualScriptNodeDescription::Type::Builtin_Branch);
      AddDataInput(branchNode, &compareNode, 0, xiiVisualScriptDataType::Bool);

      ReplaceExecution(*pAstNode, branchNode);
      ExecuteBefore(branchNode, compareNode, compareNode);
    }
    else if (xiiVisualScriptNodeDescription::Type::IsLoop(pAstNode->m_Type))
    {
      XII_SUCCEED_OR_RETURN(ReplaceLoop(pAstNode));
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::AssignInstanceVariables(AstNode* pEntryAstNode)
{
  return TraverseAstDepthFirst(pEntryAstNode,
                               [&](AstNode*& pAstNode) {
                                 for (auto& dataInput : pAstNode->m_DataInputs)
                                 {
                                   if (dataInput.IsConnectedAndLocal() == false)
                                     continue;

                                   auto pSourceNode = dataInput.m_pSourceNode;
                                   XII_ASSERT_DEBUG(pSourceNode != nullptr, "Data input source node should be always valid for local inputs");

                                   if (pSourceNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_GetVariable)
                                   {
                                     auto& dataOutput = GetDataOutputFromInput(dataInput);

                                     xiiHashedString sName;
                                     sName.Assign(pSourceNode->m_Value.Get<xiiString>());

                                     dataInput.m_pSourceNode      = nullptr;
                                     dataInput.m_uiSourcePinIndex = 0;
                                     dataInput.m_DataOffset       = GetInstanceDataOffset(sName, dataOutput.m_DataOffset.GetType());

                                     // Remove the GetVariable node from the execution flow
                                     if (pSourceNode->m_ExecOutputs.IsEmpty() == false)
                                     {
                                       AstNode* pNodeAfterGetVariable = pSourceNode->m_ExecOutputs[0].m_pTargetNode;
                                       pNodeAfterGetVariable->m_ExecInputs.Clear();

                                       for (auto& execInput : pSourceNode->m_ExecInputs)
                                       {
                                         ConnectExecution(*execInput.m_pSourceNode, *pNodeAfterGetVariable, execInput.m_uiSourcePinIndex);
                                       }

                                       pSourceNode->m_ExecInputs.Clear();
                                       pSourceNode->m_ExecOutputs.Clear();
                                     }
                                   }
                                 }

                                 if (pAstNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_SetVariable ||
                                     pAstNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_IncVariable ||
                                     pAstNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_DecVariable)
                                 {
                                   xiiHashedString sName;
                                   sName.Assign(pAstNode->m_Value.Get<xiiString>());

                                   DataOffset dataOffset = GetInstanceDataOffset(sName, pAstNode->m_DeductedDataType);

                                   if (pAstNode->m_Type != xiiVisualScriptNodeDescription::Type::Builtin_SetVariable)
                                   {
                                     if (pAstNode->m_DataInputs.IsEmpty())
                                     {
                                       auto& dataInput        = pAstNode->m_DataInputs.ExpandAndGetRef();
                                       dataInput.m_DataOffset = dataOffset;
                                     }
                                   }

                                   XII_ASSERT_DEBUG(pAstNode->m_DataOutputs.GetCount() > 0, "");
                                   auto& dataOutput        = pAstNode->m_DataOutputs[0];
                                   dataOutput.m_DataOffset = dataOffset;
                                 }

                                 return VisitorResult::Continue;
                               });
}

xiiResult xiiVisualScriptCompiler::AssignLocalVariables(AstNode* pEntryAstNode, xiiVisualScriptDataDescription& inout_localDataDesc)
{
  m_CompilationState.m_LiveLocalVars.Reserve(m_CompilationState.m_uiNextLocalVarId);
  for (xiiUInt32 i = 0; i < m_CompilationState.m_uiNextLocalVarId; ++i)
  {
    auto& liveLocalVar  = m_CompilationState.m_LiveLocalVars.ExpandAndGetRef();
    liveLocalVar.m_uiId = i;
  }

  xiiUInt32 uiNodeIndex = 0;

  xiiResult r = TraverseAstTopologicalOrder(pEntryAstNode,
                                            [&](const AstNode* pAstNode) {
                                              for (auto& dataInput : pAstNode->m_DataInputs)
                                              {
                                                if (dataInput.IsConnectedAndLocal() == false)
                                                  continue;

                                                const xiiUInt32 id           = dataInput.m_DataOffset.m_uiByteOffset;
                                                auto&           liveLocalVar = m_CompilationState.m_LiveLocalVars[id];

                                                liveLocalVar.m_uiEnd = xiiMath::Max(liveLocalVar.m_uiEnd, uiNodeIndex);
                                              }

                                              for (auto& dataOutput : pAstNode->m_DataOutputs)
                                              {
                                                if (dataOutput.IsValidAndLocal() == false)
                                                  continue;

                                                const xiiUInt32 id           = dataOutput.m_DataOffset.m_uiByteOffset;
                                                auto&           liveLocalVar = m_CompilationState.m_LiveLocalVars[id];

                                                liveLocalVar.m_uiId       = id;
                                                liveLocalVar.m_DataOffset = dataOutput.m_DataOffset;
                                                liveLocalVar.m_uiStart    = xiiMath::Min(liveLocalVar.m_uiStart, uiNodeIndex);
                                              }

                                              ++uiNodeIndex;
                                              return VisitorResult::Continue;
                                            });
  if (r.Failed())
    return XII_FAILURE;

  // This is an implementation of the linear scan register allocation algorithm without spilling
  // https://www2.seas.gwu.edu/~hchoi/teaching/cs160d/linearscan.pdf

  // Sort lifetime by start index
  m_CompilationState.m_LiveLocalVars.Sort(
    [](const LiveLocalVar& a, const LiveLocalVar& b) {
      if (a.m_uiStart != b.m_uiStart)
        return a.m_uiStart < b.m_uiStart;

      return a.m_uiId < b.m_uiId;
    });

  // Assign local vars
  xiiHybridArray<LiveLocalVar, 64> activeIntervals;
  xiiHybridArray<DataOffset, 64>   freeDataOffsets;

  for (auto& liveInterval : m_CompilationState.m_LiveLocalVars)
  {
    // Expire old intervals with less comparison instead of less equal (as in the original paper) so we don't end up using the same data as input and output
    for (xiiUInt32 uiActiveIndex = activeIntervals.GetCount(); uiActiveIndex-- > 0;)
    {
      auto& activeInterval = activeIntervals[uiActiveIndex];
      if (activeInterval.m_uiEnd < liveInterval.m_uiStart)
      {
        freeDataOffsets.PushBack(activeInterval.m_DataOffset);

        activeIntervals.RemoveAtAndCopy(uiActiveIndex);
      }
    }

    // Unused var
    if (liveInterval.m_uiEnd <= liveInterval.m_uiStart)
    {
      liveInterval.m_DataOffset = {};
      continue;
    }

    // Allocate local var
    {
      DataOffset dataOffset;
      dataOffset.m_uiType = liveInterval.m_DataOffset.m_uiType;

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

      liveInterval.m_DataOffset = dataOffset;
    }

    activeIntervals.PushBack(liveInterval);
  }

  // Sort by id and copy back to outputs and inputs
  m_CompilationState.m_LiveLocalVars.Sort([](const LiveLocalVar& a, const LiveLocalVar& b) { return a.m_uiId < b.m_uiId; });

  return TraverseAstDepthFirst(pEntryAstNode,
                               [&](AstNode*& pAstNode) {
                                 for (auto& dataInput : pAstNode->m_DataInputs)
                                 {
                                   if (dataInput.IsConnectedAndLocal() == false)
                                     continue;

                                   const xiiUInt32 id = dataInput.m_DataOffset.m_uiByteOffset;
                                   XII_ASSERT_DEBUG(m_CompilationState.m_LiveLocalVars[id].m_uiId == id, "");
                                   dataInput.m_DataOffset = m_CompilationState.m_LiveLocalVars[id].m_DataOffset;
                                 }

                                 for (auto& dataOutput : pAstNode->m_DataOutputs)
                                 {
                                   if (dataOutput.IsValidAndLocal() == false)
                                     continue;

                                   const xiiUInt32 id = dataOutput.m_DataOffset.m_uiByteOffset;
                                   XII_ASSERT_DEBUG(m_CompilationState.m_LiveLocalVars[id].m_uiId == id, "");
                                   dataOutput.m_DataOffset = m_CompilationState.m_LiveLocalVars[id].m_DataOffset;
                                 }

                                 return VisitorResult::Continue;
                               });
}

xiiResult xiiVisualScriptCompiler::CopyOutputsToInputs(AstNode* pEntryAstNode)
{
  return TraverseAstDepthFirst(pEntryAstNode,
                               [&](AstNode*& pAstNode) {
                                 for (auto& dataInput : pAstNode->m_DataInputs)
                                 {
                                   if (dataInput.IsConnected() == false)
                                     continue;

                                   auto& dataOutput                      = GetDataOutputFromInput(dataInput);
                                   dataInput.m_DataOffset.m_uiByteOffset = dataOutput.m_DataOffset.m_uiByteOffset;
                                   dataInput.m_DataOffset.m_uiSource     = dataOutput.m_DataOffset.m_uiSource;
                                   XII_ASSERT_DEBUG(dataInput.m_DataOffset.GetType() == xiiVisualScriptDataType::Variant || dataInput.m_DataOffset.GetType() == dataOutput.m_DataOffset.GetType(), "");
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

    for (auto& dataInput : astNode.m_DataInputs)
    {
      nodeDesc.m_InputDataOffsets.PushBack(dataInput.m_DataOffset);
    }

    for (auto& dataOutput : astNode.m_DataOutputs)
    {
      nodeDesc.m_OutputDataOffsets.PushBack(dataOutput.m_DataOffset);
    }

    nodeDesc.m_ExecutionIndices.SetCount(astNode.m_ExecOutputs.GetCount(), xiiSmallInvalidIndex);

    XII_VERIFY(astNodeToNodeDescIndices.Insert(&astNode, out_uiNodeDescIndex) == false, "");
    return XII_SUCCESS;
  };

  return TraverseAstTopologicalOrder(pEntryAstNode,
                                     [&](const AstNode* pAstNode) {
                                       xiiUInt32 uiTargetIndex = 0;

                                       // Jump nodes should not end up in the final node descriptions
                                       if (pAstNode->m_Type == xiiVisualScriptNodeDescription::Type::Builtin_Jump)
                                       {
                                         xiiUInt64      uiPtr          = pAstNode->m_Value.Get<xiiUInt64>();
                                         const AstNode* pTargetAstNode = *reinterpret_cast<const AstNode**>(&uiPtr);

                                         if (astNodeToNodeDescIndices.TryGetValue(pTargetAstNode, uiTargetIndex) == false)
                                           return VisitorResult::Error;
                                       }
                                       else
                                       {
                                         if (CreateNodeDesc(*pAstNode, uiTargetIndex).Failed())
                                           return VisitorResult::Error;
                                       }

                                       for (auto& execInput : pAstNode->m_ExecInputs)
                                       {
                                         if (execInput.m_pSourceNode == nullptr)
                                           continue;

                                         xiiUInt32 uiSourceIndex = 0;
                                         XII_VERIFY(astNodeToNodeDescIndices.TryGetValue(execInput.m_pSourceNode, uiSourceIndex), "Topological sort failed");

                                         auto pSourceNodeDesc                                              = &out_NodeDescriptions[uiSourceIndex];
                                         pSourceNodeDesc->m_ExecutionIndices[execInput.m_uiSourcePinIndex] = uiTargetIndex;
                                       }

                                       return VisitorResult::Continue;
                                     });
}

xiiResult xiiVisualScriptCompiler::FinalizeConstantData()
{
  m_Module.m_ConstantDataDesc.CalculatePerTypeStartOffsets();
  m_Module.m_ConstantDataStorage.AllocateStorage(xiiFoundation::GetDefaultAllocator());

  for (auto& it : m_Module.m_ConstantDataToIndex)
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

xiiResult xiiVisualScriptCompiler::TraverseAstDepthFirst(AstNode* pEntryAstNode, xiiDelegate<VisitorResult(AstNode*& pAstNode)> func)
{
  if (pEntryAstNode == nullptr)
    return XII_SUCCESS;

  m_CompilationState.m_VisitedNodes.Clear();

  xiiHybridArray<AstNode*, 64> nodeStack;
  nodeStack.PushBack(pEntryAstNode);

  while (nodeStack.IsEmpty() == false)
  {
    AstNode* pAstNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    AstNode* pOldAstNode = pAstNode;

    VisitorResult r = func(pAstNode);
    if (r == VisitorResult::Error)
      return XII_FAILURE;

    // Check whether the node has been replaced by a new one and if so mark the new one as visited as well
    if (pAstNode != pOldAstNode)
    {
      XII_VERIFY(m_CompilationState.m_VisitedNodes.Insert(pAstNode) == false, "");
    }

    for (xiiUInt32 i = 0; i < pAstNode->m_ExecOutputs.GetCount(); ++i)
    {
      auto& execOutput = pAstNode->m_ExecOutputs[i];

      if (execOutput.m_pTargetNode == nullptr)
        continue;
      if (m_CompilationState.m_VisitedNodes.Insert(execOutput.m_pTargetNode))
        continue;

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      bool bExecInputFound = false;
      for (auto& execInput : execOutput.m_pTargetNode->m_ExecInputs)
      {
        if (execInput.m_pSourceNode == pAstNode && execInput.m_uiSourcePinIndex == i)
        {
          bExecInputFound = true;
          break;
        }
      }
      XII_ASSERT_DEBUG(bExecInputFound, "Execution connection corrupted");
#endif

      nodeStack.PushBack(execOutput.m_pTargetNode);
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::TraverseAstTopologicalOrder(const AstNode* pEntryAstNode, xiiDelegate<VisitorResult(const AstNode* pAstNode)> func)
{
  if (pEntryAstNode == nullptr)
    return XII_SUCCESS;

  m_CompilationState.m_VisitedNodes.Clear();

  xiiHybridArray<const AstNode*, 64> nodeStack;
  nodeStack.PushBack(pEntryAstNode);

  while (nodeStack.IsEmpty() == false)
  {
    // Find next node with all execution dependencies visited
    const AstNode* pAstNode = nullptr;
    for (xiiUInt32 i = nodeStack.GetCount(); i-- > 0;)
    {
      const AstNode* pCandidateAstNode = nodeStack[i];
      bool           bAllVisited       = true;
      for (auto& execInput : pCandidateAstNode->m_ExecInputs)
      {
        if (execInput.m_pSourceNode != nullptr && m_CompilationState.m_VisitedNodes.Contains(execInput.m_pSourceNode) == false)
        {
          bAllVisited = false;
          break;
        }
      }

      if (bAllVisited)
      {
        pAstNode = pCandidateAstNode;
        nodeStack.RemoveAtAndCopy(i);
        break;
      }
    }

    if (pAstNode == nullptr)
    {
      XII_REPORT_FAILURE("Execution connection corrupted or loop detected");
      return XII_FAILURE;
    }

    XII_VERIFY(m_CompilationState.m_VisitedNodes.Insert(pAstNode) == false, "");

    VisitorResult r = func(pAstNode);
    if (r == VisitorResult::Error)
      return XII_FAILURE;

    // Since we use a stack we need to iterate the execution outputs backwards to ensure that they are processed in order.
    // Strictly speaking this is not necessary but improves data locality especially for loops,
    // which in the end results in shorter variable lifetimes and thus in fewer local variables.
    for (xiiUInt32 i = pAstNode->m_ExecOutputs.GetCount(); i-- > 0;)
    {
      auto& execOutput = pAstNode->m_ExecOutputs[i];

      if (execOutput.m_pTargetNode == nullptr)
        continue;
      if (m_CompilationState.m_VisitedNodes.Contains(execOutput.m_pTargetNode))
        continue;

      if (nodeStack.Contains(execOutput.m_pTargetNode) == false)
      {
        nodeStack.PushBack(execOutput.m_pTargetNode);
      }
    }
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
    xiiUInt32                               uiNodeIndex = 0;

    // First build all the dgml graph nodes
    TraverseAstTopologicalOrder(pEntryAstNode,
                                [&](const AstNode* pAstNode) {
                                  xiiStringView sTypeName = xiiVisualScriptNodeDescription::Type::GetName(pAstNode->m_Type);
                                  sb.SetFormat("{} (id: {})", sTypeName, uiNodeIndex);
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

                                  float colorX = xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(xiiHashingUtils::StringHash(sTypeName))).x();

                                  xiiDGMLGraph::NodeDesc nd;
                                  nd.m_Color            = xiiColorScheme::LightUI(colorX);
                                  xiiUInt32 uiGraphNode = dgmlGraph.AddNode(sb, &nd);
                                  XII_VERIFY(nodeCache.Insert(pAstNode, uiGraphNode) == false, "");

                                  ++uiNodeIndex;
                                  return VisitorResult::Continue;
                                })
      .IgnoreResult();

    // Then build all the connections
    for (auto it : nodeCache)
    {
      const AstNode*  pAstNode    = it.Key();
      const xiiUInt32 uiGraphNode = it.Value();

      for (auto& execInput : pAstNode->m_ExecInputs)
      {
        xiiUInt32 uiSourceGraphNode = 0;
        XII_VERIFY(nodeCache.TryGetValue(execInput.m_pSourceNode, uiSourceGraphNode), "");

        xiiUInt64  uiConnectionKey = uiSourceGraphNode | xiiUInt64(uiGraphNode) << 32;
        xiiString& sLabel          = connectionCache[uiConnectionKey];

        xiiStringBuilder sb = sLabel;
        if (sb.IsEmpty() == false)
        {
          sb.Append(" + ");
        }
        sb.AppendFormat("Exec{}", execInput.m_uiSourcePinIndex);
        sLabel = sb;
      }

      for (xiiUInt32 i = 0; i < pAstNode->m_DataInputs.GetCount(); ++i)
      {
        auto& dataInput = pAstNode->m_DataInputs[i];
        if (dataInput.m_pSourceNode == nullptr)
          continue;

        // Exclude GetScriptOwner connections as they create too much noise
        if (dataInput.m_pSourceNode->m_Type == xiiVisualScriptNodeDescription::Type::GetScriptOwner)
          continue;

        auto& dataOutput = GetDataOutputFromInput(dataInput);

        xiiUInt32 uiSourceGraphNode = 0;
        XII_VERIFY(nodeCache.TryGetValue(dataInput.m_pSourceNode, uiSourceGraphNode), "");

        xiiUInt64  uiConnectionKey = uiSourceGraphNode | xiiUInt64(uiGraphNode) << 32;
        xiiString& sLabel          = connectionCache[uiConnectionKey];

        xiiStringBuilder sb = sLabel;
        if (sb.IsEmpty() == false)
        {
          sb.Append(" + ");
        }
        const xiiUInt32 uiOutputId = dataOutput.m_DataOffset.m_uiByteOffset;
        const xiiUInt32 uiInputId  = dataInput.m_DataOffset.m_uiByteOffset;
        sb.AppendFormat("o{}:{} (id: {})->i{}:{} (id: {})", dataInput.m_uiSourcePinIndex, xiiVisualScriptDataType::GetName(dataOutput.m_DataOffset.GetType()), uiOutputId, i, xiiVisualScriptDataType::GetName(dataInput.m_DataOffset.GetType()), uiInputId);
        sLabel = sb;
      }
    }

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
    for (xiiUInt32 i = 0; i < nodeDescriptions.GetCount(); ++i)
    {
      auto& nodeDesc = nodeDescriptions[i];

      xiiStringView sTypeName = xiiVisualScriptNodeDescription::Type::GetName(nodeDesc.m_Type);
      sTmp                    = sTypeName;

      nodeDesc.AppendUserDataName(sTmp);

      sTmp.AppendFormat(" (id: {})", i);

      for (auto& dataOffset : nodeDesc.m_InputDataOffsets)
      {
        sTmp.AppendFormat("\n Input {} {}[{}]", DataOffset::Source::GetName(dataOffset.GetSource()), xiiVisualScriptDataType::GetName(dataOffset.GetType()), dataOffset.m_uiByteOffset);

        if (dataOffset.GetSource() == DataOffset::Source::Constant)
        {
          for (auto& it : m_Module.m_ConstantDataToIndex)
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

    for (xiiUInt32 uiCurrentIndex = 0; uiCurrentIndex < nodeDescriptions.GetCount(); ++uiCurrentIndex)
    {
      auto& executionIndices = nodeDescriptions[uiCurrentIndex].m_ExecutionIndices;
      for (xiiUInt32 uiExecIndex = 0; uiExecIndex < executionIndices.GetCount(); ++uiExecIndex)
      {
        const xiiUInt32 uiNextIndex = executionIndices[uiExecIndex];
        if (uiNextIndex == xiiSmallInvalidIndex)
          continue;

        sTmp.SetFormat("Exec{}", uiExecIndex);
        dgmlGraph.AddConnection(uiCurrentIndex, uiNextIndex, sTmp);
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
