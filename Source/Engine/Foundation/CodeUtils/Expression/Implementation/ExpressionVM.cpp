#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/SimdMath/SimdMath.h>

namespace
{
  //#define DEBUG_VM

#ifdef DEBUG_VM
#  define VM_INLINE
#else
#  define VM_INLINE XII_ALWAYS_INLINE
#endif

  template <typename Func>
  VM_INLINE void VMOperation1(const xiiExpressionByteCode::StorageType*& pByteCode, xiiSimdVec4f* pRegisters, xiiUInt32 uiNumRegisters, Func func)
  {
    xiiSimdVec4f* r  = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    xiiSimdVec4f* re = r + uiNumRegisters;

    xiiSimdVec4f* x = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);

    while (r != re)
    {
      *r = func(*x);
#ifdef DEBUG_VM
      XII_ASSERT_DEV(r->IsValid<4>(), "");
#endif

      ++r;
      ++x;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation1_C(const xiiExpressionByteCode::StorageType*& pByteCode, xiiSimdVec4f* pRegisters, xiiUInt32 uiNumRegisters, Func func)
  {
    xiiSimdVec4f* r  = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    xiiSimdVec4f* re = r + uiNumRegisters;

    xiiSimdVec4f x = xiiExpressionByteCode::GetConstant(pByteCode);

    while (r != re)
    {
      *r = func(x);
#ifdef DEBUG_VM
      XII_ASSERT_DEV(r->IsValid<4>(), "");
#endif

      ++r;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation2(const xiiExpressionByteCode::StorageType*& pByteCode, xiiSimdVec4f* pRegisters, xiiUInt32 uiNumRegisters, Func func)
  {
    xiiSimdVec4f* r  = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    xiiSimdVec4f* re = r + uiNumRegisters;

    xiiSimdVec4f* a = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    xiiSimdVec4f* b = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);

    while (r != re)
    {
      *r = func(*a, *b);
#ifdef DEBUG_VM
      XII_ASSERT_DEV(r->IsValid<4>(), "");
#endif

      ++r;
      ++a;
      ++b;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation2_C(const xiiExpressionByteCode::StorageType*& pByteCode, xiiSimdVec4f* pRegisters, xiiUInt32 uiNumRegisters, Func func)
  {
    xiiSimdVec4f* r  = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    xiiSimdVec4f* re = r + uiNumRegisters;

    xiiSimdVec4f  a = xiiExpressionByteCode::GetConstant(pByteCode);
    xiiSimdVec4f* b = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);

    while (r != re)
    {
      *r = func(a, *b);
#ifdef DEBUG_VM
      XII_ASSERT_DEV(r->IsValid<4>(), "");
#endif

      ++r;
      ++b;
    }
  }

  VM_INLINE float ReadInputData(const xiiUInt8* pData) { return *reinterpret_cast<const float*>(pData); }

  void VMLoadInput(const xiiExpressionByteCode::StorageType*& pByteCode, xiiSimdVec4f* pRegisters, xiiUInt32 uiNumRegisters, xiiArrayPtr<const xiiProcessingStream> inputs, xiiArrayPtr<xiiUInt32> inputMapping)
  {
    xiiSimdVec4f* r  = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    xiiSimdVec4f* re = r + uiNumRegisters;

    xiiUInt32 uiInputIndex        = xiiExpressionByteCode::GetRegisterIndex(pByteCode, 1);
    uiInputIndex                  = inputMapping[uiInputIndex];
    auto&           input         = inputs[uiInputIndex];
    xiiUInt32       uiByteStride  = input.GetElementStride();
    const xiiUInt8* pInputData    = input.GetData<xiiUInt8>();
    const xiiUInt8* pInputDataEnd = pInputData + input.GetDataSize() - uiByteStride;

    while (r != re)
    {
      float x = ReadInputData(pInputData);
      pInputData += pInputData < pInputDataEnd ? uiByteStride : 0;
      float y = ReadInputData(pInputData);
      pInputData += pInputData < pInputDataEnd ? uiByteStride : 0;
      float z = ReadInputData(pInputData);
      pInputData += pInputData < pInputDataEnd ? uiByteStride : 0;
      float w = ReadInputData(pInputData);
      pInputData += pInputData < pInputDataEnd ? uiByteStride : 0;

      r->Set(x, y, z, w);
#ifdef DEBUG_VM
      XII_ASSERT_DEV(r->IsValid<4>(), "");
#endif

      ++r;
    }
  }

  VM_INLINE void StoreOutputData(xiiUInt8* pData, float fData) { *reinterpret_cast<float*>(pData) = fData; }

  void VMStoreOutput(const xiiExpressionByteCode::StorageType*& pByteCode, xiiSimdVec4f* pRegisters, xiiUInt32 uiNumRegisters, xiiArrayPtr<xiiProcessingStream> outputs, xiiArrayPtr<xiiUInt32> outputMapping)
  {
    xiiUInt32 uiOutputIndex  = xiiExpressionByteCode::GetRegisterIndex(pByteCode, 1);
    uiOutputIndex            = outputMapping[uiOutputIndex];
    auto&     output         = outputs[uiOutputIndex];
    xiiUInt32 uiByteStride   = output.GetElementStride();
    xiiUInt8* pOutputData    = output.GetWritableData<xiiUInt8>();
    xiiUInt8* pOutputDataEnd = pOutputData + output.GetDataSize() - uiByteStride;

    xiiSimdVec4f* r  = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    xiiSimdVec4f* re = r + uiNumRegisters;

    while (r != re)
    {
      float data[4];
      r->Store<4>(data);

      StoreOutputData(pOutputData, data[0]);
      pOutputData += pOutputData < pOutputDataEnd ? uiByteStride : 0;
      StoreOutputData(pOutputData, data[1]);
      pOutputData += pOutputData < pOutputDataEnd ? uiByteStride : 0;
      StoreOutputData(pOutputData, data[2]);
      pOutputData += pOutputData < pOutputDataEnd ? uiByteStride : 0;
      StoreOutputData(pOutputData, data[3]);
      pOutputData += pOutputData < pOutputDataEnd ? uiByteStride : 0;

      ++r;
    }
  }

  void VMCall(const xiiExpressionByteCode::StorageType*& pByteCode, xiiSimdVec4f* pRegisters, xiiUInt32 uiNumRegisters, const xiiExpression::GlobalData& globalData, xiiExpressionFunction& func)
  {
    xiiSimdVec4f* r         = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    xiiUInt32     uiNumArgs = xiiExpressionByteCode::GetFunctionArgCount(pByteCode);

    xiiHybridArray<xiiArrayPtr<const xiiSimdVec4f>, 32> inputs;
    inputs.Reserve(uiNumArgs);
    for (xiiUInt32 uiArgIndex = 0; uiArgIndex < uiNumArgs; ++uiArgIndex)
    {
      xiiSimdVec4f* x = pRegisters + xiiExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
      inputs.PushBack(xiiMakeArrayPtr(x, uiNumRegisters));
    }

    xiiExpression::Output output = xiiMakeArrayPtr(r, uiNumRegisters);

    func(inputs, output, globalData);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

xiiExpressionVM::xiiExpressionVM()  = default;
xiiExpressionVM::~xiiExpressionVM() = default;

void xiiExpressionVM::RegisterFunction(
  const char*                     szName,
  xiiExpressionFunction           func,
  xiiExpressionValidateGlobalData validationFunc /*= xiiExpressionValidateGlobalData()*/)
{
  xiiUInt32 uiFunctionIndex = m_Functions.GetCount();

  auto& functionInfo = m_Functions.ExpandAndGetRef();
  functionInfo.m_sName.Assign(szName);
  functionInfo.m_Func           = func;
  functionInfo.m_ValidationFunc = validationFunc;

  m_FunctionNamesToIndex.Insert(functionInfo.m_sName, uiFunctionIndex);
}

void xiiExpressionVM::RegisterDefaultFunctions()
{
  RegisterFunction("Random", &xiiDefaultExpressionFunctions::Random);
  RegisterFunction("PerlinNoise", &xiiDefaultExpressionFunctions::PerlinNoise);
}

xiiResult xiiExpressionVM::Execute(const xiiExpressionByteCode& byteCode, xiiArrayPtr<const xiiProcessingStream> inputs, xiiArrayPtr<xiiProcessingStream> outputs, xiiUInt32 uiNumInstances, const xiiExpression::GlobalData& globalData)
{
  // Input mapping
  {
    auto inputNames = byteCode.GetInputs();

    m_InputMapping.Clear();
    m_InputMapping.Reserve(inputNames.GetCount());

    for (auto& inputName : inputNames)
    {
      bool bInputFound = false;

      for (xiiUInt32 i = 0; i < inputs.GetCount(); ++i)
      {
        if (inputs[i].GetName() == inputName)
        {
          ValidateDataSize(inputs[i], uiNumInstances, "Input");

          m_InputMapping.PushBack(i);
          bInputFound = true;
          break;
        }
      }

      if (!bInputFound)
      {
        xiiLog::Error("Bytecode expects an input '{0}'", inputName);
        return XII_FAILURE;
      }
    }
  }

  // Output mapping
  {
    auto outputNames = byteCode.GetOutputs();

    m_OutputMapping.Clear();
    m_OutputMapping.Reserve(outputNames.GetCount());

    for (auto& outputName : outputNames)
    {
      bool bOutputFound = false;

      for (xiiUInt32 i = 0; i < outputs.GetCount(); ++i)
      {
        if (outputs[i].GetName() == outputName)
        {
          ValidateDataSize(outputs[i], uiNumInstances, "Output");

          m_OutputMapping.PushBack(i);
          bOutputFound = true;
          break;
        }
      }

      if (!bOutputFound)
      {
        xiiLog::Error("Bytecode expects an output '{0}'", outputName);
        return XII_FAILURE;
      }
    }
  }

  // Function mapping and validation
  {
    auto functionNames = byteCode.GetFunctions();

    m_FunctionMapping.Clear();
    m_FunctionMapping.Reserve(functionNames.GetCount());

    for (auto& functionName : functionNames)
    {
      xiiUInt32 uiFunctionIndex = 0;
      if (!m_FunctionNamesToIndex.TryGetValue(functionName, uiFunctionIndex))
      {
        xiiLog::Error("Bytecode expects a function called '{0}' but it was not registered for this VM", functionName);
        return XII_FAILURE;
      }

      m_FunctionMapping.PushBack(uiFunctionIndex);

      auto& validationFunction = m_Functions[uiFunctionIndex].m_ValidationFunc;
      if (validationFunction.IsValid())
      {
        if (validationFunction(globalData).Failed())
        {
          xiiLog::Error("Global data validation for function '{0}' failed.", functionName);
          return XII_FAILURE;
        }
      }
    }
  }

  const xiiUInt32 uiNumRegisters      = (uiNumInstances + 3) / 4;
  const xiiUInt32 uiLastInstanceIndex = uiNumInstances - 1;

  const xiiUInt32 uiTotalNumRegisters = byteCode.GetNumTempRegisters() * uiNumRegisters;
  m_Registers.SetCountUninitialized(uiTotalNumRegisters);

  xiiSimdVec4f* pRegisters = m_Registers.GetData();

  // Execute bytecode
  const xiiExpressionByteCode::StorageType* pByteCode    = byteCode.GetByteCode();
  const xiiExpressionByteCode::StorageType* pByteCodeEnd = byteCode.GetByteCodeEnd();

#ifdef DEBUG_VM
  xiiUInt32 uiInstructionIndex = 0;
#endif

  while (pByteCode < pByteCodeEnd)
  {
    xiiExpressionByteCode::OpCode::Enum opCode = xiiExpressionByteCode::GetOpCode(pByteCode);

#ifdef DEBUG_VM
    xiiLog::Info("{}: {}", uiInstructionIndex, xiiExpressionByteCode::GetOpCodeName(opCode));

    uiInstructionIndex++;
#endif

    switch (opCode)
    {
        // unary
      case xiiExpressionByteCode::OpCode::Abs_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& x) { return x.Abs(); });
        break;

      case xiiExpressionByteCode::OpCode::Sqrt_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& x) { return x.GetSqrt(); });
        break;

      case xiiExpressionByteCode::OpCode::Sin_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& x) { return xiiSimdMath::Sin(x); });
        break;

      case xiiExpressionByteCode::OpCode::Cos_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& x) { return xiiSimdMath::Cos(x); });
        break;

      case xiiExpressionByteCode::OpCode::Tan_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& x) { return xiiSimdMath::Tan(x); });
        break;

      case xiiExpressionByteCode::OpCode::ASin_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& x) { return xiiSimdMath::ASin(x); });
        break;

      case xiiExpressionByteCode::OpCode::ACos_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& x) { return xiiSimdMath::ACos(x); });
        break;

      case xiiExpressionByteCode::OpCode::ATan_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& x) { return xiiSimdMath::ATan(x); });
        break;

      case xiiExpressionByteCode::OpCode::Mov_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& x) { return x; });
        break;

      case xiiExpressionByteCode::OpCode::Mov_C:
        VMOperation1_C(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& x) { return x; });
        break;

      case xiiExpressionByteCode::OpCode::Load:
        VMLoadInput(pByteCode, pRegisters, uiNumRegisters, inputs, m_InputMapping);
        break;

      case xiiExpressionByteCode::OpCode::Store:
        VMStoreOutput(pByteCode, pRegisters, uiNumRegisters, outputs, m_OutputMapping);
        break;

        // binary
      case xiiExpressionByteCode::OpCode::Add_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a + b; });
        break;

      case xiiExpressionByteCode::OpCode::Add_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a + b; });
        break;

      case xiiExpressionByteCode::OpCode::Sub_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a - b; });
        break;

      case xiiExpressionByteCode::OpCode::Sub_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a - b; });
        break;

      case xiiExpressionByteCode::OpCode::Mul_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a.CompMul(b); });
        break;

      case xiiExpressionByteCode::OpCode::Mul_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a.CompMul(b); });
        break;

      case xiiExpressionByteCode::OpCode::Div_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a.CompDiv(b); });
        break;

      case xiiExpressionByteCode::OpCode::Div_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a.CompDiv(b); });
        break;

      case xiiExpressionByteCode::OpCode::Min_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a.CompMin(b); });
        break;

      case xiiExpressionByteCode::OpCode::Min_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a.CompMin(b); });
        break;

      case xiiExpressionByteCode::OpCode::Max_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a.CompMax(b); });
        break;

      case xiiExpressionByteCode::OpCode::Max_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const xiiSimdVec4f& a, const xiiSimdVec4f& b) { return a.CompMax(b); });
        break;

        // call
      case xiiExpressionByteCode::OpCode::Call:
      {
        xiiUInt32 uiFunctionIndex = xiiExpressionByteCode::GetFunctionIndex(pByteCode);
        uiFunctionIndex           = m_FunctionMapping[uiFunctionIndex];
        auto& func                = m_Functions[uiFunctionIndex].m_Func;

        VMCall(pByteCode, pRegisters, uiNumRegisters, globalData, func);
      }
      break;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

void xiiExpressionVM::ValidateDataSize(const xiiProcessingStream& stream, xiiUInt32 uiNumInstances, const char* szDataName) const
{
  XII_ASSERT_DEV(stream.GetDataType() == xiiProcessingStream::DataType::Float, "Only float stream are supported");

  xiiUInt32 uiElementSize  = stream.GetElementSize();
  xiiUInt32 uiExpectedSize = stream.GetElementStride() * (uiNumInstances - 1) + uiElementSize;

  XII_ASSERT_DEV(stream.GetDataSize() >= uiExpectedSize, "{0} data size must be {1} bytes or more. Only {2} bytes given", szDataName, uiExpectedSize, stream.GetDataSize());
}
