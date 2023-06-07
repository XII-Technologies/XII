#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace
{
  struct AssignOperator
  {
    xiiStringView                    m_sName;
    xiiExpressionAST::NodeType::Enum m_NodeType;
  };

  static constexpr AssignOperator s_assignOperators[] = {
    {"+="_xiisv, xiiExpressionAST::NodeType::Add},
    {"-="_xiisv, xiiExpressionAST::NodeType::Subtract},
    {"*="_xiisv, xiiExpressionAST::NodeType::Multiply},
    {"/="_xiisv, xiiExpressionAST::NodeType::Divide},
    {"%="_xiisv, xiiExpressionAST::NodeType::Modulo},
    {"<<="_xiisv, xiiExpressionAST::NodeType::BitshiftLeft},
    {">>="_xiisv, xiiExpressionAST::NodeType::BitshiftRight},
    {"&="_xiisv, xiiExpressionAST::NodeType::BitwiseAnd},
    {"^="_xiisv, xiiExpressionAST::NodeType::BitwiseXor},
    {"|="_xiisv, xiiExpressionAST::NodeType::BitwiseOr},
  };

  struct BinaryOperator
  {
    xiiStringView                    m_sName;
    xiiExpressionAST::NodeType::Enum m_NodeType;
    int                              m_iPrecedence;
  };

  // Operator precedence according to https://en.cppreference.com/w/cpp/language/operator_precedence,
  // lower value means higher precedence
  // sorted by string length to simplify the test against a token stream
  static constexpr BinaryOperator s_binaryOperators[] = {
    {"&&"_xiisv, xiiExpressionAST::NodeType::LogicalAnd, 14},
    {"||"_xiisv, xiiExpressionAST::NodeType::LogicalOr, 15},
    {"<<"_xiisv, xiiExpressionAST::NodeType::BitshiftLeft, 7},
    {">>"_xiisv, xiiExpressionAST::NodeType::BitshiftRight, 7},
    {"=="_xiisv, xiiExpressionAST::NodeType::Equal, 10},
    {"!="_xiisv, xiiExpressionAST::NodeType::NotEqual, 10},
    {"<="_xiisv, xiiExpressionAST::NodeType::LessEqual, 9},
    {">="_xiisv, xiiExpressionAST::NodeType::GreaterEqual, 9},
    {"<"_xiisv, xiiExpressionAST::NodeType::Less, 9},
    {">"_xiisv, xiiExpressionAST::NodeType::Greater, 9},
    {"&"_xiisv, xiiExpressionAST::NodeType::BitwiseAnd, 11},
    {"^"_xiisv, xiiExpressionAST::NodeType::BitwiseXor, 12},
    {"|"_xiisv, xiiExpressionAST::NodeType::BitwiseOr, 13},
    {"?"_xiisv, xiiExpressionAST::NodeType::Select, 16},
    {"+"_xiisv, xiiExpressionAST::NodeType::Add, 6},
    {"-"_xiisv, xiiExpressionAST::NodeType::Subtract, 6},
    {"*"_xiisv, xiiExpressionAST::NodeType::Multiply, 5},
    {"/"_xiisv, xiiExpressionAST::NodeType::Divide, 5},
    {"%"_xiisv, xiiExpressionAST::NodeType::Modulo, 5},
  };

} // namespace

using namespace xiiTokenParseUtils;

xiiExpressionParser::xiiExpressionParser()
{
  RegisterKnownTypes();
  RegisterBuiltinFunctions();
}

xiiExpressionParser::~xiiExpressionParser() = default;

void xiiExpressionParser::RegisterFunction(const xiiExpression::FunctionDesc& funcDesc)
{
  XII_ASSERT_DEV(funcDesc.m_uiNumRequiredInputs <= funcDesc.m_InputTypes.GetCount(), "Not enough input types defined. {} inputs are required but only {} types given.", funcDesc.m_uiNumRequiredInputs, funcDesc.m_InputTypes.GetCount());

  auto& functionDescs = m_FunctionDescs[funcDesc.m_sName];
  if (functionDescs.Contains(funcDesc) == false)
  {
    functionDescs.PushBack(funcDesc);
  }
}

void xiiExpressionParser::UnregisterFunction(const xiiExpression::FunctionDesc& funcDesc)
{
  if (auto pFunctionDescs = m_FunctionDescs.GetValue(funcDesc.m_sName))
  {
    pFunctionDescs->RemoveAndCopy(funcDesc);
  }
}

xiiResult xiiExpressionParser::Parse(xiiStringView sCode, xiiArrayPtr<xiiExpression::StreamDesc> inputs, xiiArrayPtr<xiiExpression::StreamDesc> outputs, const Options& options, xiiExpressionAST& out_ast)
{
  if (sCode.IsEmpty())
    return XII_FAILURE;

  m_Options = options;

  m_pAST = &out_ast;
  SetupInAndOutputs(inputs, outputs);

  xiiTokenizer tokenizer;
  tokenizer.Tokenize(xiiArrayPtr<const xiiUInt8>((const xiiUInt8*)sCode.GetStartPointer(), sCode.GetElementCount()), xiiLog::GetThreadLocalLogSystem());

  xiiUInt32 readTokens = 0;
  while (tokenizer.GetNextLine(readTokens, m_TokenStream).Succeeded())
  {
    m_uiCurrentToken = 0;

    while (m_uiCurrentToken < m_TokenStream.GetCount())
    {
      XII_SUCCEED_OR_RETURN(ParseStatement());

      if (m_uiCurrentToken < m_TokenStream.GetCount() && AcceptStatementTerminator() == false)
      {
        auto pCurrentToken = m_TokenStream[m_uiCurrentToken];
        ReportError(pCurrentToken, xiiFmt("Syntax error, unexpected token '{}'", pCurrentToken->m_DataView));
        return XII_FAILURE;
      }
    }
  }

  XII_SUCCEED_OR_RETURN(CheckOutputs());

  return XII_SUCCESS;
}

void xiiExpressionParser::RegisterKnownTypes()
{
  m_KnownTypes.Insert(xiiMakeHashedString("var"), xiiExpressionAST::DataType::Unknown);

  m_KnownTypes.Insert(xiiMakeHashedString("vec2"), xiiExpressionAST::DataType::Float2);
  m_KnownTypes.Insert(xiiMakeHashedString("vec3"), xiiExpressionAST::DataType::Float3);
  m_KnownTypes.Insert(xiiMakeHashedString("vec4"), xiiExpressionAST::DataType::Float4);

  m_KnownTypes.Insert(xiiMakeHashedString("vec2i"), xiiExpressionAST::DataType::Int2);
  m_KnownTypes.Insert(xiiMakeHashedString("vec3i"), xiiExpressionAST::DataType::Int3);
  m_KnownTypes.Insert(xiiMakeHashedString("vec4i"), xiiExpressionAST::DataType::Int4);

  xiiStringBuilder sTypeName;
  for (xiiUInt32 type = xiiExpressionAST::DataType::Bool; type < xiiExpressionAST::DataType::Count; ++type)
  {
    sTypeName = xiiExpressionAST::DataType::GetName(static_cast<xiiExpressionAST::DataType::Enum>(type));
    sTypeName.ToLower();

    xiiHashedString sTypeNameHashed;
    sTypeNameHashed.Assign(sTypeName);

    m_KnownTypes.Insert(sTypeNameHashed, static_cast<xiiExpressionAST::DataType::Enum>(type));
  }
}

void xiiExpressionParser::RegisterBuiltinFunctions()
{
  // Unary
  m_BuiltinFunctions.Insert(xiiMakeHashedString("abs"), xiiExpressionAST::NodeType::Absolute);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("saturate"), xiiExpressionAST::NodeType::Saturate);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("sqrt"), xiiExpressionAST::NodeType::Sqrt);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("exp"), xiiExpressionAST::NodeType::Exp);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("ln"), xiiExpressionAST::NodeType::Ln);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("log2"), xiiExpressionAST::NodeType::Log2);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("log10"), xiiExpressionAST::NodeType::Log10);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("pow2"), xiiExpressionAST::NodeType::Pow2);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("sin"), xiiExpressionAST::NodeType::Sin);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("cos"), xiiExpressionAST::NodeType::Cos);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("tan"), xiiExpressionAST::NodeType::Tan);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("asin"), xiiExpressionAST::NodeType::ASin);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("acos"), xiiExpressionAST::NodeType::ACos);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("atan"), xiiExpressionAST::NodeType::ATan);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("radToDeg"), xiiExpressionAST::NodeType::RadToDeg);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("rad_to_deg"), xiiExpressionAST::NodeType::RadToDeg);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("degToRad"), xiiExpressionAST::NodeType::DegToRad);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("deg_to_rad"), xiiExpressionAST::NodeType::DegToRad);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("round"), xiiExpressionAST::NodeType::Round);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("floor"), xiiExpressionAST::NodeType::Floor);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("ceil"), xiiExpressionAST::NodeType::Ceil);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("trunc"), xiiExpressionAST::NodeType::Trunc);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("frac"), xiiExpressionAST::NodeType::Frac);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("length"), xiiExpressionAST::NodeType::Length);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("normalize"), xiiExpressionAST::NodeType::Normalize);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("all"), xiiExpressionAST::NodeType::All);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("any"), xiiExpressionAST::NodeType::Any);

  // Binary
  m_BuiltinFunctions.Insert(xiiMakeHashedString("mod"), xiiExpressionAST::NodeType::Modulo);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("log"), xiiExpressionAST::NodeType::Log);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("pow"), xiiExpressionAST::NodeType::Pow);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("min"), xiiExpressionAST::NodeType::Min);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("max"), xiiExpressionAST::NodeType::Max);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("dot"), xiiExpressionAST::NodeType::Dot);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("cross"), xiiExpressionAST::NodeType::Cross);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("reflect"), xiiExpressionAST::NodeType::Reflect);

  // Ternary
  m_BuiltinFunctions.Insert(xiiMakeHashedString("clamp"), xiiExpressionAST::NodeType::Clamp);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("lerp"), xiiExpressionAST::NodeType::Lerp);
}

void xiiExpressionParser::SetupInAndOutputs(xiiArrayPtr<xiiExpression::StreamDesc> inputs, xiiArrayPtr<xiiExpression::StreamDesc> outputs)
{
  m_KnownVariables.Clear();

  for (auto& inputDesc : inputs)
  {
    auto pInput = m_pAST->CreateInput(inputDesc);
    m_KnownVariables.Insert(inputDesc.m_sName, pInput);
  }

  for (auto& outputDesc : outputs)
  {
    auto pOutputNode = m_pAST->CreateOutput(outputDesc, nullptr);
    m_pAST->m_OutputNodes.PushBack(pOutputNode);
    m_KnownVariables.Insert(outputDesc.m_sName, pOutputNode);
  }
}

xiiResult xiiExpressionParser::ParseStatement()
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  if (AcceptStatementTerminator())
  {
    // empty statement
    return XII_SUCCESS;
  }

  if (m_uiCurrentToken >= m_TokenStream.GetCount())
    return XII_FAILURE;

  const xiiToken* pIdentifierToken = m_TokenStream[m_uiCurrentToken];
  if (pIdentifierToken->m_iType != xiiTokenType::Identifier)
  {
    ReportError(pIdentifierToken, "Syntax error, expected type or variable");
  }

  xiiEnum<xiiExpressionAST::DataType> type;
  if (ParseType(pIdentifierToken->m_DataView, type).Succeeded())
  {
    return ParseVariableDefinition(type);
  }

  return ParseAssignment();
}

xiiResult xiiExpressionParser::ParseType(xiiStringView sTypeName, xiiEnum<xiiExpressionAST::DataType>& out_type)
{
  xiiTempHashedString sTypeNameHashed(sTypeName);
  if (m_KnownTypes.TryGetValue(sTypeNameHashed, out_type))
  {
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiExpressionParser::ParseVariableDefinition(xiiEnum<xiiExpressionAST::DataType> type)
{
  // skip type
  XII_SUCCEED_OR_RETURN(Expect(xiiTokenType::Identifier));

  const xiiToken* pIdentifierToken = nullptr;
  XII_SUCCEED_OR_RETURN(Expect(xiiTokenType::Identifier, &pIdentifierToken));

  xiiHashedString sHashedVarName;
  sHashedVarName.Assign(pIdentifierToken->m_DataView);

  xiiExpressionAST::Node* pVariableNode;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pVariableNode))
  {
    const char* szExisting = "a variable";
    if (xiiExpressionAST::NodeType::IsInput(pVariableNode->m_Type))
    {
      szExisting = "an input";
    }
    else if (xiiExpressionAST::NodeType::IsOutput(pVariableNode->m_Type))
    {
      szExisting = "an output";
    }

    ReportError(pIdentifierToken, xiiFmt("Local variable '{}' cannot be defined because {} of the same name already exists", pIdentifierToken->m_DataView, szExisting));
    return XII_FAILURE;
  }

  XII_SUCCEED_OR_RETURN(Expect("="));
  xiiExpressionAST::Node* pExpression = ParseExpression();
  if (pExpression == nullptr)
    return XII_FAILURE;

  m_KnownVariables.Insert(sHashedVarName, EnsureExpectedType(pExpression, type));
  return XII_SUCCESS;
}

xiiResult xiiExpressionParser::ParseAssignment()
{
  const xiiToken* pIdentifierToken = nullptr;
  XII_SUCCEED_OR_RETURN(Expect(xiiTokenType::Identifier, &pIdentifierToken));

  const xiiStringView     sIdentifier = pIdentifierToken->m_DataView;
  xiiExpressionAST::Node* pVarNode    = GetVariable(sIdentifier);
  if (pVarNode == nullptr)
  {
    ReportError(pIdentifierToken, "Syntax error, expected a valid variable");
    return XII_FAILURE;
  }

  xiiStringView sPartialAssignmentMask;
  if (Accept(m_TokenStream, m_uiCurrentToken, "."))
  {
    const xiiToken* pSwizzleToken = nullptr;
    if (Expect(xiiTokenType::Identifier, &pSwizzleToken).Failed())
    {
      ReportError(m_TokenStream[m_uiCurrentToken], "Invalid partial assignment");
      return XII_FAILURE;
    }

    sPartialAssignmentMask = pSwizzleToken->m_DataView;
  }

  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  xiiExpressionAST::NodeType::Enum assignOperator = xiiExpressionAST::NodeType::Invalid;
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(s_assignOperators); ++i)
  {
    auto& op = s_assignOperators[i];
    if (AcceptOperator(op.m_sName))
    {
      assignOperator = op.m_NodeType;
      m_uiCurrentToken += op.m_sName.GetElementCount();
      break;
    }
  }

  if (assignOperator == xiiExpressionAST::NodeType::Invalid)
  {
    XII_SUCCEED_OR_RETURN(Expect("="));
  }

  xiiExpressionAST::Node* pExpression = ParseExpression();
  if (pExpression == nullptr)
    return XII_FAILURE;

  if (assignOperator != xiiExpressionAST::NodeType::Invalid)
  {
    pExpression = m_pAST->CreateBinaryOperator(assignOperator, Unpack(pVarNode), pExpression);
  }

  if (sPartialAssignmentMask.IsEmpty() == false)
  {
    auto pConstructor = m_pAST->CreateConstructorCall(Unpack(pVarNode, false), pExpression, sPartialAssignmentMask);
    if (pConstructor == nullptr)
    {
      ReportError(pIdentifierToken, xiiFmt("Invalid partial assignment .{} = {}", sPartialAssignmentMask, xiiExpressionAST::DataType::GetName(pExpression->m_ReturnType)));
      return XII_FAILURE;
    }

    pExpression = pConstructor;
  }

  if (xiiExpressionAST::NodeType::IsInput(pVarNode->m_Type))
  {
    ReportError(pIdentifierToken, xiiFmt("Input '{}' is not assignable", sIdentifier));
    return XII_FAILURE;
  }
  else if (xiiExpressionAST::NodeType::IsOutput(pVarNode->m_Type))
  {
    auto pOutput           = static_cast<xiiExpressionAST::Output*>(pVarNode);
    pOutput->m_pExpression = pExpression;
    return XII_SUCCESS;
  }

  xiiHashedString sHashedVarName;
  sHashedVarName.Assign(sIdentifier);
  m_KnownVariables[sHashedVarName] = EnsureExpectedType(pExpression, pVarNode->m_ReturnType);
  return XII_SUCCESS;
}

xiiExpressionAST::Node* xiiExpressionParser::ParseFactor()
{
  xiiUInt32 uiIdentifierToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, xiiTokenType::Identifier, &uiIdentifierToken))
  {
    auto                pIdentifierToken = m_TokenStream[uiIdentifierToken];
    const xiiStringView sIdentifier      = pIdentifierToken->m_DataView;

    xiiExpressionAST::Node* pNode = nullptr;

    if (Accept(m_TokenStream, m_uiCurrentToken, "("))
    {
      return ParseSwizzle(ParseFunctionCall(sIdentifier));
    }
    else if (sIdentifier == "true")
    {
      return m_pAST->CreateConstant(true, xiiExpressionAST::DataType::Bool);
    }
    else if (sIdentifier == "false")
    {
      return m_pAST->CreateConstant(false, xiiExpressionAST::DataType::Bool);
    }
    else if (sIdentifier == "PI")
    {
      return m_pAST->CreateConstant(xiiMath::Pi<float>(), xiiExpressionAST::DataType::Float);
    }
    else
    {
      auto pVariable = GetVariable(sIdentifier);
      if (pVariable == nullptr)
      {
        ReportError(pIdentifierToken, xiiFmt("Undeclared identifier '{}'", sIdentifier));
      }
      return ParseSwizzle(Unpack(pVariable));
    }
  }

  xiiUInt32 uiValueToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, xiiTokenType::Integer, &uiValueToken))
  {
    const xiiString sVal = m_TokenStream[uiValueToken]->m_DataView;

    xiiInt64 iConstant = 0;
    if (sVal.StartsWith_NoCase("0x"))
    {
      xiiUInt64 uiHexConstant = 0;
      xiiConversionUtils::ConvertHexStringToUInt64(sVal, uiHexConstant).IgnoreResult();
      iConstant = uiHexConstant;
    }
    else
    {
      xiiConversionUtils::StringToInt64(sVal, iConstant).IgnoreResult();
    }

    return m_pAST->CreateConstant((int)iConstant, xiiExpressionAST::DataType::Int);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, xiiTokenType::Float, &uiValueToken))
  {
    const xiiString sVal = m_TokenStream[uiValueToken]->m_DataView;

    double fConstant = 0;
    xiiConversionUtils::StringToFloat(sVal, fConstant).IgnoreResult();

    return m_pAST->CreateConstant((float)fConstant, xiiExpressionAST::DataType::Float);
  }

  if (Accept(m_TokenStream, m_uiCurrentToken, "("))
  {
    auto pExpression = ParseExpression();
    if (Expect(")").Failed())
      return nullptr;

    return ParseSwizzle(pExpression);
  }

  return nullptr;
}

// Parsing the expression - recursive parser using "precedence climbing".
// http://www.engr.mun.ca/~theo/Misc/exp_parsing.htm
xiiExpressionAST::Node* xiiExpressionParser::ParseExpression(int iPrecedence /* = s_iLowestPrecedence*/)
{
  auto pExpression = ParseUnaryExpression();
  if (pExpression == nullptr)
    return nullptr;

  xiiExpressionAST::NodeType::Enum binaryOp;
  int                              iBinaryOpPrecedence = 0;
  xiiUInt32                        uiOperatorLength    = 0;
  while (AcceptBinaryOperator(binaryOp, iBinaryOpPrecedence, uiOperatorLength) && iBinaryOpPrecedence < iPrecedence)
  {
    // Consume token.
    m_uiCurrentToken += uiOperatorLength;

    auto pSecondOperand = ParseExpression(iBinaryOpPrecedence);
    if (pSecondOperand == nullptr)
      return nullptr;

    if (binaryOp == xiiExpressionAST::NodeType::Select)
    {
      if (Expect(":").Failed())
        return nullptr;

      auto pThirdOperand = ParseExpression(iBinaryOpPrecedence);
      if (pThirdOperand == nullptr)
        return nullptr;

      pExpression = m_pAST->CreateTernaryOperator(xiiExpressionAST::NodeType::Select, pExpression, pSecondOperand, pThirdOperand);
    }
    else
    {
      pExpression = m_pAST->CreateBinaryOperator(binaryOp, pExpression, pSecondOperand);
    }
  }

  return pExpression;
}

xiiExpressionAST::Node* xiiExpressionParser::ParseUnaryExpression()
{
  while (Accept(m_TokenStream, m_uiCurrentToken, "+"))
  {
  }

  if (Accept(m_TokenStream, m_uiCurrentToken, "-"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(xiiExpressionAST::NodeType::Negate, pOperand);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "~"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(xiiExpressionAST::NodeType::BitwiseNot, pOperand);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "!"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(xiiExpressionAST::NodeType::LogicalNot, pOperand);
  }

  return ParseFactor();
}

xiiExpressionAST::Node* xiiExpressionParser::ParseFunctionCall(xiiStringView sFunctionName)
{
  // "(" of the function call
  const xiiToken* pFunctionToken = m_TokenStream[m_uiCurrentToken - 1];

  xiiSmallArray<xiiExpressionAST::Node*, 8> arguments;
  if (Accept(m_TokenStream, m_uiCurrentToken, ")") == false)
  {
    do
    {
      arguments.PushBack(ParseExpression());
    } while (Accept(m_TokenStream, m_uiCurrentToken, ","));

    if (Expect(")").Failed())
      return nullptr;
  }

  auto CheckArgumentCount = [&](xiiUInt32 uiExpectedArgumentCount) -> xiiResult {
    if (arguments.GetCount() != uiExpectedArgumentCount)
    {
      ReportError(pFunctionToken, xiiFmt("Invalid argument count for '{}'. Expected {} but got {}", sFunctionName, uiExpectedArgumentCount, arguments.GetCount()));
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  };

  xiiHashedString sHashedFuncName;
  sHashedFuncName.Assign(sFunctionName);

  xiiEnum<xiiExpressionAST::DataType> dataType;
  if (m_KnownTypes.TryGetValue(sHashedFuncName, dataType))
  {
    xiiUInt32 uiElementCount = xiiExpressionAST::DataType::GetElementCount(dataType);
    if (arguments.GetCount() > uiElementCount)
    {
      ReportError(pFunctionToken, xiiFmt("Invalid argument count for '{}'. Expected 0 - {} but got {}", sFunctionName, uiElementCount, arguments.GetCount()));
      return nullptr;
    }

    return m_pAST->CreateConstructorCall(dataType, arguments);
  }

  xiiEnum<xiiExpressionAST::NodeType> builtinType;
  if (m_BuiltinFunctions.TryGetValue(sHashedFuncName, builtinType))
  {
    if (xiiExpressionAST::NodeType::IsUnary(builtinType))
    {
      if (CheckArgumentCount(1).Failed())
        return nullptr;

      return m_pAST->CreateUnaryOperator(builtinType, arguments[0]);
    }
    else if (xiiExpressionAST::NodeType::IsBinary(builtinType))
    {
      if (CheckArgumentCount(2).Failed())
        return nullptr;

      return m_pAST->CreateBinaryOperator(builtinType, arguments[0], arguments[1]);
    }
    else if (xiiExpressionAST::NodeType::IsTernary(builtinType))
    {
      if (CheckArgumentCount(3).Failed())
        return nullptr;

      return m_pAST->CreateTernaryOperator(builtinType, arguments[0], arguments[1], arguments[2]);
    }

    XII_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  // external function
  const xiiHybridArray<xiiExpression::FunctionDesc, 1>* pFunctionDescs = nullptr;
  if (m_FunctionDescs.TryGetValue(sHashedFuncName, pFunctionDescs))
  {
    xiiUInt32 uiMinArgumentCount = xiiInvalidIndex;
    for (auto& funcDesc : *pFunctionDescs)
    {
      uiMinArgumentCount = xiiMath::Min<xiiUInt32>(uiMinArgumentCount, funcDesc.m_uiNumRequiredInputs);
    }

    if (arguments.GetCount() < uiMinArgumentCount)
    {
      ReportError(pFunctionToken, xiiFmt("Invalid argument count for '{}'. Expected at least {} but got {}", sFunctionName, uiMinArgumentCount, arguments.GetCount()));
      return nullptr;
    }

    return m_pAST->CreateFunctionCall(*pFunctionDescs, arguments);
  }

  ReportError(pFunctionToken, xiiFmt("Undeclared function '{}'", sFunctionName));
  return nullptr;
}

xiiExpressionAST::Node* xiiExpressionParser::ParseSwizzle(xiiExpressionAST::Node* pExpression)
{
  if (Accept(m_TokenStream, m_uiCurrentToken, "."))
  {
    const xiiToken* pSwizzleToken = nullptr;
    if (Expect(xiiTokenType::Identifier, &pSwizzleToken).Failed())
      return nullptr;

    pExpression = m_pAST->CreateSwizzle(pSwizzleToken->m_DataView, pExpression);
    if (pExpression == nullptr)
    {
      ReportError(pSwizzleToken, xiiFmt("Invalid swizzle '{}'", pSwizzleToken->m_DataView));
    }
  }

  return pExpression;
}

// Does NOT advance the current token beyond the operator!
bool xiiExpressionParser::AcceptOperator(xiiStringView sName)
{
  const xiiUInt32 uiOperatorLength = sName.GetElementCount();

  if (m_uiCurrentToken + uiOperatorLength - 1 >= m_TokenStream.GetCount())
    return false;

  for (xiiUInt32 charIndex = 0; charIndex < uiOperatorLength; ++charIndex)
  {
    if (m_TokenStream[m_uiCurrentToken + charIndex]->m_DataView.GetCharacter() != sName.GetStartPointer()[charIndex])
    {
      return false;
    }
  }

  return true;
}

// Does NOT advance the current token beyond the binary operator!
bool xiiExpressionParser::AcceptBinaryOperator(xiiExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence, xiiUInt32& out_uiOperatorLength)
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(s_binaryOperators); ++i)
  {
    auto& op = s_binaryOperators[i];
    if (AcceptOperator(op.m_sName))
    {
      out_binaryOp            = op.m_NodeType;
      out_iOperatorPrecedence = op.m_iPrecedence;
      out_uiOperatorLength    = op.m_sName.GetElementCount();
      return true;
    }
  }

  return false;
}

xiiExpressionAST::Node* xiiExpressionParser::GetVariable(xiiStringView sVarName)
{
  xiiHashedString sHashedVarName;
  sHashedVarName.Assign(sVarName);

  xiiExpressionAST::Node* pVariableNode;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pVariableNode) == false && m_Options.m_bTreatUnknownVariablesAsInputs)
  {
    pVariableNode = m_pAST->CreateInput({sHashedVarName, xiiProcessingStream::DataType::Float});
    m_KnownVariables.Insert(sHashedVarName, pVariableNode);
  }

  return pVariableNode;
}

xiiExpressionAST::Node* xiiExpressionParser::EnsureExpectedType(xiiExpressionAST::Node* pNode, xiiExpressionAST::DataType::Enum expectedType)
{
  if (expectedType != xiiExpressionAST::DataType::Unknown)
  {
    const auto nodeRegisterType     = xiiExpressionAST::DataType::GetRegisterType(pNode->m_ReturnType);
    const auto expectedRegisterType = xiiExpressionAST::DataType::GetRegisterType(expectedType);
    if (nodeRegisterType != expectedRegisterType)
    {
      pNode = m_pAST->CreateUnaryOperator(xiiExpressionAST::NodeType::TypeConversion, pNode, expectedType);
    }

    const xiiUInt32 nodeElementCount     = xiiExpressionAST::DataType::GetElementCount(pNode->m_ReturnType);
    const xiiUInt32 expectedElementCount = xiiExpressionAST::DataType::GetElementCount(expectedType);
    if (nodeElementCount < expectedElementCount)
    {
      pNode = m_pAST->CreateConstructorCall(expectedType, xiiMakeArrayPtr(&pNode, 1));
    }
  }

  return pNode;
}

xiiExpressionAST::Node* xiiExpressionParser::Unpack(xiiExpressionAST::Node* pNode, bool bUnassignedError /*= true*/)
{
  if (xiiExpressionAST::NodeType::IsOutput(pNode->m_Type))
  {
    auto pOutput = static_cast<xiiExpressionAST::Output*>(pNode);
    if (pOutput->m_pExpression == nullptr && bUnassignedError)
    {
      ReportError(m_TokenStream[m_uiCurrentToken], xiiFmt("Output '{}' has not been assigned yet", pOutput->m_Desc.m_sName));
    }

    return pOutput->m_pExpression;
  }

  return pNode;
}

xiiResult xiiExpressionParser::CheckOutputs()
{
  for (auto pOutputNode : m_pAST->m_OutputNodes)
  {
    if (pOutputNode->m_pExpression == nullptr)
    {
      xiiLog::Error("Output '{}' was never written", pOutputNode->m_Desc.m_sName);
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}


XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionParser);
