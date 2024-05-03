#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Containers/Blob.h>

class xiiStreamWriter;
class xiiStreamReader;

class XII_FOUNDATION_DLL xiiExpressionByteCode
{
public:
  struct OpCode
  {
    enum Enum
    {
      Nop,

      FirstUnary,

      AbsF_R,
      AbsI_R,
      SqrtF_R,

      ExpF_R,
      LnF_R,
      Log2F_R,
      Log2I_R,
      Log10F_R,
      Pow2F_R,

      SinF_R,
      CosF_R,
      TanF_R,

      ASinF_R,
      ACosF_R,
      ATanF_R,

      RoundF_R,
      FloorF_R,
      CeilF_R,
      TruncF_R,

      NotI_R,
      NotB_R,

      IToF_R,
      FToI_R,

      LastUnary,

      FirstBinary,

      AddF_RR,
      AddI_RR,

      SubF_RR,
      SubI_RR,

      MulF_RR,
      MulI_RR,

      DivF_RR,
      DivI_RR,

      MinF_RR,
      MinI_RR,

      MaxF_RR,
      MaxI_RR,

      ShlI_RR,
      ShrI_RR,
      AndI_RR,
      XorI_RR,
      OrI_RR,

      EqF_RR,
      EqI_RR,
      EqB_RR,

      NEqF_RR,
      NEqI_RR,
      NEqB_RR,

      LtF_RR,
      LtI_RR,

      LEqF_RR,
      LEqI_RR,

      GtF_RR,
      GtI_RR,

      GEqF_RR,
      GEqI_RR,

      AndB_RR,
      OrB_RR,

      LastBinary,

      FirstBinaryWithConstant,

      AddF_RC,
      AddI_RC,

      SubF_RC,
      SubI_RC,

      MulF_RC,
      MulI_RC,

      DivF_RC,
      DivI_RC,

      MinF_RC,
      MinI_RC,

      MaxF_RC,
      MaxI_RC,

      ShlI_RC,
      ShrI_RC,
      AndI_RC,
      XorI_RC,
      OrI_RC,

      EqF_RC,
      EqI_RC,
      EqB_RC,

      NEqF_RC,
      NEqI_RC,
      NEqB_RC,

      LtF_RC,
      LtI_RC,

      LEqF_RC,
      LEqI_RC,

      GtF_RC,
      GtI_RC,

      GEqF_RC,
      GEqI_RC,

      AndB_RC,
      OrB_RC,

      LastBinaryWithConstant,

      FirstTernary,

      SelF_RRR,
      SelI_RRR,
      SelB_RRR,

      LastTernary,

      FirstSpecial,

      MovX_R,
      MovX_C,
      LoadF,
      LoadI,
      StoreF,
      StoreI,

      Call,

      LastSpecial,

      Count
    };

    static const char* GetName(Enum code);
  };

  using StorageType = xiiUInt32;

  xiiExpressionByteCode();
  xiiExpressionByteCode(const xiiExpressionByteCode& other);
  ~xiiExpressionByteCode();

  void operator=(const xiiExpressionByteCode& other);

  bool operator==(const xiiExpressionByteCode& other) const;
  bool operator!=(const xiiExpressionByteCode& other) const { return !(*this == other); }

  void Clear();
  bool IsEmpty() const { return m_uiByteCodeCount == 0; }

  const StorageType*             GetByteCodeStart() const;
  const StorageType*             GetByteCodeEnd() const;
  xiiArrayPtr<const StorageType> GetByteCode() const;

  xiiUInt32                                      GetNumInstructions() const;
  xiiUInt32                                      GetNumTempRegisters() const;
  xiiArrayPtr<const xiiExpression::StreamDesc>   GetInputs() const;
  xiiArrayPtr<const xiiExpression::StreamDesc>   GetOutputs() const;
  xiiArrayPtr<const xiiExpression::FunctionDesc> GetFunctions() const;

  static OpCode::Enum            GetOpCode(const StorageType*& ref_pByteCode);
  static xiiUInt32               GetRegisterIndex(const StorageType*& ref_pByteCode);
  static xiiExpression::Register GetConstant(const StorageType*& ref_pByteCode);
  static xiiUInt32               GetFunctionIndex(const StorageType*& ref_pByteCode);
  static xiiUInt32               GetFunctionArgCount(const StorageType*& ref_pByteCode);

  void Disassemble(xiiStringBuilder& out_sDisassembly) const;

  xiiResult Save(xiiStreamWriter& inout_stream) const;
  xiiResult Load(xiiStreamReader& inout_stream, xiiByteArrayPtr externalMemory = xiiByteArrayPtr());

  xiiConstByteBlobPtr GetDataBlob() const { return m_Data.GetByteBlobPtr(); }

private:
  friend class xiiExpressionCompiler;

  void Init(xiiArrayPtr<const StorageType> byteCode, xiiArrayPtr<const xiiExpression::StreamDesc> inputs, xiiArrayPtr<const xiiExpression::StreamDesc> outputs, xiiArrayPtr<const xiiExpression::FunctionDesc> functions, xiiUInt32 uiNumTempRegisters, xiiUInt32 uiNumInstructions);

  xiiBlob m_Data;

  xiiExpression::StreamDesc*   m_pInputs    = nullptr;
  xiiExpression::StreamDesc*   m_pOutputs   = nullptr;
  xiiExpression::FunctionDesc* m_pFunctions = nullptr;
  StorageType*                 m_pByteCode  = nullptr;

  xiiUInt32 m_uiByteCodeCount = 0;
  xiiUInt16 m_uiNumInputs     = 0;
  xiiUInt16 m_uiNumOutputs    = 0;
  xiiUInt16 m_uiNumFunctions  = 0;

  xiiUInt16 m_uiNumTempRegisters = 0;
  xiiUInt32 m_uiNumInstructions  = 0;
};

#if XII_ENABLED(XII_PLATFORM_64BIT)
static_assert(sizeof(xiiExpressionByteCode) == 64);
#endif

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiExpressionByteCode);
XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiExpressionByteCode);

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionByteCode_inl.h>
