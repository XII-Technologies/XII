#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Types/SharedPtr.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

struct XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptDataDescription : public xiiRefCounted
{
  struct DataOffset
  {
    XII_DECLARE_POD_TYPE();

    struct XII_VISUALSCRIPTPLUGIN_DLL Source
    {
      enum Enum
      {
        Local,
        Instance,
        Constant,

        Count
      };

      static const char* GetName(Enum source);
    };

    enum
    {
      BYTE_OFFSET_BITS = 24,
      TYPE_BITS = 6,
      SOURCE_BITS = 2,
      INVALID_BYTE_OFFSET = XII_BIT(BYTE_OFFSET_BITS) - 1
    };

    XII_ALWAYS_INLINE DataOffset()
    {
      m_uiByteOffset = INVALID_BYTE_OFFSET;
      m_uiType = xiiVisualScriptDataType::Invalid;
      m_uiSource = Source::Local;
    }

    XII_ALWAYS_INLINE DataOffset(xiiUInt32 uiOffset, xiiVisualScriptDataType::Enum dataType, Source::Enum source)
    {
      m_uiByteOffset = uiOffset;
      m_uiType = dataType;
      m_uiSource = source;
    }

    XII_ALWAYS_INLINE bool IsValid() const
    {
      return m_uiByteOffset != INVALID_BYTE_OFFSET &&
             m_uiType != xiiVisualScriptDataType::Invalid;
    }

    XII_ALWAYS_INLINE xiiVisualScriptDataType::Enum GetType() const { return static_cast<xiiVisualScriptDataType::Enum>(m_uiType); }
    XII_ALWAYS_INLINE Source::Enum GetSource() const { return static_cast<Source::Enum>(m_uiSource); }
    XII_ALWAYS_INLINE bool IsLocal() const { return m_uiSource == Source::Local; }
    XII_ALWAYS_INLINE bool IsInstance() const { return m_uiSource == Source::Instance; }
    XII_ALWAYS_INLINE bool IsConstant() const { return m_uiSource == Source::Constant; }

    XII_ALWAYS_INLINE xiiResult Serialize(xiiStreamWriter& inout_stream) const { return inout_stream.WriteDWordValue(this); }
    XII_ALWAYS_INLINE xiiResult Deserialize(xiiStreamReader& inout_stream) { return inout_stream.ReadDWordValue(this); }

    xiiUInt32 m_uiByteOffset : BYTE_OFFSET_BITS;
    xiiUInt32 m_uiType : TYPE_BITS;
    xiiUInt32 m_uiSource : SOURCE_BITS;
  };

  struct OffsetAndCount
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiStartOffset = 0;
    xiiUInt32 m_uiCount = 0;
  };

  OffsetAndCount m_PerTypeInfo[xiiVisualScriptDataType::Count];
  xiiUInt32 m_uiStorageSizeNeeded = 0;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  void Clear();
  void CalculatePerTypeStartOffsets();
  void CheckOffset(DataOffset dataOffset, const xiiRTTI* pType) const;

  DataOffset GetOffset(xiiVisualScriptDataType::Enum dataType, xiiUInt32 uiIndex, DataOffset::Source::Enum source) const;
};

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptDataStorage : public xiiRefCounted
{
public:
  xiiVisualScriptDataStorage(const xiiSharedPtr<const xiiVisualScriptDataDescription>& pDesc);
  ~xiiVisualScriptDataStorage();

  bool IsAllocated() const;
  void AllocateStorage();
  void DeallocateStorage();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  using DataOffset = xiiVisualScriptDataDescription::DataOffset;

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  xiiTypedPointer GetPointerData(DataOffset dataOffset, xiiUInt32 uiExecutionCounter) const;

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const xiiRTTI* pType, xiiUInt32 uiExecutionCounter);

  xiiVariant GetDataAsVariant(DataOffset dataOffset, const xiiRTTI* pExpectedType, xiiUInt32 uiExecutionCounter) const;
  void SetDataFromVariant(DataOffset dataOffset, const xiiVariant& value, xiiUInt32 uiExecutionCounter);

private:
  xiiSharedPtr<const xiiVisualScriptDataDescription> m_pDesc;
  xiiBlob m_Storage;
};

struct xiiVisualScriptInstanceData
{
  xiiVisualScriptDataDescription::DataOffset m_DataOffset;
  xiiVariant m_DefaultValue;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

using xiiVisualScriptInstanceDataMapping = xiiRefCountedContainer<xiiHashTable<xiiHashedString, xiiVisualScriptInstanceData>>;

#include <VisualScriptPlugin/Runtime/VisualScriptData_inl.h>
