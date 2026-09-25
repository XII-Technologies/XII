/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

XII_CREATE_SIMPLE_TEST_GROUP(Declarations);

XII_CREATE_SIMPLE_TEST(Declarations, GraphicsTypes)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Value type sizes")
  {
    XII_TEST_INT(xiiGALValueType::GetSize(xiiGALValueType::Int8), 1U);
    XII_TEST_INT(xiiGALValueType::GetSize(xiiGALValueType::UInt8), 1U);
    XII_TEST_INT(xiiGALValueType::GetSize(xiiGALValueType::Int16), 2U);
    XII_TEST_INT(xiiGALValueType::GetSize(xiiGALValueType::UInt16), 2U);
    XII_TEST_INT(xiiGALValueType::GetSize(xiiGALValueType::Float16), 2U);
    XII_TEST_INT(xiiGALValueType::GetSize(xiiGALValueType::Int32), 4U);
    XII_TEST_INT(xiiGALValueType::GetSize(xiiGALValueType::UInt32), 4U);
    XII_TEST_INT(xiiGALValueType::GetSize(xiiGALValueType::Float32), 4U);
    XII_TEST_INT(xiiGALValueType::GetSize(xiiGALValueType::Float64), 8U);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shader stage index round trip")
  {
    for (xiiUInt32 uiStageIndex = 0; uiStageIndex < xiiGALShaderType::ENUM_COUNT; ++uiStageIndex)
    {
      const xiiGALShaderType::Enum stage = xiiGALShaderType::GetStageFlag(uiStageIndex);
      XII_TEST_INT(xiiGALShaderType::GetStageIndex(stage), uiStageIndex);
      XII_TEST_BOOL(stage != xiiGALShaderType::Unknown);
      XII_TEST_BOOL(xiiGALShaderType::Names[uiStageIndex] != nullptr);
      XII_TEST_BOOL(xiiGALShaderType::Names[uiStageIndex][0] != '\0');
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Resource format classification")
  {
    XII_TEST_BOOL(xiiGALResourceFormat::IsDepthFormat(xiiGALResourceFormat::D16UNormalized));
    XII_TEST_BOOL(xiiGALResourceFormat::IsDepthFormat(xiiGALResourceFormat::D32FloatS8X24UInt));
    XII_TEST_BOOL(!xiiGALResourceFormat::IsDepthFormat(xiiGALResourceFormat::RGBA8UNormalized));

    XII_TEST_BOOL(xiiGALResourceFormat::IsStencilFormat(xiiGALResourceFormat::D24UNormalizedS8UInt));
    XII_TEST_BOOL(!xiiGALResourceFormat::IsStencilFormat(xiiGALResourceFormat::D32Float));

    XII_TEST_BOOL(xiiGALResourceFormat::IsTypeless(xiiGALResourceFormat::RGBA32Typeless));
    XII_TEST_BOOL(xiiGALResourceFormat::IsTypeless(xiiGALResourceFormat::BC7Typeless));
    XII_TEST_BOOL(!xiiGALResourceFormat::IsTypeless(xiiGALResourceFormat::RGBA32Float));

    XII_TEST_BOOL(xiiGALResourceFormat::IsMultiplanar(xiiGALResourceFormat::NV12));
    XII_TEST_BOOL(xiiGALResourceFormat::IsMultiplanar(xiiGALResourceFormat::P416));
    XII_TEST_BOOL(!xiiGALResourceFormat::IsMultiplanar(xiiGALResourceFormat::YUY2));

    constexpr xiiGALResourceFormat::Enum linearFormats[] = {
      xiiGALResourceFormat::RGBA8UNormalized,
      xiiGALResourceFormat::BGRA8UNormalized,
      xiiGALResourceFormat::BGRX8UNormalized,
      xiiGALResourceFormat::BC1UNormalized,
      xiiGALResourceFormat::BC2UNormalized,
      xiiGALResourceFormat::BC3UNormalized,
      xiiGALResourceFormat::BC7UNormalized,
    };

    for (const xiiGALResourceFormat::Enum linearFormat : linearFormats)
    {
      const xiiGALResourceFormat::Enum srgbFormat = xiiGALResourceFormat::AsSrgb(linearFormat);
      XII_TEST_BOOL(xiiGALResourceFormat::IsSrgb(srgbFormat));
      XII_TEST_BOOL(xiiGALResourceFormat::AsLinear(srgbFormat) == linearFormat);
    }

    XII_TEST_BOOL(xiiGALResourceFormat::AsLinear(xiiGALResourceFormat::R32Float) == xiiGALResourceFormat::R32Float);
    XII_TEST_BOOL(xiiGALResourceFormat::AsSrgb(xiiGALResourceFormat::R32Float) == xiiGALResourceFormat::R32Float);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Filter classification")
  {
    XII_TEST_BOOL(xiiGALFilterType::IsComparisonFilter(xiiGALFilterType::ComparisonPoint));
    XII_TEST_BOOL(xiiGALFilterType::IsComparisonFilter(xiiGALFilterType::ComparisonLinear));
    XII_TEST_BOOL(!xiiGALFilterType::IsComparisonFilter(xiiGALFilterType::Linear));
    XII_TEST_BOOL(xiiGALFilterType::IsAnisotropicFilter(xiiGALFilterType::Anisotropic));
    XII_TEST_BOOL(xiiGALFilterType::IsAnisotropicFilter(xiiGALFilterType::MaximumAnisotropic));
    XII_TEST_BOOL(!xiiGALFilterType::IsAnisotropicFilter(xiiGALFilterType::MaximumLinear));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Primitive topology counts")
  {
    XII_TEST_INT(xiiGALPrimitiveTopology::VerticesPerPrimitive(xiiGALPrimitiveTopology::PointList), 1U);
    XII_TEST_INT(xiiGALPrimitiveTopology::VerticesPerPrimitive(xiiGALPrimitiveTopology::LineList), 2U);
    XII_TEST_INT(xiiGALPrimitiveTopology::VerticesPerPrimitive(xiiGALPrimitiveTopology::TriangleList), 3U);
    XII_TEST_INT(xiiGALPrimitiveTopology::VerticesPerPrimitive(xiiGALPrimitiveTopology::TriangleListAdjacent), 6U);

    XII_TEST_INT(xiiGALPrimitiveTopology::GetIndexCount(xiiGALPrimitiveTopology::PointList, 7U), 7U);
    XII_TEST_INT(xiiGALPrimitiveTopology::GetIndexCount(xiiGALPrimitiveTopology::LineList, 7U), 14U);
    XII_TEST_INT(xiiGALPrimitiveTopology::GetIndexCount(xiiGALPrimitiveTopology::TriangleList, 7U), 21U);
    XII_TEST_INT(xiiGALPrimitiveTopology::GetIndexCount(xiiGALPrimitiveTopology::TriangleStrip, 7U), 9U);
    XII_TEST_INT(xiiGALPrimitiveTopology::GetIndexCount(xiiGALPrimitiveTopology::TriangleStripAdjacent, 7U), 11U);

    for (xiiUInt32 uiControlPoints = 1; uiControlPoints <= 32; ++uiControlPoints)
    {
      const auto topology = static_cast<xiiGALPrimitiveTopology::Enum>(xiiGALPrimitiveTopology::ControlPointPatchList1 + uiControlPoints - 1U);
      XII_TEST_INT(xiiGALPrimitiveTopology::VerticesPerPrimitive(topology), uiControlPoints);
      XII_TEST_INT(xiiGALPrimitiveTopology::GetIndexCount(topology, 3U), uiControlPoints * 3U);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Modified range accumulation")
  {
    xiiGAL::ModifiedRange range;
    XII_TEST_BOOL(!range.IsValid());

    range.SetToIncludeValue(7U);
    XII_TEST_BOOL(range.IsValid());
    XII_TEST_INT(range.GetCount(), 1U);
    XII_TEST_BOOL(range.HasIncludeValue(7U));
    XII_TEST_BOOL(!range.HasIncludeValue(6U));

    range.SetToIncludeRange(2U, 11U);
    XII_TEST_INT(range.m_uiMin, 2U);
    XII_TEST_INT(range.m_uiMax, 11U);
    XII_TEST_INT(range.GetCount(), 10U);
    XII_TEST_BOOL(range.HasIncludeValue(5U));

    range.Reset();
    XII_TEST_BOOL(!range.IsValid());
  }
}
