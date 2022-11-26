#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

class XII_FOUNDATION_DLL xiiExpressionParser
{
public:
  xiiExpressionParser();
  ~xiiExpressionParser();

  struct Stream
  {
    Stream(xiiStringView sName, xiiProcessingStream::DataType dataType) :
      m_DataType(dataType)
    {
      m_sName.Assign(sName);
    }

    xiiHashedString               m_sName;
    xiiProcessingStream::DataType m_DataType;
  };

  struct Options
  {
    bool m_bTreatUnknownVariablesAsInputs = false;
  };

  xiiResult Parse(xiiStringView code, xiiArrayPtr<Stream> inputs, xiiArrayPtr<Stream> outputs, const Options& options, xiiExpressionAST& out_ast);

private:
  static constexpr int s_iLowestPrecedence = 20;

  void RegisterBuiltinFunctions();
  void SetupInAndOutputs(xiiArrayPtr<Stream> inputs, xiiArrayPtr<Stream> outputs);

  xiiResult ParseStatement();
  xiiResult ParseType(xiiStringView sTypeName);
  xiiResult ParseVariableDefinition();
  xiiResult ParseAssignment();

  xiiExpressionAST::Node* ParseFactor();
  xiiExpressionAST::Node* ParseExpression(int iPrecedence = s_iLowestPrecedence);
  xiiExpressionAST::Node* ParseUnaryExpression();
  xiiExpressionAST::Node* ParseFunctionCall(xiiStringView sFunctionName);

  bool                    AcceptStatementTerminator();
  bool                    AcceptBinaryOperator(xiiExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence);
  xiiExpressionAST::Node* GetVariable(xiiStringView sVarName);

  xiiResult Expect(const char* szToken, const xiiToken** pExpectedToken = nullptr);
  xiiResult Expect(xiiTokenType::Enum Type, const xiiToken** pExpectedToken = nullptr);

  void ReportError(const xiiToken* pToken, const xiiFormatString& message);

  /// \brief Checks whether all outputs have been written
  xiiResult CheckOutputs();

  Options m_Options;

  xiiTokenParseUtils::TokenStream m_TokenStream;
  xiiUInt32                       m_uiCurrentToken = 0;
  xiiExpressionAST*               m_pAST           = nullptr;

  xiiHashTable<xiiHashedString, xiiExpressionAST::Node*>             m_KnownVariables;
  xiiHashTable<xiiHashedString, xiiEnum<xiiExpressionAST::NodeType>> m_BuiltinFunctions;
};

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionParser_inl.h>
