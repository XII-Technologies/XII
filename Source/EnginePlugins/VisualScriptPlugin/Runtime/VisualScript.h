#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Types/SharedPtr.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class xiiVisualScriptInstance;

struct XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptNodeDescription
{
  struct XII_VISUALSCRIPTPLUGIN_DLL Type
  {
    using StorageType = xiiUInt8;

    enum Enum
    {
      Invalid,
      EntryCall,
      MessageHandler,
      ReflectedFunction,
      GetScriptOwner,

      FirstBuiltin,

      Builtin_Branch,
      Builtin_And,
      Builtin_Or,
      Builtin_Not,
      Builtin_Compare,
      Builtin_IsValid,

      Builtin_Add,
      Builtin_Subtract,
      Builtin_Multiply,
      Builtin_Divide,

      Builtin_ToBool,
      Builtin_ToByte,
      Builtin_ToInt,
      Builtin_ToInt64,
      Builtin_ToFloat,
      Builtin_ToDouble,
      Builtin_ToString,
      Builtin_ToVariant,
      Builtin_Variant_ConvertTo,

      Builtin_MakeArray,

      Builtin_TryGetComponentOfBaseType,

      LastBuiltin,

      Count,
      Default = Invalid
    };

    XII_ALWAYS_INLINE static bool IsBuiltin(Enum type) { return type > FirstBuiltin && type < LastBuiltin; }

    static Enum GetConversionType(xiiVisualScriptDataType::Enum targetDataType);

    static const char* GetName(Enum type);
  };

  struct DataOffset
  {
    XII_DECLARE_POD_TYPE();

    XII_ALWAYS_INLINE DataOffset()
    {
      m_uiByteOffset = xiiInvalidIndex;
      m_uiDataType   = xiiVisualScriptDataType::Invalid;
      m_uiIsConstant = 0;
    }

    XII_ALWAYS_INLINE DataOffset(xiiUInt32 uiOffset, xiiVisualScriptDataType::Enum dataType, bool bIsConstant)
    {
      m_uiByteOffset = uiOffset;
      m_uiDataType   = dataType;
      m_uiIsConstant = bIsConstant ? 1 : 0;
    }

    XII_ALWAYS_INLINE bool IsValid() const
    {
      return m_uiByteOffset != (XII_BIT(24) - 1) &&
        m_uiDataType != xiiVisualScriptDataType::Invalid;
    }

    xiiUInt32 m_uiByteOffset : 24;
    xiiUInt32 m_uiDataType : 7;
    xiiUInt32 m_uiIsConstant : 1;
  };

  xiiEnum<Type>                    m_Type;
  xiiEnum<xiiVisualScriptDataType> m_DeductedDataType;
  xiiSmallArray<xiiUInt16, 4>      m_ExecutionIndices;
  xiiSmallArray<DataOffset, 4>     m_InputDataOffsets;
  xiiSmallArray<DataOffset, 2>     m_OutputDataOffsets;

  union
  {
    struct
    {
      const xiiRTTI*             m_pTargetType;
      const xiiAbstractProperty* m_pTargetProperty;
    };

    xiiComparisonOperator::Enum m_ComparisonOperator;

    xiiUInt32 m_RawData[4] = {};
  } m_UserData;

  void AppendUserDataName(xiiStringBuilder& out_sResult) const;
};

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptGraphDescription
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiVisualScriptGraphDescription);

public:
  xiiVisualScriptGraphDescription();
  ~xiiVisualScriptGraphDescription();

  static xiiResult Serialize(xiiArrayPtr<const xiiVisualScriptNodeDescription> nodes, xiiStreamWriter& inout_stream);
  xiiResult        Deserialize(xiiStreamReader& inout_stream);

  template <typename T, xiiUInt32 Size>
  struct EmbeddedArrayOrPointer
  {
    union
    {
      T  m_Embedded[Size] = {};
      T* m_Ptr;
    };

    static void AddAdditionalDataSize(xiiArrayPtr<const T> a, xiiUInt32& inout_additionalDataSize);
    static void AddAdditionalDataSize(xiiUInt32 uiSize, xiiUInt32 uiAlignment, xiiUInt32& inout_additionalDataSize);

    T*        Init(xiiUInt8 uiCount, xiiUInt8*& inout_pAdditionalData);
    xiiResult ReadFromStream(xiiUInt8& out_uiCount, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData);
  };

  struct ReturnValue
  {
    enum Enum
    {
      Completed         = 0,
      ContinueNextFrame = -1,

      Error = -100,
    };
  };

  struct Node;
  using ExecuteFunction        = int (*)(xiiVisualScriptInstance& ref_instance, const Node& node);
  using DataOffset             = xiiVisualScriptNodeDescription::DataOffset;
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

    template <typename T>
    const T& GetUserData() const;

    template <typename T>
    void SetUserData(const T& data, xiiUInt8*& inout_pAdditionalData);
  };

  const Node* GetNode(xiiUInt32 uiIndex);

private:
  xiiArrayPtr<const Node> m_Nodes;
  xiiBlob                 m_Storage;
};

struct XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptDataDescription : public xiiRefCounted
{
  using DataOffset = xiiVisualScriptNodeDescription::DataOffset;

  struct OffsetAndCount
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiStartOffset = 0;
    xiiUInt32 m_uiCount       = 0;
  };

  OffsetAndCount m_PerTypeInfo[xiiVisualScriptDataType::Count];
  xiiUInt32      m_uiStorageSizeNeeded = 0;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  void Clear();
  void CalculatePerTypeStartOffsets();
  void CheckOffset(DataOffset dataOffset, const xiiRTTI* pType) const;

  DataOffset GetOffset(xiiVisualScriptDataType::Enum dataType, xiiUInt32 uiIndex, bool bIsConstant) const;
};

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptDataStorage : public xiiRefCounted
{
public:
  using DataOffset = xiiVisualScriptNodeDescription::DataOffset;

  xiiVisualScriptDataStorage(const xiiSharedPtr<const xiiVisualScriptDataDescription>& pDesc);
  ~xiiVisualScriptDataStorage();

  void AllocateStorage();
  void DeallocateStorage();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  xiiTypedPointer GetPointerData(DataOffset dataOffset, xiiUInt32 uiExecutionCounter);

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const xiiRTTI* pType, xiiUInt32 uiExecutionCounter);

  xiiVariant GetDataAsVariant(DataOffset dataOffset, xiiVariantType::Enum expectedType, xiiUInt32 uiExecutionCounter) const;
  void       SetDataFromVariant(DataOffset dataOffset, const xiiVariant& value, xiiUInt32 uiExecutionCounter);

private:
  xiiSharedPtr<const xiiVisualScriptDataDescription> m_pDesc;
  xiiBlob                                            m_Storage;
};

#include <VisualScriptPlugin/Runtime/VisualScript_inl.h>
