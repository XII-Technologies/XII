#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Types/Delegate.h>

class xiiExpressionByteCode;

class XII_FOUNDATION_DLL xiiExpressionCompiler
{
public:
  xiiExpressionCompiler();
  ~xiiExpressionCompiler();

  xiiResult Compile(xiiExpressionAST& ast, xiiExpressionByteCode& out_byteCode);

private:
  xiiResult TransformAndOptimizeAST(xiiExpressionAST& ast);
  xiiResult BuildNodeInstructions(const xiiExpressionAST& ast);
  xiiResult UpdateRegisterLifetime(const xiiExpressionAST& ast);
  xiiResult AssignRegisters();
  xiiResult GenerateByteCode(const xiiExpressionAST& ast, xiiExpressionByteCode& out_byteCode);

  using TransformFunc = xiiDelegate<xiiExpressionAST::Node*(xiiExpressionAST::Node*)>;
  xiiResult TransformASTPreOrder(xiiExpressionAST& ast, TransformFunc func);
  xiiResult TransformASTPostOrder(xiiExpressionAST& ast, TransformFunc func);

  xiiHybridArray<xiiExpressionAST::Node*, 64>                    m_NodeStack;
  xiiHybridArray<xiiExpressionAST::Node*, 64>                    m_NodeInstructions;
  xiiHashTable<const xiiExpressionAST::Node*, xiiUInt32>         m_NodeToRegisterIndex;
  xiiHashTable<xiiExpressionAST::Node*, xiiExpressionAST::Node*> m_TransformCache;

  xiiHashTable<xiiHashedString, xiiUInt32> m_InputToIndex;
  xiiHashTable<xiiHashedString, xiiUInt32> m_OutputToIndex;
  xiiHashTable<xiiHashedString, xiiUInt32> m_FunctionToIndex;

  struct LiveInterval
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32                     m_uiStart;
    xiiUInt32                     m_uiEnd;
    const xiiExpressionAST::Node* m_pNode;
  };

  xiiDynamicArray<LiveInterval> m_LiveIntervals;
};
