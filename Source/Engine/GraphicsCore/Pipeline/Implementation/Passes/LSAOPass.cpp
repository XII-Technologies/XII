#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/Passes/LSAOPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Profiling/Profiling.h>
#include <GraphicsFoundation/Resources/Buffer.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiLSAODepthCompareFunction, 1)
  XII_ENUM_CONSTANT(xiiLSAODepthCompareFunction::Depth),
  XII_ENUM_CONSTANT(xiiLSAODepthCompareFunction::Normal),
  XII_ENUM_CONSTANT(xiiLSAODepthCompareFunction::NormalAndSampleDistance),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLSAOPass, 1, xiiRTTIDefaultAllocator<xiiLSAOPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth", m_PinDepthInput),
    XII_MEMBER_PROPERTY("AmbientObscurance", m_PinOutput),
    XII_ACCESSOR_PROPERTY("LineToLineDistance", GetLineToLinePixelOffset, SetLineToLinePixelOffset)->AddAttributes(new xiiDefaultValueAttribute(2), new xiiClampValueAttribute(1, 20)),
    XII_ACCESSOR_PROPERTY("LineSampleDistanceFactor", GetLineSamplePixelOffset, SetLineSamplePixelOffset)->AddAttributes(new xiiDefaultValueAttribute(1), new xiiClampValueAttribute(1, 10)),
    XII_ACCESSOR_PROPERTY("OcclusionFalloff", GetOcclusionFalloff, SetOcclusionFalloff)->AddAttributes(new xiiDefaultValueAttribute(0.2f), new xiiClampValueAttribute(0.01f, 2.0f)),
    XII_ENUM_MEMBER_PROPERTY("DepthCompareFunction", xiiLSAODepthCompareFunction, m_DepthCompareFunction),
    XII_ACCESSOR_PROPERTY("DepthCutoffDistance", GetDepthCutoffDistance, SetDepthCutoffDistance)->AddAttributes(new xiiDefaultValueAttribute(4.0f), new xiiClampValueAttribute(0.1f, 100.0f)),
    XII_MEMBER_PROPERTY("DistributedGathering", m_bDistributedGathering)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  float HaltonSequence(int iBase, int j)
  {
    static int primes[61] = {
      2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283};

    XII_ASSERT_DEV(iBase < 61, "Don't have prime number for this base.");

    // Halton sequence with reverse permutation
    const xiiInt32 p   = primes[iBase];
    float          h   = 0.0f;
    float          f   = 1.0f / static_cast<float>(p);
    float          fct = f;
    while (j > 0)
    {
      xiiInt32 i = j % p;
      h += (i == 0 ? i : p - i) * fct;
      j /= p;
      fct *= f;
    }
    return h;
  }
} // namespace

xiiLSAOPass::xiiLSAOPass() :
  xiiRenderPipelinePass("LSAOPass", true)
{
  {
    // Load shader.
    m_hShaderLineSweep = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/LSAOSweep.xiiShader");
    XII_ASSERT_DEV(m_hShaderLineSweep.IsValid(), "Could not lsao sweep shader!");
    m_hShaderGather = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/LSAOGather.xiiShader");
    XII_ASSERT_DEV(m_hShaderGather.IsValid(), "Could not lsao gather shader!");
    m_hShaderAverage = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/LSAOAverage.xiiShader");
    XII_ASSERT_DEV(m_hShaderGather.IsValid(), "Could not lsao average shader!");
  }

  {
    m_hLineSweepCB = xiiRenderContext::CreateConstantBufferStorage<xiiLSAOConstants>();
  }
}

xiiLSAOPass::~xiiLSAOPass()
{
  DestroyLineSweepData();

  xiiRenderContext::DeleteConstantBufferStorage(m_hLineSweepCB);
  m_hLineSweepCB.Invalidate();
}

bool xiiLSAOPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  XII_ASSERT_DEBUG(inputs.GetCount() == 1, "Unexpected number of inputs for xiiScreenSpaceAmbientOcclusionPass.");

  // Depth
  if (!inputs[m_PinDepthInput.m_uiInputIndex])
  {
    xiiLog::Error("No depth input connected to ssao pass!");
    return false;
  }
  if (!inputs[m_PinDepthInput.m_uiInputIndex]->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
  {
    xiiLog::Error("All ssao pass inputs must be bound with xiiGALBindFlags::ShaderResource.");
    return false;
  }
  if (inputs[m_PinDepthInput.m_uiInputIndex]->m_uiSampleCount != 1)
  {
    xiiLog::Error("'{0}' input must be resolved", GetName());
    return false;
  }

  // Output format matches input format but is f16.
  outputs[m_PinOutput.m_uiOutputIndex]          = *inputs[m_PinDepthInput.m_uiInputIndex];
  outputs[m_PinOutput.m_uiOutputIndex].m_Format = xiiGALTextureFormat::RG16Float;

  return true;
}

void xiiLSAOPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  // Todo: Support half resolution.
  const xiiGALTextureCreationDescription& desc = inputs[m_PinDepthInput.m_uiInputIndex]->m_Desc;
  SetupLineSweepData(xiiVec3I32(desc.m_Size.width, desc.m_Size.height, desc.m_uiArraySizeOrDepth));
}

void xiiLSAOPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  if (m_bConstantsDirty)
  {
    xiiLSAOConstants* cb    = xiiRenderContext::GetConstantBufferData<xiiLSAOConstants>(m_hLineSweepCB);
    cb->DepthCutoffDistance = m_fDepthCutoffDistance;
    cb->OcclusionFalloff    = m_fOcclusionFalloff;
  }

  if (m_bSweepDataDirty)
  {
    const xiiGALTextureCreationDescription& desc = inputs[m_PinDepthInput.m_uiInputIndex]->m_Desc;
    SetupLineSweepData(xiiVec3I32(desc.m_Size.width, desc.m_Size.height, desc.m_uiArraySizeOrDepth));
  }
  if (outputs[m_PinOutput.m_uiOutputIndex] == nullptr)
    return;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALRenderingSetup renderingSetup;
  xiiGALTextureHandle  tempTexture;
  if (m_bDistributedGathering)
  {
    xiiGALTextureCreationDescription tempTextureDesc = outputs[m_PinOutput.m_uiOutputIndex]->m_Desc;
    tempTextureDesc.m_BindFlags.Add(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget);
    tempTexture = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempTextureDesc);
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(tempTexture)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }
  else
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }

  // Line Sweep part (compute)
  {
    XII_PROFILE_SCOPE("Line Sweep");
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginComputeScope(renderViewContext, "Line Sweep");
    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiLSAOConstants", m_hLineSweepCB);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", pDevice->GetTexture(inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    renderViewContext.m_pRenderContext->BindShader(m_hShaderLineSweep);
    renderViewContext.m_pRenderContext->BindBuffer("LineInstructions", m_hLineSweepInfoSRV);
    renderViewContext.m_pRenderContext->BindBufferUAV("LineSweepOutputBuffer", m_hLineSweepOutputUAV);

    const xiiUInt32 dispatchSize        = m_uiNumSweepLines / SSAO_LINESWEEP_THREAD_GROUP + (m_uiNumSweepLines % SSAO_LINESWEEP_THREAD_GROUP != 0 ? 1 : 0);
    const xiiUInt32 uiRenderedInstances = renderViewContext.m_pCamera->IsStereoscopic() ? 2 : 1;
    renderViewContext.m_pRenderContext->Dispatch(dispatchSize, uiRenderedInstances).IgnoreResult();
  }

  // Gather samples.
  {
    XII_PROFILE_SCOPE("Gather");
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(renderViewContext, renderingSetup, "Gather Samples", renderViewContext.m_pCamera->IsStereoscopic());

    if (m_bDistributedGathering)
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DISTRIBUTED_SSAO_GATHERING", "TRUE");
    else
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DISTRIBUTED_SSAO_GATHERING", "FALSE");

    switch (m_DepthCompareFunction)
    {
      case xiiLSAODepthCompareFunction::Depth:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_DEPTH");
        break;
      case xiiLSAODepthCompareFunction::Normal:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL");
        break;
      case xiiLSAODepthCompareFunction::NormalAndSampleDistance:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL_AND_SAMPLE_DISTANCE");
        break;
    }

    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiLSAOConstants", m_hLineSweepCB);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", pDevice->GetTexture(inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    renderViewContext.m_pRenderContext->BindShader(m_hShaderGather);
    renderViewContext.m_pRenderContext->BindBuffer("LineInstructions", m_hLineSweepInfoSRV);
    renderViewContext.m_pRenderContext->BindBuffer("LineSweepOutputBuffer", m_hLineSweepOutputSRV);
    renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // If enabled, average distributed gather samples and write to output.
  if (m_bDistributedGathering)
  {
    XII_PROFILE_SCOPE("Averaging");

    switch (m_DepthCompareFunction)
    {
      case xiiLSAODepthCompareFunction::Depth:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_DEPTH");
        break;
      case xiiLSAODepthCompareFunction::Normal:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL");
        break;
      case xiiLSAODepthCompareFunction::NormalAndSampleDistance:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL_AND_SAMPLE_DISTANCE");
        break;
    }

    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::RenderTarget));

    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(renderViewContext, renderingSetup, "Averaging", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindConstantBuffer("xiiLSAOConstants", m_hLineSweepCB);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", pDevice->GetTexture(inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    renderViewContext.m_pRenderContext->BindShader(m_hShaderAverage);
    renderViewContext.m_pRenderContext->BindTexture2D("SSAOGatherOutput", pDevice->GetTexture(tempTexture)->GetDefaultView(xiiGALTextureViewType::ShaderResource));

    renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    // Give back temp texture.
    xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempTexture);
  }
}

void xiiLSAOPass::ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(pOutput->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor              = xiiColor::White;

  auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, "Clear");
}

xiiResult xiiLSAOPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_iLineToLinePixelOffset;
  inout_stream << m_iLineSamplePixelOffsetFactor;
  inout_stream << m_fOcclusionFalloff;
  inout_stream << m_fDepthCutoffDistance;
  inout_stream << m_DepthCompareFunction;
  inout_stream << m_bDistributedGathering;
  return XII_SUCCESS;
}

xiiResult xiiLSAOPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_iLineToLinePixelOffset;
  inout_stream >> m_iLineSamplePixelOffsetFactor;
  inout_stream >> m_fOcclusionFalloff;
  inout_stream >> m_fDepthCutoffDistance;
  inout_stream >> m_DepthCompareFunction;
  inout_stream >> m_bDistributedGathering;
  return XII_SUCCESS;
}

void xiiLSAOPass::SetLineToLinePixelOffset(xiiUInt32 uiPixelOffset)
{
  m_iLineToLinePixelOffset = uiPixelOffset;
  m_bSweepDataDirty        = true;
}

void xiiLSAOPass::SetLineSamplePixelOffset(xiiUInt32 uiPixelOffset)
{
  m_iLineSamplePixelOffsetFactor = uiPixelOffset;
  m_bSweepDataDirty              = true;
}

float xiiLSAOPass::GetDepthCutoffDistance() const
{
  return m_fDepthCutoffDistance;
}

void xiiLSAOPass::SetDepthCutoffDistance(float fDepthCutoffDistance)
{
  m_fDepthCutoffDistance = fDepthCutoffDistance;
  m_bConstantsDirty      = true;
}

float xiiLSAOPass::GetOcclusionFalloff() const
{
  return m_fOcclusionFalloff;
}

void xiiLSAOPass::SetOcclusionFalloff(float fFalloff)
{
  m_fOcclusionFalloff = fFalloff;
  m_bConstantsDirty   = true;
}

void xiiLSAOPass::DestroyLineSweepData()
{
  xiiGALDevice* device = xiiGALDevice::GetDefaultDevice();

  if (!m_hLineSweepOutputUAV.IsInvalidated())
    device->DestroyBufferView(m_hLineSweepOutputUAV);
  m_hLineSweepOutputUAV.Invalidate();

  if (!m_hLineSweepOutputSRV.IsInvalidated())
    device->DestroyBufferView(m_hLineSweepOutputSRV);
  m_hLineSweepOutputSRV.Invalidate();

  if (!m_hLineSweepOutputBuffer.IsInvalidated())
    device->DestroyBuffer(m_hLineSweepOutputBuffer);
  m_hLineSweepOutputBuffer.Invalidate();

  if (!m_hLineInfoBuffer.IsInvalidated())
    device->DestroyBuffer(m_hLineInfoBuffer);
  m_hLineInfoBuffer.Invalidate();
}

void xiiLSAOPass::SetupLineSweepData(const xiiVec3I32& imageResolution)
{
  // imageResolution.z defines the number of render layers (1 for mono, 2 for stereo rendering).
  DestroyLineSweepData();

  xiiDynamicArray<LineInstruction> lineInstructions;
  xiiUInt32                        totalNumberOfSamples = 0;
  xiiLSAOConstants*                cb                   = xiiRenderContext::GetConstantBufferData<xiiLSAOConstants>(m_hLineSweepCB);
  cb->LineToLinePixelOffset                             = m_iLineToLinePixelOffset;

  // Compute general information per direction and create line instructions.

  // As long as we don't span out different line samplings across multiple frames, the number of prepared directions here is always equal to
  // the number of directions per frame. Note that if we were to do temporal sampling with a different line set every frame, we would need
  // to precompute all *possible* sampling directions still as a whole here!
  xiiVec2I32 samplingDir[NUM_SWEEP_DIRECTIONS_PER_FRAME];
  {
    constexpr int numSweepDirs = NUM_SWEEP_DIRECTIONS_PER_FRAME;

    // As described in the paper, all directions are aligned so that we always hit  pixels on a square.
    static_assert(numSweepDirs % 4 == 0, "Invalid number of sweep directions for LSAO!");
    // static_assert((numSweepDirs * NUM_SWEEP_DIRECTIONS_PER_PIXEL) % 9 == 0, "Invalid number of sweep directions for LSAO!");
    const int perSide     = (numSweepDirs + 4) / 4 - 1; // side length of the square on which all directions lie -1
    const int halfPerSide = perSide / 2 + (perSide % 2);
    for (int i = 0; i < perSide; ++i)
    {
      // Put opposing directions next to each other, so that a gather pass that doesn't sample all directions, only needs to sample an even
      // number of directions to end up with non-negative occlusion.
      samplingDir[i * 4 + 0] = xiiVec2I32(i - halfPerSide, halfPerSide) * m_iLineSamplePixelOffsetFactor; // Top
      samplingDir[i * 4 + 1] = -samplingDir[i * 4 + 0];                                                   // Bottom
      samplingDir[i * 4 + 2] = xiiVec2I32(halfPerSide, halfPerSide - i) * m_iLineSamplePixelOffsetFactor; // Right
      samplingDir[i * 4 + 3] = -samplingDir[i * 4 + 2];                                                   // Left
    }

    // todo: Ddd debug test to check whether any direction is duplicated. Mistakes in the equations above can easily happen!
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    for (int i = 0; i < numSweepDirs - 1; ++i)
    {
      for (int j = i + 1; j < numSweepDirs; ++j)
        XII_ASSERT_DEBUG(samplingDir[i] != samplingDir[j], "Two SSAO sampling directions are equal. Implementation for direction determination is broken.");
    }
#endif
  }

  for (int dirIndex = 0; dirIndex < XII_ARRAY_SIZE(samplingDir); ++dirIndex)
  {
    xiiUInt32 totalLineCountBefore = lineInstructions.GetCount();
    AddLinesForDirection(imageResolution, samplingDir[dirIndex], dirIndex, lineInstructions, totalNumberOfSamples);
    XII_ASSERT_DEBUG(totalNumberOfSamples % 2 == 0, "Only even number of line samples are allowed");

    cb->Directions[dirIndex].Direction             = xiiVec2(static_cast<float>(samplingDir[dirIndex].x), static_cast<float>(samplingDir[dirIndex].y));
    cb->Directions[dirIndex].NumLines              = lineInstructions.GetCount() - totalLineCountBefore;
    cb->Directions[dirIndex].LineInstructionOffset = totalLineCountBefore;
  }
  m_uiNumSweepLines        = lineInstructions.GetCount();
  cb->TotalLineNumber      = m_uiNumSweepLines;
  cb->TotalNumberOfSamples = totalNumberOfSamples;
  // Allocate and upload data structures to GPU
  {
    xiiGALDevice* device = xiiGALDevice::GetDefaultDevice();
    DestroyLineSweepData();

    // Output UAV for line sweep pass.
    // DX11 allows only float and int for writing RWBuffer, so we need to do manual packing.
    {
      xiiGALBufferCreationDescription bufferDesc;
      bufferDesc.m_Mode                = xiiGALBufferMode::Formatted;
      bufferDesc.m_uiElementByteStride = 4;
      bufferDesc.m_uiSize              = imageResolution.z * 2 * totalNumberOfSamples;
      bufferDesc.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      bufferDesc.m_ResourceUsage       = xiiGALResourceUsage::Default;
      bufferDesc.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;

      m_hLineSweepOutputBuffer = device->CreateBuffer(bufferDesc);

      xiiGALBufferViewCreationDescription uavDesc;
      uavDesc.m_ViewType     = xiiGALBufferViewType::UnorderedAccess;
      uavDesc.m_hBuffer      = m_hLineSweepOutputBuffer;
      uavDesc.m_Format       = xiiGALBufferFormat{.m_ValueType = xiiGALValueType::UInt32, .m_uiComponents = 1, .m_bIsNormalized = false};
      uavDesc.m_uiByteOffset = 0;
      uavDesc.m_uiByteWidth  = imageResolution.z * totalNumberOfSamples / 2;
      m_hLineSweepOutputUAV  = device->CreateBufferView(uavDesc);

      xiiGALBufferViewCreationDescription srvDesc;
      srvDesc.m_hBuffer      = m_hLineSweepOutputBuffer;
      uavDesc.m_Format       = xiiGALBufferFormat{.m_ValueType = xiiGALValueType::UInt32, .m_uiComponents = 1, .m_bIsNormalized = false};
      srvDesc.m_uiByteOffset = 0;
      srvDesc.m_uiByteWidth  = imageResolution.z * totalNumberOfSamples / 2;
      m_hLineSweepOutputSRV  = device->CreateBufferView(srvDesc);
    }

    // Structured buffer per line.
    {
      xiiGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiElementByteStride = sizeof(LineInstruction);
      bufferDesc.m_uiSize              = bufferDesc.m_uiElementByteStride * m_uiNumSweepLines;
      bufferDesc.m_Mode                = xiiGALBufferMode::Structured;
      bufferDesc.m_BindFlags           = xiiGALBindFlags::ShaderResource;
      bufferDesc.m_ResourceUsage       = xiiGALResourceUsage::Immutable;

      auto pInitialData = xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(lineInstructions.GetData()), lineInstructions.GetCount() * sizeof(LineInstruction));

      xiiGALBufferData initData;
      initData.m_pData      = pInitialData.GetPtr();
      initData.m_uiDataSize = pInitialData.GetCount();
      m_hLineInfoBuffer     = device->CreateBuffer(bufferDesc, &initData);

      m_hLineSweepInfoSRV = device->GetBuffer(m_hLineInfoBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource);
    }
  }

  m_bSweepDataDirty = false;
}

void xiiLSAOPass::AddLinesForDirection(const xiiVec3I32& imageResolution, const xiiVec2I32& sampleDir, xiiUInt32 lineIndex, xiiDynamicArray<LineInstruction>& outinLineInstructions, xiiUInt32& outinTotalNumberOfSamples)
{
  XII_ASSERT_DEBUG(sampleDir.x != 0 || sampleDir.y != 0, "Sample direction is null (not pointing anywhere)");

  xiiUInt32 firstNewLineInstructionIndex = outinLineInstructions.GetCount();

  // Always walk positive and flip if necessary later.
  xiiVec2I32 walkDir(xiiMath::Abs(sampleDir.x), xiiMath::Abs(sampleDir.y));
  xiiVec2    walkDirF(static_cast<float>(walkDir.x), static_cast<float>(walkDir.y));

  // Line "creation" always starts from 0,0 and walks along EITHER x or y depending which one is the less dominant axis.

  // Helper to avoid duplication for dominant x/y
  int domDir = walkDir.x > walkDir.y ? 0 : 1;
  int secDir = 1 - domDir;
#define DOM GetData()[domDir]
#define SEC GetData()[secDir]

  // Walk along secondary axis backwards.
  for (xiiInt32 sec = imageResolution.SEC - 1; true; sec -= m_iLineToLinePixelOffset)
  {
    LineInstruction& newLine   = outinLineInstructions.ExpandAndGetRef();
    newLine.FirstSamplePos.DOM = 0.0f;
    newLine.FirstSamplePos.SEC = static_cast<float>(sec);

    // If we are already outside of the screen with sec, this is not a point inside the screen!
    if (sec < 0)
    {
      // If we don't walk in the secondary direction at all this means that we're done.
      if (walkDir.SEC == 0)
      {
        outinLineInstructions.PopBack();
        break;
      }
      // Otherwise we just need to walk long enough to hit the screen again.
      else
      {
        // Find new start on the sec axis. (dom axis is fine)
        xiiVec2 minimalStepToBorder = walkDirF * xiiMath::Ceil(static_cast<float>(-sec) / walkDirF.SEC); // Remember: Only walk discrete steps!
        newLine.FirstSamplePos.DOM += minimalStepToBorder.DOM;
        newLine.FirstSamplePos.SEC += minimalStepToBorder.SEC;

        // Outside, we're done.
        if (newLine.FirstSamplePos.DOM >= imageResolution.DOM - walkDir.DOM * 2)
        {
          outinLineInstructions.PopBack();
          break;
        }
      }
    }

    // Add a pseudo random offset to distributed the samples a bit.
    // We still want to go from discrete pixel to discrete pixel so we have to round which can mess up our line placement.
    // So this is introducing some error. Visual comparison clearly shows that it's worth it though.
    float offset = HaltonSequence(lineIndex, sec + lineIndex);
    newLine.FirstSamplePos.DOM += xiiMath::Round(offset * walkDir.DOM);
    newLine.FirstSamplePos.SEC += xiiMath::Round(offset * walkDir.SEC);

    // Clamp back to possible area.
    // Due to the way we jump from pixels to line in the gather shader, we can't just discard lines.
    newLine.FirstSamplePos.x = xiiMath::Clamp<float>(newLine.FirstSamplePos.x, 0.0f, imageResolution.x - 1.0f);
    newLine.FirstSamplePos.y = xiiMath::Clamp<float>(newLine.FirstSamplePos.y, 0.0f, imageResolution.y - 1.0f);

    // Compute how many samples this line will consume.
    unsigned int stepsToDOMBorder = static_cast<unsigned int>((imageResolution.DOM - newLine.FirstSamplePos.DOM) / walkDir.DOM + 1);
    unsigned int numSamples       = 0;
    if (walkDir.SEC > 0)
    {
      unsigned int stepsToSECBorder = static_cast<unsigned int>((imageResolution.SEC - newLine.FirstSamplePos.SEC) / walkDir.SEC + 1);
      numSamples                    = xiiMath::Min(stepsToSECBorder, stepsToDOMBorder);
    }
    else
      numSamples = stepsToDOMBorder;

    // Due to output packing restrictions only even number of samples are allowed. Remove one if necessary.
    if (numSamples % 2 != 0)
      --numSamples;

    newLine.LineSweepOutputBufferOffset = outinTotalNumberOfSamples;
    outinTotalNumberOfSamples += numSamples;
    newLine.LineDirIndex_NumSamples = lineIndex | (numSamples << 16);
  }

#undef SEC
#undef DOM

  // Now consider x/y being negative.
  for (int c = 0; c < 2; ++c)
  {
    if (sampleDir.GetData()[c] < 0)
    {
      for (xiiUInt32 i = firstNewLineInstructionIndex; i < outinLineInstructions.GetCount(); ++i)
      {
        outinLineInstructions[i].FirstSamplePos.GetData()[c] = imageResolution.GetData()[c] - 1 - outinLineInstructions[i].FirstSamplePos.GetData()[c];
      }
    }
  }

  // Validation.
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  for (xiiUInt32 i = firstNewLineInstructionIndex; i < outinLineInstructions.GetCount(); ++i)
  {
    auto p = outinLineInstructions[i].FirstSamplePos;
    XII_ASSERT_DEV(p.x >= 0 && p.y >= 0 && p.x < imageResolution.x && p.y < imageResolution.y, "First sweep line sample pos is invalid. Something is wrong with the sweep line generation algorithm.");
  }
#endif
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_LSAOPass);
