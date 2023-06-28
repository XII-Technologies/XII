#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
  xiiResult ExtractPropertyName(xiiStringView sPinName, xiiStringView& out_sPropertyName, xiiUInt32* out_uiArrayIndex = nullptr)
  {
    const char* szBracket = sPinName.FindSubString("[");
    if (szBracket == nullptr)
      return XII_FAILURE;

    out_sPropertyName = xiiStringView(sPinName.GetStartPointer(), szBracket);

    if (out_uiArrayIndex != nullptr)
    {
      return xiiConversionUtils::StringToUInt(szBracket + 1, *out_uiArrayIndex);
    }

    return XII_SUCCESS;
  }

  using FillUserDataFunction = xiiResult (*)(xiiVisualScriptNodeDescription& ref_nodeDesc, const xiiDocumentObject* pObject);

  static xiiResult FillUserData_ReflectedPropertyOrFunction(xiiVisualScriptNodeDescription& ref_nodeDesc, const xiiDocumentObject* pObject)
  {
    auto pNodeDesc                            = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    ref_nodeDesc.m_UserData.m_pTargetType     = pNodeDesc->m_pTargetType;
    ref_nodeDesc.m_UserData.m_pTargetProperty = pNodeDesc->m_pTargetProperty;
    return XII_SUCCESS;
  }

  static xiiResult FillUserData_Builtin_Compare(xiiVisualScriptNodeDescription& ref_nodeDesc, const xiiDocumentObject* pObject)
  {
    auto compOp                                  = pObject->GetTypeAccessor().GetValue("Operator");
    ref_nodeDesc.m_UserData.m_ComparisonOperator = static_cast<xiiComparisonOperator::Enum>(compOp.Get<xiiInt64>());
    return XII_SUCCESS;
  }

  static xiiResult FillUserData_Builtin_TryGetComponentOfBaseType(xiiVisualScriptNodeDescription& ref_nodeDesc, const xiiDocumentObject* pObject)
  {
    auto           typeName = pObject->GetTypeAccessor().GetValue("TypeName");
    const xiiRTTI* pType    = xiiRTTI::FindTypeByName(typeName.Get<xiiString>());
    if (pType == nullptr)
    {
      xiiLog::Error("Invalid type '{}' for GameObject::TryGetComponentOfBaseType node.", typeName);
      return XII_FAILURE;
    }

    ref_nodeDesc.m_UserData.m_pTargetType = pType;
    return XII_SUCCESS;
  }

  static FillUserDataFunction s_TypeToFillUserDataFunctions[] = {
    nullptr,                                   // Invalid,
    nullptr,                                   // EntryCall,
    nullptr,                                   // MessageHandler,
    &FillUserData_ReflectedPropertyOrFunction, // ReflectedFunction,
    nullptr,                                   // GetOwner,

    nullptr, // FirstBuiltin,

    nullptr,                       // Builtin_Branch,
    nullptr,                       // Builtin_And,
    nullptr,                       // Builtin_Or,
    nullptr,                       // Builtin_Not,
    &FillUserData_Builtin_Compare, // Builtin_Compare,
    nullptr,                       // Builtin_IsValid,

    nullptr, // Builtin_Add,
    nullptr, // Builtin_Subtract,
    nullptr, // Builtin_Multiply,
    nullptr, // Builtin_Divide,

    nullptr, // Builtin_ToBool,
    nullptr, // Builtin_ToByte,
    nullptr, // Builtin_ToInt,
    nullptr, // Builtin_ToInt64,
    nullptr, // Builtin_ToFloat,
    nullptr, // Builtin_ToDouble,
    nullptr, // Builtin_ToString,
    nullptr, // Builtin_ToVariant,
    nullptr, // Builtin_Variant_ConvertTo,

    nullptr, // Builtin_MakeArray

    &FillUserData_Builtin_TryGetComponentOfBaseType, // Builtin_TryGetComponentOfBaseType

    nullptr, // LastBuiltin,
  };

  static_assert(XII_ARRAY_SIZE(s_TypeToFillUserDataFunctions) == xiiVisualScriptNodeDescription::Type::Count);

  xiiResult FillUserData(xiiVisualScriptNodeDescription& ref_nodeDesc, const xiiDocumentObject* pObject)
  {
    if (pObject == nullptr)
      return XII_SUCCESS;

    auto nodeType = ref_nodeDesc.m_Type;
    XII_ASSERT_DEBUG(nodeType >= 0 && nodeType < XII_ARRAY_SIZE(s_TypeToFillUserDataFunctions), "Out of bounds access");
    auto func = s_TypeToFillUserDataFunctions[nodeType];

    if (func != nullptr)
    {
      XII_SUCCEED_OR_RETURN(func(ref_nodeDesc, pObject));
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

xiiResult xiiVisualScriptCompiler::CompiledModule::Serialize(xiiStreamWriter& inout_stream, xiiStringView sBaseClassName, xiiStringView sScriptClassName) const
{
  xiiStringDeduplicationWriteContext stringDedup(inout_stream);

  xiiChunkStreamWriter chunk(stringDedup.Begin());
  chunk.BeginStream(1);

  {
    chunk.BeginChunk("Header", 1);
    chunk << sBaseClassName;
    chunk << sScriptClassName;
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("FunctionGraphs", 1);
    chunk << m_Functions.GetCount();

    for (auto& function : m_Functions)
    {
      chunk << function.m_sName;
      chunk << function.m_Type;

      XII_SUCCEED_OR_RETURN(xiiVisualScriptGraphDescription::Serialize(function.m_NodeDescriptions, chunk));
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
    chunk.BeginChunk("VariableDataDesc", 1);
    XII_SUCCEED_OR_RETURN(m_VariableDataDesc.Serialize(chunk));
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

xiiResult xiiVisualScriptCompiler::AddFunction(xiiStringView sName, xiiVisualScriptNodeDescription::Type::Enum type, const xiiDocumentObject* pEntryObject)
{
  if (m_pManager == nullptr)
  {
    m_pManager = static_cast<const xiiVisualScriptNodeManager*>(pEntryObject->GetDocumentObjectManager());
  }
  XII_ASSERT_DEV(m_pManager == pEntryObject->GetDocumentObjectManager(), "Can't add functions from different document");

  AstNode* pEntryAstNode = BuildAST(pEntryObject);
  if (pEntryAstNode == nullptr)
    return XII_FAILURE;

  auto& function   = m_Module.m_Functions.ExpandAndGetRef();
  function.m_sName = sName;
  function.m_Type  = type;

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

    XII_SUCCEED_OR_RETURN(InsertTypeConversions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_01_TypeConv");

    XII_SUCCEED_OR_RETURN(BuildDataExecutions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_02_FlattenedExec");

    XII_SUCCEED_OR_RETURN(FillDataOutputConnections(pEntryAstNode));
    XII_SUCCEED_OR_RETURN(CollectData(pEntryAstNode));
    XII_SUCCEED_OR_RETURN(BuildNodeDescriptions(pEntryAstNode, function.m_NodeDescriptions));

    DumpGraph(function.m_NodeDescriptions, sDebugAstOutputPath, function.m_sName, "_Graph");
  }

  m_Module.m_VariableDataDesc.CalculatePerTypeStartOffsets();
  m_Module.m_ConstantDataDesc.CalculatePerTypeStartOffsets();

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
  for (auto& dataOutput : dataInput.m_pSourceNode->m_Outputs)
  {
    if (dataOutput.m_uiSourcePinIndex == dataInput.m_uiSourcePinIndex)
    {
      return dataOutput;
    }
  }

  XII_ASSERT_DEBUG(false, "This code should be never reached");
  static DataOutput dummy;
  return dummy;
}

xiiVisualScriptCompiler::AstNode* xiiVisualScriptCompiler::BuildAST(const xiiDocumentObject* pEntryObject)
{
  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;

  auto CreateAstNode = [&](const xiiDocumentObject* pObject) {
    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    XII_ASSERT_DEV(pNodeDesc != nullptr, "Invalid node type");

    auto& astNode                = m_AstNodes.ExpandAndGetRef();
    astNode.m_Type               = pNodeDesc->m_Type;
    astNode.m_DeductedDataType   = GetDeductedType(pObject);
    astNode.m_bImplicitExecution = pNodeDesc->m_bImplicitExecution;
    astNode.m_pObject            = pObject;

    m_ObjectToAstNode.Insert(pObject, &astNode);

    return &astNode;
  };

  AstNode* pEntryAstNode = CreateAstNode(pEntryObject);
  if (pEntryAstNode == nullptr)
    return nullptr;

  xiiHybridArray<const xiiDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pEntryObject);

  while (nodeStack.IsEmpty() == false)
  {
    const xiiDocumentObject* pObject = nodeStack.PeekBack();
    nodeStack.PopBack();

    AstNode* pAstNode = nullptr;
    XII_VERIFY(m_ObjectToAstNode.TryGetValue(pObject, pAstNode), "Implementation error");

    m_pManager->GetInputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto connections = m_pManager->GetConnections(*pPin);
      if (pPin->IsRequired() && connections.IsEmpty())
      {
        xiiLog::Error("Required input '{}' for '{}' is not connected", pPin->GetName(), GetNiceTypeName(pObject));
        return nullptr;
      }

      auto& dataInput              = pAstNode->m_Inputs.ExpandAndGetRef();
      dataInput.m_uiId             = GetPinId(pPin);
      dataInput.m_uiTargetPinIndex = pPin->GetPinIndex();

      if (connections.IsEmpty() == false)
      {
        auto&                    sourcePin     = static_cast<const xiiVisualScriptPin&>(connections[0]->GetSourcePin());
        const xiiDocumentObject* pSourceObject = sourcePin.GetParent();

        AstNode* pSourceAstNode;
        if (m_ObjectToAstNode.TryGetValue(pSourceObject, pSourceAstNode) == false)
        {
          pSourceAstNode = CreateAstNode(pSourceObject);
          if (pSourceAstNode == nullptr)
            return nullptr;

          nodeStack.PushBack(pSourceObject);
        }

        xiiVisualScriptDataType::Enum sourceDeductedDataType = GetDeductedType(pSourceObject);
        if (sourcePin.GetScriptDataType() == xiiVisualScriptDataType::Any && sourceDeductedDataType == xiiVisualScriptDataType::Invalid)
        {
          xiiLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pSourceObject), sourcePin.GetName());
          return nullptr;
        }

        xiiVisualScriptDataType::Enum targetDeductedDataType = GetDeductedType(pObject);
        if (pPin->GetScriptDataType() == xiiVisualScriptDataType::Any && targetDeductedDataType == xiiVisualScriptDataType::Invalid)
        {
          xiiLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pObject), pPin->GetName());
          return nullptr;
        }

        if (sourcePin.CanConvertTo(*pPin, sourceDeductedDataType, targetDeductedDataType) == false)
        {
          xiiLog::Error("Can't implicitly convert pin '{}.{}' of type '{}' connected to pin '{}.{}' of type '{}'", GetNiceTypeName(pSourceObject), sourcePin.GetName(), sourcePin.GetDataTypeName(sourceDeductedDataType), GetNiceTypeName(pObject), pPin->GetName(), pPin->GetDataTypeName(targetDeductedDataType));
          return nullptr;
        }

        dataInput.m_pSourceNode      = pSourceAstNode;
        dataInput.m_uiSourcePinIndex = sourcePin.GetPinIndex();
        dataInput.m_DataType         = pPin->GetScriptDataType();
        if (dataInput.m_DataType == xiiVisualScriptDataType::Any)
          dataInput.m_DataType = targetDeductedDataType;
      }
    }

    m_pManager->GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& dataOutput              = pAstNode->m_Outputs.ExpandAndGetRef();
      dataOutput.m_uiId             = GetPinId(pPin);
      dataOutput.m_uiSourcePinIndex = pPin->GetPinIndex();
      dataOutput.m_DataType         = pPin->GetScriptDataType();
      if (dataOutput.m_DataType == xiiVisualScriptDataType::Any)
        dataOutput.m_DataType = GetDeductedType(pObject);
    }

    m_pManager->GetOutputExecutionPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto connections = m_pManager->GetConnections(*pPin);
      if (connections.IsEmpty())
      {
        pAstNode->m_Next.PushBack(nullptr);
      }
      else
      {
        XII_ASSERT_DEV(connections.GetCount() == 1, "Output execution pins should only have one connection");
        const xiiDocumentObject* pNextNode = connections[0]->GetTargetPin().GetParent();

        AstNode* pNextAstNode;
        if (m_ObjectToAstNode.TryGetValue(pNextNode, pNextAstNode) == false)
        {
          pNextAstNode = CreateAstNode(pNextNode);
          if (pNextAstNode == nullptr)
            return nullptr;

          nodeStack.PushBack(pNextNode);
        }

        pAstNode->m_Next.PushBack(pNextAstNode);
      }
    }
  }

  return pEntryAstNode;
}

xiiResult xiiVisualScriptCompiler::InsertMakeArrayForDynamicPin(AstNode* pNode, const xiiVisualScriptNodeRegistry::PinDesc& pinDesc, xiiPin::Type pinType)
{
  if (pinDesc.m_sDynamicPinProperty.IsEmpty() || pinDesc.IsExecutionPin())
    return XII_SUCCESS;

  const xiiAbstractProperty* pProp = pNode->m_pObject->GetType()->FindPropertyByName(pinDesc.m_sDynamicPinProperty);
  if (pProp == nullptr)
    return XII_FAILURE;

  if (pProp->GetCategory() != xiiPropertyCategory::Array)
    return XII_SUCCESS;

  auto& astNode                = m_AstNodes.ExpandAndGetRef();
  astNode.m_Type               = xiiVisualScriptNodeDescription::Type::Builtin_MakeArray;
  astNode.m_bImplicitExecution = true;
  astNode.m_pObject            = pNode->m_pObject;

  auto& newDataOutput              = astNode.m_Outputs.ExpandAndGetRef();
  newDataOutput.m_uiId             = GetPinId(nullptr);
  newDataOutput.m_uiSourcePinIndex = 0;
  newDataOutput.m_DataType         = xiiVisualScriptDataType::Array;

  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;
  if (pinType == xiiPin::Type::Input)
  {
    m_pManager->GetInputDataPins(pNode->m_pObject, pins);
  }
  else
  {
    m_pManager->GetOutputDataPins(pNode->m_pObject, pins);
  }

  xiiUInt32 uiNewInputIndex = pins.GetCount();
  for (xiiUInt32 i = pins.GetCount(); i-- > 0;)
  {
    auto pPin = pins[i];

    xiiStringView sPropertyName;
    xiiUInt32     uiArrayIndex;
    if (ExtractPropertyName(pPin->GetName(), sPropertyName, &uiArrayIndex).Failed())
      continue;

    if (pinDesc.m_sDynamicPinProperty.GetView() != sPropertyName)
      continue;

    auto& oldDataInput = pNode->m_Inputs[i];

    astNode.m_Inputs.EnsureCount(uiArrayIndex + 1);
    auto& newDataInput              = astNode.m_Inputs[uiArrayIndex];
    newDataInput.m_pSourceNode      = oldDataInput.m_pSourceNode;
    newDataInput.m_uiId             = GetPinId(nullptr);
    newDataInput.m_uiSourcePinIndex = oldDataInput.m_uiSourcePinIndex;
    newDataInput.m_uiTargetPinIndex = oldDataInput.m_pSourceNode != nullptr ? uiArrayIndex : oldDataInput.m_uiTargetPinIndex;
    newDataInput.m_DataType         = oldDataInput.m_DataType;
    newDataInput.m_uiArrayIndex     = uiArrayIndex;

    pNode->m_Inputs.RemoveAtAndCopy(i);
    uiNewInputIndex = i;
  }

  {
    DataInput newDataInput;
    newDataInput.m_pSourceNode      = &astNode;
    newDataInput.m_uiId             = GetPinId(nullptr);
    newDataInput.m_uiSourcePinIndex = 0;
    newDataInput.m_uiTargetPinIndex = uiNewInputIndex;
    newDataInput.m_DataType         = xiiVisualScriptDataType::Array;

    pNode->m_Inputs.Insert(newDataInput, uiNewInputIndex);
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::InsertTypeConversions(AstNode* pEntryAstNode)
{
  xiiHashSet<const AstNode*> nodesWithInsertedMakeArrayNode;

  return TraverseAst(pEntryAstNode, ConnectionType::All,
                     [&](const Connection& connection) {
                       if (connection.m_Type == ConnectionType::Data)
                       {
                         auto& dataInput  = connection.m_pPrev->m_Inputs[connection.m_uiPrevPinIndex];
                         auto& dataOutput = GetDataOutput(dataInput);

                         if (dataOutput.m_DataType != dataInput.m_DataType)
                         {
                           auto nodeType = xiiVisualScriptNodeDescription::Type::GetConversionType(dataInput.m_DataType);

                           auto& astNode                = m_AstNodes.ExpandAndGetRef();
                           astNode.m_Type               = nodeType;
                           astNode.m_DeductedDataType   = dataOutput.m_DataType;
                           astNode.m_bImplicitExecution = true;

                           auto& newDataInput              = astNode.m_Inputs.ExpandAndGetRef();
                           newDataInput.m_pSourceNode      = dataInput.m_pSourceNode;
                           newDataInput.m_uiId             = GetPinId(nullptr);
                           newDataInput.m_uiSourcePinIndex = dataInput.m_uiSourcePinIndex;
                           newDataInput.m_uiTargetPinIndex = 0;
                           newDataInput.m_DataType         = dataOutput.m_DataType;

                           auto& newDataOutput              = astNode.m_Outputs.ExpandAndGetRef();
                           newDataOutput.m_uiId             = GetPinId(nullptr);
                           newDataOutput.m_uiSourcePinIndex = 0;
                           newDataOutput.m_DataType         = dataInput.m_DataType;

                           dataInput.m_pSourceNode      = &astNode;
                           dataInput.m_uiSourcePinIndex = 0;
                         }
                       }

                       AstNode* pNode = connection.m_pCurrent;
                       if (pNode->m_pObject != nullptr && pNode->m_Type != xiiVisualScriptNodeDescription::Type::Builtin_MakeArray)
                       {
                         auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pNode->m_pObject->GetType());
                         if (pNodeDesc != nullptr && pNodeDesc->m_bHasDynamicPins && nodesWithInsertedMakeArrayNode.Insert(pNode) == false)
                         {
                           for (auto& pinDesc : pNodeDesc->m_InputPins)
                           {
                             if (InsertMakeArrayForDynamicPin(pNode, pinDesc, xiiPin::Type::Input).Failed())
                               return VisitorResult::Error;
                           }
                         }
                       }

                       return VisitorResult::Continue;
                     });
}


xiiResult xiiVisualScriptCompiler::BuildDataStack(AstNode* pEntryAstNode, xiiDynamicArray<AstNode*>& out_Stack)
{
  xiiHashSet<const AstNode*> visitedNodes;
  out_Stack.Clear();

  XII_SUCCEED_OR_RETURN(TraverseAst(pEntryAstNode, ConnectionType::Data,
                                    [&](const Connection& connection) {
                                      if (visitedNodes.Insert(connection.m_pCurrent))
                                        return VisitorResult::Stop;

                                      if (connection.m_pCurrent->m_bImplicitExecution == false)
                                        return VisitorResult::Stop;

                                      out_Stack.PushBack(connection.m_pCurrent);

                                      return VisitorResult::Continue;
                                    }));

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
      auto& newDataNode                = m_AstNodes.ExpandAndGetRef();
      newDataNode.m_Type               = pDataNode->m_Type;
      newDataNode.m_DeductedDataType   = pDataNode->m_DeductedDataType;
      newDataNode.m_bImplicitExecution = pDataNode->m_bImplicitExecution;
      newDataNode.m_pObject            = pDataNode->m_pObject;

      for (auto& dataInput : pDataNode->m_Inputs)
      {
        auto& newDataInput = newDataNode.m_Inputs.ExpandAndGetRef();
        if (dataInput.m_pSourceNode != nullptr)
        {
          XII_VERIFY(oldToNewNodes.TryGetValue(dataInput.m_pSourceNode, newDataInput.m_pSourceNode), "");
        }
        newDataInput.m_uiId             = GetPinId(nullptr);
        newDataInput.m_uiSourcePinIndex = dataInput.m_uiSourcePinIndex;
        newDataInput.m_uiTargetPinIndex = dataInput.m_uiTargetPinIndex;
        newDataInput.m_DataType         = dataInput.m_DataType;
      }

      for (auto& dataOutput : pDataNode->m_Outputs)
      {
        auto& newDataOutput              = newDataNode.m_Outputs.ExpandAndGetRef();
        newDataOutput.m_uiId             = GetPinId(nullptr);
        newDataOutput.m_uiSourcePinIndex = dataOutput.m_uiSourcePinIndex;
        newDataOutput.m_DataType         = dataOutput.m_DataType;
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

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::BuildDataExecutions(AstNode* pEntryAstNode)
{
  xiiHybridArray<AstNode*, 64>     nodeStack;
  xiiHashTable<AstNode*, AstNode*> nodeToFirstDataNode;

  return TraverseAst(pEntryAstNode, ConnectionType::Execution,
                     [&](const Connection& connection) {
                       AstNode* pFirstDataNode = nullptr;
                       if (nodeToFirstDataNode.TryGetValue(connection.m_pCurrent, pFirstDataNode) == false)
                       {
                         if (BuildDataStack(connection.m_pCurrent, nodeStack).Failed())
                           return VisitorResult::Error;

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

                       return VisitorResult::Continue;
                     });
}

xiiResult xiiVisualScriptCompiler::FillDataOutputConnections(AstNode* pEntryAstNode)
{
  return TraverseAst(pEntryAstNode, ConnectionType::All,
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

xiiResult xiiVisualScriptCompiler::CollectData(AstNode* pEntryAstNode)
{
  xiiDynamicArray<DataOffset> freeDataOffsets;

  return TraverseAst(pEntryAstNode, ConnectionType::Execution,
                     [&](const Connection& connection) {
                       // Outputs first so we don't end up using the same data as input and output
                       for (auto& dataOutput : connection.m_pCurrent->m_Outputs)
                       {
                         if (m_PinIdToDataDesc.Contains(dataOutput.m_uiId))
                           continue;

                         if (dataOutput.m_TargetNodes.IsEmpty() == false)
                         {
                           DataOffset dataOffset;
                           dataOffset.m_uiDataType = dataOutput.m_DataType;

                           for (xiiUInt32 i = 0; i < freeDataOffsets.GetCount(); ++i)
                           {
                             auto freeDataOffset = freeDataOffsets[i];
                             if (freeDataOffset.m_uiDataType == dataOffset.m_uiDataType)
                             {
                               dataOffset = freeDataOffset;
                               freeDataOffsets.RemoveAtAndSwap(i);
                               break;
                             }
                           }

                           if (dataOffset.IsValid() == false)
                           {
                             XII_ASSERT_DEBUG(dataOffset.m_uiDataType < xiiVisualScriptDataType::Count, "Invalid data type");
                             auto& offsetAndCount      = m_Module.m_VariableDataDesc.m_PerTypeInfo[dataOffset.m_uiDataType];
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
                         if (m_PinIdToDataDesc.Contains(dataInput.m_uiId))
                           continue;

                         if (dataInput.m_pSourceNode == nullptr)
                         {
                           const xiiDocumentObject* pObject  = connection.m_pCurrent->m_pObject;
                           auto&                    inputPin = static_cast<const xiiVisualScriptPin&>(*(m_pManager->GetInputPins(pObject)[dataInput.m_uiTargetPinIndex]));

                           xiiStringView sPropertyName = inputPin.GetName();
                           if (inputPin.HasDynamicPinProperty())
                           {
                             XII_VERIFY(ExtractPropertyName(sPropertyName, sPropertyName).Succeeded(), "");
                           }

                           xiiStringBuilder sTmp;
                           const char*      szPropertyName = sPropertyName.GetData(sTmp);

                           xiiVariant value = pObject->GetTypeAccessor().GetValue(szPropertyName);
                           if (value.IsValid() && inputPin.HasDynamicPinProperty())
                           {
                             XII_ASSERT_DEBUG(value.IsA<xiiVariantArray>(), "Implementation error");
                             value = value.Get<xiiVariantArray>()[dataInput.m_uiArrayIndex];
                           }

                           auto dataType = xiiVisualScriptDataType::FromVariantType(value.GetType());
                           if (dataType == xiiVisualScriptDataType::Invalid)
                           {
                             auto pProp = pObject->GetType()->FindPropertyByName(szPropertyName);
                             if (pProp != nullptr && pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
                             {
                               dataType = xiiVisualScriptDataType::Variant;
                             }
                             else
                             {
                               xiiLog::Error("Constant value for '{}.{}' is invalid", GetNiceTypeName(pObject), inputPin.GetName());
                               return VisitorResult::Error;
                             }
                           }

                           xiiVisualScriptDataType::Enum deductedType = connection.m_pCurrent->m_DeductedDataType;
                           if (deductedType != xiiVisualScriptDataType::Invalid)
                           {
                             value = value.ConvertTo(xiiVisualScriptDataType::GetVariantType(deductedType));
                             if (value.IsValid() == false)
                             {
                               xiiLog::Error("Failed to convert '{}.{}' of type '{}' to '{}'.", GetNiceTypeName(pObject), inputPin.GetName(), xiiVisualScriptDataType::GetName(dataType), xiiVisualScriptDataType::GetName(deductedType));
                               return VisitorResult::Error;
                             }

                             dataType = deductedType;
                           }

                           xiiUInt32 uiIndex = 0;
                           if (m_ConstantDataToIndex.TryGetValue(value, uiIndex) == false)
                           {
                             auto& offsetAndCount = m_Module.m_ConstantDataDesc.m_PerTypeInfo[dataType];
                             uiIndex              = offsetAndCount.m_uiCount;
                             ++offsetAndCount.m_uiCount;

                             m_ConstantDataToIndex.Insert(value, uiIndex);
                           }

                           DataDesc dataDesc;
                           dataDesc.m_DataOffset = DataOffset(uiIndex, dataType, true);
                           m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);
                         }
                         else
                         {
                           DataDesc* pDataDesc = nullptr;
                           XII_VERIFY(m_PinIdToDataDesc.TryGetValue(GetDataOutput(dataInput).m_uiId, pDataDesc), "Implementation error");
                           if (pDataDesc == nullptr)
                             return VisitorResult::Error;

                           --pDataDesc->m_uiUsageCounter;
                           if (pDataDesc->m_uiUsageCounter == 0)
                           {
                             freeDataOffsets.PushBack(pDataDesc->m_DataOffset);
                           }

                           // Make a copy first because Insert() might re-allocate and the pointer might point to dead memory afterwards.
                           DataDesc dataDesc = *pDataDesc;
                           m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);
                         }
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

    XII_SUCCEED_OR_RETURN(FillUserData(nodeDesc, astNode.m_pObject));

    for (auto& dataInput : astNode.m_Inputs)
    {
      DataDesc dataDesc;
      XII_VERIFY(m_PinIdToDataDesc.TryGetValue(dataInput.m_uiId, dataDesc), "Implementation error");
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

  return TraverseAst(pEntryAstNode, ConnectionType::Execution,
                     [&](const Connection& connection) {
                       xiiUInt32 uiCurrentIndex = 0;
                       XII_VERIFY(astNodeToNodeDescIndices.TryGetValue(connection.m_pCurrent, uiCurrentIndex), "Implementation error");
                       auto pNodeDesc = &out_NodeDescriptions[uiCurrentIndex];
                       if (pNodeDesc->m_ExecutionIndices.GetCount() == connection.m_pCurrent->m_Next.GetCount())
                       {
                         return VisitorResult::Continue;
                       }

                       for (auto pNextAstNode : connection.m_pCurrent->m_Next)
                       {
                         if (pNextAstNode == nullptr)
                         {
                           pNodeDesc->m_ExecutionIndices.PushBack(xiiInvalidIndex);
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

xiiResult xiiVisualScriptCompiler::TraverseAst(AstNode* pEntryAstNode, xiiUInt32 uiConnectionTypes, AstNodeVisitorFunc func)
{
  m_ReportedConnections.Clear();
  xiiHybridArray<AstNode*, 64> nodeStack;

  if ((uiConnectionTypes & ConnectionType::Execution) != 0)
  {
    Connection connection = {nullptr, pEntryAstNode, ConnectionType::Execution, xiiInvalidIndex};
    auto       res        = func(connection);
    if (res == VisitorResult::Stop)
      return XII_SUCCESS;
    if (res == VisitorResult::Error)
      return XII_FAILURE;
  }

  nodeStack.PushBack(pEntryAstNode);

  while (nodeStack.IsEmpty() == false)
  {
    AstNode* pCurrentAstNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    if ((uiConnectionTypes & ConnectionType::Data) != 0)
    {
      for (xiiUInt32 i = 0; i < pCurrentAstNode->m_Inputs.GetCount(); ++i)
      {
        auto& dataInput = pCurrentAstNode->m_Inputs[i];

        if (dataInput.m_pSourceNode == nullptr)
          continue;

        Connection connection = {pCurrentAstNode, dataInput.m_pSourceNode, ConnectionType::Data, i};
        if (m_ReportedConnections.Insert(connection))
          continue;

        auto res = func(connection);
        if (res == VisitorResult::Stop)
          continue;
        if (res == VisitorResult::Error)
          return XII_FAILURE;

        nodeStack.PushBack(dataInput.m_pSourceNode);
      }
    }

    if ((uiConnectionTypes & ConnectionType::Execution) != 0)
    {
      for (xiiUInt32 i = 0; i < pCurrentAstNode->m_Next.GetCount(); ++i)
      {
        auto pNextAstNode = pCurrentAstNode->m_Next[i];
        XII_ASSERT_DEBUG(pNextAstNode != pCurrentAstNode, "");

        if (pNextAstNode == nullptr)
          continue;

        Connection connection = {pCurrentAstNode, pNextAstNode, ConnectionType::Execution, i};
        if (m_ReportedConnections.Insert(connection))
          continue;

        auto res = func(connection);
        if (res == VisitorResult::Stop)
          continue;
        if (res == VisitorResult::Error)
          return XII_FAILURE;

        nodeStack.PushBack(pNextAstNode);
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptCompiler::FinalizeDataOffsets()
{
  for (auto& function : m_Module.m_Functions)
  {
    for (auto& nodeDesc : function.m_NodeDescriptions)
    {
      for (auto& dataOffset : nodeDesc.m_InputDataOffsets)
      {
        auto dataType = static_cast<xiiVisualScriptDataType::Enum>(dataOffset.m_uiDataType);
        if (dataOffset.m_uiIsConstant)
        {
          dataOffset = m_Module.m_ConstantDataDesc.GetOffset(dataType, dataOffset.m_uiByteOffset, true);
        }
        else
        {
          dataOffset = m_Module.m_VariableDataDesc.GetOffset(dataType, dataOffset.m_uiByteOffset, false);
        }
      }

      for (auto& dataOffset : nodeDesc.m_OutputDataOffsets)
      {
        XII_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Cannot write to constant data");
        auto dataType = static_cast<xiiVisualScriptDataType::Enum>(dataOffset.m_uiDataType);
        dataOffset    = m_Module.m_VariableDataDesc.GetOffset(dataType, dataOffset.m_uiByteOffset, false);
      }
    }
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

    auto dataOffset = m_Module.m_ConstantDataDesc.GetOffset(scriptDataType, uiIndex, true);

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
    TraverseAst(pEntryAstNode, ConnectionType::All,
                [&](const Connection& connection) {
                  xiiUInt32 uiGraphNode = 0;
                  if (nodeCache.TryGetValue(connection.m_pCurrent, uiGraphNode) == false)
                  {
                    const char* szTypeName = xiiVisualScriptNodeDescription::Type::GetName(connection.m_pCurrent->m_Type);
                    float       colorX     = xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(xiiHashingUtils::StringHash(szTypeName))).x();

                    xiiDGMLGraph::NodeDesc nd;
                    nd.m_Color  = xiiColorScheme::LightUI(colorX);
                    uiGraphNode = dgmlGraph.AddNode(szTypeName, &nd);
                    nodeCache.Insert(connection.m_pCurrent, uiGraphNode);
                  }

                  if (connection.m_pPrev != nullptr)
                  {
                    xiiUInt32 uiPrevGraphNode = 0;
                    XII_VERIFY(nodeCache.TryGetValue(connection.m_pPrev, uiPrevGraphNode), "");

                    if (connection.m_Type == ConnectionType::Execution)
                    {
                      dgmlGraph.AddConnection(uiPrevGraphNode, uiGraphNode, "Exec");
                    }
                    else
                    {
                      auto& dataInput  = connection.m_pPrev->m_Inputs[connection.m_uiPrevPinIndex];
                      auto& dataOutput = GetDataOutput(dataInput);

                      xiiStringBuilder sLabel;
                      sLabel.Format("o{}:{} (id: {})->i{}:{} (id: {})", dataOutput.m_uiSourcePinIndex, xiiVisualScriptDataType::GetName(dataOutput.m_DataType), dataOutput.m_uiId, dataInput.m_uiTargetPinIndex, xiiVisualScriptDataType::GetName(dataInput.m_DataType), dataInput.m_uiId);

                      dgmlGraph.AddConnection(uiGraphNode, uiPrevGraphNode, sLabel);
                    }
                  }

                  return VisitorResult::Continue;
                })
      .IgnoreResult();
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
        sTmp.AppendFormat("\n Input {} {}[{}]", dataOffset.m_uiIsConstant ? "Const" : "Var", xiiVisualScriptDataType::GetName(static_cast<xiiVisualScriptDataType::Enum>(dataOffset.m_uiDataType)), dataOffset.m_uiByteOffset);
      }

      for (auto& dataOffset : nodeDesc.m_OutputDataOffsets)
      {
        sTmp.AppendFormat("\n Output {}[{}]", xiiVisualScriptDataType::GetName(static_cast<xiiVisualScriptDataType::Enum>(dataOffset.m_uiDataType)), dataOffset.m_uiByteOffset);
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
