#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMeshNormalPrecision, 1)
  XII_ENUM_CONSTANT(xiiMeshNormalPrecision::_8Bit),
  XII_ENUM_CONSTANT(xiiMeshNormalPrecision::_10Bit),
  XII_ENUM_CONSTANT(xiiMeshNormalPrecision::_16Bit),
  XII_ENUM_CONSTANT(xiiMeshNormalPrecision::_32Bit),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMeshTexCoordPrecision, 1)
  XII_ENUM_CONSTANT(xiiMeshTexCoordPrecision::_16Bit),
  XII_ENUM_CONSTANT(xiiMeshTexCoordPrecision::_32Bit),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMeshBoneWeigthPrecision, 1)
  XII_ENUM_CONSTANT(xiiMeshBoneWeigthPrecision::_8Bit),
  XII_ENUM_CONSTANT(xiiMeshBoneWeigthPrecision::_10Bit),
  XII_ENUM_CONSTANT(xiiMeshBoneWeigthPrecision::_16Bit),
  XII_ENUM_CONSTANT(xiiMeshBoneWeigthPrecision::_32Bit),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMeshVertexColorConversion, 1)
  XII_ENUM_CONSTANT(xiiMeshVertexColorConversion::None),
  XII_ENUM_CONSTANT(xiiMeshVertexColorConversion::LinearToSrgb),
  XII_ENUM_CONSTANT(xiiMeshVertexColorConversion::SrgbToLinear),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
namespace
{
  template <xiiUInt32 Bits>
  XII_ALWAYS_INLINE xiiUInt32 ColorFloatToUNorm(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (xiiMath::IsNaN(value))
    {
      return 0;
    }
    else
    {
      float fMaxValue = ((1 << Bits) - 1);
      return static_cast<xiiUInt32>(xiiMath::Saturate(value) * fMaxValue + 0.5f);
    }
  }

  template <xiiUInt32 Bits>
  constexpr inline float ColorUNormToFloat(xiiUInt32 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    xiiUInt32 uiMaxValue = ((1 << Bits) - 1);
    float     fMaxValue  = ((1 << Bits) - 1);
    return (value & uiMaxValue) * (1.0f / fMaxValue);
  }
} // namespace
// clang-format on

// static
xiiResult xiiMeshBufferUtils::EncodeFromFloat(const float fSource, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALResourceFormat> destFormat)
{
  XII_ASSERT_DEBUG(dest.GetCount() >= xiiGALTextureUtilities::GetResourceFormatProperties(destFormat).GetElementSize(), "Destination buffer is too small");

  switch (destFormat)
  {
    case xiiGALResourceFormat::R32Float:
      *reinterpret_cast<float*>(dest.GetPtr()) = fSource;
      return XII_SUCCESS;
    case xiiGALResourceFormat::R16Float:
      *reinterpret_cast<xiiFloat16*>(dest.GetPtr()) = fSource;
      return XII_SUCCESS;
    default:
      return XII_FAILURE;
  }
}

// static
xiiResult xiiMeshBufferUtils::EncodeFromVec2(const xiiVec2& vSource, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALResourceFormat> destFormat)
{
  XII_ASSERT_DEBUG(dest.GetCount() >= xiiGALTextureUtilities::GetResourceFormatProperties(destFormat).GetElementSize(), "Destination buffer is too small");

  switch (destFormat)
  {
    case xiiGALResourceFormat::RG32Float:
      *reinterpret_cast<xiiVec2*>(dest.GetPtr()) = vSource;
      return XII_SUCCESS;

    case xiiGALResourceFormat::RG16Float:
      *reinterpret_cast<xiiFloat16Vec2*>(dest.GetPtr()) = vSource;
      return XII_SUCCESS;

    default:
      return XII_FAILURE;
  }
}

// static
xiiResult xiiMeshBufferUtils::EncodeFromVec3(const xiiVec3& vSource, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALResourceFormat> destFormat)
{
  XII_ASSERT_DEBUG(dest.GetCount() >= xiiGALTextureUtilities::GetResourceFormatProperties(destFormat).GetElementSize(), "Destination buffer is too small");

  switch (destFormat)
  {
    case xiiGALResourceFormat::RGB32Float:
      *reinterpret_cast<xiiVec3*>(dest.GetPtr()) = vSource;
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA16UNormalized:
      reinterpret_cast<xiiUInt16*>(dest.GetPtr())[0] = xiiMath::ColorFloatToShort(vSource.x);
      reinterpret_cast<xiiUInt16*>(dest.GetPtr())[1] = xiiMath::ColorFloatToShort(vSource.y);
      reinterpret_cast<xiiUInt16*>(dest.GetPtr())[2] = xiiMath::ColorFloatToShort(vSource.z);
      reinterpret_cast<xiiUInt16*>(dest.GetPtr())[3] = 0;
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA16SNormalized:
      reinterpret_cast<xiiInt16*>(dest.GetPtr())[0] = xiiMath::ColorFloatToSignedShort(vSource.x);
      reinterpret_cast<xiiInt16*>(dest.GetPtr())[1] = xiiMath::ColorFloatToSignedShort(vSource.y);
      reinterpret_cast<xiiInt16*>(dest.GetPtr())[2] = xiiMath::ColorFloatToSignedShort(vSource.z);
      reinterpret_cast<xiiInt16*>(dest.GetPtr())[3] = 0;
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGB10A2UNormalized:
      *reinterpret_cast<xiiUInt32*>(dest.GetPtr()) = ColorFloatToUNorm<10>(vSource.x);
      *reinterpret_cast<xiiUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.y) << 10;
      *reinterpret_cast<xiiUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.z) << 20;
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA8UNormalized:
      dest.GetPtr()[0] = xiiMath::ColorFloatToByte(vSource.x);
      dest.GetPtr()[1] = xiiMath::ColorFloatToByte(vSource.y);
      dest.GetPtr()[2] = xiiMath::ColorFloatToByte(vSource.z);
      dest.GetPtr()[3] = 0;
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA8SNormalized:
      dest.GetPtr()[0] = xiiMath::ColorFloatToSignedByte(vSource.x);
      dest.GetPtr()[1] = xiiMath::ColorFloatToSignedByte(vSource.y);
      dest.GetPtr()[2] = xiiMath::ColorFloatToSignedByte(vSource.z);
      dest.GetPtr()[3] = 0;
      return XII_SUCCESS;
    default:
      return XII_FAILURE;
  }
}

// static
xiiResult xiiMeshBufferUtils::EncodeFromVec4(const xiiVec4& vSource, xiiArrayPtr<xiiUInt8> dest, xiiEnum<xiiGALResourceFormat> destFormat)
{
  XII_ASSERT_DEBUG(dest.GetCount() >= xiiGALTextureUtilities::GetResourceFormatProperties(destFormat).GetElementSize(), "Destination buffer is too small");

  switch (destFormat)
  {
    case xiiGALResourceFormat::RGBA32Float:
      *reinterpret_cast<xiiVec4*>(dest.GetPtr()) = vSource;
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA16Float:
      *reinterpret_cast<xiiFloat16Vec4*>(dest.GetPtr()) = vSource;
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA16UNormalized:
      reinterpret_cast<xiiUInt16*>(dest.GetPtr())[0] = xiiMath::ColorFloatToShort(vSource.x);
      reinterpret_cast<xiiUInt16*>(dest.GetPtr())[1] = xiiMath::ColorFloatToShort(vSource.y);
      reinterpret_cast<xiiUInt16*>(dest.GetPtr())[2] = xiiMath::ColorFloatToShort(vSource.z);
      reinterpret_cast<xiiUInt16*>(dest.GetPtr())[3] = xiiMath::ColorFloatToShort(vSource.w);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA16SNormalized:
      reinterpret_cast<xiiInt16*>(dest.GetPtr())[0] = xiiMath::ColorFloatToSignedShort(vSource.x);
      reinterpret_cast<xiiInt16*>(dest.GetPtr())[1] = xiiMath::ColorFloatToSignedShort(vSource.y);
      reinterpret_cast<xiiInt16*>(dest.GetPtr())[2] = xiiMath::ColorFloatToSignedShort(vSource.z);
      reinterpret_cast<xiiInt16*>(dest.GetPtr())[3] = xiiMath::ColorFloatToSignedShort(vSource.w);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGB10A2UNormalized:
      *reinterpret_cast<xiiUInt32*>(dest.GetPtr()) = ColorFloatToUNorm<10>(vSource.x);
      *reinterpret_cast<xiiUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.y) << 10;
      *reinterpret_cast<xiiUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.z) << 20;
      *reinterpret_cast<xiiUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<2>(vSource.w) << 30;
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA8UNormalized:
      dest.GetPtr()[0] = xiiMath::ColorFloatToByte(vSource.x);
      dest.GetPtr()[1] = xiiMath::ColorFloatToByte(vSource.y);
      dest.GetPtr()[2] = xiiMath::ColorFloatToByte(vSource.z);
      dest.GetPtr()[3] = xiiMath::ColorFloatToByte(vSource.w);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA8SNormalized:
      dest.GetPtr()[0] = xiiMath::ColorFloatToSignedByte(vSource.x);
      dest.GetPtr()[1] = xiiMath::ColorFloatToSignedByte(vSource.y);
      dest.GetPtr()[2] = xiiMath::ColorFloatToSignedByte(vSource.z);
      dest.GetPtr()[3] = xiiMath::ColorFloatToSignedByte(vSource.w);
      return XII_SUCCESS;

    default:
      return XII_FAILURE;
  }
}

// static
xiiResult xiiMeshBufferUtils::DecodeToFloat(xiiArrayPtr<const xiiUInt8> source, xiiEnum<xiiGALResourceFormat> sourceFormat, float& ref_fDest)
{
  XII_ASSERT_DEBUG(source.GetCount() >= xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat).GetElementSize(), "Source buffer is too small");

  switch (sourceFormat)
  {
    case xiiGALResourceFormat::R32Float:
      ref_fDest = *reinterpret_cast<const float*>(source.GetPtr());
      return XII_SUCCESS;
    case xiiGALResourceFormat::R16Float:
      ref_fDest = *reinterpret_cast<const xiiFloat16*>(source.GetPtr());
      return XII_SUCCESS;
    default:
      return XII_FAILURE;
  }
}

// static
xiiResult xiiMeshBufferUtils::DecodeToVec2(xiiArrayPtr<const xiiUInt8> source, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiVec2& ref_vDest)
{
  XII_ASSERT_DEBUG(source.GetCount() >= xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat).GetElementSize(), "Source buffer is too small");

  switch (sourceFormat)
  {
    case xiiGALResourceFormat::RG32Float:
      ref_vDest = *reinterpret_cast<const xiiVec2*>(source.GetPtr());
      return XII_SUCCESS;
    case xiiGALResourceFormat::RG16Float:
      ref_vDest = *reinterpret_cast<const xiiFloat16Vec2*>(source.GetPtr());
      return XII_SUCCESS;
    default:
      return XII_FAILURE;
  }
}

// static
xiiResult xiiMeshBufferUtils::DecodeToVec3(xiiArrayPtr<const xiiUInt8> source, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiVec3& ref_vDest)
{
  XII_ASSERT_DEBUG(source.GetCount() >= xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat).GetElementSize(), "Source buffer is too small");

  switch (sourceFormat)
  {
    case xiiGALResourceFormat::RGB32Float:
      ref_vDest = *reinterpret_cast<const xiiVec3*>(source.GetPtr());
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA16UNormalized:
      ref_vDest.x = xiiMath::ColorShortToFloat(reinterpret_cast<const xiiUInt16*>(source.GetPtr())[0]);
      ref_vDest.y = xiiMath::ColorShortToFloat(reinterpret_cast<const xiiUInt16*>(source.GetPtr())[1]);
      ref_vDest.z = xiiMath::ColorShortToFloat(reinterpret_cast<const xiiUInt16*>(source.GetPtr())[2]);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA16SNormalized:
      ref_vDest.x = xiiMath::ColorSignedShortToFloat(reinterpret_cast<const xiiInt16*>(source.GetPtr())[0]);
      ref_vDest.y = xiiMath::ColorSignedShortToFloat(reinterpret_cast<const xiiInt16*>(source.GetPtr())[1]);
      ref_vDest.z = xiiMath::ColorSignedShortToFloat(reinterpret_cast<const xiiInt16*>(source.GetPtr())[2]);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGB10A2UNormalized:
      ref_vDest.x = ColorUNormToFloat<10>(*reinterpret_cast<const xiiUInt32*>(source.GetPtr()));
      ref_vDest.y = ColorUNormToFloat<10>(*reinterpret_cast<const xiiUInt32*>(source.GetPtr()) >> 10);
      ref_vDest.z = ColorUNormToFloat<10>(*reinterpret_cast<const xiiUInt32*>(source.GetPtr()) >> 20);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA8UNormalized:
      ref_vDest.x = xiiMath::ColorByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = xiiMath::ColorByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = xiiMath::ColorByteToFloat(source.GetPtr()[2]);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA8SNormalized:
      ref_vDest.x = xiiMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = xiiMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = xiiMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      return XII_SUCCESS;
    default:
      return XII_FAILURE;
  }
}

// static
xiiResult xiiMeshBufferUtils::DecodeToVec4(xiiArrayPtr<const xiiUInt8> source, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiVec4& ref_vDest)
{
  XII_ASSERT_DEBUG(source.GetCount() >= xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat).GetElementSize(), "Source buffer is too small");

  switch (sourceFormat)
  {
    case xiiGALResourceFormat::RGBA32Float:
      ref_vDest = *reinterpret_cast<const xiiVec4*>(source.GetPtr());
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA16Float:
      ref_vDest = *reinterpret_cast<const xiiFloat16Vec4*>(source.GetPtr());
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA16UNormalized:
      ref_vDest.x = xiiMath::ColorShortToFloat(reinterpret_cast<const xiiUInt16*>(source.GetPtr())[0]);
      ref_vDest.y = xiiMath::ColorShortToFloat(reinterpret_cast<const xiiUInt16*>(source.GetPtr())[1]);
      ref_vDest.z = xiiMath::ColorShortToFloat(reinterpret_cast<const xiiUInt16*>(source.GetPtr())[2]);
      ref_vDest.w = xiiMath::ColorShortToFloat(reinterpret_cast<const xiiUInt16*>(source.GetPtr())[3]);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA16SNormalized:
      ref_vDest.x = xiiMath::ColorSignedShortToFloat(reinterpret_cast<const xiiInt16*>(source.GetPtr())[0]);
      ref_vDest.y = xiiMath::ColorSignedShortToFloat(reinterpret_cast<const xiiInt16*>(source.GetPtr())[1]);
      ref_vDest.z = xiiMath::ColorSignedShortToFloat(reinterpret_cast<const xiiInt16*>(source.GetPtr())[2]);
      ref_vDest.w = xiiMath::ColorSignedShortToFloat(reinterpret_cast<const xiiInt16*>(source.GetPtr())[3]);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGB10A2UNormalized:
      ref_vDest.x = ColorUNormToFloat<10>(*reinterpret_cast<const xiiUInt32*>(source.GetPtr()));
      ref_vDest.y = ColorUNormToFloat<10>(*reinterpret_cast<const xiiUInt32*>(source.GetPtr()) >> 10);
      ref_vDest.z = ColorUNormToFloat<10>(*reinterpret_cast<const xiiUInt32*>(source.GetPtr()) >> 20);
      ref_vDest.w = ColorUNormToFloat<2>(*reinterpret_cast<const xiiUInt32*>(source.GetPtr()) >> 30);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA8UNormalized:
      ref_vDest.x = xiiMath::ColorByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = xiiMath::ColorByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = xiiMath::ColorByteToFloat(source.GetPtr()[2]);
      ref_vDest.w = xiiMath::ColorByteToFloat(source.GetPtr()[3]);
      return XII_SUCCESS;

    case xiiGALResourceFormat::RGBA8SNormalized:
      ref_vDest.x = xiiMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = xiiMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = xiiMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      ref_vDest.w = xiiMath::ColorSignedByteToFloat(source.GetPtr()[3]);
      return XII_SUCCESS;

    default:
      return XII_FAILURE;
  }
}

// static
xiiResult xiiMeshBufferUtils::GetPositionStream(const xiiMeshBufferResourceDescriptor& meshBufferDesc, const xiiVec3*& out_pPositions, xiiUInt32& out_uiElementStride)
{
  const xiiInputLayoutInfo& vdi            = meshBufferDesc.GetInputLayout();
  const xiiUInt8*           pRawVertexData = meshBufferDesc.GetVertexBufferData().GetPtr();

  const xiiVec3* pPositions = nullptr;

  for (xiiUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == xiiGALInputLayoutSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != xiiGALResourceFormat::RGB32Float)
      {
        xiiLog::Error("Unsupported vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return XII_FAILURE; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const xiiVec3*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
  }

  if (pPositions == nullptr)
  {
    xiiLog::Error("No position stream found");
    return XII_FAILURE;
  }

  out_pPositions      = pPositions;
  out_uiElementStride = meshBufferDesc.GetVertexDataSize();
  return XII_SUCCESS;
}

// static
xiiResult xiiMeshBufferUtils::GetPositionAndNormalStream(const xiiMeshBufferResourceDescriptor& meshBufferDesc, const xiiVec3*& out_pPositions, const xiiUInt8*& out_pNormals, xiiEnum<xiiGALResourceFormat>& out_normalFormat, xiiUInt32& out_uiElementStride)
{
  const xiiInputLayoutInfo& vdi            = meshBufferDesc.GetInputLayout();
  const xiiUInt8*           pRawVertexData = meshBufferDesc.GetVertexBufferData().GetPtr();

  const xiiVec3*                pPositions   = nullptr;
  const xiiUInt8*               pNormals     = nullptr;
  xiiEnum<xiiGALResourceFormat> normalFormat = xiiGALResourceFormat::Unknown;

  for (xiiUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == xiiGALInputLayoutSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != xiiGALResourceFormat::RGB32Float)
      {
        xiiLog::Error("Unsupported vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return XII_FAILURE; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const xiiVec3*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == xiiGALInputLayoutSemantic::Normal)
    {
      pNormals     = pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset;
      normalFormat = vdi.m_VertexStreams[vs].m_Format;
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
  {
    xiiLog::Error("No position and normal stream found");
    return XII_FAILURE;
  }

  xiiUInt8 dummySource[16] = {};
  xiiVec3  vNormal;
  if (DecodeNormal(xiiMakeArrayPtr(dummySource), normalFormat, vNormal).Failed())
  {
    xiiLog::Error("Unsupported vertex normal format {0}", normalFormat);
    return XII_FAILURE;
  }

  out_pPositions      = pPositions;
  out_pNormals        = pNormals;
  out_normalFormat    = normalFormat;
  out_uiElementStride = meshBufferDesc.GetVertexDataSize();
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshBufferUtils);
