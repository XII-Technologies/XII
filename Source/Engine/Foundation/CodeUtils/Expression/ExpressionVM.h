#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/Types/UniquePtr.h>

class XII_FOUNDATION_DLL xiiExpressionVM
{
public:
  xiiExpressionVM();
  ~xiiExpressionVM();

  void RegisterFunction(const xiiExpressionFunction& func);
  void UnregisterFunction(const xiiExpressionFunction& func);

  xiiResult Execute(const xiiExpressionByteCode& byteCode, xiiArrayPtr<const xiiProcessingStream> inputs, xiiArrayPtr<xiiProcessingStream> outputs, xiiUInt32 uiNumInstances, const xiiExpression::GlobalData& globalData = xiiExpression::GlobalData());

private:
  void RegisterDefaultFunctions();

  xiiResult ScalarizeStreams(xiiArrayPtr<const xiiProcessingStream> streams, xiiDynamicArray<xiiProcessingStream>& out_ScalarizedStreams);
  xiiResult MapStreams(xiiArrayPtr<const xiiExpression::StreamDesc> streamDescs, xiiArrayPtr<xiiProcessingStream> streams, const char* szStreamType, xiiUInt32 uiNumInstances, xiiDynamicArray<xiiProcessingStream*>& out_MappedStreams);
  xiiResult MapFunctions(xiiArrayPtr<const xiiExpression::FunctionDesc> functionDescs, const xiiExpression::GlobalData& globalData);

  xiiDynamicArray<xiiExpression::Register, xiiAlignedAllocatorWrapper> m_Registers;

  xiiDynamicArray<xiiProcessingStream> m_ScalarizedInputs;
  xiiDynamicArray<xiiProcessingStream> m_ScalarizedOutputs;

  xiiDynamicArray<xiiProcessingStream*>         m_MappedInputs;
  xiiDynamicArray<xiiProcessingStream*>         m_MappedOutputs;
  xiiDynamicArray<const xiiExpressionFunction*> m_MappedFunctions;

  xiiDynamicArray<xiiExpressionFunction>   m_Functions;
  xiiHashTable<xiiHashedString, xiiUInt32> m_FunctionNamesToIndex;
};
