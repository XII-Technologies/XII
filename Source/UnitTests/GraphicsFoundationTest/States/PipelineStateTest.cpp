/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

XII_CREATE_SIMPLE_TEST_GROUP(States);

XII_CREATE_SIMPLE_TEST(States, PipelineState)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pipeline type classification")
  {
    xiiGALGraphicsPipelineStateCreationDescription graphics;
    XII_TEST_BOOL(graphics.m_PipelineType == xiiGALPipelineType::Graphics);
    XII_TEST_BOOL(graphics.IsAnyGraphicsPipeline());
    XII_TEST_BOOL(!graphics.IsComputePipeline());
    XII_TEST_BOOL(!graphics.IsRayTracingPipeline());
    XII_TEST_BOOL(!graphics.IsTilePipeline());

    graphics.m_PipelineType = xiiGALPipelineType::Mesh;
    XII_TEST_BOOL(graphics.IsAnyGraphicsPipeline());

    xiiGALComputePipelineStateCreationDescription compute;
    XII_TEST_BOOL(compute.m_PipelineType == xiiGALPipelineType::Compute);
    XII_TEST_BOOL(compute.IsComputePipeline());
    XII_TEST_BOOL(!compute.IsAnyGraphicsPipeline());

    xiiGALRayTracingPipelineStateCreationDescription rayTracing;
    XII_TEST_BOOL(rayTracing.m_PipelineType == xiiGALPipelineType::RayTracing);
    XII_TEST_BOOL(rayTracing.IsRayTracingPipeline());

    xiiGALTilePipelineStateCreationDescription tile;
    XII_TEST_BOOL(tile.m_PipelineType == xiiGALPipelineType::Tile);
    XII_TEST_BOOL(tile.IsTilePipeline());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Graphics pipeline descriptor hashing")
  {
    xiiGALGraphicsPipelineStateCreationDescription a;
    xiiGALGraphicsPipelineStateCreationDescription b = a;
    XII_TEST_BOOL(xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_INT(xiiGALDescriptorHash::Hash(a), xiiGALDescriptorHash::Hash(b));

    b.m_GraphicsPipeline.m_uiSampleMask = 0x0F0F0F0FU;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    b = a;
    b.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::LineList;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    b = a;
    b.m_GraphicsPipeline.m_uiViewportCount = 2U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    b = a;
    b.m_GraphicsPipeline.m_uiSubpassIndex = 1U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    b = a;
    b.m_GraphicsPipeline.m_SampleDescription.m_uiCount = 4U;
    b.m_GraphicsPipeline.m_SampleDescription.m_uiQuality = 2U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    b = a;
    b.m_GraphicsPipeline.m_ShadingRateFlags = xiiGALPipelineShadingRateFlags::TextureBased;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compute, ray tracing, and tile descriptor hashing")
  {
    xiiGALComputePipelineStateCreationDescription computeA;
    xiiGALComputePipelineStateCreationDescription computeB = computeA;
    XII_TEST_BOOL(xiiGALDescriptorHash::Equal(computeA, computeB));
    XII_TEST_INT(xiiGALDescriptorHash::Hash(computeA), xiiGALDescriptorHash::Hash(computeB));
    computeB.m_PipelineType = xiiGALPipelineType::Tile;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(computeA, computeB));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(computeA) != xiiGALDescriptorHash::Hash(computeB));

    xiiGALRayTracingPipelineStateCreationDescription rayA;
    rayA.m_sShaderRecordName.Assign("LocalConstants");
    rayA.m_RayTracingPipeline.m_uiShaderRecordSize  = 32U;
    rayA.m_RayTracingPipeline.m_uiMaxRecursionDepth = 2U;
    rayA.m_uiMaximumAttributeSize                    = 8U;
    rayA.m_uiMaximumPayloadSize                      = 24U;
    xiiGALRayTracingPipelineStateCreationDescription rayB = rayA;
    XII_TEST_BOOL(xiiGALDescriptorHash::Equal(rayA, rayB));
    XII_TEST_INT(xiiGALDescriptorHash::Hash(rayA), xiiGALDescriptorHash::Hash(rayB));

    rayB.m_RayTracingPipeline.m_uiMaxRecursionDepth = 3U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(rayA, rayB));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(rayA) != xiiGALDescriptorHash::Hash(rayB));

    rayB = rayA;
    rayB.m_uiMaximumPayloadSize = 32U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(rayA, rayB));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(rayA) != xiiGALDescriptorHash::Hash(rayB));

    xiiGALTilePipelineStateCreationDescription tileA;
    tileA.m_TilePipeline.m_SampleCount = xiiGALSampleCount::FourSamples;
    tileA.m_TilePipeline.m_RenderTargetFormats.PushBack(xiiGALResourceFormat::RGBA8UNormalized);
    xiiGALTilePipelineStateCreationDescription tileB = tileA;
    XII_TEST_BOOL(xiiGALDescriptorHash::Equal(tileA, tileB));
    XII_TEST_INT(xiiGALDescriptorHash::Hash(tileA), xiiGALDescriptorHash::Hash(tileB));

    tileB.m_TilePipeline.m_RenderTargetFormats[0] = xiiGALResourceFormat::BGRA8UNormalized;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(tileA, tileB));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(tileA) != xiiGALDescriptorHash::Hash(tileB));
  }
}
