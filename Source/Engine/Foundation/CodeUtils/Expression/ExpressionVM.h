#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionFunctions.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

class xiiExpressionByteCode;

class XII_FOUNDATION_DLL xiiExpressionVM
{
public:
  xiiExpressionVM();
  ~xiiExpressionVM();

  void RegisterFunction(const char* szName, xiiExpressionFunction func, xiiExpressionValidateGlobalData validationFunc = xiiExpressionValidateGlobalData());

  void RegisterDefaultFunctions();

  xiiResult Execute(const xiiExpressionByteCode& byteCode, xiiArrayPtr<const xiiProcessingStream> inputs, xiiArrayPtr<xiiProcessingStream> outputs, xiiUInt32 uiNumInstances, const xiiExpression::GlobalData& globalData = xiiExpression::GlobalData());

private:
  void ValidateDataSize(const xiiProcessingStream& stream, xiiUInt32 uiNumInstances, const char* szDataName) const;

  xiiDynamicArray<xiiSimdVec4f, xiiAlignedAllocatorWrapper> m_Registers;

  xiiDynamicArray<xiiUInt32> m_InputMapping;
  xiiDynamicArray<xiiUInt32> m_OutputMapping;
  xiiDynamicArray<xiiUInt32> m_FunctionMapping;

  struct FunctionInfo
  {
    xiiHashedString                 m_sName;
    xiiExpressionFunction           m_Func;
    xiiExpressionValidateGlobalData m_ValidationFunc;
  };

  xiiDynamicArray<FunctionInfo>            m_Functions;
  xiiHashTable<xiiHashedString, xiiUInt32> m_FunctionNamesToIndex;
};
