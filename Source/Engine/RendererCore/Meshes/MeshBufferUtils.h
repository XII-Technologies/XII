
#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct xiiMeshBufferResourceDescriptor;

struct xiiMeshNormalPrecision
{
  typedef xiiUInt8 StorageType;

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
  typedef xiiUInt8 StorageType;

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

struct XII_RENDERERCORE_DLL xiiMeshBufferUtils
{
  static xiiResult EncodeNormal(const xiiVec3& normal, xiiArrayPtr<xiiUInt8> dest, xiiMeshNormalPrecision::Enum normalPrecision);
  static xiiResult EncodeTangent(const xiiVec3& tangent, float biTangentSign, xiiArrayPtr<xiiUInt8> dest, xiiMeshNormalPrecision::Enum tangentPrecision);
  static xiiResult EncodeTexCoord(const xiiVec2& texCoord, xiiArrayPtr<xiiUInt8> dest, xiiMeshTexCoordPrecision::Enum texCoordPrecision);

  static xiiResult EncodeNormal(const xiiVec3& normal, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeTangent(const xiiVec3& tangent, float biTangentSign, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeTexCoord(const xiiVec2& texCoord, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);

  static xiiResult DecodeNormal(xiiArrayPtr<const xiiUInt8> source, xiiVec3& destNormal, xiiMeshNormalPrecision::Enum normalPrecision);
  static xiiResult DecodeTangent(
    xiiArrayPtr<const xiiUInt8>  source,
    xiiVec3&                     destTangent,
    float&                       destBiTangentSign,
    xiiMeshNormalPrecision::Enum tangentPrecision);
  static xiiResult DecodeTexCoord(xiiArrayPtr<const xiiUInt8> source, xiiVec2& destTexCoord, xiiMeshTexCoordPrecision::Enum texCoordPrecision);

  static xiiResult DecodeNormal(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec3& destNormal);
  static xiiResult DecodeTangent(
    xiiArrayPtr<const xiiUInt8> source,
    xiiGALResourceFormat::Enum  sourceFormat,
    xiiVec3&                    destTangent,
    float&                      destBiTangentSign);
  static xiiResult DecodeTexCoord(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec2& destTexCoord);

  // low level conversion functions
  static xiiResult EncodeFromFloat(const float source, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeFromVec2(const xiiVec2& source, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeFromVec3(const xiiVec3& source, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);
  static xiiResult EncodeFromVec4(const xiiVec4& source, xiiArrayPtr<xiiUInt8> dest, xiiGALResourceFormat::Enum destFormat);

  static xiiResult DecodeToFloat(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, float& dest);
  static xiiResult DecodeToVec2(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec2& dest);
  static xiiResult DecodeToVec3(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec3& dest);
  static xiiResult DecodeToVec4(xiiArrayPtr<const xiiUInt8> source, xiiGALResourceFormat::Enum sourceFormat, xiiVec4& dest);

  /// \brief Helper function to get the position stream from the given mesh buffer descriptor
  static xiiResult GetPositionStream(const xiiMeshBufferResourceDescriptor& meshBufferDesc, const xiiVec3*& out_pPositions, xiiUInt32& out_uiElementStride);

  /// \brief Helper function to get the position and normal stream from the given mesh buffer descriptor
  static xiiResult GetPositionAndNormalStream(const xiiMeshBufferResourceDescriptor& meshBufferDesc, const xiiVec3*& out_pPositions, const xiiUInt8*& out_pNormals, xiiGALResourceFormat::Enum& out_NormalFormat, xiiUInt32& out_uiElementStride);
};

#include <RendererCore/Meshes/Implementation/MeshBufferUtils_inl.h>
