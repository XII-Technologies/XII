/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  static xiiUInt32 s_uiNumASTDumps = 0;

  void MakeASTOutputPath(xiiStringView sOutputName, xiiStringBuilder& out_sOutputPath)
  {
    xiiUInt32 uiCounter = s_uiNumASTDumps;
    ++s_uiNumASTDumps;

    out_sOutputPath.SetFormat(":output/Expression/{}_{}_AST.dgml", xiiArgU(uiCounter, 2, true), sOutputName);
  }

  void DumpDisassembly(const xiiExpressionByteCode& byteCode, xiiStringView sOutputName, xiiUInt32 uiCounter)
  {
    xiiStringBuilder sDisassembly;
    byteCode.Disassemble(sDisassembly);

    xiiStringBuilder sFileName;
    sFileName.SetFormat(":output/Expression/{}_{}_ByteCode.txt", xiiArgU(uiCounter, 2, true), sOutputName);

    xiiFileWriter fileWriter;
    if (fileWriter.Open(sFileName).Succeeded())
    {
      fileWriter.WriteBytes(sDisassembly.GetData(), sDisassembly.GetElementCount()).IgnoreResult();

      xiiLog::Error("Disassembly was dumped to: {}", sFileName);
    }
    else
    {
      xiiLog::Error("Failed to dump Disassembly to: {}", sFileName);
    }
  }

  static xiiUInt32 s_uiNumByteCodeComparisons = 0;

  bool CompareByteCode(const xiiExpressionByteCode& testCode, const xiiExpressionByteCode& referenceCode)
  {
    xiiUInt32 uiCounter = s_uiNumByteCodeComparisons;
    ++s_uiNumByteCodeComparisons;

    if (testCode != referenceCode)
    {
      DumpDisassembly(referenceCode, "Reference", uiCounter);
      DumpDisassembly(testCode, "Test", uiCounter);
      return false;
    }

    return true;
  }

  static xiiHashedString s_sA      = xiiMakeHashedString("a");
  static xiiHashedString s_sB      = xiiMakeHashedString("b");
  static xiiHashedString s_sC      = xiiMakeHashedString("c");
  static xiiHashedString s_sD      = xiiMakeHashedString("d");
  static xiiHashedString s_sOutput = xiiMakeHashedString("output");

  static xiiUniquePtr<xiiExpressionParser>   s_pParser;
  static xiiUniquePtr<xiiExpressionCompiler> s_pCompiler;
  static xiiUniquePtr<xiiExpressionVM>       s_pVM;

  template <typename T>
  struct StreamDataTypeDeduction
  {
  };

  template <>
  struct StreamDataTypeDeduction<xiiFloat16>
  {
    static constexpr xiiProcessingStream::DataType Type = xiiProcessingStream::DataType::Half;
    static xiiFloat16                              Default() { return xiiMath::MinValue<float>(); }
  };

  template <>
  struct StreamDataTypeDeduction<float>
  {
    static constexpr xiiProcessingStream::DataType Type = xiiProcessingStream::DataType::Float;
    static float                                   Default() { return xiiMath::MinValue<float>(); }
  };

  template <>
  struct StreamDataTypeDeduction<xiiInt8>
  {
    static constexpr xiiProcessingStream::DataType Type = xiiProcessingStream::DataType::Byte;
    static xiiInt8                                 Default() { return xiiMath::MinValue<xiiInt8>(); }
  };

  template <>
  struct StreamDataTypeDeduction<xiiInt16>
  {
    static constexpr xiiProcessingStream::DataType Type = xiiProcessingStream::DataType::Short;
    static xiiInt16                                Default() { return xiiMath::MinValue<xiiInt16>(); }
  };

  template <>
  struct StreamDataTypeDeduction<xiiInt32>
  {
    static constexpr xiiProcessingStream::DataType Type = xiiProcessingStream::DataType::Int;
    static xiiInt32                                Default() { return xiiMath::MinValue<xiiInt32>(); }
  };

  template <>
  struct StreamDataTypeDeduction<xiiVec3>
  {
    static constexpr xiiProcessingStream::DataType Type = xiiProcessingStream::DataType::Float3;
    static xiiVec3                                 Default() { return xiiVec3(xiiMath::MinValue<float>()); }
  };

  template <>
  struct StreamDataTypeDeduction<xiiVec3I32>
  {
    static constexpr xiiProcessingStream::DataType Type = xiiProcessingStream::DataType::Int3;
    static xiiVec3I32                              Default() { return xiiVec3I32(xiiMath::MinValue<xiiInt32>()); }
  };

  template <typename T>
  void Compile(xiiStringView sCode, xiiExpressionByteCode& out_byteCode, xiiStringView sDumpAstOutputName = xiiStringView())
  {
    xiiExpression::StreamDesc inputs[] = {
      {s_sA, StreamDataTypeDeduction<T>::Type},
      {s_sB, StreamDataTypeDeduction<T>::Type},
      {s_sC, StreamDataTypeDeduction<T>::Type},
      {s_sD, StreamDataTypeDeduction<T>::Type},
    };

    xiiExpression::StreamDesc outputs[] = {
      {s_sOutput, StreamDataTypeDeduction<T>::Type},
    };

    xiiExpressionAST ast;
    XII_TEST_BOOL(s_pParser->Parse(sCode, inputs, outputs, {}, ast).Succeeded());

    xiiStringBuilder sOutputPath;
    if (sDumpAstOutputName.IsEmpty() == false)
    {
      MakeASTOutputPath(sDumpAstOutputName, sOutputPath);
    }
    XII_TEST_BOOL(s_pCompiler->Compile(ast, out_byteCode, sOutputPath).Succeeded());
  }

  template <typename T>
  T Execute(const xiiExpressionByteCode& byteCode, T a = T(0), T b = T(0), T c = T(0), T d = T(0))
  {
    xiiProcessingStream inputs[] = {
      xiiProcessingStream(s_sA, xiiMakeArrayPtr(&a, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      xiiProcessingStream(s_sB, xiiMakeArrayPtr(&b, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      xiiProcessingStream(s_sC, xiiMakeArrayPtr(&c, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      xiiProcessingStream(s_sD, xiiMakeArrayPtr(&d, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
    };

    T                   output    = StreamDataTypeDeduction<T>::Default();
    xiiProcessingStream outputs[] = {
      xiiProcessingStream(s_sOutput, xiiMakeArrayPtr(&output, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
    };

    XII_TEST_BOOL(s_pVM->Execute(byteCode, inputs, outputs, 1).Succeeded());

    return output;
  };

  template <typename T>
  T TestInstruction(xiiStringView sCode, T a = T(0), T b = T(0), T c = T(0), T d = T(0), bool bDumpASTs = false)
  {
    xiiExpressionByteCode byteCode;
    Compile<T>(sCode, byteCode, bDumpASTs ? "TestInstruction" : "");
    return Execute<T>(byteCode, a, b, c, d);
  }

  template <typename T>
  T TestConstant(xiiStringView sCode, bool bDumpASTs = false)
  {
    xiiExpressionByteCode byteCode;
    Compile<T>(sCode, byteCode, bDumpASTs ? "TestConstant" : "");
    XII_TEST_INT(byteCode.GetNumInstructions(), 2); // MovX_C, StoreX
    XII_TEST_INT(byteCode.GetNumTempRegisters(), 1);
    return Execute<T>(byteCode);
  }

  enum TestBinaryFlags
  {
    LeftConstantOptimization = XII_BIT(0),
    NoInstructionsCountCheck = XII_BIT(2),
  };

  template <typename R, typename T, xiiUInt32 flags>
  void TestBinaryInstruction(xiiStringView sOp, T a, T b, T expectedResult, bool bDumpASTs = false)
  {
    constexpr bool boolInputs = std::is_same<T, bool>::value;
    using U                   = typename std::conditional<boolInputs, xiiInt32, T>::type;

    U aAsU;
    U bAsU;
    U expectedResultAsU;
    if constexpr (boolInputs)
    {
      aAsU              = a ? 1 : 0;
      bAsU              = b ? 1 : 0;
      expectedResultAsU = expectedResult ? 1 : 0;
    }
    else
    {
      aAsU              = a;
      bAsU              = b;
      expectedResultAsU = expectedResult;
    }

    auto TestRes = [](U res, U expectedRes, const char* szCode, const char* szAValue, const char* szBValue) {
      if constexpr (std::is_same<T, float>::value)
      {
        XII_TEST_FLOAT_MSG(res, expectedRes, xiiMath::DefaultEpsilon<float>(), "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, xiiInt32>::value)
      {
        XII_TEST_INT_MSG(res, expectedRes, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, bool>::value)
      {
        const char* szRes         = (res != 0) ? "true" : "false";
        const char* szExpectedRes = (expectedRes != 0) ? "true" : "false";
        XII_TEST_STRING_MSG(szRes, szExpectedRes, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, xiiVec3>::value)
      {
        XII_TEST_VEC3_MSG(res, expectedRes, xiiMath::DefaultEpsilon<float>(), "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, xiiVec3I32>::value)
      {
        XII_TEST_INT_MSG(res.x, expectedRes.x, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
        XII_TEST_INT_MSG(res.y, expectedRes.y, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
        XII_TEST_INT_MSG(res.z, expectedRes.z, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
    };

    const bool  functionStyleSyntax = sOp.FindSubString("(");
    const char* formatString        = functionStyleSyntax ? "output = {0}{1}, {2})" : "output = {1} {0} {2}";
    const char* aInput              = boolInputs ? "(a != 0)" : "a";
    const char* bInput              = boolInputs ? "(b != 0)" : "b";

    xiiStringBuilder aValue;
    xiiStringBuilder bValue;
    if constexpr (std::is_same<T, xiiVec3>::value || std::is_same<T, xiiVec3I32>::value)
    {
      aValue.SetFormat("vec3({}, {}, {})", a.x, a.y, a.z);
      bValue.SetFormat("vec3({}, {}, {})", b.x, b.y, b.z);
    }
    else
    {
      aValue.SetFormat("{}", a);
      bValue.SetFormat("{}", b);
    }

    xiiInt32 oneConstantInstructions = 3; // LoadX, OpX_RC, StoreX
    xiiInt32 oneConstantRegisters    = 1;
    if constexpr (std::is_same<R, bool>::value)
    {
      oneConstantInstructions += 3; // + MovX_C, MovX_C, SelI_RRR
      oneConstantRegisters += 2;    // Two more registers needed for constants above
    }
    if constexpr (boolInputs)
    {
      oneConstantInstructions += 1; // + NotEqI_RC
    }

    xiiInt32 numOutputElements          = 1;
    bool     hasDifferentOutputElements = false;
    if constexpr (std::is_same<T, xiiVec3>::value || std::is_same<T, xiiVec3I32>::value)
    {
      numOutputElements = 3;

      for (xiiInt32 i = 1; i < 3; ++i)
      {
        if (expectedResult.GetData()[i] != expectedResult.GetData()[i - 1])
        {
          hasDifferentOutputElements = true;
          break;
        }
      }
    }

    xiiStringBuilder      code;
    xiiExpressionByteCode byteCode;

    code.SetFormat(formatString, sOp, aInput, bInput);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryNoConstants" : "");
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.SetFormat(formatString, sOp, aValue, bInput);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryLeftConstant" : "");
    if constexpr ((flags & NoInstructionsCountCheck) == 0)
    {
      xiiInt32 leftConstantInstructions = oneConstantInstructions;
      xiiInt32 leftConstantRegisters    = oneConstantRegisters;
      if constexpr ((flags & LeftConstantOptimization) == 0)
      {
        leftConstantInstructions += 1;
        leftConstantRegisters += 1;
      }

      if (byteCode.GetNumInstructions() != leftConstantInstructions || byteCode.GetNumTempRegisters() != leftConstantRegisters)
      {
        DumpDisassembly(byteCode, "BinaryLeftConstant", 0);
        XII_TEST_INT(byteCode.GetNumInstructions(), leftConstantInstructions);
        XII_TEST_INT(byteCode.GetNumTempRegisters(), leftConstantRegisters);
      }
    }
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.SetFormat(formatString, sOp, aInput, bValue);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryRightConstant" : "");
    if constexpr ((flags & NoInstructionsCountCheck) == 0)
    {
      if (byteCode.GetNumInstructions() != oneConstantInstructions || byteCode.GetNumTempRegisters() != oneConstantRegisters)
      {
        DumpDisassembly(byteCode, "BinaryRightConstant", 0);
        XII_TEST_INT(byteCode.GetNumInstructions(), oneConstantInstructions);
        XII_TEST_INT(byteCode.GetNumTempRegisters(), oneConstantRegisters);
      }
    }
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.SetFormat(formatString, sOp, aValue, bValue);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryConstant" : "");
    if (hasDifferentOutputElements == false)
    {
      xiiInt32 bothConstantsInstructions = 1 + numOutputElements; // MovX_C + StoreX * numOutputElements
      xiiInt32 bothConstantsRegisters    = 1;
      if (byteCode.GetNumInstructions() != bothConstantsInstructions || byteCode.GetNumTempRegisters() != bothConstantsRegisters)
      {
        DumpDisassembly(byteCode, "BinaryConstant", 0);
        XII_TEST_INT(byteCode.GetNumInstructions(), bothConstantsInstructions);
        XII_TEST_INT(byteCode.GetNumTempRegisters(), bothConstantsRegisters);
      }
    }
    TestRes(Execute<U>(byteCode), expectedResultAsU, code, aValue, bValue);
  }

  template <typename T>
  bool CompareCode(xiiStringView sTestCode, xiiStringView sReferenceCode, xiiExpressionByteCode& out_testByteCode, bool bDumpASTs = false)
  {
    Compile<T>(sTestCode, out_testByteCode, bDumpASTs ? "Test" : "");

    xiiExpressionByteCode referenceByteCode;
    Compile<T>(sReferenceCode, referenceByteCode, bDumpASTs ? "Reference" : "");

    return CompareByteCode(out_testByteCode, referenceByteCode);
  }

  template <typename T>
  void TestInputOutput()
  {
    xiiStringView         testCode = "output = a + b * 2";
    xiiExpressionByteCode testByteCode;
    Compile<T>(testCode, testByteCode);

    constexpr xiiUInt32            uiCount = 17;
    xiiHybridArray<T, uiCount>     a;
    xiiHybridArray<T, uiCount>     b;
    xiiHybridArray<T, uiCount>     o;
    xiiHybridArray<float, uiCount> expectedOutput;
    a.SetCountUninitialized(uiCount);
    b.SetCountUninitialized(uiCount);
    o.SetCount(uiCount);
    expectedOutput.SetCountUninitialized(uiCount);

    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      a[i]              = static_cast<T>(3.0f * i);
      b[i]              = static_cast<T>(1.5f * i);
      expectedOutput[i] = a[i] + b[i] * 2.0f;
    }

    xiiProcessingStream inputs[] = {
      xiiProcessingStream(s_sA, a.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
      xiiProcessingStream(s_sB, b.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
      xiiProcessingStream(s_sC, a.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type), // Dummy stream, not actually used
      xiiProcessingStream(s_sD, a.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type), // Dummy stream, not actually used
    };

    xiiProcessingStream outputs[] = {
      xiiProcessingStream(s_sOutput, o.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
    };

    XII_TEST_BOOL(s_pVM->Execute(testByteCode, inputs, outputs, uiCount, xiiExpression::GlobalData(), xiiExpressionVM::Flags::BestPerformance).Succeeded());

    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      XII_TEST_FLOAT(static_cast<float>(o[i]), expectedOutput[i], xiiMath::DefaultEpsilon<float>());
    }
  }

  static const xiiEnum<xiiExpression::RegisterType> s_TestFunc1InputTypes[] = {xiiExpression::RegisterType::Float, xiiExpression::RegisterType::Int};
  static const xiiEnum<xiiExpression::RegisterType> s_TestFunc2InputTypes[] = {xiiExpression::RegisterType::Float, xiiExpression::RegisterType::Float, xiiExpression::RegisterType::Int};

  static void TestFunc1(xiiExpression::Inputs inputs, xiiExpression::Output output, const xiiExpression::GlobalData& globalData)
  {
    const xiiExpression::Register* pX      = inputs[0].GetPtr();
    const xiiExpression::Register* pY      = inputs[1].GetPtr();
    const xiiExpression::Register* pXEnd   = inputs[0].GetEndPtr();
    xiiExpression::Register*       pOutput = output.GetPtr();

    while (pX < pXEnd)
    {
      pOutput->f = pX->f.CompMul(pY->i.ToFloat());

      ++pX;
      ++pY;
      ++pOutput;
    }
  }

  static void TestFunc2(xiiExpression::Inputs inputs, xiiExpression::Output output, const xiiExpression::GlobalData& globalData)
  {
    const xiiExpression::Register* pX      = inputs[0].GetPtr();
    const xiiExpression::Register* pY      = inputs[1].GetPtr();
    const xiiExpression::Register* pXEnd   = inputs[0].GetEndPtr();
    xiiExpression::Register*       pOutput = output.GetPtr();

    if (inputs.GetCount() >= 3)
    {
      const xiiExpression::Register* pZ = inputs[2].GetPtr();

      while (pX < pXEnd)
      {
        pOutput->f = pX->f.CompMul(pY->f) * 2.0f + pZ->i.ToFloat();

        ++pX;
        ++pY;
        ++pZ;
        ++pOutput;
      }
    }
    else
    {
      while (pX < pXEnd)
      {
        pOutput->f = pX->f.CompMul(pY->f) * 2.0f;

        ++pX;
        ++pY;
        ++pOutput;
      }
    }
  }

  xiiExpressionFunction s_TestFunc1 = {
    {xiiMakeHashedString("TestFunc"), xiiExpression::FunctionDesc::TypeList(s_TestFunc1InputTypes), 2, xiiExpression::RegisterType::Float},
    &TestFunc1,
  };

  xiiExpressionFunction s_TestFunc2 = {
    {xiiMakeHashedString("TestFunc"), xiiExpression::FunctionDesc::TypeList(s_TestFunc2InputTypes), 3, xiiExpression::RegisterType::Float},
    &TestFunc2,
  };

} // namespace

XII_CREATE_SIMPLE_TEST(CodeUtils, Expression)
{
  s_uiNumByteCodeComparisons = 0;

  xiiStringBuilder outputPath = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);

  s_pParser   = XII_DEFAULT_NEW(xiiExpressionParser);
  s_pCompiler = XII_DEFAULT_NEW(xiiExpressionCompiler);
  s_pVM       = XII_DEFAULT_NEW(xiiExpressionVM);
  XII_SCOPE_EXIT(s_pParser = nullptr; s_pCompiler = nullptr; s_pVM = nullptr;);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Unary instructions")
  {
    // Negate
    XII_TEST_INT(TestInstruction("output = -a", 2), -2);
    XII_TEST_FLOAT(TestInstruction("output = -a", 2.5f), -2.5f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_INT(TestConstant<xiiInt32>("output = -2"), -2);
    XII_TEST_FLOAT(TestConstant<float>("output = -2.5"), -2.5f, xiiMath::DefaultEpsilon<float>());

    // Absolute
    XII_TEST_INT(TestInstruction("output = abs(a)", -2), 2);
    XII_TEST_FLOAT(TestInstruction("output = abs(a)", -2.5f), 2.5f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_INT(TestConstant<xiiInt32>("output = abs(-2)"), 2);
    XII_TEST_FLOAT(TestConstant<float>("output = abs(-2.5)"), 2.5f, xiiMath::DefaultEpsilon<float>());

    // Saturate
    XII_TEST_INT(TestInstruction("output = saturate(a)", -1), 0);
    XII_TEST_INT(TestInstruction("output = saturate(a)", 2), 1);
    XII_TEST_FLOAT(TestInstruction("output = saturate(a)", -1.5f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = saturate(a)", 2.5f), 1.0f, xiiMath::DefaultEpsilon<float>());

    XII_TEST_INT(TestConstant<xiiInt32>("output = saturate(-1)"), 0);
    XII_TEST_INT(TestConstant<xiiInt32>("output = saturate(2)"), 1);
    XII_TEST_FLOAT(TestConstant<float>("output = saturate(-1.5)"), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = saturate(2.5)"), 1.0f, xiiMath::DefaultEpsilon<float>());

    // Sqrt
    XII_TEST_FLOAT(TestInstruction("output = sqrt(a)", 25.0f), 5.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = sqrt(a)", 2.0f), xiiMath::Sqrt(2.0f), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = sqrt(25)"), 5.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = sqrt(2)"), xiiMath::Sqrt(2.0f), xiiMath::DefaultEpsilon<float>());

    // Exp
    XII_TEST_FLOAT(TestInstruction("output = exp(a)", 0.0f), 1.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = exp(a)", 2.0f), xiiMath::Exp(2.0f), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = exp(0.0)"), 1.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = exp(2.0)"), xiiMath::Exp(2.0f), xiiMath::DefaultEpsilon<float>());

    // Ln
    XII_TEST_FLOAT(TestInstruction("output = ln(a)", 1.0f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = ln(a)", 2.0f), xiiMath::Ln(2.0f), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = ln(1.0)"), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = ln(2.0)"), xiiMath::Ln(2.0f), xiiMath::DefaultEpsilon<float>());

    // Log2
    XII_TEST_INT(TestInstruction("output = log2(a)", 1), 0);
    XII_TEST_INT(TestInstruction("output = log2(a)", 8), 3);
    XII_TEST_FLOAT(TestInstruction("output = log2(a)", 1.0f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = log2(a)", 4.0f), 2.0f, xiiMath::DefaultEpsilon<float>());

    XII_TEST_INT(TestConstant<xiiInt32>("output = log2(1)"), 0);
    XII_TEST_INT(TestConstant<xiiInt32>("output = log2(16)"), 4);
    XII_TEST_FLOAT(TestConstant<float>("output = log2(1.0)"), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = log2(32.0)"), 5.0f, xiiMath::DefaultEpsilon<float>());

    // Log10
    XII_TEST_FLOAT(TestInstruction("output = log10(a)", 10.0f), 1.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = log10(a)", 1000.0f), 3.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = log10(10.0)"), 1.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = log10(100.0)"), 2.0f, xiiMath::DefaultEpsilon<float>());

    // Pow2
    XII_TEST_INT(TestInstruction("output = pow2(a)", 0), 1);
    XII_TEST_INT(TestInstruction("output = pow2(a)", 3), 8);
    XII_TEST_FLOAT(TestInstruction("output = pow2(a)", 4.0f), 16.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = pow2(a)", 6.0f), 64.0f, xiiMath::DefaultEpsilon<float>());

    XII_TEST_INT(TestConstant<xiiInt32>("output = pow2(0)"), 1);
    XII_TEST_INT(TestConstant<xiiInt32>("output = pow2(3)"), 8);
    XII_TEST_FLOAT(TestConstant<float>("output = pow2(3.0)"), 8.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = pow2(5.0)"), 32.0f, xiiMath::DefaultEpsilon<float>());

    // Sin
    XII_TEST_FLOAT(TestInstruction("output = sin(a)", xiiAngle::MakeFromDegree(90.0f).GetRadian()), 1.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = sin(a)", xiiAngle::MakeFromDegree(45.0f).GetRadian()), xiiMath::Sin(xiiAngle::MakeFromDegree(45.0f)), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = sin(PI / 2)"), 1.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = sin(PI / 4)"), xiiMath::Sin(xiiAngle::MakeFromDegree(45.0f)), xiiMath::DefaultEpsilon<float>());

    // Cos
    XII_TEST_FLOAT(TestInstruction("output = cos(a)", 0.0f), 1.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = cos(a)", xiiAngle::MakeFromDegree(45.0f).GetRadian()), xiiMath::Cos(xiiAngle::MakeFromDegree(45.0f)), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = cos(0)"), 1.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = cos(PI / 4)"), xiiMath::Cos(xiiAngle::MakeFromDegree(45.0f)), xiiMath::DefaultEpsilon<float>());

    // Tan
    XII_TEST_FLOAT(TestInstruction("output = tan(a)", 0.0f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = tan(a)", xiiAngle::MakeFromDegree(45.0f).GetRadian()), 1.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = tan(0)"), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = tan(PI / 4)"), 1.0f, xiiMath::DefaultEpsilon<float>());

    // ASin
    XII_TEST_FLOAT(TestInstruction("output = asin(a)", 1.0f), xiiAngle::MakeFromDegree(90.0f).GetRadian(), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = asin(a)", xiiMath::Sin(xiiAngle::MakeFromDegree(45.0f))), xiiAngle::MakeFromDegree(45.0f).GetRadian(), xiiMath::LargeEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = asin(1)"), xiiAngle::MakeFromDegree(90.0f).GetRadian(), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = asin(sin(PI / 4))"), xiiAngle::MakeFromDegree(45.0f).GetRadian(), xiiMath::LargeEpsilon<float>());

    // ACos
    XII_TEST_FLOAT(TestInstruction("output = acos(a)", 1.0f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = acos(a)", xiiMath::Cos(xiiAngle::MakeFromDegree(45.0f))), xiiAngle::MakeFromDegree(45.0f).GetRadian(), xiiMath::LargeEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = acos(1)"), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = acos(cos(PI / 4))"), xiiAngle::MakeFromDegree(45.0f).GetRadian(), xiiMath::LargeEpsilon<float>());

    // ATan
    XII_TEST_FLOAT(TestInstruction("output = atan(a)", 0.0f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = atan(a)", 1.0f), xiiAngle::MakeFromDegree(45.0f).GetRadian(), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = atan(0)"), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = atan(1)"), xiiAngle::MakeFromDegree(45.0f).GetRadian(), xiiMath::DefaultEpsilon<float>());

    // RadToDeg
    XII_TEST_FLOAT(TestInstruction("output = radToDeg(a)", xiiAngle::MakeFromDegree(135.0f).GetRadian()), 135.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = rad_to_deg(a)", xiiAngle::MakeFromDegree(180.0f).GetRadian()), 180.0f, xiiMath::LargeEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = radToDeg(PI / 2)"), 90.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = rad_to_deg(PI/4)"), 45.0f, xiiMath::DefaultEpsilon<float>());

    // DegToRad
    XII_TEST_FLOAT(TestInstruction("output = degToRad(a)", 135.0f), xiiAngle::MakeFromDegree(135.0f).GetRadian(), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = deg_to_rad(a)", 180.0f), xiiAngle::MakeFromDegree(180.0f).GetRadian(), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = degToRad(90.0)"), xiiAngle::MakeFromDegree(90.0f).GetRadian(), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = deg_to_rad(45)"), xiiAngle::MakeFromDegree(45.0f).GetRadian(), xiiMath::DefaultEpsilon<float>());

    // Round
    XII_TEST_FLOAT(TestInstruction("output = round(a)", 12.34f), 12, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = round(a)", -12.34f), -12, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = round(a)", 12.54f), 13, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = round(a)", -12.54f), -13, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = round(4.3)"), 4, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = round(4.51)"), 5, xiiMath::DefaultEpsilon<float>());

    // Floor
    XII_TEST_FLOAT(TestInstruction("output = floor(a)", 12.34f), 12, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = floor(a)", -12.34f), -13, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = floor(a)", 12.54f), 12, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = floor(a)", -12.54f), -13, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = floor(4.3)"), 4, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = floor(4.51)"), 4, xiiMath::DefaultEpsilon<float>());

    // Ceil
    XII_TEST_FLOAT(TestInstruction("output = ceil(a)", 12.34f), 13, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = ceil(a)", -12.34f), -12, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = ceil(a)", 12.54f), 13, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = ceil(a)", -12.54f), -12, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = ceil(4.3)"), 5, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = ceil(4.51)"), 5, xiiMath::DefaultEpsilon<float>());

    // Trunc
    XII_TEST_FLOAT(TestInstruction("output = trunc(a)", 12.34f), 12, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = trunc(a)", -12.34f), -12, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = trunc(a)", 12.54f), 12, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = trunc(a)", -12.54f), -12, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = trunc(4.3)"), 4, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = trunc(4.51)"), 4, xiiMath::DefaultEpsilon<float>());

    // Frac
    XII_TEST_FLOAT(TestInstruction("output = frac(a)", 12.34f), 0.34f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = frac(a)", -12.34f), -0.34f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = frac(a)", 12.54f), 0.54f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = frac(a)", -12.54f), -0.54f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = frac(4.3)"), 0.3f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = frac(4.51)"), 0.51f, xiiMath::DefaultEpsilon<float>());

    // Length
    XII_TEST_VEC3(TestInstruction<xiiVec3>("output = length(a)", xiiVec3(0, 4, 3)), xiiVec3(5), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(TestInstruction<xiiVec3>("output = length(a)", xiiVec3(-3, 4, 0)), xiiVec3(5), xiiMath::DefaultEpsilon<float>());

    // Normalize
    XII_TEST_VEC3(TestInstruction<xiiVec3>("output = normalize(a)", xiiVec3(1, 4, 3)), xiiVec3(1, 4, 3).GetNormalized(), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(TestInstruction<xiiVec3>("output = normalize(a)", xiiVec3(-3, 7, 22)), xiiVec3(-3, 7, 22).GetNormalized(), xiiMath::DefaultEpsilon<float>());

    // Length and normalize optimization
    {
      xiiStringView testCode      = "var x = length(a); var na = normalize(a); output = b * x + na";
      xiiStringView referenceCode = "var x = length(a); var na = a / x; output = b * x + na";

      xiiExpressionByteCode testByteCode;
      XII_TEST_BOOL(CompareCode<xiiVec3>(testCode, referenceCode, testByteCode));

      xiiVec3 a   = xiiVec3(0, 4, 3);
      xiiVec3 b   = xiiVec3(1, 0, 0);
      xiiVec3 res = b * a.GetLength() + a.GetNormalized();
      XII_TEST_VEC3(Execute(testByteCode, a, b), res, xiiMath::DefaultEpsilon<float>());
    }

    // BitwiseNot
    XII_TEST_INT(TestInstruction("output = ~a", 1), ~1);
    XII_TEST_INT(TestInstruction("output = ~a", 8), ~8);
    XII_TEST_INT(TestConstant<xiiInt32>("output = ~1"), ~1);
    XII_TEST_INT(TestConstant<xiiInt32>("output = ~17"), ~17);

    // LogicalNot
    XII_TEST_INT(TestInstruction("output = !(a == 1)", 1), 0);
    XII_TEST_INT(TestInstruction("output = !(a == 1)", 8), 1);
    XII_TEST_INT(TestConstant<xiiInt32>("output = !(1 == 1)"), 0);
    XII_TEST_INT(TestConstant<xiiInt32>("output = !(8 == 1)"), 1);

    // All
    XII_TEST_VEC3(TestInstruction("var t = (a == b); output = all(t)", xiiVec3(1, 2, 3), xiiVec3(1, 2, 3)), xiiVec3(1), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(TestInstruction("var t = (a == b); output = all(t)", xiiVec3(1, 2, 3), xiiVec3(1, 2, 4)), xiiVec3(0), xiiMath::DefaultEpsilon<float>());

    // Any
    XII_TEST_VEC3(TestInstruction("var t = (a == b); output = any(t)", xiiVec3(1, 2, 3), xiiVec3(4, 5, 3)), xiiVec3(1), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(TestInstruction("var t = (a == b); output = any(t)", xiiVec3(1, 2, 3), xiiVec3(4, 5, 6)), xiiVec3(0), xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Binary instructions")
  {
    // Add
    TestBinaryInstruction<xiiInt32, xiiInt32, LeftConstantOptimization>("+", 3, 5, 8);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("+", 3.5f, 5.3f, 8.8f);

    // Subtract
    TestBinaryInstruction<xiiInt32, xiiInt32, 0>("-", 9, 5, 4);
    TestBinaryInstruction<float, float, 0>("-", 9.5f, 5.3f, 4.2f);

    // Multiply
    TestBinaryInstruction<xiiInt32, xiiInt32, LeftConstantOptimization>("*", 3, 5, 15);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("*", 3.5f, 5.3f, 18.55f);

    // Divide
    TestBinaryInstruction<xiiInt32, xiiInt32, 0>("/", 11, 5, 2);
    TestBinaryInstruction<xiiInt32, xiiInt32, NoInstructionsCountCheck>("/", -11, 4, -2); // divide by power of 2 optimization
    TestBinaryInstruction<xiiInt32, xiiInt32, 0>("/", 11, -4, -2);                        // divide by power of 2 optimization only works for positive divisors
    TestBinaryInstruction<float, float, 0>("/", 12.6f, 3.0f, 4.2f);

    // Modulo
    TestBinaryInstruction<xiiInt32, xiiInt32, NoInstructionsCountCheck>("%", 13, 5, 3);
    TestBinaryInstruction<xiiInt32, xiiInt32, NoInstructionsCountCheck>("%", -13, 5, -3);
    TestBinaryInstruction<xiiInt32, xiiInt32, NoInstructionsCountCheck>("%", 13, 4, 1);
    TestBinaryInstruction<xiiInt32, xiiInt32, NoInstructionsCountCheck>("%", -13, 4, -1);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("%", 13.5, 5.0, 3.5);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("mod(", -13.5, 5.0, -3.5);

    // Log
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("log(", 2.0f, 1024.0f, 10.0f);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("log(", 7.1f, 81.62f, xiiMath::Log(7.1f, 81.62f));

    // Pow
    TestBinaryInstruction<xiiInt32, xiiInt32, NoInstructionsCountCheck>("pow(", 2, 5, 32);
    TestBinaryInstruction<xiiInt32, xiiInt32, NoInstructionsCountCheck>("pow(", 3, 3, 27);

    // Pow is replaced by multiplication for constant exponents up until 16.
    // Test all of them to ensure the multiplication tables are correct.
    for (xiiInt32 i = 0; i <= 16; ++i)
    {
      xiiStringBuilder testCode;
      testCode.SetFormat("output = pow(a, {})", i);

      xiiExpressionByteCode testByteCode;
      Compile<xiiInt32>(testCode, testByteCode);
      XII_TEST_INT(Execute(testByteCode, 3), xiiMath::Pow(3, i));
    }

    {
      xiiStringView testCode      = "output = pow(a, 7)";
      xiiStringView referenceCode = "var a2 = a * a; var a3 = a2 * a; var a6 = a3 * a3; output = a6 * a";

      xiiExpressionByteCode testByteCode;
      XII_TEST_BOOL(CompareCode<xiiInt32>(testCode, referenceCode, testByteCode));
      XII_TEST_INT(Execute(testByteCode, 3), 2187);
    }

    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("pow(", 2.0, 5.0, 32.0);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("pow(", 3.0f, 7.9f, xiiMath::Pow(3.0f, 7.9f));

    {
      xiiStringView testCode      = "output = pow(a, 15.0)";
      xiiStringView referenceCode = "var a2 = a * a; var a3 = a2 * a; var a6 = a3 * a3; var a12 = a6 * a6; output = a12 * a3";

      xiiExpressionByteCode testByteCode;
      XII_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      XII_TEST_FLOAT(Execute(testByteCode, 2.1f), xiiMath::Pow(2.1f, 15.0f), xiiMath::DefaultEpsilon<float>());
    }

    // Min
    TestBinaryInstruction<xiiInt32, xiiInt32, LeftConstantOptimization>("min(", 11, 5, 5);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("min(", 12.6f, 3.0f, 3.0f);

    // Max
    TestBinaryInstruction<xiiInt32, xiiInt32, LeftConstantOptimization>("max(", 11, 5, 11);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("max(", 12.6f, 3.0f, 12.6f);

    // Dot
    TestBinaryInstruction<xiiVec3, xiiVec3, NoInstructionsCountCheck>("dot(", xiiVec3(1, -2, 3), xiiVec3(-5, -6, 7), xiiVec3(28));
    TestBinaryInstruction<xiiVec3I32, xiiVec3I32, NoInstructionsCountCheck>("dot(", xiiVec3I32(1, -2, 3), xiiVec3I32(-5, -6, 7), xiiVec3I32(28));

    // Cross
    TestBinaryInstruction<xiiVec3, xiiVec3, NoInstructionsCountCheck>("cross(", xiiVec3(1, 0, 0), xiiVec3(0, 1, 0), xiiVec3(0, 0, 1));
    TestBinaryInstruction<xiiVec3, xiiVec3, NoInstructionsCountCheck>("cross(", xiiVec3(0, 1, 0), xiiVec3(0, 0, 1), xiiVec3(1, 0, 0));
    TestBinaryInstruction<xiiVec3, xiiVec3, NoInstructionsCountCheck>("cross(", xiiVec3(0, 0, 1), xiiVec3(1, 0, 0), xiiVec3(0, 1, 0));

    // Reflect
    TestBinaryInstruction<xiiVec3, xiiVec3, NoInstructionsCountCheck>("reflect(", xiiVec3(1, 2, -1), xiiVec3(0, 0, 1), xiiVec3(1, 2, 1));

    // BitshiftLeft
    TestBinaryInstruction<xiiInt32, xiiInt32, 0>("<<", 11, 5, 11 << 5);

    // BitshiftRight
    TestBinaryInstruction<xiiInt32, xiiInt32, 0>(">>", 0xABCD, 8, 0xAB);

    // BitwiseAnd
    TestBinaryInstruction<xiiInt32, xiiInt32, LeftConstantOptimization>("&", 0xFFCD, 0xABFF, 0xABCD);

    // BitwiseXor
    TestBinaryInstruction<xiiInt32, xiiInt32, LeftConstantOptimization>("^", 0xFFCD, 0xABFF, 0xFFCD ^ 0xABFF);

    // BitwiseOr
    TestBinaryInstruction<xiiInt32, xiiInt32, LeftConstantOptimization>("|", 0x00CD, 0xAB00, 0xABCD);

    // Equal
    TestBinaryInstruction<bool, xiiInt32, LeftConstantOptimization>("==", 11, 5, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("==", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("==", true, false, false);

    // NotEqual
    TestBinaryInstruction<bool, xiiInt32, LeftConstantOptimization>("!=", 11, 5, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("!=", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("!=", true, false, true);

    // Less
    TestBinaryInstruction<bool, xiiInt32, LeftConstantOptimization>("<", 11, 5, 0);
    TestBinaryInstruction<bool, xiiInt32, LeftConstantOptimization>("<", 11, 11, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<", 12.6f, 12.6f, 0.0f);

    // LessEqual
    TestBinaryInstruction<bool, xiiInt32, LeftConstantOptimization>("<=", 11, 5, 0);
    TestBinaryInstruction<bool, xiiInt32, LeftConstantOptimization>("<=", 11, 11, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<=", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<=", 12.6f, 12.6f, 1.0f);

    // Greater
    TestBinaryInstruction<bool, xiiInt32, LeftConstantOptimization>(">", 11, 5, 1);
    TestBinaryInstruction<bool, xiiInt32, LeftConstantOptimization>(">", 11, 11, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">", 12.6f, 12.6f, 0.0f);

    // GreaterEqual
    TestBinaryInstruction<bool, xiiInt32, LeftConstantOptimization>(">=", 11, 5, 1);
    TestBinaryInstruction<bool, xiiInt32, LeftConstantOptimization>(">=", 11, 11, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">=", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">=", 12.6f, 12.6f, 1.0f);

    // LogicalAnd
    TestBinaryInstruction<bool, bool, LeftConstantOptimization | NoInstructionsCountCheck>("&&", true, false, false);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("&&", true, true, true);

    // LogicalOr
    TestBinaryInstruction<bool, bool, LeftConstantOptimization | NoInstructionsCountCheck>("||", true, false, true);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("||", false, false, false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ternary instructions")
  {
    // Clamp
    XII_TEST_INT(TestInstruction("output = clamp(a, b, c)", -1, 0, 10), 0);
    XII_TEST_INT(TestInstruction("output = clamp(a, b, c)", 2, 0, 10), 2);
    XII_TEST_FLOAT(TestInstruction("output = clamp(a, b, c)", -1.5f, 0.0f, 1.0f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = clamp(a, b, c)", 2.5f, 0.0f, 1.0f), 1.0f, xiiMath::DefaultEpsilon<float>());

    XII_TEST_INT(TestConstant<xiiInt32>("output = clamp(-1, 0, 10)"), 0);
    XII_TEST_INT(TestConstant<xiiInt32>("output = clamp(2, 0, 10)"), 2);
    XII_TEST_FLOAT(TestConstant<float>("output = clamp(-1.5, 0, 2)"), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = clamp(2.5, 0, 2)"), 2.0f, xiiMath::DefaultEpsilon<float>());

    // Select
    XII_TEST_INT(TestInstruction("output = (a == 1) ? b : c", 1, 2, 3), 2);
    XII_TEST_INT(TestInstruction("output = a != 1 ? b : c", 1, 2, 3), 3);
    XII_TEST_FLOAT(TestInstruction("output = (a == 1) ? b : c", 1.0f, 2.4f, 3.5f), 2.4f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = a != 1 ? b : c", 1.0f, 2.4f, 3.5f), 3.5f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_INT(TestInstruction("output = (a == 1) ? (b > 2) : (c > 2)", 1, 2, 3), 0);
    XII_TEST_INT(TestInstruction("output = a != 1 ? b > 2 : c > 2", 1, 2, 3), 1);

    XII_TEST_INT(TestConstant<xiiInt32>("output = (1 == 1) ? 2 : 3"), 2);
    XII_TEST_INT(TestConstant<xiiInt32>("output = 1 != 1 ? 2 : 3"), 3);
    XII_TEST_FLOAT(TestConstant<float>("output = (1.0 == 1.0) ? 2.4 : 3.5"), 2.4f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = 1.0 != 1.0 ? 2.4 : 3.5"), 3.5f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_INT(TestConstant<xiiInt32>("output = (1 == 1) ? false : true"), 0);
    XII_TEST_INT(TestConstant<xiiInt32>("output = 1 != 1 ? false : true"), 1);

    // Lerp
    XII_TEST_FLOAT(TestInstruction("output = lerp(a, b, c)", 1.0f, 5.0f, 0.75f), 4.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = lerp(a, b, c)", -1.0f, -11.0f, 0.1f), -2.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = lerp(1, 5, 0.75)"), 4.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = lerp(-1, -11, 0.1)"), -2.0f, xiiMath::DefaultEpsilon<float>());

    // SmoothStep
    XII_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.0f, 0.0f, 1.0f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.2f, 0.0f, 1.0f), xiiMath::SmoothStep(0.2f, 0.0f, 1.0f), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.5f, 0.0f, 1.0f), 0.5f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.2f, 0.2f, 0.8f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.4f, 0.2f, 0.8f), xiiMath::SmoothStep(0.4f, 0.2f, 0.8f), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = smoothstep(0.2, 0, 1)"), xiiMath::SmoothStep(0.2f, 0.0f, 1.0f), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = smoothstep(0.4, 0.2, 0.8)"), xiiMath::SmoothStep(0.4f, 0.2f, 0.8f), xiiMath::DefaultEpsilon<float>());

    // SmootherStep
    XII_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.0f, 0.0f, 1.0f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.2f, 0.0f, 1.0f), xiiMath::SmootherStep(0.2f, 0.0f, 1.0f), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.5f, 0.0f, 1.0f), 0.5f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.2f, 0.2f, 0.8f), 0.0f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.4f, 0.2f, 0.8f), xiiMath::SmootherStep(0.4f, 0.2f, 0.8f), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = smootherstep(0.2, 0, 1)"), xiiMath::SmootherStep(0.2f, 0.0f, 1.0f), xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(TestConstant<float>("output = smootherstep(0.4, 0.2, 0.8)"), xiiMath::SmootherStep(0.4f, 0.2f, 0.8f), xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Local variables")
  {
    xiiExpressionByteCode referenceByteCode;
    {
      xiiStringView code = "output = (a + b) * 2";
      Compile<float>(code, referenceByteCode);
    }

    xiiExpressionByteCode testByteCode;

    xiiStringView code = "var e = a + b; output = e * 2";
    Compile<float>(code, testByteCode);
    XII_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e = e * 2; output = e";
    Compile<float>(code, testByteCode);
    XII_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e *= 2; output = e";
    Compile<float>(code, testByteCode);
    XII_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; var f = e; e = 2; output = f * e";
    Compile<float>(code, testByteCode);
    XII_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    XII_TEST_FLOAT(Execute(testByteCode, 2.0f, 3.0f), 10.0f, xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Assignment")
  {
    {
      xiiStringView testCode      = "output = 40; output += 2";
      xiiStringView referenceCode = "output = 42";

      xiiExpressionByteCode testByteCode;
      XII_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));

      XII_TEST_FLOAT(Execute<float>(testByteCode), 42.0f, xiiMath::DefaultEpsilon<float>());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Integer arithmetic")
  {
    xiiExpressionByteCode testByteCode;

    xiiStringView code = "output = ((a & 0xFF) << 8) | (b & 0xFFFF >> 8)";
    Compile<xiiInt32>(code, testByteCode);

    const xiiInt32 a = 0xABABABAB;
    const xiiInt32 b = 0xCDCDCDCD;
    XII_TEST_INT(Execute(testByteCode, a, b), 0xABCD);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constant folding")
  {
    xiiStringView testCode = "var x = abs(-7) + saturate(2) + 2\n"
                             "var v = (sqrt(25) - 4) * 5\n"
                             "var m = min(300, 1000) / max(1, 3);"
                             "var r = m - x * 5 - v - clamp(13, 1, 3);\n"
                             "output = r";

    xiiStringView referenceCode = "output = 42";

    {
      xiiExpressionByteCode testByteCode;
      XII_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));

      XII_TEST_FLOAT(Execute<float>(testByteCode), 42.0f, xiiMath::DefaultEpsilon<float>());
    }

    {
      xiiExpressionByteCode testByteCode;
      XII_TEST_BOOL(CompareCode<xiiInt32>(testCode, referenceCode, testByteCode));

      XII_TEST_INT(Execute<xiiInt32>(testByteCode), 42);
    }

    testCode = "";
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constant instructions")
  {
    // There are special instructions in the vm which take the constant as the first operand in place and
    // don't require an extra mov for the constant.
    // This test checks whether the compiler transforms operations with constants as second operands to the preferred form.

    xiiStringView testCode = "output = (2 + a) + (-1 + b) + (2 * c) + (d / 5) + min(1, c) + max(2, d)";

    {
      xiiStringView referenceCode = "output = (a + 2) + (b + -1) + (c * 2) + (d * 0.2) + min(c, 1) + max(d, 2)";

      xiiExpressionByteCode testByteCode;
      XII_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      XII_TEST_INT(testByteCode.GetNumInstructions(), 16);
      XII_TEST_INT(testByteCode.GetNumTempRegisters(), 4);
      XII_TEST_FLOAT(Execute(testByteCode, 1.0f, 2.0f, 3.0f, 40.f), 59.0f, xiiMath::DefaultEpsilon<float>());
    }

    {
      xiiStringView referenceCode = "output = (a + 2) + (b + -1) + (c * 2) + (d / 5) + min(c, 1) + max(d, 2)";

      xiiExpressionByteCode testByteCode;
      XII_TEST_BOOL(CompareCode<xiiInt32>(testCode, referenceCode, testByteCode));
      XII_TEST_INT(testByteCode.GetNumInstructions(), 16);
      XII_TEST_INT(testByteCode.GetNumTempRegisters(), 4);
      XII_TEST_INT(Execute(testByteCode, 1, 2, 3, 40), 59);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Integer and float conversions")
  {
    xiiStringView testCode = "var x = 7; var y = 0.6\n"
                             "var e = a * x * b * y\n"
                             "int i = c * 2; i *= i; e += i\n"
                             "output = e";

    xiiStringView referenceCode = "int i = (int(c) * 2); output = int((float(a * 7 * b) * 0.6) + float(i * i))";

    xiiExpressionByteCode testByteCode;
    XII_TEST_BOOL(CompareCode<xiiInt32>(testCode, referenceCode, testByteCode));
    XII_TEST_INT(Execute(testByteCode, 1, 2, 3), 44);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Bool conversions")
  {
    xiiStringView testCode = "var x = true\n"
                             "bool y = a\n"
                             "output = x == y";

    {
      xiiStringView referenceCode = "bool r = true == (a != 0); output = r ? 1 : 0";

      xiiExpressionByteCode testByteCode;
      XII_TEST_BOOL(CompareCode<xiiInt32>(testCode, referenceCode, testByteCode));
      XII_TEST_INT(Execute(testByteCode, 14), 1);
    }

    {
      xiiStringView referenceCode = "bool r = true == (a != 0); output = r ? 1.0 : 0.0";

      xiiExpressionByteCode testByteCode;
      XII_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      XII_TEST_FLOAT(Execute(testByteCode, 15.0f), 1.0f, xiiMath::DefaultEpsilon<float>());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Load Inputs/Store Outputs")
  {
    TestInputOutput<float>();
    TestInputOutput<xiiFloat16>();

    TestInputOutput<xiiInt32>();
    TestInputOutput<xiiInt16>();
    TestInputOutput<xiiInt8>();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Function overloads")
  {
    s_pParser->RegisterFunction(s_TestFunc1.m_Desc);
    s_pParser->RegisterFunction(s_TestFunc2.m_Desc);

    s_pVM->RegisterFunction(s_TestFunc1);
    s_pVM->RegisterFunction(s_TestFunc2);

    {
      // take TestFunc1 overload for all xiiInt32s
      xiiStringView         testCode = "output = TestFunc(1, 2, 3)";
      xiiExpressionByteCode testByteCode;
      Compile<xiiInt32>(testCode, testByteCode);
      XII_TEST_INT(Execute<xiiInt32>(testByteCode), 2);
    }

    {
      // take TestFunc1 overload for float, xiiInt32
      xiiStringView         testCode = "output = TestFunc(1.0, 2, 3)";
      xiiExpressionByteCode testByteCode;
      Compile<xiiInt32>(testCode, testByteCode);
      XII_TEST_INT(Execute<xiiInt32>(testByteCode), 2);
    }

    {
      // take TestFunc2 overload for xiiInt32, float
      xiiStringView         testCode = "output = TestFunc(1, 2.0, 3)";
      xiiExpressionByteCode testByteCode;
      Compile<xiiInt32>(testCode, testByteCode);
      XII_TEST_INT(Execute<xiiInt32>(testByteCode), 7);
    }

    {
      // take TestFunc2 overload for all float
      xiiStringView         testCode = "output = TestFunc(1.0, 2.0, 3)";
      xiiExpressionByteCode testByteCode;
      Compile<xiiInt32>(testCode, testByteCode);
      XII_TEST_INT(Execute<xiiInt32>(testByteCode), 7);
    }

    {
      // take TestFunc1 overload when only two params are given
      xiiStringView         testCode = "output = TestFunc(1.0, 2.0)";
      xiiExpressionByteCode testByteCode;
      Compile<xiiInt32>(testCode, testByteCode);
      XII_TEST_INT(Execute<xiiInt32>(testByteCode), 2);
    }

    s_pParser->UnregisterFunction(s_TestFunc1.m_Desc);
    s_pParser->UnregisterFunction(s_TestFunc2.m_Desc);

    s_pVM->UnregisterFunction(s_TestFunc1);
    s_pVM->UnregisterFunction(s_TestFunc2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Common subexpression elimination")
  {
    xiiStringView testCode = "var x1 = a * max(b, c)\n"
                             "var x2 = max(c, b) * a\n"
                             "var y1 = a * pow(2, 3)\n"
                             "var y2 = 8 * a\n"
                             "output = x1 + x2 + y1 + y2";

    xiiStringView referenceCode = "var x = a * max(b, c); var y = a * 8; output = x + x + y + y";

    xiiExpressionByteCode testByteCode;
    XII_TEST_BOOL(CompareCode<xiiInt32>(testCode, referenceCode, testByteCode));
    XII_TEST_INT(Execute(testByteCode, 2, 4, 8), 64);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Vector constructors")
  {
    {
      xiiStringView testCode = "var x = vec3(1, 2, 3)\n"
                               "var y = vec4(x, 4)\n"
                               "vec3 z = vec2(1, 2)\n"
                               "var w = vec4()\n"
                               "output = vec4(x) + y + vec4(z) + w";

      xiiExpressionByteCode testByteCode;
      Compile<xiiVec3>(testCode, testByteCode);
      XII_TEST_VEC3(Execute<xiiVec3>(testByteCode), xiiVec3(3, 6, 6), xiiMath::DefaultEpsilon<float>());
    }

    {
      xiiStringView testCode = "var x = vec4(a.xy, (vec2(6, 8) - vec2(3, 4)).xy)\n"
                               "var y = vec4(1, vec2(2, 3), 4)\n"
                               "var z = vec4(1, vec3(2, 3, 4))\n"
                               "var w = vec4(1, 2, a.zw)\n"
                               "var one = vec4(1)\n"
                               "output = vec4(x) + y + vec4(z) + w + one";

      xiiExpressionByteCode testByteCode;
      Compile<xiiVec3>(testCode, testByteCode);
      XII_TEST_VEC3(Execute(testByteCode, xiiVec3(1, 2, 3)), xiiVec3(5, 9, 13), xiiMath::DefaultEpsilon<float>());
    }

    {
      xiiStringView testCode = "var x = vec4(1, 2, 3, 4)\n"
                               "var y = x.z\n"
                               "x.yz = 7\n"
                               "x.xz = vec2(2, 7)\n"
                               "output = x * y";

      xiiExpressionByteCode testByteCode;
      Compile<xiiVec3>(testCode, testByteCode);
      XII_TEST_VEC3(Execute<xiiVec3>(testByteCode), xiiVec3(6, 21, 21), xiiMath::DefaultEpsilon<float>());
    }

    {
      xiiStringView testCode = "var x = 1\n"
                               "x.z = 7.5\n"
                               "output = x";

      xiiExpressionByteCode testByteCode;
      Compile<xiiVec3>(testCode, testByteCode);
      XII_TEST_VEC3(Execute<xiiVec3>(testByteCode), xiiVec3(1, 0, 7), xiiMath::DefaultEpsilon<float>());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Vector instructions")
  {
    // The VM does only support scalar data types.
    // This test checks whether the compiler transforms everything correctly to scalar operation.

    xiiStringView testCode = "output = a * vec3(1, 2, 3) + sqrt(b)";

    xiiStringView referenceCode = "output.x = a.x + sqrt(b.x)\n"
                                  "output.y = a.y * 2 + sqrt(b.y)\n"
                                  "output.z = a.z * 3 + sqrt(b.z)";

    xiiExpressionByteCode testByteCode;
    XII_TEST_BOOL(CompareCode<xiiVec3>(testCode, referenceCode, testByteCode));
    XII_TEST_VEC3(Execute(testByteCode, xiiVec3(1, 3, 5), xiiVec3(4, 9, 16)), xiiVec3(3, 9, 19), xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Vector swizzle")
  {
    xiiStringView testCode = "var n = vec4(1, 2, 3, 4)\n"
                             "var m = vec4(5, 6, 7, 8)\n"
                             "var p = n.xxyy + m.zzww * m.abgr + n.w\n"
                             "output = p";

    // vec3(1, 1, 2) + vec3(7, 7, 8) * vec3(8, 7, 6) + 4
    // output.x = 1 + 7 * 8 + 4 = 61
    // output.y = 1 + 7 * 7 + 4 = 54
    // output.z = 2 + 8 * 6 + 4 = 54

    xiiExpressionByteCode testByteCode;
    Compile<xiiVec3>(testCode, testByteCode);
    XII_TEST_VEC3(Execute<xiiVec3>(testByteCode), xiiVec3(61, 54, 54), xiiMath::DefaultEpsilon<float>());
  }
}
