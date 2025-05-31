#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Types/Delegate.h>

class xiiExpressionByteCode;

class XII_FOUNDATION_DLL xiiExpressionCompiler
{
public:
  xiiExpressionCompiler();
  ~xiiExpressionCompiler();

  xiiResult Compile(xiiExpressionAST& ref_ast, xiiExpressionByteCode& out_byteCode, xiiStringView sDebugAstOutputPath = xiiStringView());

private:
  xiiResult TransformAndOptimizeAST(xiiExpressionAST& ast, xiiStringView sDebugAstOutputPath);
  xiiResult BuildNodeInstructions(const xiiExpressionAST& ast);
  xiiResult UpdateRegisterLifetime();
  xiiResult AssignRegisters();
  xiiResult GenerateByteCode(const xiiExpressionAST& ast, xiiExpressionByteCode& out_byteCode);
  xiiResult GenerateConstantByteCode(const xiiExpressionAST::Constant* pConstant);

  using TransformFunc = xiiDelegate<xiiExpressionAST::Node*(xiiExpressionAST::Node*)>;
  xiiResult TransformASTPreOrder(xiiExpressionAST& ast, TransformFunc func);
  xiiResult TransformASTPostOrder(xiiExpressionAST& ast, TransformFunc func);
  xiiResult TransformNode(xiiExpressionAST::Node*& pNode, TransformFunc& func);
  xiiResult TransformOutputNode(xiiExpressionAST::Output*& pOutputNode, TransformFunc& func);

  void DumpAST(const xiiExpressionAST& ast, xiiStringView sOutputPath, xiiStringView sSuffix);

  xiiHybridArray<xiiExpressionAST::Node*, 64>                    m_NodeStack;
  xiiHybridArray<xiiExpressionAST::Node*, 64>                    m_NodeInstructions;
  xiiHashTable<const xiiExpressionAST::Node*, xiiUInt32>         m_NodeToRegisterIndex;
  xiiHashTable<xiiExpressionAST::Node*, xiiExpressionAST::Node*> m_TransformCache;

  xiiHashTable<xiiHashedString, xiiUInt32> m_InputToIndex;
  xiiHashTable<xiiHashedString, xiiUInt32> m_OutputToIndex;
  xiiHashTable<xiiHashedString, xiiUInt32> m_FunctionToIndex;

  xiiDynamicArray<xiiUInt32> m_ByteCode;

  struct LiveInterval
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32                     m_uiStart;
    xiiUInt32                     m_uiEnd;
    const xiiExpressionAST::Node* m_pNode;
  };

  xiiDynamicArray<LiveInterval> m_LiveIntervals;
};
