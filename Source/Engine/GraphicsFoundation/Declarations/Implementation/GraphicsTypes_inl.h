
// static
XII_ALWAYS_INLINE xiiUInt32 xiiGALValueType::GetSize(const xiiGALValueType::Enum type)
{
  switch (type)
  {
    case xiiGALValueType::Int8:
      return 1U;
    case xiiGALValueType::Int16:
      return 2U;
    case xiiGALValueType::Int32:
      return 4U;
    case xiiGALValueType::UInt8:
      return 1U;
    case xiiGALValueType::UInt16:
      return 2U;
    case xiiGALValueType::UInt32:
      return 4U;
    case xiiGALValueType::Float16:
      return 2U;
    case xiiGALValueType::Float32:
      return 4U;
    case xiiGALValueType::Float64:
      return 8U;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0U;
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiGALShaderType::GetStageIndex(xiiGALShaderType::Enum stage)
{
  switch (stage)
  {
    case xiiGALShaderType::Vertex:
      return 0U;
    case xiiGALShaderType::Pixel:
      return 1U;
    case xiiGALShaderType::Geometry:
      return 2U;
    case xiiGALShaderType::Hull:
      return 3U;
    case xiiGALShaderType::Domain:
      return 4U;
    case xiiGALShaderType::Compute:
      return 5U;
    case xiiGALShaderType::Amplification:
      return 6U;
    case xiiGALShaderType::Mesh:
      return 7U;
    case xiiGALShaderType::RayGeneration:
      return 8U;
    case xiiGALShaderType::RayMiss:
      return 9U;
    case xiiGALShaderType::RayClosestHit:
      return 10U;
    case xiiGALShaderType::RayAnyHit:
      return 11U;
    case xiiGALShaderType::RayIntersection:
      return 12U;
    case xiiGALShaderType::Callable:
      return 13U;
    case xiiGALShaderType::Tile:
      return 14U;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiInvalidIndex;
}

// static
XII_ALWAYS_INLINE xiiGALShaderType::Enum xiiGALShaderType::GetStageFlag(xiiUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0U:
      return xiiGALShaderType::Vertex;
    case 1U:
      return xiiGALShaderType::Pixel;
    case 2U:
      return xiiGALShaderType::Geometry;
    case 3U:
      return xiiGALShaderType::Hull;
    case 4U:
      return xiiGALShaderType::Domain;
    case 5U:
      return xiiGALShaderType::Compute;
    case 6U:
      return xiiGALShaderType::Amplification;
    case 7U:
      return xiiGALShaderType::Mesh;
    case 8U:
      return xiiGALShaderType::RayGeneration;
    case 9U:
      return xiiGALShaderType::RayMiss;
    case 10U:
      return xiiGALShaderType::RayClosestHit;
    case 11U:
      return xiiGALShaderType::RayAnyHit;
    case 12U:
      return xiiGALShaderType::RayIntersection;
    case 13U:
      return xiiGALShaderType::Callable;
    case 14U:
      return xiiGALShaderType::Tile;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALShaderType::Unknown;
}

// static
XII_FORCE_INLINE bool xiiGALTextureFormat::IsDepthFormat(xiiGALTextureFormat::Enum format)
{
  return format == D16UNormalized || format == D24UNormalizedS8UInt || format == D32Float || format == D32FloatS8X24UInt;
}

// static
XII_FORCE_INLINE bool xiiGALTextureFormat::IsStencilFormat(xiiGALTextureFormat::Enum format)
{
  return format == D24UNormalizedS8UInt || format == D32FloatS8X24UInt;
}

// static
XII_FORCE_INLINE bool xiiGALTextureFormat::IsSrgb(xiiGALTextureFormat::Enum format)
{
  return format == RGBA8UNormalizedSRGB || format == BGRX8UNormalizedSRGB || format == BGRA8UNormalizedSRGB || format == BC1UNormalizedSRGB || format == BC2UNormalizedSRGB || format == BC3UNormalizedSRGB || format == BC7UNormalizedSRGB;
}

// static
XII_FORCE_INLINE bool xiiGALFilterType::IsComparisonFilter(xiiGALFilterType::Enum e)
{
  return e == ComparisonPoint || e == ComparisonLinear || e == ComparisonAnisotropic;
}

// static
XII_FORCE_INLINE bool xiiGALFilterType::IsAnisotropicFilter(xiiGALFilterType::Enum e)
{
  return e == Anisotropic || e == ComparisonAnisotropic || e == MinimumAnisotropic || e == MaximumAnisotropic;
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiGALPrimitiveTopology::VerticesPerPrimitive(xiiGALPrimitiveTopology::Enum e)
{
  switch (e)
  {
    case xiiGALPrimitiveTopology::TriangleList:
      return 3;
    case xiiGALPrimitiveTopology::TriangleStrip:
      return 3;
    case xiiGALPrimitiveTopology::PointList:
      return 1;
    case xiiGALPrimitiveTopology::LineList:
      return 2;
    case xiiGALPrimitiveTopology::LineStrip:
      return 2;
    case xiiGALPrimitiveTopology::TriangleListAdjacent:
      return 6;
    case xiiGALPrimitiveTopology::TriangleStripAdjacent:
      return 6;
    case xiiGALPrimitiveTopology::LineListAdjacent:
      return 4;
    case xiiGALPrimitiveTopology::LineStripAdjacent:
      return 4;
    case xiiGALPrimitiveTopology::ControlPointPatchList1:
      return 1;
    case xiiGALPrimitiveTopology::ControlPointPatchList2:
      return 4;
    case xiiGALPrimitiveTopology::ControlPointPatchList3:
      return 9;
    case xiiGALPrimitiveTopology::ControlPointPatchList4:
      return 16;
    case xiiGALPrimitiveTopology::ControlPointPatchList5:
      return 25;
    case xiiGALPrimitiveTopology::ControlPointPatchList6:
      return 36;
    case xiiGALPrimitiveTopology::ControlPointPatchList7:
      return 49;
    case xiiGALPrimitiveTopology::ControlPointPatchList8:
      return 64;
    case xiiGALPrimitiveTopology::ControlPointPatchList9:
      return 81;
    case xiiGALPrimitiveTopology::ControlPointPatchList10:
      return 100;
    case xiiGALPrimitiveTopology::ControlPointPatchList11:
      return 121;
    case xiiGALPrimitiveTopology::ControlPointPatchList12:
      return 144;
    case xiiGALPrimitiveTopology::ControlPointPatchList13:
      return 169;
    case xiiGALPrimitiveTopology::ControlPointPatchList14:
      return 196;
    case xiiGALPrimitiveTopology::ControlPointPatchList15:
      return 225;
    case xiiGALPrimitiveTopology::ControlPointPatchList16:
      return 256;
    case xiiGALPrimitiveTopology::ControlPointPatchList17:
      return 289;
    case xiiGALPrimitiveTopology::ControlPointPatchList18:
      return 324;
    case xiiGALPrimitiveTopology::ControlPointPatchList19:
      return 361;
    case xiiGALPrimitiveTopology::ControlPointPatchList20:
      return 400;
    case xiiGALPrimitiveTopology::ControlPointPatchList21:
      return 441;
    case xiiGALPrimitiveTopology::ControlPointPatchList22:
      return 484;
    case xiiGALPrimitiveTopology::ControlPointPatchList23:
      return 529;
    case xiiGALPrimitiveTopology::ControlPointPatchList24:
      return 576;
    case xiiGALPrimitiveTopology::ControlPointPatchList25:
      return 625;
    case xiiGALPrimitiveTopology::ControlPointPatchList26:
      return 676;
    case xiiGALPrimitiveTopology::ControlPointPatchList27:
      return 729;
    case xiiGALPrimitiveTopology::ControlPointPatchList28:
      return 784;
    case xiiGALPrimitiveTopology::ControlPointPatchList29:
      return 841;
    case xiiGALPrimitiveTopology::ControlPointPatchList30:
      return 900;
    case xiiGALPrimitiveTopology::ControlPointPatchList31:
      return 961;
    case xiiGALPrimitiveTopology::ControlPointPatchList32:
      return 1024;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return 0;
}
