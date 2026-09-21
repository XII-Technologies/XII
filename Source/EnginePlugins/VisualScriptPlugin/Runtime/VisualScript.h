/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

class xiiVisualScriptInstance;
class xiiVisualScriptExecutionContext;

struct XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptNodeDescription
{
  /// Native node types for visual script graphs.
  /// Editor only types are not supported at runtime and will be replaced by the visual script compiler during asset transform.
  struct XII_VISUALSCRIPTPLUGIN_DLL Type
  {
    using StorageType = xiiUInt8;

    enum Enum
    {
      Invalid,
      EntryCall,
      EntryCall_Coroutine,
      MessageHandler,
      MessageHandler_Coroutine,
      ReflectedFunction,
      GetReflectedProperty,
      SetReflectedProperty,
      InplaceCoroutine,
      GetScriptOwner,
      SendMessage,

      FirstBuiltin,

      Builtin_Constant,    // Editor only
      Builtin_GetVariable, // Editor only
      Builtin_SetVariable,
      Builtin_IncVariable,
      Builtin_DecVariable,
      Builtin_TempVariable,

      Builtin_Branch,
      Builtin_Switch,
      Builtin_WhileLoop,          // Editor only
      Builtin_ForLoop,            // Editor only
      Builtin_ForEachLoop,        // Editor only
      Builtin_ReverseForEachLoop, // Editor only
      Builtin_Break,              // Editor only
      Builtin_Jump,               // Editor only

      Builtin_And,
      Builtin_Or,
      Builtin_Not,
      Builtin_Compare,
      Builtin_CompareExec, // Editor only
      Builtin_IsValid,
      Builtin_Select,

      Builtin_Add,
      Builtin_Subtract,
      Builtin_Multiply,
      Builtin_Divide,
      Builtin_Expression,

      Builtin_ToBool,
      Builtin_ToByte,
      Builtin_ToInt,
      Builtin_ToInt64,
      Builtin_ToFloat,
      Builtin_ToDouble,
      Builtin_ToString,
      Builtin_String_Format,
      Builtin_ToHashedString,
      Builtin_ToVariant,
      Builtin_Variant_ConvertTo,

      Builtin_MakeArray,
      Builtin_Array_GetElement,
      Builtin_Array_SetElement,
      Builtin_Array_GetCount,
      Builtin_Array_IsEmpty,
      Builtin_Array_Clear,
      Builtin_Array_Contains,
      Builtin_Array_IndexOf,
      Builtin_Array_Insert,
      Builtin_Array_PushBack,
      Builtin_Array_PushBackRange,
      Builtin_Array_Remove,
      Builtin_Array_RemoveAt,

      Builtin_TryGetComponentOfBaseType,

      Builtin_StartCoroutine,
      Builtin_StopCoroutine,
      Builtin_StopAllCoroutines,
      Builtin_WaitForAll,
      Builtin_WaitForAny,
      Builtin_Yield,

      LastBuiltin,

      Count,
      Default = Invalid
    };

    XII_ALWAYS_INLINE static bool IsEntry(Enum type) { return type >= EntryCall && type <= MessageHandler_Coroutine; }
    XII_ALWAYS_INLINE static bool IsLoop(Enum type) { return type >= Builtin_WhileLoop && type <= Builtin_ReverseForEachLoop; }

    XII_ALWAYS_INLINE static bool MakesOuterCoroutine(Enum type) { return type == InplaceCoroutine || (type >= Builtin_WaitForAll && type <= Builtin_Yield); }

    XII_ALWAYS_INLINE static bool IsBuiltin(Enum type) { return type > FirstBuiltin && type < LastBuiltin; }

    static Enum GetConversionType(xiiVisualScriptDataType::Enum targetDataType);

    static const char* GetName(Enum type);
  };

  using DataOffset = xiiVisualScriptDataDescription::DataOffset;

  xiiEnum<Type>                    m_Type;
  xiiEnum<xiiVisualScriptDataType> m_DeductedDataType;
  xiiSmallArray<xiiUInt16, 4>      m_ExecutionIndices;
  xiiSmallArray<DataOffset, 4>     m_InputDataOffsets;
  xiiSmallArray<DataOffset, 2>     m_OutputDataOffsets;

  xiiHashedString m_sTargetTypeName;

  xiiVariant m_Value;

  void AppendUserDataName(xiiStringBuilder& out_sResult) const;
};

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptGraphDescription : public xiiRefCounted
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiVisualScriptGraphDescription);

public:
  xiiVisualScriptGraphDescription();
  ~xiiVisualScriptGraphDescription();

  static xiiResult Serialize(xiiArrayPtr<const xiiVisualScriptNodeDescription> nodes, const xiiVisualScriptDataDescription& localDataDesc, xiiStreamWriter& inout_stream);
  xiiResult        Deserialize(xiiStreamReader& inout_stream, const xiiVisualScriptDataDescription& instanceDataDesc, const xiiVisualScriptDataDescription& constantDataDesc);

  template <typename T, xiiUInt32 Size>
  struct EmbeddedArrayOrPointer
  {
    union
    {
      T  m_Embedded[Size] = {};
      T* m_Ptr;
    };

    static void AddAdditionalDataSize(xiiArrayPtr<const T> a, xiiUInt32& inout_uiAdditionalDataSize);
    static void AddAdditionalDataSize(xiiUInt32 uiSize, xiiUInt32 uiAlignment, xiiUInt32& inout_uiAdditionalDataSize);

    T*        Init(xiiUInt8 uiCount, xiiUInt32 uiAlignment, xiiUInt8*& inout_pAdditionalData);
    xiiResult ReadFromStream(xiiUInt8& out_uiCount, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData);
  };

  struct ExecResult
  {
    struct State
    {
      enum Enum
      {
        Completed     = 0,
        ContinueLater = -1,

        Error = -100,
      };
    };

    static XII_ALWAYS_INLINE ExecResult Completed() { return {0}; }
    static XII_ALWAYS_INLINE ExecResult RunNext(int iExecSlot) { return {iExecSlot}; }
    static XII_ALWAYS_INLINE ExecResult ContinueLater(xiiTime maxDelay) { return {State::ContinueLater, maxDelay}; }
    static XII_ALWAYS_INLINE ExecResult Error() { return {State::Error}; }

    int     m_NextExecAndState = 0;
    xiiTime m_MaxDelay         = xiiTime::MakeZero();
  };

  struct Node;
  using ExecuteFunction        = ExecResult (*)(xiiVisualScriptExecutionContext& inout_context, const Node& node);
  using DataOffset             = xiiVisualScriptDataDescription::DataOffset;
  using ExecutionIndicesArray  = EmbeddedArrayOrPointer<xiiUInt16, 4>;
  using InputDataOffsetsArray  = EmbeddedArrayOrPointer<DataOffset, 4>;
  using OutputDataOffsetsArray = EmbeddedArrayOrPointer<DataOffset, 2>;
  using UserDataArray          = EmbeddedArrayOrPointer<xiiUInt32, 4>;

  struct Node
  {
    ExecuteFunction m_Function = nullptr;
#if XII_ENABLED(XII_PLATFORM_32BIT)
    xiiUInt32 m_uiPadding = 0;
#endif

    ExecutionIndicesArray  m_ExecutionIndices;
    InputDataOffsetsArray  m_InputDataOffsets;
    OutputDataOffsetsArray m_OutputDataOffsets;
    UserDataArray          m_UserData;

    xiiEnum<xiiVisualScriptNodeDescription::Type> m_Type;
    xiiUInt8                                      m_NumExecutionIndices;
    xiiUInt8                                      m_NumInputDataOffsets;
    xiiUInt8                                      m_NumOutputDataOffsets;

    xiiUInt16                        m_UserDataByteSize;
    xiiEnum<xiiVisualScriptDataType> m_DeductedDataType;
    xiiUInt8                         m_Reserved = 0;

    xiiUInt32  GetExecutionIndex(xiiUInt32 uiSlot) const;
    DataOffset GetInputDataOffset(xiiUInt32 uiSlot) const;
    DataOffset GetOutputDataOffset(xiiUInt32 uiSlot) const;

    DataOffset* GetInputDataOffsets();
    DataOffset* GetOutputDataOffsets();

    template <typename T>
    static constexpr xiiUInt32 GetUserDataAlignment();

    template <typename T>
    const T& GetUserData() const;

    template <typename T>
    T& InitUserData(xiiUInt8*& inout_pAdditionalData, xiiUInt32 uiByteSize = sizeof(T), xiiUInt32 uiAlignment = GetUserDataAlignment<T>());
  };

  const Node* GetNode(xiiUInt32 uiIndex) const;

  bool                 IsCoroutine() const;
  xiiScriptMessageDesc GetMessageDesc() const;

  const xiiSharedPtr<const xiiVisualScriptDataDescription>& GetLocalDataDesc() const;

private:
  xiiArrayPtr<const Node> m_Nodes;
  xiiBlob                 m_Storage;

  xiiSharedPtr<const xiiVisualScriptDataDescription> m_pLocalDataDesc;
};


class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptExecutionContext
{
public:
  xiiVisualScriptExecutionContext(const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc, xiiAllocator* pAllocator);
  ~xiiVisualScriptExecutionContext();

  void Initialize(xiiVisualScriptInstance& inout_instance, xiiArrayPtr<xiiVariant> arguments);
  void Deinitialize();

  using ExecResult = xiiVisualScriptGraphDescription::ExecResult;
  ExecResult Execute(xiiTime deltaTimeSinceLastExecution);

  xiiVisualScriptInstance& GetInstance() { return *m_pInstance; }

  using DataOffset = xiiVisualScriptDataDescription::DataOffset;

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  xiiTypedPointer GetPointerData(DataOffset dataOffset);

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const xiiRTTI* pType = nullptr);

  xiiVariant GetDataAsVariant(DataOffset dataOffset, const xiiRTTI* pExpectedType) const;
  void       SetDataFromVariant(DataOffset dataOffset, const xiiVariant& value);

  xiiScriptCoroutine* GetCurrentCoroutine() { return m_pCurrentCoroutine; }
  void                SetCurrentCoroutine(xiiScriptCoroutine* pCoroutine);

  xiiTime GetDeltaTimeSinceLastExecution();

private:
  xiiSharedPtr<const xiiVisualScriptGraphDescription> m_pDesc;
  xiiVisualScriptInstance*                            m_pInstance          = nullptr;
  xiiUInt32                                           m_uiCurrentNode      = 0;
  xiiUInt32                                           m_uiExecutionCounter = 0;
  xiiTime                                             m_DeltaTimeSinceLastExecution;

  xiiVisualScriptDataStorage  m_LocalDataStorage;
  xiiVisualScriptDataStorage* m_DataStorage[DataOffset::Source::Count] = {};

  xiiScriptCoroutine* m_pCurrentCoroutine = nullptr;
};

struct xiiVisualScriptSendMessageMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Direct,    ///< Directly send the message to the target game object
    Recursive, ///< Send the message to the target game object and its children
    Event,     ///< Send the message as event. \sa xiiGameObject::SendEventMessage()

    Default = Direct
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_VISUALSCRIPTPLUGIN_DLL, xiiVisualScriptSendMessageMode);

#include <VisualScriptPlugin/Runtime/Implementation/VisualScript_inl.h>
