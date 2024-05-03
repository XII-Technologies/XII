#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

namespace
{
  static constexpr const char* s_szOpCodeNames[] = {
    "Nop",

    "",

    "AbsF_R",
    "AbsI_R",
    "SqrtF_R",

    "ExpF_R",
    "LnF_R",
    "Log2F_R",
    "Log2I_R",
    "Log10F_R",
    "Pow2F_R",

    "SinF_R",
    "CosF_R",
    "TanF_R",

    "ASinF_R",
    "ACosF_R",
    "ATanF_R",

    "RoundF_R",
    "FloorF_R",
    "CeilF_R",
    "TruncF_R",

    "NotB_R",
    "NotI_R",

    "IToF_R",
    "FToI_R",

    "",
    "",

    "AddF_RR",
    "AddI_RR",

    "SubF_RR",
    "SubI_RR",

    "MulF_RR",
    "MulI_RR",

    "DivF_RR",
    "DivI_RR",

    "MinF_RR",
    "MinI_RR",

    "MaxF_RR",
    "MaxI_RR",

    "ShlI_RR",
    "ShrI_RR",
    "AndI_RR",
    "XorI_RR",
    "OrI_RR",

    "EqF_RR",
    "EqI_RR",
    "EqB_RR",

    "NEqF_RR",
    "NEqI_RR",
    "NEqB_RR",

    "LtF_RR",
    "LtI_RR",

    "LEqF_RR",
    "LEqI_RR",

    "GtF_RR",
    "GtI_RR",

    "GEqF_RR",
    "GEqI_RR",

    "AndB_RR",
    "OrB_RR",

    "",
    "",

    "AddF_RC",
    "AddI_RC",

    "SubF_RC",
    "SubI_RC",

    "MulF_RC",
    "MulI_RC",

    "DivF_RC",
    "DivI_RC",

    "MinF_RC",
    "MinI_RC",

    "MaxF_RC",
    "MaxI_RC",

    "ShlI_RC",
    "ShrI_RC",
    "AndI_RC",
    "XorI_RC",
    "OrI_RC",

    "EqF_RC",
    "EqI_RC",
    "EqB_RC",

    "NEqF_RC",
    "NEqI_RC",
    "NEqB_RC",

    "LtF_RC",
    "LtI_RC",

    "LEqF_RC",
    "LEqI_RC",

    "GtF_RC",
    "GtI_RC",

    "GEqF_RC",
    "GEqI_RC",

    "AndB_RC",
    "OrB_RC",

    "",
    "",

    "SelF_RRR",
    "SelI_RRR",
    "SelB_RRR",

    "",
    "",

    "MovX_R",
    "MovX_C",
    "LoadF",
    "LoadI",
    "StoreF",
    "StoreI",

    "Call",

    "",
  };

  static_assert(XII_ARRAY_SIZE(s_szOpCodeNames) == xiiExpressionByteCode::OpCode::Count);
  static_assert(xiiExpressionByteCode::OpCode::LastBinary - xiiExpressionByteCode::OpCode::FirstBinary == xiiExpressionByteCode::OpCode::LastBinaryWithConstant - xiiExpressionByteCode::OpCode::FirstBinaryWithConstant);


  static constexpr xiiUInt32 GetMaxOpCodeLength()
  {
    xiiUInt32 uiMaxLength = 0;
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(s_szOpCodeNames); ++i)
    {
      uiMaxLength = xiiMath::Max(uiMaxLength, xiiStringUtils::GetStringElementCount(s_szOpCodeNames[i]));
    }
    return uiMaxLength;
  }

  static constexpr xiiUInt32 s_uiMaxOpCodeLength = GetMaxOpCodeLength();

} // namespace

const char* xiiExpressionByteCode::OpCode::GetName(Enum code)
{
  XII_ASSERT_DEBUG(code >= 0 && code < XII_ARRAY_SIZE(s_szOpCodeNames), "Out of bounds access");
  return s_szOpCodeNames[code];
}

//////////////////////////////////////////////////////////////////////////

//clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiExpressionByteCode, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;
//clang-format on

xiiExpressionByteCode::xiiExpressionByteCode() = default;

xiiExpressionByteCode::xiiExpressionByteCode(const xiiExpressionByteCode& other)
{
  *this = other;
}

xiiExpressionByteCode::~xiiExpressionByteCode()
{
  Clear();
}

void xiiExpressionByteCode::operator=(const xiiExpressionByteCode& other)
{
  Clear();
  Init(other.GetByteCode(), other.GetInputs(), other.GetOutputs(), other.GetFunctions(), other.GetNumTempRegisters(), other.GetNumInstructions());
}

bool xiiExpressionByteCode::operator==(const xiiExpressionByteCode& other) const
{
  return GetByteCode() == other.GetByteCode() &&
    GetInputs() == other.GetInputs() &&
    GetOutputs() == other.GetOutputs() &&
    GetFunctions() == other.GetFunctions();
}

void xiiExpressionByteCode::Clear()
{
  xiiMemoryUtils::Destruct(m_pInputs, m_uiNumInputs);
  xiiMemoryUtils::Destruct(m_pOutputs, m_uiNumOutputs);
  xiiMemoryUtils::Destruct(m_pFunctions, m_uiNumFunctions);

  m_pInputs    = nullptr;
  m_pOutputs   = nullptr;
  m_pFunctions = nullptr;
  m_pByteCode  = nullptr;

  m_uiByteCodeCount = 0;
  m_uiNumInputs     = 0;
  m_uiNumOutputs    = 0;
  m_uiNumFunctions  = 0;

  m_uiNumTempRegisters = 0;
  m_uiNumInstructions  = 0;

  m_Data.Clear();
}

void xiiExpressionByteCode::Disassemble(xiiStringBuilder& out_sDisassembly) const
{
  out_sDisassembly.Append("// Inputs:\n");
  for (xiiUInt32 i = 0; i < m_uiNumInputs; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {}({})\n", i, m_pInputs[i].m_sName, xiiProcessingStream::GetDataTypeName(m_pInputs[i].m_DataType));
  }

  out_sDisassembly.Append("\n// Outputs:\n");
  for (xiiUInt32 i = 0; i < m_uiNumOutputs; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {}({})\n", i, m_pOutputs[i].m_sName, xiiProcessingStream::GetDataTypeName(m_pOutputs[i].m_DataType));
  }

  out_sDisassembly.Append("\n// Functions:\n");
  for (xiiUInt32 i = 0; i < m_uiNumFunctions; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {} {}(", i, xiiExpression::RegisterType::GetName(m_pFunctions[i].m_OutputType), m_pFunctions[i].m_sName);
    const xiiUInt32 uiNumArguments = m_pFunctions[i].m_InputTypes.GetCount();
    for (xiiUInt32 j = 0; j < uiNumArguments; ++j)
    {
      out_sDisassembly.Append(xiiExpression::RegisterType::GetName(m_pFunctions[i].m_InputTypes[j]));
      if (j < uiNumArguments - 1)
      {
        out_sDisassembly.Append(", ");
      }
    }
    out_sDisassembly.Append(")\n");
  }

  out_sDisassembly.AppendFormat("\n// Temp Registers: {}\n", GetNumTempRegisters());
  out_sDisassembly.AppendFormat("// Instructions: {}\n\n", GetNumInstructions());

  auto AppendConstant = [](xiiUInt32 x, xiiStringBuilder& out_sString) {
    out_sString.AppendFormat("0x{}({})", xiiArgU(x, 8, true, 16), xiiArgF(*reinterpret_cast<float*>(&x), 6));
  };

  const StorageType* pByteCode    = GetByteCodeStart();
  const StorageType* pByteCodeEnd = GetByteCodeEnd();

  while (pByteCode < pByteCodeEnd)
  {
    OpCode::Enum opCode = GetOpCode(pByteCode);
    {
      const char* szOpCode       = OpCode::GetName(opCode);
      xiiUInt32   uiOpCodeLength = xiiStringUtils::GetStringElementCount(szOpCode);

      out_sDisassembly.Append(szOpCode);
      for (xiiUInt32 i = uiOpCodeLength; i < s_uiMaxOpCodeLength + 1; ++i)
      {
        out_sDisassembly.Append(" ");
      }
    }

    if (opCode > OpCode::FirstUnary && opCode < OpCode::LastUnary)
    {
      xiiUInt32 r = GetRegisterIndex(pByteCode);
      xiiUInt32 x = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{}\n", r, x);
    }
    else if (opCode > OpCode::FirstBinary && opCode < OpCode::LastBinary)
    {
      xiiUInt32 r = GetRegisterIndex(pByteCode);
      xiiUInt32 a = GetRegisterIndex(pByteCode);
      xiiUInt32 b = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} r{}\n", r, a, b);
    }
    else if (opCode > OpCode::FirstBinaryWithConstant && opCode < OpCode::LastBinaryWithConstant)
    {
      xiiUInt32 r = GetRegisterIndex(pByteCode);
      xiiUInt32 a = GetRegisterIndex(pByteCode);
      xiiUInt32 b = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} ", r, a);
      AppendConstant(b, out_sDisassembly);
      out_sDisassembly.Append("\n");
    }
    else if (opCode > OpCode::FirstTernary && opCode < OpCode::LastTernary)
    {
      xiiUInt32 r = GetRegisterIndex(pByteCode);
      xiiUInt32 a = GetRegisterIndex(pByteCode);
      xiiUInt32 b = GetRegisterIndex(pByteCode);
      xiiUInt32 c = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} r{} r{}\n", r, a, b, c);
    }
    else if (opCode == OpCode::MovX_C)
    {
      xiiUInt32 r = GetRegisterIndex(pByteCode);
      xiiUInt32 x = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} ", r);
      AppendConstant(x, out_sDisassembly);
      out_sDisassembly.Append("\n");
    }
    else if (opCode == OpCode::LoadF || opCode == OpCode::LoadI)
    {
      xiiUInt32 r = GetRegisterIndex(pByteCode);
      xiiUInt32 i = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} i{}({})\n", r, i, m_pInputs[i].m_sName);
    }
    else if (opCode == OpCode::StoreF || opCode == OpCode::StoreI)
    {
      xiiUInt32 o = GetRegisterIndex(pByteCode);
      xiiUInt32 r = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("o{}({}) r{}\n", o, m_pOutputs[o].m_sName, r);
    }
    else if (opCode == OpCode::Call)
    {
      xiiUInt32   uiIndex = GetFunctionIndex(pByteCode);
      const char* szName  = m_pFunctions[uiIndex].m_sName;

      xiiStringBuilder sName;
      if (xiiStringUtils::IsNullOrEmpty(szName))
      {
        sName.SetFormat("Unknown_{0}", uiIndex);
      }
      else
      {
        sName = szName;
      }

      xiiUInt32 r = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("{1} r{2}", sName, r);

      xiiUInt32 uiNumArgs = GetFunctionArgCount(pByteCode);
      for (xiiUInt32 uiArgIndex = 0; uiArgIndex < uiNumArgs; ++uiArgIndex)
      {
        xiiUInt32 x = GetRegisterIndex(pByteCode);
        out_sDisassembly.AppendFormat(" r{0}", x);
      }

      out_sDisassembly.Append("\n");
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

static constexpr xiiTypeVersion s_uiByteCodeVersion = 6;

xiiResult xiiExpressionByteCode::Save(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiByteCodeVersion);

  xiiUInt32 uiDataSize = static_cast<xiiUInt32>(m_Data.GetByteBlobPtr().GetCount());

  inout_stream << uiDataSize;

  inout_stream << m_uiNumInputs;
  for (auto& input : GetInputs())
  {
    XII_SUCCEED_OR_RETURN(input.Serialize(inout_stream));
  }

  inout_stream << m_uiNumOutputs;
  for (auto& output : GetOutputs())
  {
    XII_SUCCEED_OR_RETURN(output.Serialize(inout_stream));
  }

  inout_stream << m_uiNumFunctions;
  for (auto& function : GetFunctions())
  {
    XII_SUCCEED_OR_RETURN(function.Serialize(inout_stream));
  }

  inout_stream << m_uiByteCodeCount;
  XII_SUCCEED_OR_RETURN(inout_stream.WriteBytes(m_pByteCode, m_uiByteCodeCount * sizeof(StorageType)));

  inout_stream << m_uiNumTempRegisters;
  inout_stream << m_uiNumInstructions;

  return XII_SUCCESS;
}

xiiResult xiiExpressionByteCode::Load(xiiStreamReader& inout_stream, xiiByteArrayPtr externalMemory /*= xiiByteArrayPtr()*/)
{
  xiiTypeVersion version = inout_stream.ReadVersion(s_uiByteCodeVersion);
  if (version != s_uiByteCodeVersion)
  {
    xiiLog::Error("Invalid expression byte code version {}. Expected {}", version, s_uiByteCodeVersion);
    return XII_FAILURE;
  }

  xiiUInt32 uiDataSize = 0;
  inout_stream >> uiDataSize;

  void* pData = nullptr;
  if (externalMemory.IsEmpty())
  {
    m_Data.SetCountUninitialized(uiDataSize);
    m_Data.ZeroFill();
    pData = m_Data.GetByteBlobPtr().GetPtr();
  }
  else
  {
    if (externalMemory.GetCount() < uiDataSize)
    {
      xiiLog::Error("External memory is too small. Expected at least {} bytes but got {} bytes.", uiDataSize, externalMemory.GetCount());
      return XII_FAILURE;
    }

    if (xiiMemoryUtils::IsAligned(externalMemory.GetPtr(), XII_ALIGNMENT_OF(xiiExpression::StreamDesc)) == false)
    {
      xiiLog::Error("External memory is not properly aligned. Expected an alignment of at least {} bytes.", XII_ALIGNMENT_OF(xiiExpression::StreamDesc));
      return XII_FAILURE;
    }

    pData = externalMemory.GetPtr();
  }

  // Inputs
  {
    inout_stream >> m_uiNumInputs;
    m_pInputs = static_cast<xiiExpression::StreamDesc*>(pData);
    for (xiiUInt32 i = 0; i < m_uiNumInputs; ++i)
    {
      XII_SUCCEED_OR_RETURN(m_pInputs[i].Deserialize(inout_stream));
    }

    pData = xiiMemoryUtils::AddByteOffset(pData, GetInputs().ToByteArray().GetCount());
  }

  // Outputs
  {
    inout_stream >> m_uiNumOutputs;
    m_pOutputs = static_cast<xiiExpression::StreamDesc*>(pData);
    for (xiiUInt32 i = 0; i < m_uiNumOutputs; ++i)
    {
      XII_SUCCEED_OR_RETURN(m_pOutputs[i].Deserialize(inout_stream));
    }

    pData = xiiMemoryUtils::AddByteOffset(pData, GetOutputs().ToByteArray().GetCount());
  }

  // Functions
  {
    pData = xiiMemoryUtils::AlignForwards(pData, XII_ALIGNMENT_OF(xiiExpression::FunctionDesc));

    inout_stream >> m_uiNumFunctions;
    m_pFunctions = static_cast<xiiExpression::FunctionDesc*>(pData);
    for (xiiUInt32 i = 0; i < m_uiNumFunctions; ++i)
    {
      XII_SUCCEED_OR_RETURN(m_pFunctions[i].Deserialize(inout_stream));
    }

    pData = xiiMemoryUtils::AddByteOffset(pData, GetFunctions().ToByteArray().GetCount());
  }

  // ByteCode
  {
    pData = xiiMemoryUtils::AlignForwards(pData, XII_ALIGNMENT_OF(StorageType));

    inout_stream >> m_uiByteCodeCount;
    m_pByteCode = static_cast<StorageType*>(pData);
    inout_stream.ReadBytes(m_pByteCode, m_uiByteCodeCount * sizeof(StorageType));
  }

  inout_stream >> m_uiNumTempRegisters;
  inout_stream >> m_uiNumInstructions;

  return XII_SUCCESS;
}

void xiiExpressionByteCode::Init(xiiArrayPtr<const StorageType> byteCode, xiiArrayPtr<const xiiExpression::StreamDesc> inputs, xiiArrayPtr<const xiiExpression::StreamDesc> outputs, xiiArrayPtr<const xiiExpression::FunctionDesc> functions, xiiUInt32 uiNumTempRegisters, xiiUInt32 uiNumInstructions)
{
  xiiUInt32 uiOutputsOffset   = 0;
  xiiUInt32 uiFunctionsOffset = 0;
  xiiUInt32 uiByteCodeOffset  = 0;

  xiiUInt32 uiDataSize = 0;
  uiDataSize += inputs.ToByteArray().GetCount();
  uiOutputsOffset = uiDataSize;
  uiDataSize += outputs.ToByteArray().GetCount();

  uiDataSize        = xiiMemoryUtils::AlignSize<xiiUInt32>(uiDataSize, XII_ALIGNMENT_OF(xiiExpression::FunctionDesc));
  uiFunctionsOffset = uiDataSize;
  uiDataSize += functions.ToByteArray().GetCount();

  uiDataSize       = xiiMemoryUtils::AlignSize<xiiUInt32>(uiDataSize, XII_ALIGNMENT_OF(StorageType));
  uiByteCodeOffset = uiDataSize;
  uiDataSize += byteCode.ToByteArray().GetCount();

  m_Data.SetCountUninitialized(uiDataSize);
  m_Data.ZeroFill();

  void* pData = m_Data.GetByteBlobPtr().GetPtr();

  XII_ASSERT_DEV(inputs.GetCount() < xiiSmallInvalidIndex, "Too many inputs");
  m_pInputs     = static_cast<xiiExpression::StreamDesc*>(pData);
  m_uiNumInputs = static_cast<xiiUInt16>(inputs.GetCount());
  xiiMemoryUtils::Copy(m_pInputs, inputs.GetPtr(), m_uiNumInputs);

  XII_ASSERT_DEV(outputs.GetCount() < xiiSmallInvalidIndex, "Too many outputs");
  m_pOutputs     = static_cast<xiiExpression::StreamDesc*>(xiiMemoryUtils::AddByteOffset(pData, uiOutputsOffset));
  m_uiNumOutputs = static_cast<xiiUInt16>(outputs.GetCount());
  xiiMemoryUtils::Copy(m_pOutputs, outputs.GetPtr(), m_uiNumOutputs);

  XII_ASSERT_DEV(functions.GetCount() < xiiSmallInvalidIndex, "Too many functions");
  m_pFunctions     = static_cast<xiiExpression::FunctionDesc*>(xiiMemoryUtils::AddByteOffset(pData, uiFunctionsOffset));
  m_uiNumFunctions = static_cast<xiiUInt16>(functions.GetCount());
  xiiMemoryUtils::Copy(m_pFunctions, functions.GetPtr(), m_uiNumFunctions);

  m_pByteCode       = static_cast<StorageType*>(xiiMemoryUtils::AddByteOffset(pData, uiByteCodeOffset));
  m_uiByteCodeCount = byteCode.GetCount();
  xiiMemoryUtils::Copy(m_pByteCode, byteCode.GetPtr(), m_uiByteCodeCount);

  XII_ASSERT_DEV(uiNumTempRegisters < xiiSmallInvalidIndex, "Too many temp registers");
  m_uiNumTempRegisters = static_cast<xiiUInt16>(uiNumTempRegisters);
  m_uiNumInstructions  = uiNumInstructions;
}

XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionByteCode);
