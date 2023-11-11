
#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsFoundation/Resources/ResourceFormats.h>

struct xiiMeshBufferResourceDescriptor;

struct xiiMeshNormalPrecision
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    _10Bit,
    _16Bit,
    _32Bit,

    Default = _10Bit
  };

  /// \brief Convert mesh normal precision to actual resource format used for normals
  static xiiGALResourceFormat::Enum ToResourceFormatNormal(Enum value);

  /// \brief Convert mesh normal precision to actual resource format used for tangents
  static xiiGALResourceFormat::Enum ToResourceFormatTangent(Enum value);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERCORE_DLL, xiiMeshNormalPrecision);

struct xiiMeshTexCoordPrecision
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    _16Bit,
    _32Bit,

    Default = _16Bit
  };

  /// \brief Convert mesh texcoord precision to actual resource format
  static xiiGALResourceFormat::Enum ToResourceFormat(Enum value);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERCORE_DLL, xiiMeshTexCoordPrecision);

struct xiiMeshBoneWeigthPrecision
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    _8Bit,
    _10Bit,
    _16Bit,
    _32Bit,

    Default = _8Bit
  };

  /// \brief Convert mesh texcoord precision to actual resource format
  static xiiGALResourceFormat::Enum ToResourceFormat(Enum value);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERCORE_DLL, xiiMeshBoneWeigthPrecision);

struct XII_RENDERERCORE_DLL xiiMeshBufferUtils
{
  static xiiResult EncodeNormal(const xiiVec3& vNormal, xiiArrayPtr<xiiUInt8> dest, xiiMeshNormalPrecision::Enum normalPrecision);
  static xiiResult EncodeTangent(const xiiVec3& vTangent, float fTangentSign, xiiArrayPtr<xiiUInt8> dest, xiiMeshNormalPrecision::Enum tangentPrecision);
  static xiiResult EncodeTexCoord(const xiiVec2& vTexCoord, xiiArrayPtr<xiiUInt8> dest, xiiMeshTexCoordPrecision::Enum texCoordPrecision);
  static xiiResult EncodeBoneWeights(const xiiVec4& vWeights, xiiArrayPtr<xiiUInt8> dest, xiiMeshBoneWeigthPrecision::Enum precision);

  static xiiResult EncodeNormal(const xiiVec3& vNormal, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeTangent(const xiiVec3& vTangent, float fTangentSign, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeTexCoord(const xiiVec2& vTexCoord, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeBoneWeights(const xiiVec4& vWeights, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);

  static xiiResult DecodeNormal(xiiArrayPtr<const xiiUInt8> source, xiiVec3& ref_vDestNormal, xiiMeshNormalPrecision::Enum normalPrecision);
  static xiiResult DecodeTangent(
    xiiArrayPtr<const xiiUInt8>  source,
    xiiVec3&                     ref_vDestTangent,
    float&                       ref_fDestBiTangentSign,
    xiiMeshNormalPrecision::Enum tangentPrecision);
  static xiiResult DecodeTexCoord(xiiArrayPtr<const xiiUInt8> source, xiiVec2& ref_vDestTexCoord, xiiMeshTexCoordPrecision::Enum texCoordPrecision);

  static xiiResult DecodeNormal(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec3& ref_vDestNormal);
  static xiiResult DecodeTangent(
    xiiArrayPtr<const xiiUInt8> source,
    xiiGALResourceFormat::Enum  sourceFormat,
    xiiVec3&                    ref_vDestTangent,
    float&                      ref_fDestBiTangentSign);
  static xiiResult DecodeTexCoord(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec2& ref_vDestTexCoord);

  // low level conversion functions
  static xiiResult EncodeFromFloat(const float fSource, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeFromVec2(const xiiVec2& vSource, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeFromVec3(const xiiVec3& vSource, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeFromVec4(const xiiVec4& vSource, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);

  static xiiResult DecodeToFloat(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, float& ref_fDest);
  static xiiResult DecodeToVec2(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec2& ref_vDest);
  static xiiResult DecodeToVec3(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec3& ref_vDest);
  static xiiResult DecodeToVec4(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec4& ref_vDest);

  /// \brief Helper function to get the position stream from the given mesh buffer descriptor
  static xiiResult GetPositionStream(const xiiMeshBufferResourceDescriptor& meshBufferDesc, const xiiVec3*& out_pPositions, xiiUInt32& out_uiElementStride);

  /// \brief Helper function to get the position and normal stream from the given mesh buffer descriptor
  static xiiResult GetPositionAndNormalStream(const xiiMeshBufferResourceDescriptor& meshBufferDesc, const xiiVec3*& out_pPositions, const xiiUInt8*& out_pNormals, xiiGALResourceFormat::Enum& out_normalFormat, xiiUInt32& out_uiElementStride);
};

#include <GraphicsCore/Meshes/Implementation/MeshBufferUtils_inl.h>
