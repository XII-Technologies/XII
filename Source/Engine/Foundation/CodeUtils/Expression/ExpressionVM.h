/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

  struct Flags
  {
    using StorageType = xiiUInt32;

    enum Enum
    {
      MapStreamsByName = XII_BIT(0),
      ScalarizeStreams = XII_BIT(1),

      UserFriendly    = MapStreamsByName | ScalarizeStreams,
      BestPerformance = 0,

      Default = UserFriendly
    };

    struct Bits
    {
      StorageType MapStreamsByName : 1;
      StorageType ScalarizeStreams : 1;
    };
  };

  xiiResult Execute(const xiiExpressionByteCode& byteCode, xiiArrayPtr<const xiiProcessingStream> inputs, xiiArrayPtr<xiiProcessingStream> outputs, xiiUInt32 uiNumInstances, const xiiExpression::GlobalData& globalData = xiiExpression::GlobalData(), xiiBitflags<Flags> flags = Flags::Default);

private:
  void RegisterDefaultFunctions();

  static xiiResult ScalarizeStreams(xiiArrayPtr<const xiiProcessingStream> streams, xiiDynamicArray<xiiProcessingStream>& out_ScalarizedStreams);
  static xiiResult AreStreamsScalarized(xiiArrayPtr<const xiiProcessingStream> streams);
  static xiiResult ValidateStream(const xiiProcessingStream& stream, const xiiExpression::StreamDesc& streamDesc, xiiStringView sStreamType, xiiUInt32 uiNumInstances);

  template <typename T>
  static xiiResult MapStreams(xiiArrayPtr<const xiiExpression::StreamDesc> streamDescs, xiiArrayPtr<T> streams, xiiStringView sStreamType, xiiUInt32 uiNumInstances, xiiBitflags<Flags> flags, xiiDynamicArray<T*>& out_MappedStreams);
  xiiResult        MapFunctions(xiiArrayPtr<const xiiExpression::FunctionDesc> functionDescs, const xiiExpression::GlobalData& globalData);

  xiiDynamicArray<xiiExpression::Register, xiiAlignedAllocatorWrapper> m_Registers;

  xiiDynamicArray<xiiProcessingStream> m_ScalarizedInputs;
  xiiDynamicArray<xiiProcessingStream> m_ScalarizedOutputs;

  xiiDynamicArray<const xiiProcessingStream*>   m_MappedInputs;
  xiiDynamicArray<xiiProcessingStream*>         m_MappedOutputs;
  xiiDynamicArray<const xiiExpressionFunction*> m_MappedFunctions;

  xiiDynamicArray<xiiExpressionFunction>   m_Functions;
  xiiHashTable<xiiHashedString, xiiUInt32> m_FunctionNamesToIndex;
};
