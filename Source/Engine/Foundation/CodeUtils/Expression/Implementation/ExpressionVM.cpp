/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/CodeUtils/Expression/Implementation/ExpressionVMOperations.h>
#include <Foundation/Logging/Log.h>

xiiExpressionVM::xiiExpressionVM()
{
  RegisterDefaultFunctions();
}
xiiExpressionVM::~xiiExpressionVM() = default;

void xiiExpressionVM::RegisterFunction(const xiiExpressionFunction& func)
{
  XII_ASSERT_DEV(func.m_Desc.m_uiNumRequiredInputs <= func.m_Desc.m_InputTypes.GetCount(), "Not enough input types defined. {} inputs are required but only {} types given.", func.m_Desc.m_uiNumRequiredInputs, func.m_Desc.m_InputTypes.GetCount());

  xiiUInt32 uiFunctionIndex = m_Functions.GetCount();
  m_FunctionNamesToIndex.Insert(func.m_Desc.GetMangledName(), uiFunctionIndex);

  m_Functions.PushBack(func);
}

void xiiExpressionVM::UnregisterFunction(const xiiExpressionFunction& func)
{
  xiiUInt32 uiFunctionIndex = 0;
  if (m_FunctionNamesToIndex.Remove(func.m_Desc.GetMangledName(), &uiFunctionIndex))
  {
    m_Functions.RemoveAtAndSwap(uiFunctionIndex);
    if (uiFunctionIndex != m_Functions.GetCount())
    {
      m_FunctionNamesToIndex[m_Functions[uiFunctionIndex].m_Desc.GetMangledName()] = uiFunctionIndex;
    }
  }
}

xiiResult xiiExpressionVM::Execute(const xiiExpressionByteCode& byteCode, xiiArrayPtr<const xiiProcessingStream> inputs, xiiArrayPtr<xiiProcessingStream> outputs, xiiUInt32 uiNumInstances, const xiiExpression::GlobalData& globalData, xiiBitflags<Flags> flags)
{
  if (flags.IsSet(Flags::ScalarizeStreams))
  {
    XII_SUCCEED_OR_RETURN(ScalarizeStreams(inputs, m_ScalarizedInputs));
    XII_SUCCEED_OR_RETURN(ScalarizeStreams(outputs, m_ScalarizedOutputs));

    inputs  = m_ScalarizedInputs;
    outputs = m_ScalarizedOutputs;
  }
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  else
  {
    AreStreamsScalarized(inputs).AssertSuccess("Input streams are not scalarized");
    AreStreamsScalarized(outputs).AssertSuccess("Output streams are not scalarized");
  }
#endif

  XII_SUCCEED_OR_RETURN(MapStreams(byteCode.GetInputs(), inputs, "Input", uiNumInstances, flags, m_MappedInputs));
  XII_SUCCEED_OR_RETURN(MapStreams(byteCode.GetOutputs(), outputs, "Output", uiNumInstances, flags, m_MappedOutputs));

  XII_SUCCEED_OR_RETURN(MapFunctions(byteCode.GetFunctions(), globalData));

  const xiiUInt32 uiTotalNumRegisters = byteCode.GetNumTempRegisters() * ((uiNumInstances + 3) / 4);
  m_Registers.SetCountUninitialized(uiTotalNumRegisters);

  // Execute bytecode
  const xiiExpressionByteCode::StorageType* pByteCode    = byteCode.GetByteCodeStart();
  const xiiExpressionByteCode::StorageType* pByteCodeEnd = byteCode.GetByteCodeEnd();

  ExecutionContext context;
  context.m_pRegisters          = m_Registers.GetData();
  context.m_uiNumInstances      = uiNumInstances;
  context.m_uiNumSimd4Instances = (uiNumInstances + 3) / 4;
  context.m_Inputs              = m_MappedInputs;
  context.m_Outputs             = m_MappedOutputs;
  context.m_Functions           = m_MappedFunctions;
  context.m_pGlobalData         = &globalData;

  while (pByteCode < pByteCodeEnd)
  {
    xiiExpressionByteCode::OpCode::Enum opCode = xiiExpressionByteCode::GetOpCode(pByteCode);

    OpFunc func = s_Simd4Funcs[opCode];
    if (func != nullptr)
    {
      func(pByteCode, context);
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
      xiiLog::Error("Unknown OpCode '{}'. Execution aborted.", opCode);
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

void xiiExpressionVM::RegisterDefaultFunctions()
{
  RegisterFunction(xiiDefaultExpressionFunctions::s_RandomFunc);
  RegisterFunction(xiiDefaultExpressionFunctions::s_PerlinNoiseFunc);
}

xiiResult xiiExpressionVM::ScalarizeStreams(xiiArrayPtr<const xiiProcessingStream> streams, xiiDynamicArray<xiiProcessingStream>& out_ScalarizedStreams)
{
  out_ScalarizedStreams.Clear();

  for (auto& stream : streams)
  {
    const xiiUInt32 uiNumElements = xiiExpressionAST::DataType::GetElementCount(xiiExpressionAST::DataType::FromStreamType(stream.GetDataType()));
    if (uiNumElements == 1)
    {
      out_ScalarizedStreams.PushBack(stream);
    }
    else
    {
      xiiStringBuilder sNewName;
      xiiHashedString  sNewNameHashed;
      auto             data            = xiiMakeArrayPtr((xiiUInt8*)(stream.GetData()), static_cast<xiiUInt32>(stream.GetDataSize()));
      auto             elementDataType = static_cast<xiiProcessingStream::DataType>((xiiUInt32)stream.GetDataType() & ~3u);

      for (xiiUInt32 i = 0; i < uiNumElements; ++i)
      {
        sNewName.Set(stream.GetName(), ".", xiiExpressionAST::VectorComponent::GetName(static_cast<xiiExpressionAST::VectorComponent::Enum>(i)));
        sNewNameHashed.Assign(sNewName);

        auto newData = data.GetSubArray(i * xiiProcessingStream::GetDataTypeSize(elementDataType));

        out_ScalarizedStreams.PushBack(xiiProcessingStream(sNewNameHashed, newData, elementDataType, stream.GetElementStride()));
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiExpressionVM::AreStreamsScalarized(xiiArrayPtr<const xiiProcessingStream> streams)
{
  for (auto& stream : streams)
  {
    const xiiUInt32 uiNumElements = xiiExpressionAST::DataType::GetElementCount(xiiExpressionAST::DataType::FromStreamType(stream.GetDataType()));
    if (uiNumElements > 1)
    {
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}


xiiResult xiiExpressionVM::ValidateStream(const xiiProcessingStream& stream, const xiiExpression::StreamDesc& streamDesc, xiiStringView sStreamType, xiiUInt32 uiNumInstances)
{
  // verify stream data type
  if (stream.GetDataType() != streamDesc.m_DataType)
  {
    xiiLog::Error("{} stream '{}' expects data of type '{}' or a compatible type. Given type '{}' is not compatible.", sStreamType, streamDesc.m_sName, xiiProcessingStream::GetDataTypeName(streamDesc.m_DataType), xiiProcessingStream::GetDataTypeName(stream.GetDataType()));
    return XII_FAILURE;
  }

  // verify stream size
  xiiUInt32 uiElementSize  = stream.GetElementSize();
  xiiUInt32 uiExpectedSize = stream.GetElementStride() * (uiNumInstances - 1) + uiElementSize;

  if (stream.GetDataSize() < uiExpectedSize)
  {
    xiiLog::Error("{} stream '{}' data size must be {} bytes or more. Only {} bytes given", sStreamType, streamDesc.m_sName, uiExpectedSize, stream.GetDataSize());
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiExpressionVM::MapStreams(xiiArrayPtr<const xiiExpression::StreamDesc> streamDescs, xiiArrayPtr<T> streams, xiiStringView sStreamType, xiiUInt32 uiNumInstances, xiiBitflags<Flags> flags, xiiDynamicArray<T*>& out_MappedStreams)
{
  out_MappedStreams.Clear();
  out_MappedStreams.Reserve(streamDescs.GetCount());

  if (flags.IsSet(Flags::MapStreamsByName))
  {
    for (auto& streamDesc : streamDescs)
    {
      bool bFound = false;

      for (xiiUInt32 i = 0; i < streams.GetCount(); ++i)
      {
        auto& stream = streams[i];
        if (stream.GetName() == streamDesc.m_sName)
        {
          XII_SUCCEED_OR_RETURN(ValidateStream(stream, streamDesc, sStreamType, uiNumInstances));

          out_MappedStreams.PushBack(&stream);
          bFound = true;
          break;
        }
      }

      if (!bFound)
      {
        xiiLog::Error("Bytecode expects an {} stream '{}'", sStreamType, streamDesc.m_sName);
        return XII_FAILURE;
      }
    }
  }
  else
  {
    if (streams.GetCount() != streamDescs.GetCount())
      return XII_FAILURE;

    for (xiiUInt32 i = 0; i < streams.GetCount(); ++i)
    {
      auto& stream = streams.GetPtr()[i];

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      auto& streamDesc = streamDescs.GetPtr()[i];
      XII_SUCCEED_OR_RETURN(ValidateStream(stream, streamDesc, sStreamType, uiNumInstances));
#endif

      out_MappedStreams.PushBack(&stream);
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiExpressionVM::MapFunctions(xiiArrayPtr<const xiiExpression::FunctionDesc> functionDescs, const xiiExpression::GlobalData& globalData)
{
  m_MappedFunctions.Clear();
  m_MappedFunctions.Reserve(functionDescs.GetCount());

  for (auto& functionDesc : functionDescs)
  {
    xiiUInt32 uiFunctionIndex = 0;
    if (!m_FunctionNamesToIndex.TryGetValue(functionDesc.m_sName, uiFunctionIndex))
    {
      xiiLog::Error("Bytecode expects a function called '{0}' but it was not registered for this VM", functionDesc.m_sName);
      return XII_FAILURE;
    }

    auto& registeredFunction = m_Functions[uiFunctionIndex];

    // verify signature
    if (functionDesc.m_InputTypes != registeredFunction.m_Desc.m_InputTypes || functionDesc.m_OutputType != registeredFunction.m_Desc.m_OutputType)
    {
      xiiLog::Error("Signature for registered function '{}' does not match the expected signature from bytecode", functionDesc.m_sName);
      return XII_FAILURE;
    }

    if (registeredFunction.m_ValidateGlobalDataFunc != nullptr)
    {
      if (registeredFunction.m_ValidateGlobalDataFunc(globalData).Failed())
      {
        xiiLog::Error("Global data validation for function '{0}' failed.", functionDesc.m_sName);
        return XII_FAILURE;
      }
    }

    m_MappedFunctions.PushBack(&registeredFunction);
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionVM);
