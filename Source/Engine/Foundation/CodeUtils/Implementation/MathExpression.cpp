/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/MathExpression.h>

static xiiHashedString s_sOutput = xiiMakeHashedString("output");

xiiMathExpression::xiiMathExpression() = default;

xiiMathExpression::xiiMathExpression(xiiStringView sExpressionString)
{
  Reset(sExpressionString);
}

void xiiMathExpression::Reset(xiiStringView sExpressionString)
{
  m_sOriginalExpression.Assign(sExpressionString);
  m_ByteCode.Clear();
  m_bIsValid = false;

  if (sExpressionString.IsEmpty())
    return;

  xiiStringBuilder tmp = s_sOutput.GetView();
  tmp.Append(" = ", sExpressionString);

  xiiExpression::StreamDesc outputs[] = {
    {s_sOutput, xiiProcessingStream::DataType::Float},
  };

  xiiExpressionParser          parser;
  xiiExpressionParser::Options parserOptions;
  parserOptions.m_bTreatUnknownVariablesAsInputs = true;

  xiiExpressionAST ast;
  if (parser.Parse(tmp, xiiArrayPtr<xiiExpression::StreamDesc>(), outputs, parserOptions, ast).Failed())
    return;

  xiiExpressionCompiler compiler;
  if (compiler.Compile(ast, m_ByteCode).Failed())
    return;

  m_bIsValid = true;
}

float xiiMathExpression::Evaluate(xiiArrayPtr<Input> inputs)
{
  float fOutput = xiiMath::NaN<float>();

  if (!IsValid() || m_ByteCode.IsEmpty())
  {
    xiiLog::Error("Can't evaluate invalid math expression '{0}'", m_sOriginalExpression);
    return fOutput;
  }

  xiiTemporaryHybridArray<xiiProcessingStream, 8> inputStreams;
  for (auto& input : inputs)
  {
    if (input.m_sName.IsEmpty())
      continue;

    inputStreams.PushBack(xiiProcessingStream(input.m_sName, xiiMakeArrayPtr(&input.m_fValue, 1).ToByteArray(), xiiProcessingStream::DataType::Float));
  }

  xiiProcessingStream              outputStream(s_sOutput, xiiMakeArrayPtr(&fOutput, 1).ToByteArray(), xiiProcessingStream::DataType::Float);
  xiiArrayPtr<xiiProcessingStream> outputStreams = xiiMakeArrayPtr(&outputStream, 1);

  if (m_VM.Execute(m_ByteCode, inputStreams, outputStreams, 1).Failed())
  {
    xiiLog::Error("Failed to execute expression VM");
  }

  return fOutput;
}

XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_MathExpression);
