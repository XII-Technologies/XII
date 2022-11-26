#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
  void DumpAST(const xiiExpressionAST& ast, xiiStringView sOutputName)
  {
    xiiDGMLGraph dgmlGraph;
    ast.PrintGraph(dgmlGraph);

    xiiStringBuilder sFileName;
    sFileName.Format(":output/Expression/{}_AST.dgml", sOutputName);

    xiiDGMLGraphWriter dgmlGraphWriter;
    if (dgmlGraphWriter.WriteGraphToFile(sFileName, dgmlGraph).Succeeded())
    {
      xiiLog::Info("AST was dumped to: {}", sFileName);
    }
    else
    {
      xiiLog::Error("Failed to dump AST to: {}", sFileName);
    }
  }

  void DumpDisassembly(const xiiExpressionByteCode& byteCode, xiiStringView sOutputName, xiiUInt32 uiCounter)
  {
    xiiStringBuilder sDisassembly;
    byteCode.Disassemble(sDisassembly);

    xiiStringBuilder sFileName;
    sFileName.Format(":output/Expression/{}_{}_ByteCode.txt", xiiArgU(uiCounter, 2, true), sOutputName);

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
} // namespace

XII_CREATE_SIMPLE_TEST(CodeUtils, Expression)
{
  s_uiNumByteCodeComparisons = 0;

  xiiStringBuilder outputPath = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", xiiFileSystem::AllowWrites) == XII_SUCCESS);

  xiiExpressionParser   parser;
  xiiExpressionCompiler compiler;
  xiiExpressionVM       vm;

  auto Compile = [&](xiiStringView code, xiiExpressionByteCode& out_ByteCode, bool dumpASTs = false) {
    xiiExpressionParser::Stream inputs[] = {
      xiiExpressionParser::Stream(s_sA, xiiProcessingStream::DataType::Float),
      xiiExpressionParser::Stream(s_sB, xiiProcessingStream::DataType::Float),
      xiiExpressionParser::Stream(s_sC, xiiProcessingStream::DataType::Float),
      xiiExpressionParser::Stream(s_sD, xiiProcessingStream::DataType::Float),
    };

    xiiExpressionParser::Stream outputs[] = {
      xiiExpressionParser::Stream(s_sOutput, xiiProcessingStream::DataType::Float),
    };

    xiiExpressionAST ast;
    XII_TEST_BOOL(parser.Parse(code, inputs, outputs, {}, ast).Succeeded());

    if (dumpASTs)
    {
      DumpAST(ast, "ParserTest");
    }

    XII_TEST_BOOL(compiler.Compile(ast, out_ByteCode).Succeeded());

    if (dumpASTs)
    {
      DumpAST(ast, "ParserTest_Opt");
    }
  };

  auto Execute = [&](const xiiExpressionByteCode& byteCode, float a = 0.0f, float b = 0.0f, float c = 0.0f, float d = 0.0f) {
    xiiProcessingStream inputs[] = {
      xiiProcessingStream(s_sA, xiiMakeArrayPtr(&a, 1).ToByteArray(), xiiProcessingStream::DataType::Float),
      xiiProcessingStream(s_sB, xiiMakeArrayPtr(&b, 1).ToByteArray(), xiiProcessingStream::DataType::Float),
      xiiProcessingStream(s_sC, xiiMakeArrayPtr(&c, 1).ToByteArray(), xiiProcessingStream::DataType::Float),
      xiiProcessingStream(s_sD, xiiMakeArrayPtr(&d, 1).ToByteArray(), xiiProcessingStream::DataType::Float),
    };

    float               fOutput   = xiiMath::NaN<float>();
    xiiProcessingStream outputs[] = {
      xiiProcessingStream(s_sOutput, xiiMakeArrayPtr(&fOutput, 1).ToByteArray(), xiiProcessingStream::DataType::Float),
    };

    XII_TEST_BOOL(vm.Execute(byteCode, inputs, outputs, 1).Succeeded());

    return fOutput;
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Local variables")
  {
    xiiExpressionByteCode referenceByteCode;
    {
      xiiStringView code = "output = (a + b) * 2";
      Compile(code, referenceByteCode);
    }

    xiiExpressionByteCode testByteCode;

    xiiStringView code = "var e = a + b; output = e * 2";
    Compile(code, testByteCode);
    XII_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e = e * 2; output = e";
    Compile(code, testByteCode);
    XII_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e *= 2; output = e";
    Compile(code, testByteCode);
    XII_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; var f = e; e = 2; output = f * e";
    Compile(code, testByteCode);
    XII_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    const float a = 2;
    const float b = 3;
    XII_TEST_FLOAT(Execute(testByteCode, a, b), 10.0f, xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constant folding")
  {
    xiiExpressionByteCode referenceByteCode;
    {
      xiiStringView code = "output = 42";
      Compile(code, referenceByteCode);
    }

    xiiExpressionByteCode testByteCode;

    xiiStringView code = "var x = abs(-7) + saturate(2) + 2\n"
                         "var v = (sqrt(25) - 4) * 5\n"
                         "var m = min(300, 1000) / max(1, 3);"
                         "var r = m - x * 5 - v - clamp(13, 1, 3);\n"
                         "output = r";

    Compile(code, testByteCode);
    XII_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    XII_TEST_FLOAT(Execute(testByteCode), 42.0f, xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constant instructions")
  {
    // There are special instructions in the vm which take the constant as the first operand in place and
    // don't require an extra mov for the constant.
    // This test checks whether the compiler transforms operations with constants as second operands to the preferred form.

    xiiExpressionByteCode referenceByteCode;
    {
      xiiStringView code = "output = (2 + a) + (-1 + b) + (2 * c) + (0.1 * d) + min(1, c) + max(2, d)";
      Compile(code, referenceByteCode);
    }

    xiiExpressionByteCode testByteCode;

    xiiStringView code = "output = (a + 2) + (b - 1) + (c * 2) + (d / 10) + min(c, 1) + max(d, 2)";
    Compile(code, testByteCode);
    XII_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    const float a = 1;
    const float b = 2;
    const float c = 3;
    const float d = 40;
    XII_TEST_FLOAT(Execute(testByteCode, a, b, c, d), 55.0f, xiiMath::DefaultEpsilon<float>());
  }
}
