/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>

class XII_FOUNDATION_DLL xiiExpressionParser
{
public:
  xiiExpressionParser();
  ~xiiExpressionParser();

  static const xiiHashTable<xiiHashedString, xiiEnum<xiiExpressionAST::DataType>>& GetKnownTypes();
  static const xiiHashTable<xiiHashedString, xiiEnum<xiiExpressionAST::NodeType>>& GetBuiltinFunctions();

  void RegisterFunction(const xiiExpression::FunctionDesc& funcDesc);
  void UnregisterFunction(const xiiExpression::FunctionDesc& funcDesc);

  struct Options
  {
    bool m_bTreatUnknownVariablesAsInputs = false;
  };

  xiiResult Parse(xiiStringView sCode, xiiArrayPtr<xiiExpression::StreamDesc> inputs, xiiArrayPtr<xiiExpression::StreamDesc> outputs, const Options& options, xiiExpressionAST& out_ast);

private:
  static constexpr xiiInt32 s_iLowestPrecedence = 20;

  static void RegisterKnownTypes();
  static void RegisterBuiltinFunctions();
  void        SetupInAndOutputs(xiiArrayPtr<xiiExpression::StreamDesc> inputs, xiiArrayPtr<xiiExpression::StreamDesc> outputs);

  xiiResult ParseStatement();
  xiiResult ParseType(xiiStringView sTypeName, xiiEnum<xiiExpressionAST::DataType>& out_type);
  xiiResult ParseVariableDefinition(xiiEnum<xiiExpressionAST::DataType> type);
  xiiResult ParseAssignment();

  xiiExpressionAST::Node* ParseFactor();
  xiiExpressionAST::Node* ParseExpression(xiiInt32 iPrecedence = s_iLowestPrecedence);
  xiiExpressionAST::Node* ParseUnaryExpression();
  xiiExpressionAST::Node* ParseFunctionCall(xiiStringView sFunctionName);
  xiiExpressionAST::Node* ParseSwizzle(xiiExpressionAST::Node* pExpression);

  bool                    AcceptStatementTerminator();
  bool                    AcceptOperator(xiiStringView sName);
  bool                    AcceptBinaryOperator(xiiExpressionAST::NodeType::Enum& out_binaryOp, xiiInt32& out_iOperatorPrecedence, xiiUInt32& out_uiOperatorLength);
  xiiExpressionAST::Node* GetVariable(xiiStringView sVarName);
  xiiExpressionAST::Node* EnsureExpectedType(xiiExpressionAST::Node* pNode, xiiExpressionAST::DataType::Enum expectedType);
  xiiExpressionAST::Node* Unpack(xiiExpressionAST::Node* pNode, bool bUnassignedError = true);

  xiiResult Expect(xiiStringView sToken, const xiiToken** pExpectedToken = nullptr);
  xiiResult Expect(xiiTokenType::Enum Type, const xiiToken** pExpectedToken = nullptr);

  void ReportError(const xiiToken* pToken, const xiiFormatString& message);

  /// Checks whether all outputs have been written
  xiiResult CheckOutputs();

  Options m_Options;

  xiiTokenParseUtils::TokenStream m_TokenStream;
  xiiUInt32                       m_uiCurrentToken = 0;
  xiiExpressionAST*               m_pAST           = nullptr;

  xiiHashTable<xiiHashedString, xiiExpressionAST::Node*>                        m_KnownVariables;
  xiiHashTable<xiiHashedString, xiiHybridArray<xiiExpression::FunctionDesc, 1>> m_FunctionDescs;
};

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionParser_inl.h>
