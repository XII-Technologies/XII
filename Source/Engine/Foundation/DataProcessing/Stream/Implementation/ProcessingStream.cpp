#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

#if XII_ENABLED(XII_PLATFORM_64BIT)
static_assert(sizeof(xiiProcessingStream) == 32);
#endif

xiiProcessingStream::xiiProcessingStream() = default;

xiiProcessingStream::xiiProcessingStream(const xiiHashedString& sName, DataType Type, xiiUInt16 uiStride, xiiUInt16 uiAlignment) :
  m_uiAlignment(uiAlignment), m_uiTypeSize(GetDataTypeSize(Type)), m_uiStride(uiStride), m_Type(Type), m_sName(sName)
{
}

xiiProcessingStream::xiiProcessingStream(const xiiHashedString& sName, xiiArrayPtr<xiiUInt8> data, DataType Type, xiiUInt16 uiStride) :
  m_pData(data.GetPtr()), m_uiDataSize(data.GetCount()), m_uiTypeSize(GetDataTypeSize(Type)), m_uiStride(uiStride), m_Type(Type), m_bExternalMemory(true), m_sName(sName)
{
}

xiiProcessingStream::xiiProcessingStream(const xiiHashedString& sName, xiiArrayPtr<xiiUInt8> data, DataType Type) :
  m_pData(data.GetPtr()), m_uiDataSize(data.GetCount()), m_uiTypeSize(GetDataTypeSize(Type)), m_uiStride(m_uiTypeSize), m_Type(Type), m_bExternalMemory(true), m_sName(sName)
{
}

xiiProcessingStream::~xiiProcessingStream()
{
  FreeData();
}

void xiiProcessingStream::SetSize(xiiUInt64 uiNumElements)
{
  xiiUInt64 uiNewDataSize = uiNumElements * m_uiTypeSize;
  if (m_uiDataSize == uiNewDataSize)
    return;

  FreeData();

  if (uiNewDataSize == 0)
  {
    return;
  }

  /// \todo Allow to reuse memory from a pool ?
  if (m_uiAlignment > 0)
  {
    m_pData = xiiFoundation::GetAlignedAllocator()->Allocate(static_cast<size_t>(uiNewDataSize), static_cast<size_t>(m_uiAlignment));
  }
  else
  {
    m_pData = xiiFoundation::GetDefaultAllocator()->Allocate(static_cast<size_t>(uiNewDataSize), 0);
  }

  XII_ASSERT_DEV(m_pData != nullptr, "Allocating {0} elements of {1} bytes each, with {2} bytes alignment, failed", uiNumElements, ((xiiUInt32)GetDataTypeSize(m_Type)), m_uiAlignment);
  m_uiDataSize = uiNewDataSize;
}

void xiiProcessingStream::FreeData()
{
  if (m_pData != nullptr && m_bExternalMemory == false)
  {
    if (m_uiAlignment > 0)
    {
      xiiFoundation::GetAlignedAllocator()->Deallocate(m_pData);
    }
    else
    {
      xiiFoundation::GetDefaultAllocator()->Deallocate(m_pData);
    }
  }

  m_pData      = nullptr;
  m_uiDataSize = 0;
}

static xiiUInt16 s_TypeSize[] = {
  2, // Half,
  4, // Half2,
  6, // Half3,
  8, // Half4,

  4,  // Float,
  8,  // Float2,
  12, // Float3,
  16, // Float4,

  1, // Byte,
  2, // Byte2,
  3, // Byte3,
  4, // Byte4,

  2, // Short,
  4, // Short2,
  6, // Short3,
  8, // Short4,

  4,  // Int,
  8,  // Int2,
  12, // Int3,
  16, // Int4,
};
static_assert(XII_ARRAY_SIZE(s_TypeSize) == (size_t)xiiProcessingStream::DataType::Count);

// static
xiiUInt16 xiiProcessingStream::GetDataTypeSize(DataType Type)
{
  return s_TypeSize[(xiiUInt32)Type];
}

static xiiStringView s_TypeName[] = {
  "Half"_xiisv,  // Half,
  "Half2"_xiisv, // Half2,
  "Half3"_xiisv, // Half3,
  "Half4"_xiisv, // Half4,

  "Float"_xiisv,  // Float,
  "Float2"_xiisv, // Float2,
  "Float3"_xiisv, // Float3,
  "Float4"_xiisv, // Float4,

  "Byte"_xiisv,  // Byte,
  "Byte2"_xiisv, // Byte2,
  "Byte3"_xiisv, // Byte3,
  "Byte4"_xiisv, // Byte4,

  "Short"_xiisv,  // Short,
  "Short2"_xiisv, // Short2,
  "Short3"_xiisv, // Short3,
  "Short4"_xiisv, // Short4,

  "Int"_xiisv,  // Int,
  "Int2"_xiisv, // Int2,
  "Int3"_xiisv, // Int3,
  "Int4"_xiisv, // Int4,
};
static_assert(XII_ARRAY_SIZE(s_TypeName) == (size_t)xiiProcessingStream::DataType::Count);

// static
xiiStringView xiiProcessingStream::GetDataTypeName(DataType Type)
{
  return s_TypeName[(xiiUInt32)Type];
}

XII_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_Implementation_ProcessingStream);
