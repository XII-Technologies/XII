#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

xiiVisualScriptGraphDescription::ExecuteFunction GetExecuteFunction(xiiVisualScriptNodeDescription::Type::Enum nodeType, xiiVisualScriptDataType::Enum dataType);

namespace
{
  static const char* s_NodeDescTypeNames[] = {
    "", // Invalid,
    "EntryCall",
    "EntryCall_Coroutine",
    "MessageHandler",
    "MessageHandler_Coroutine",
    "ReflectedFunction",
    "GetReflectedProperty",
    "SetReflectedProperty",
    "InplaceCoroutine",
    "GetScriptOwner",
    "SendMessage",

    "", // FirstBuiltin,

    "Builtin_Constant",
    "Builtin_GetVariable",
    "Builtin_SetVariable",
    "Builtin_IncVariable",
    "Builtin_DecVariable",
    "Builtin_TempVariable",

    "Builtin_Branch",
    "Builtin_Switch",
    "Builtin_WhileLoop",
    "Builtin_ForLoop",
    "Builtin_ForEachLoop",
    "Builtin_ReverseForEachLoop",
    "Builtin_Break",
    "Builtin_Jump",

    "Builtin_And",
    "Builtin_Or",
    "Builtin_Not",
    "Builtin_Compare",
    "Builtin_CompareExec",
    "Builtin_IsValid",
    "Builtin_Select",

    "Builtin_Add",
    "Builtin_Subtract",
    "Builtin_Multiply",
    "Builtin_Divide",
    "Builtin_Expression",

    "Builtin_ToBool",
    "Builtin_ToByte",
    "Builtin_ToInt",
    "Builtin_ToInt64",
    "Builtin_ToFloat",
    "Builtin_ToDouble",
    "Builtin_ToString",
    "Builtin_String_Format",
    "Builtin_ToHashedString",
    "Builtin_ToVariant",
    "Builtin_Variant_ConvertTo",

    "Builtin_MakeArray",
    "Builtin_Array_GetElement",
    "Builtin_Array_SetElement",
    "Builtin_Array_GetCount",
    "Builtin_Array_IsEmpty",
    "Builtin_Array_Clear",
    "Builtin_Array_Contains",
    "Builtin_Array_IndexOf",
    "Builtin_Array_Insert",
    "Builtin_Array_PushBack",
    "Builtin_Array_PushBackRange",
    "Builtin_Array_Remove",
    "Builtin_Array_RemoveAt",

    "Builtin_TryGetComponentOfBaseType",

    "Builtin_StartCoroutine",
    "Builtin_StopCoroutine",
    "Builtin_StopAllCoroutines",
    "Builtin_WaitForAll",
    "Builtin_WaitForAny",
    "Builtin_Yield",

    "", // LastBuiltin,
  };
  static_assert(XII_ARRAY_SIZE(s_NodeDescTypeNames) == (size_t)xiiVisualScriptNodeDescription::Type::Count);

  template <typename T>
  xiiResult WriteNodeArray(xiiArrayPtr<T> a, xiiStreamWriter& inout_stream)
  {
    xiiUInt16 uiCount = static_cast<xiiUInt16>(a.GetCount());
    inout_stream << uiCount;

    return inout_stream.WriteBytes(a.GetPtr(), a.GetCount() * sizeof(T));
  }

} // namespace

// static
xiiVisualScriptNodeDescription::Type::Enum xiiVisualScriptNodeDescription::Type::GetConversionType(xiiVisualScriptDataType::Enum targetDataType)
{
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Bool - xiiVisualScriptDataType::Bool) == Builtin_ToBool);
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Byte - xiiVisualScriptDataType::Bool) == Builtin_ToByte);
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Int - xiiVisualScriptDataType::Bool) == Builtin_ToInt);
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Int64 - xiiVisualScriptDataType::Bool) == Builtin_ToInt64);
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Float - xiiVisualScriptDataType::Bool) == Builtin_ToFloat);
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Double - xiiVisualScriptDataType::Bool) == Builtin_ToDouble);

  if (xiiVisualScriptDataType::IsNumberOrBool(targetDataType))
    return static_cast<Enum>(Builtin_ToBool + (targetDataType - xiiVisualScriptDataType::Bool));

  if (targetDataType == xiiVisualScriptDataType::String)
    return Builtin_ToString;

  if (targetDataType == xiiVisualScriptDataType::HashedString)
    return Builtin_ToHashedString;

  if (targetDataType == xiiVisualScriptDataType::Variant)
    return Builtin_ToVariant;

  XII_ASSERT_NOT_IMPLEMENTED;
  return Invalid;
}

// static
const char* xiiVisualScriptNodeDescription::Type::GetName(Enum type)
{
  XII_ASSERT_DEBUG(type >= 0 && static_cast<xiiUInt32>(type) < XII_ARRAY_SIZE(s_NodeDescTypeNames), "Out of bounds access");
  return s_NodeDescTypeNames[type];
}

void xiiVisualScriptNodeDescription::AppendUserDataName(xiiStringBuilder& out_sResult) const
{
  if (auto func = GetUserDataContext(m_Type).m_ToStringFunc)
  {
    out_sResult.Append(" ");

    func(*this, out_sResult);
  }
}

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptGraphDescription::xiiVisualScriptGraphDescription()
{
  static_assert(sizeof(Node) == 64);
}

xiiVisualScriptGraphDescription::~xiiVisualScriptGraphDescription() = default;

static const xiiTypeVersion s_uiVisualScriptGraphDescriptionVersion = 7;

// static
xiiResult xiiVisualScriptGraphDescription::Serialize(xiiArrayPtr<const xiiVisualScriptNodeDescription> nodes, const xiiVisualScriptDataDescription& localDataDesc, xiiStreamWriter& inout_stream)
{
  inout_stream.WriteVersion(s_uiVisualScriptGraphDescriptionVersion);

  XII_SUCCEED_OR_RETURN(localDataDesc.Serialize(inout_stream));

  xiiDefaultMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter         stream(&streamStorage);
  xiiUInt32                     additionalDataSize = 0;
  {
    for (auto& nodeDesc : nodes)
    {
      stream << nodeDesc.m_Type;
      stream << nodeDesc.m_DeductedDataType;
      XII_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_ExecutionIndices.GetArrayPtr(), stream));
      XII_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_InputDataOffsets.GetArrayPtr(), stream));
      XII_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_OutputDataOffsets.GetArrayPtr(), stream));

      ExecutionIndicesArray::AddAdditionalDataSize(nodeDesc.m_ExecutionIndices, additionalDataSize);
      InputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_InputDataOffsets, additionalDataSize);
      OutputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_OutputDataOffsets, additionalDataSize);

      if (auto func = GetUserDataContext(nodeDesc.m_Type).m_SerializeFunc)
      {
        xiiUInt32 uiSize      = 0;
        xiiUInt32 uiAlignment = 0;
        XII_SUCCEED_OR_RETURN(func(nodeDesc, stream, uiSize, uiAlignment));

        UserDataArray::AddAdditionalDataSize(uiSize, uiAlignment, additionalDataSize);
      }
    }
  }

  const xiiUInt32 uiRequiredStorageSize = nodes.GetCount() * sizeof(Node) + additionalDataSize;
  inout_stream << uiRequiredStorageSize;
  inout_stream << nodes.GetCount();

  XII_SUCCEED_OR_RETURN(streamStorage.CopyToStream(inout_stream));

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptGraphDescription::Deserialize(xiiStreamReader& inout_stream, const xiiVisualScriptDataDescription& instanceDataDesc, const xiiVisualScriptDataDescription& constantDataDesc)
{
  xiiTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptGraphDescriptionVersion);
  if (uiVersion < s_uiVisualScriptGraphDescriptionVersion)
  {
    xiiLog::Error("Invalid visual script desc version. Expected >= {} but got {}. Visual Script needs re-export", s_uiVisualScriptGraphDescriptionVersion, uiVersion);
    return XII_FAILURE;
  }

  {
    xiiSharedPtr<xiiVisualScriptDataDescription> pLocalDataDesc = XII_SCRIPT_NEW(xiiVisualScriptDataDescription);
    XII_SUCCEED_OR_RETURN(pLocalDataDesc->Deserialize(inout_stream));
    m_pLocalDataDesc = std::move(pLocalDataDesc);
  }

  {
    xiiUInt32 uiStorageSize;
    inout_stream >> uiStorageSize;

    m_Storage.SetCountUninitialized(uiStorageSize);
    m_Storage.ZeroFill();
  }

  xiiUInt32 uiNumNodes;
  inout_stream >> uiNumNodes;

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();
  auto nodes = xiiMakeArrayPtr(reinterpret_cast<Node*>(pData), uiNumNodes);

  xiiUInt8* pAdditionalData = pData + uiNumNodes * sizeof(Node);

  auto GetDataDesc = [&](DataOffset dataOffset) -> const xiiVisualScriptDataDescription* {
    switch (dataOffset.GetSource())
    {
      case DataOffset::Source::Local:
        return m_pLocalDataDesc.Borrow();
      case DataOffset::Source::Instance:
        return &instanceDataDesc;
      case DataOffset::Source::Constant:
        return &constantDataDesc;
        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return nullptr;
  };

  auto CalculateDataOffsets = [&](DataOffset* pDataOffsets, xiiUInt32 uiNumDataOffsets) {
    DataOffset* pDataOffsetsEnd = pDataOffsets + uiNumDataOffsets;
    while (pDataOffsets < pDataOffsetsEnd)
    {
      auto& dataOffset = *pDataOffsets;
      dataOffset       = GetDataDesc(dataOffset)->GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
      ++pDataOffsets;
    }
  };

  for (auto& node : nodes)
  {
    inout_stream >> node.m_Type;
    inout_stream >> node.m_DeductedDataType;

    node.m_Function = GetExecuteFunction(node.m_Type, node.m_DeductedDataType);

    XII_SUCCEED_OR_RETURN(node.m_ExecutionIndices.ReadFromStream(node.m_NumExecutionIndices, inout_stream, pAdditionalData));
    XII_SUCCEED_OR_RETURN(node.m_InputDataOffsets.ReadFromStream(node.m_NumInputDataOffsets, inout_stream, pAdditionalData));
    XII_SUCCEED_OR_RETURN(node.m_OutputDataOffsets.ReadFromStream(node.m_NumOutputDataOffsets, inout_stream, pAdditionalData));

    CalculateDataOffsets(node.GetInputDataOffsets(), node.m_NumInputDataOffsets);
    CalculateDataOffsets(node.GetOutputDataOffsets(), node.m_NumOutputDataOffsets);

    if (auto func = GetUserDataContext(node.m_Type).m_DeserializeFunc)
    {
      XII_SUCCEED_OR_RETURN(func(node, inout_stream, pAdditionalData));
    }
  }

  m_Nodes = nodes;

  return XII_SUCCESS;
}

xiiScriptMessageDesc xiiVisualScriptGraphDescription::GetMessageDesc() const
{
  auto pEntryNode = GetNode(0);
  XII_ASSERT_DEBUG(pEntryNode != nullptr && (pEntryNode->m_Type == xiiVisualScriptNodeDescription::Type::MessageHandler || pEntryNode->m_Type == xiiVisualScriptNodeDescription::Type::MessageHandler_Coroutine || pEntryNode->m_Type == xiiVisualScriptNodeDescription::Type::SendMessage), "Entry node is invalid or not a message handler");

  auto& userData = pEntryNode->GetUserData<NodeUserData_TypeAndProperties>();

  xiiScriptMessageDesc desc;
  desc.m_pType      = userData.m_pType;
  desc.m_Properties = xiiMakeArrayPtr(userData.m_Properties, userData.m_uiNumProperties);
  return desc;
}

//////////////////////////////////////////////////////////////////////////

xiiCVarInt cvar_MaxNodeExecutions("VisualScript.MaxNodeExecutions", 100000, xiiCVarFlags::Default, "The maximum number of nodes executed within a script invocation");

xiiVisualScriptExecutionContext::xiiVisualScriptExecutionContext(const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc, xiiAllocator* pAllocator) :
  m_pDesc(pDesc), m_LocalDataStorage(pDesc->GetLocalDataDesc())
{
  m_LocalDataStorage.AllocateStorage(pAllocator);
  m_DataStorage[DataOffset::Source::Local] = &m_LocalDataStorage;
}

xiiVisualScriptExecutionContext::~xiiVisualScriptExecutionContext()
{
  Deinitialize();
}

void xiiVisualScriptExecutionContext::Initialize(xiiVisualScriptInstance& inout_instance, xiiArrayPtr<xiiVariant> arguments)
{
  m_pInstance = &inout_instance;

  m_DataStorage[DataOffset::Source::Instance] = inout_instance.GetInstanceDataStorage();
  m_DataStorage[DataOffset::Source::Constant] = inout_instance.GetConstantDataStorage();

  auto pNode = m_pDesc->GetNode(0);
  XII_ASSERT_DEV(xiiVisualScriptNodeDescription::Type::IsEntry(pNode->m_Type), "Invalid entry node");

  for (xiiUInt32 i = 0; i < arguments.GetCount(); ++i)
  {
    SetDataFromVariant(pNode->GetOutputDataOffset(i), arguments[i]);
  }

  m_uiCurrentNode = pNode->GetExecutionIndex(0);
}

void xiiVisualScriptExecutionContext::Deinitialize()
{
  // 0x1 is a marker value to indicate that we are in a yield
  if (m_pCurrentCoroutine > reinterpret_cast<xiiScriptCoroutine*>(0x1))
  {
    auto pModule = m_pInstance->GetWorld()->GetOrCreateModule<xiiScriptWorldModule>();
    pModule->StopAndDeleteCoroutine(m_pCurrentCoroutine->GetHandle());
    m_pCurrentCoroutine = nullptr;
  }
}

xiiVisualScriptExecutionContext::ExecResult xiiVisualScriptExecutionContext::Execute(xiiTime deltaTimeSinceLastExecution)
{
  XII_ASSERT_DEV(m_pInstance != nullptr, "Invalid instance");
  ++m_uiExecutionCounter;
  m_DeltaTimeSinceLastExecution = deltaTimeSinceLastExecution;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiUInt32 uiCounter = 0;
#endif

  auto pNode = m_pDesc->GetNode(m_uiCurrentNode);
  while (pNode != nullptr)
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (pNode->m_Function == nullptr)
    {
      xiiLog::Error("Node '{}' is not supported by runtime and should have been removed by the compiler.", xiiVisualScriptNodeDescription::Type::GetName(pNode->m_Type));
      return ExecResult::Error();
    }
#endif

    ExecResult result = pNode->m_Function(*this, *pNode);
    if (result.m_NextExecAndState < ExecResult::State::Completed)
    {
      return result;
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    ++uiCounter;
    if (uiCounter >= xiiUInt32(cvar_MaxNodeExecutions))
    {
      xiiLog::Error("Maximum node executions ({}) reached, execution will be aborted. Does the script contain an infinite loop?", cvar_MaxNodeExecutions);
      return ExecResult::Error();
    }
#endif

    m_uiCurrentNode     = pNode->GetExecutionIndex(result.m_NextExecAndState);
    m_pCurrentCoroutine = nullptr;

    pNode = m_pDesc->GetNode(m_uiCurrentNode);
  }

  return ExecResult::RunNext(0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVisualScriptSendMessageMode, 1)
  XII_ENUM_CONSTANTS(xiiVisualScriptSendMessageMode::Direct, xiiVisualScriptSendMessageMode::Recursive, xiiVisualScriptSendMessageMode::Event)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

XII_STATICLINK_FILE(VisualScriptPlugin, VisualScriptPlugin_Runtime_VisualScript);
