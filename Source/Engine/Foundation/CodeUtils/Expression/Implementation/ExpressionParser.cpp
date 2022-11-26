#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Tokenizer.h>

using namespace xiiTokenParseUtils;

xiiExpressionParser::xiiExpressionParser()
{
  RegisterBuiltinFunctions();
}

xiiExpressionParser::~xiiExpressionParser() = default;

xiiResult xiiExpressionParser::Parse(xiiStringView code, xiiArrayPtr<Stream> inputs, xiiArrayPtr<Stream> outputs, const Options& options, xiiExpressionAST& out_ast)
{
  if (code.IsEmpty())
    return XII_FAILURE;

  m_Options = options;

  m_pAST = &out_ast;
  SetupInAndOutputs(inputs, outputs);

  xiiTokenizer tokenizer;
  tokenizer.Tokenize(xiiArrayPtr<const xiiUInt8>((const xiiUInt8*)code.GetStartPointer(), code.GetElementCount()), xiiLog::GetThreadLocalLogSystem());

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

void xiiExpressionParser::RegisterBuiltinFunctions()
{
  // Unary
  m_BuiltinFunctions.Insert(xiiMakeHashedString("abs"), xiiExpressionAST::NodeType::Absolute);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("saturate"), xiiExpressionAST::NodeType::Saturate);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("sqrt"), xiiExpressionAST::NodeType::Sqrt);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("sin"), xiiExpressionAST::NodeType::Sin);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("cos"), xiiExpressionAST::NodeType::Cos);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("tan"), xiiExpressionAST::NodeType::Tan);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("asin"), xiiExpressionAST::NodeType::ASin);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("acos"), xiiExpressionAST::NodeType::ACos);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("atan"), xiiExpressionAST::NodeType::ATan);

  // Binary
  m_BuiltinFunctions.Insert(xiiMakeHashedString("min"), xiiExpressionAST::NodeType::Min);
  m_BuiltinFunctions.Insert(xiiMakeHashedString("max"), xiiExpressionAST::NodeType::Max);

  // Ternary
  m_BuiltinFunctions.Insert(xiiMakeHashedString("clamp"), xiiExpressionAST::NodeType::Clamp);
}

void xiiExpressionParser::SetupInAndOutputs(xiiArrayPtr<Stream> inputs, xiiArrayPtr<Stream> outputs)
{
  m_KnownVariables.Clear();

  for (auto& input : inputs)
  {
    auto pInputNode = m_pAST->CreateInput(input.m_sName, input.m_DataType);
    m_KnownVariables.Insert(input.m_sName, pInputNode);
  }

  for (auto& output : outputs)
  {
    auto pOutputNode = m_pAST->CreateOutput(output.m_sName, output.m_DataType, nullptr);
    m_KnownVariables.Insert(output.m_sName, pOutputNode);

    m_pAST->m_OutputNodes.PushBack(pOutputNode);
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

  if (ParseType(pIdentifierToken->m_DataView).Succeeded())
  {
    return ParseVariableDefinition();
  }

  return ParseAssignment();
}

xiiResult xiiExpressionParser::ParseType(xiiStringView sTypeName)
{
  if (sTypeName == "var" || sTypeName == "float")
  {
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiExpressionParser::ParseVariableDefinition()
{
  // skip type
  XII_SUCCEED_OR_RETURN(Expect(xiiTokenType::Identifier));

  const xiiToken* pIdentifierToken = nullptr;
  XII_SUCCEED_OR_RETURN(Expect(xiiTokenType::Identifier, &pIdentifierToken));

  xiiHashedString sHashedVarName;
  sHashedVarName.Assign(pIdentifierToken->m_DataView);

  xiiExpressionAST::Node* pNode = nullptr;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pNode))
  {
    const char* szExisting = "a variable";
    if (xiiExpressionAST::NodeType::IsInput(pNode->m_Type))
    {
      szExisting = "an input";
    }
    else if (xiiExpressionAST::NodeType::IsOutput(pNode->m_Type))
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

  m_KnownVariables.Insert(sHashedVarName, pExpression);
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

  xiiExpressionAST::NodeType::Enum assignOperator = xiiExpressionAST::NodeType::Invalid;
  if (Accept(m_TokenStream, m_uiCurrentToken, "+", "="))
  {
    assignOperator = xiiExpressionAST::NodeType::Add;
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "-", "="))
  {
    assignOperator = xiiExpressionAST::NodeType::Subtract;
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "*", "="))
  {
    assignOperator = xiiExpressionAST::NodeType::Multiply;
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "/", "="))
  {
    assignOperator = xiiExpressionAST::NodeType::Divide;
  }
  else
  {
    XII_SUCCEED_OR_RETURN(Expect("="));
  }

  xiiExpressionAST::Node* pExpression = ParseExpression();
  if (pExpression == nullptr)
    return XII_FAILURE;

  if (assignOperator != xiiExpressionAST::NodeType::Invalid)
  {
    pExpression = m_pAST->CreateBinaryOperator(assignOperator, pVarNode, pExpression);
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
  m_KnownVariables.Insert(sHashedVarName, pExpression);
  return XII_SUCCESS;
}

xiiExpressionAST::Node* xiiExpressionParser::ParseFactor()
{
  xiiUInt32 uiIdentifierToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, xiiTokenType::Identifier, &uiIdentifierToken))
  {
    auto                pIdentifierToken = m_TokenStream[uiIdentifierToken];
    const xiiStringView sIdentifier      = pIdentifierToken->m_DataView;

    if (Accept(m_TokenStream, m_uiCurrentToken, "("))
    {
      return ParseFunctionCall(sIdentifier);
    }
    else
    {
      return GetVariable(sIdentifier);
    }
  }

  xiiUInt32 uiValueToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, xiiTokenType::Integer, &uiValueToken) ||
      Accept(m_TokenStream, m_uiCurrentToken, xiiTokenType::Float, &uiValueToken))
  {
    const xiiString sVal = m_TokenStream[uiValueToken]->m_DataView;

    double fConstant = 0;
    xiiConversionUtils::StringToFloat(sVal, fConstant).IgnoreResult();

    return m_pAST->CreateConstant((float)fConstant);
  }

  if (Accept(m_TokenStream, m_uiCurrentToken, "("))
  {
    auto pExpression = ParseExpression();
    if (Expect(")").Failed())
      return nullptr;

    return pExpression;
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
  while (AcceptBinaryOperator(binaryOp, iBinaryOpPrecedence) && iBinaryOpPrecedence < iPrecedence)
  {
    // Consume token.
    ++m_uiCurrentToken;

    auto pRightOperand = ParseExpression(iBinaryOpPrecedence);
    if (pRightOperand == nullptr)
      return nullptr;

    pExpression = m_pAST->CreateBinaryOperator(binaryOp, pExpression, pRightOperand);
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
    return m_pAST->CreateUnaryOperator(xiiExpressionAST::NodeType::Negate, pOperand);
  }

  return ParseFactor();
}

xiiExpressionAST::Node* xiiExpressionParser::ParseFunctionCall(xiiStringView sFunctionName)
{
  // "(" of the function call
  const xiiToken* pFunctionToken = m_TokenStream[m_uiCurrentToken - 1];

  xiiHybridArray<xiiExpressionAST::Node*, 8> arguments;
  if (Accept(m_TokenStream, m_uiCurrentToken, ")") == false)
  {
    do
    {
      arguments.PushBack(ParseExpression());
    } while (Accept(m_TokenStream, m_uiCurrentToken, ","));
  }
  if (Expect(")").Failed())
    return nullptr;

  xiiHashedString sHashedFuncName;
  sHashedFuncName.Assign(sFunctionName);

  xiiEnum<xiiExpressionAST::NodeType> builtinType;
  if (m_BuiltinFunctions.TryGetValue(sHashedFuncName, builtinType))
  {
    auto CheckArgumentCount = [&](xiiUInt32 uiExpectedArgumentCount) -> xiiResult {
      if (arguments.GetCount() != uiExpectedArgumentCount)
      {
        ReportError(pFunctionToken, xiiFmt("Invalid argument count for '{}'. Expected {} but got {}", sFunctionName, uiExpectedArgumentCount, arguments.GetCount()));
        return XII_FAILURE;
      }
      return XII_SUCCESS;
    };

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
  auto pFunctionCall         = m_pAST->CreateFunctionCall(sHashedFuncName);
  pFunctionCall->m_Arguments = std::move(arguments);
  return pFunctionCall;
}

// Does NOT advance the current token beyond the binary operator!
// Operator precedence according to https://en.cppreference.com/w/cpp/language/operator_precedence,
// lower value means higher precedence
bool xiiExpressionParser::AcceptBinaryOperator(xiiExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence)
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  if (m_uiCurrentToken >= m_TokenStream.GetCount())
    return false;

  auto pCurrentToken = m_TokenStream[m_uiCurrentToken];
  if (pCurrentToken->m_DataView.GetElementCount() != 1)
    return false;

  xiiUInt32 operatorChar = pCurrentToken->m_DataView.GetCharacter();

  switch (operatorChar)
  {
    case '+':
      out_binaryOp            = xiiExpressionAST::NodeType::Add;
      out_iOperatorPrecedence = 6;
      break;
    case '-':
      out_binaryOp            = xiiExpressionAST::NodeType::Subtract;
      out_iOperatorPrecedence = 6;
      break;
    case '*':
      out_binaryOp            = xiiExpressionAST::NodeType::Multiply;
      out_iOperatorPrecedence = 5;
      break;
    case '/':
      out_binaryOp            = xiiExpressionAST::NodeType::Divide;
      out_iOperatorPrecedence = 5;
      break;
      // Currently not supported
      /*case '%':
      out_binaryOp = xiiExpressionAST::NodeType::Modulo;
      out_iOperatorPrecedence = 5;
      break;*/

    default:
      return false;
  }

  return true;
}

xiiExpressionAST::Node* xiiExpressionParser::GetVariable(xiiStringView sVarName)
{
  xiiHashedString sHashedVarName;
  sHashedVarName.Assign(sVarName);

  xiiExpressionAST::Node* pNode = nullptr;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pNode) == false && m_Options.m_bTreatUnknownVariablesAsInputs)
  {
    pNode = m_pAST->CreateInput(sHashedVarName, xiiProcessingStream::DataType::Float);
    m_KnownVariables.Insert(sHashedVarName, pNode);
  }

  return pNode;
}

xiiResult xiiExpressionParser::CheckOutputs()
{
  for (auto pOutputNode : m_pAST->m_OutputNodes)
  {
    if (pOutputNode->m_pExpression == nullptr)
    {
      xiiLog::Error("Output '{}' was never written", pOutputNode->m_sName);
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}
