#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Strings/HashedString.h>

class xiiStreamWriter;
class xiiStreamReader;

class XII_FOUNDATION_DLL xiiExpressionByteCode
{
public:
  struct OpCode
  {
    enum Enum
    {
      // Unary
      FirstUnary,

      Abs_R,
      Sqrt_R,

      Sin_R,
      Cos_R,
      Tan_R,

      ASin_R,
      ACos_R,
      ATan_R,

      Mov_R,
      Mov_C,
      Load,
      Store,

      LastUnary,

      // Binary
      FirstBinary,

      Add_RR,
      Add_CR,

      Sub_RR,
      Sub_CR,

      Mul_RR,
      Mul_CR,

      Div_RR,
      Div_CR,

      Min_RR,
      Min_CR,

      Max_RR,
      Max_CR,

      LastBinary,

      Call,

      Nop,

      Count
    };
  };

  typedef xiiUInt32 StorageType;

  xiiExpressionByteCode();
  ~xiiExpressionByteCode();

  bool operator==(const xiiExpressionByteCode& other) const;
  bool operator!=(const xiiExpressionByteCode& other) const { return !(*this == other); }

  void Clear();
  bool IsEmpty() const { return m_ByteCode.IsEmpty(); }

  const StorageType* GetByteCode() const;
  const StorageType* GetByteCodeEnd() const;

  xiiUInt32                          GetNumInstructions() const;
  xiiUInt32                          GetNumTempRegisters() const;
  xiiArrayPtr<const xiiHashedString> GetInputs() const;
  xiiArrayPtr<const xiiHashedString> GetOutputs() const;
  xiiArrayPtr<const xiiHashedString> GetFunctions() const;

  static OpCode::Enum GetOpCode(const StorageType*& pByteCode);
  static xiiUInt32    GetRegisterIndex(const StorageType*& pByteCode, xiiUInt32 uiNumRegisters);
  static xiiSimdVec4f GetConstant(const StorageType*& pByteCode);
  static xiiUInt32    GetFunctionIndex(const StorageType*& pByteCode);
  static xiiUInt32    GetFunctionArgCount(const StorageType*& pByteCode);

  void               Disassemble(xiiStringBuilder& out_sDisassembly) const;
  static const char* GetOpCodeName(OpCode::Enum opCode);

  void      Save(xiiStreamWriter& stream) const;
  xiiResult Load(xiiStreamReader& stream);

private:
  friend class xiiExpressionCompiler;

  xiiDynamicArray<StorageType>     m_ByteCode;
  xiiDynamicArray<xiiHashedString> m_Inputs;
  xiiDynamicArray<xiiHashedString> m_Outputs;
  xiiDynamicArray<xiiHashedString> m_Functions;

  xiiUInt32 m_uiNumInstructions  = 0;
  xiiUInt32 m_uiNumTempRegisters = 0;
};

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionByteCode_inl.h>
