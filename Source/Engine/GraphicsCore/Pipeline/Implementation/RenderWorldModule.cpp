#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

#include <Shaders/Pipeline/Passes/DynamicResolution/DynamicResolutionConstants.h>
#include <Shaders/Pipeline/Passes/PerFrameBufferUpload/PerFrameConstants.h>

xiiCVarFloat cvar_RenderingDynamicResolutionTargetFrameTimeMs("Rendering.DynamicResolution.TargetFrameTimeMs", 16.0f, xiiCVarFlags::Default, "The target frame time in milliseconds for dynamic resolution to aim for. The system will adjust the render resolution each frame to try to match this target time as closely as possible.");
xiiCVarFloat cvar_RenderingDynamicResolutionMinimumRenderScale("Rendering.DynamicResolution.MinimumRenderScale", 0.5f, xiiCVarFlags::Default, "The minimum render scale that dynamic resolution can use. This is a multiplier for the render resolution relative to the native resolution. For example, a value of 0.5 means the render resolution can go down to 50% of the native resolution.");
xiiCVarFloat cvar_RenderingDynamicResolutionMaximumRenderScale("Rendering.DynamicResolution.MaximumRenderScale", 1.0f, xiiCVarFlags::Default, "The maximum render scale that dynamic resolution can use. This is a multiplier for the render resolution relative to the native resolution. For example, a value of 1.0 means the render resolution can go up to 100% of the native resolution.");

XII_IMPLEMENT_WORLD_MODULE(xiiRenderWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderWorldModule::xiiRenderWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiRenderWorldModule::~xiiRenderWorldModule() = default;

void xiiRenderWorldModule::Initialize()
{
  // Register ExtractRenderData (concurrent, runs on async worker threads per component manager).
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExtractRenderData, this);
    description.m_Phase                     = xiiWorldUpdatePhase::Async;
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }

  // Register ExecuteRenderGraphs (post-async, single-threaded, after all extraction is complete).
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExecuteRenderGraphs, this);
    description.m_Phase                     = xiiWorldUpdatePhase::PostAsync;
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }
}

void xiiRenderWorldModule::Deinitialize()
{
  m_Views.Clear();

  m_uiRenderFrameIndex = 0;
}

void xiiRenderWorldModule::OnSimulationStarted()
{
}

xiiView* xiiRenderWorldModule::CreateView(xiiStringView sName)
{
  xiiUniquePtr<xiiView> pView = XII_DEFAULT_NEW(xiiView);
  pView->SetName(sName);
  xiiView* pRet = pView.Borrow();
  m_Views.PushBack(std::move(pView));
  return pRet;
}

void xiiRenderWorldModule::DestroyView(xiiView* pView)
{
  for (xiiUInt32 i = 0; i < m_Views.GetCount(); ++i)
  {
    if (m_Views[i].Borrow() == pView)
    {
      m_Views.RemoveAtAndCopy(i);
      return;
    }
  }
}

void xiiRenderWorldModule::BuildDefaultRenderGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  auto [pDynamicResolutionData, hDynamicResolutionPass] = graph.AddPass<PassData::DynamicResolutionPassData>("DynamicResolution", xiiGALCommandQueueFlags::Compute,
                                                                                                             xiiMakeDelegate(&xiiRenderWorldModule::SetupDynamicResolutionPass, this),
                                                                                                             xiiMakeDelegate(&xiiRenderWorldModule::ExecuteDynamicResolutionPass, this));

  auto [pPerFrameBufferUploadData, hPerFrameBufferUploadPass] = graph.AddPass<PassData::PerFrameBufferUploadPassData>("PerFrameBufferUpload", xiiGALCommandQueueFlags::Graphics,
                                                                                                                      xiiMakeDelegate(&xiiRenderWorldModule::SetupPerFrameBufferUploadPass, this),
                                                                                                                      xiiMakeDelegate(&xiiRenderWorldModule::ExecutePerFrameBufferUploadPass, this));
}

void xiiRenderWorldModule::ExtractRenderData(const xiiWorldModule::UpdateContext& context)
{
  for (auto& pView : m_Views)
  {
    if (!pView->IsValid())
      continue;

    xiiExtractedRenderData* pExtractedData = pView->GetExtractedRenderData();
    pExtractedData->Clear();

    xiiMsgExtractRenderData msg;
    msg.m_pView                = pView.Borrow();
    msg.m_pExtractedRenderData = pExtractedData;

    // Broadcast to all objects; each object routes to matching component message handlers.
    {
      XII_LOCK(GetWorld()->GetReadMarker());
      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        it->SendMessage(msg);
      }
    }

    // Flatten concurrent batches, then radix-sort each category by sort key.
    pExtractedData->SortAndBatches();
  }
}

void xiiRenderWorldModule::ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice)
    return;

  const xiiUInt64 uiFrameIndex = m_uiRenderFrameIndex++;

  for (auto& pView : m_Views)
  {
    if (!pView->IsValid())
      continue;

    xiiRenderGraph*              pGraph        = pView->GetRenderGraph();
    xiiRenderGraphBlackboard&    blackboard    = pView->GetBlackboard();
    xiiRenderGraphResourceCache& resourceCache = pView->GetResourceCache();

    // Clear the per-view blackboard at the start of each frame so passes start with a clean slate.
    // History data must live inside persistent GPU buffers owned by each pass.
    blackboard.Clear();
    blackboard.Set(xiiMakeHashedString("FrameIndex"), static_cast<xiiUInt32>(uiFrameIndex));

    // Reconstruct the graph for this frame.
    pGraph->BeginSetup(uiFrameIndex);

    const xiiView::RenderGraphBuilder& graphBuilder = pView->GetRenderGraphBuilder();
    if (graphBuilder.IsValid())
    {
      graphBuilder(*pView, *pGraph, blackboard);
    }
    else
    {
      BuildDefaultRenderGraph(*pView, *pGraph, blackboard);
    }

    pGraph->EndSetup();

    xiiRGCompileSettings compileSettings;
    compileSettings.m_bEnableGPUProfiling = true;

    if (pGraph->Compile(compileSettings).Succeeded())
    {
      const xiiResult executeResult = pGraph->Execute(pDevice, pView.Borrow(), &blackboard, &resourceCache);
      XII_ASSERT_DEV(executeResult.Succeeded(), "Render graph execution failed for view '{0}'.", pView->GetName());
    }
  }
}

void xiiRenderWorldModule::SetupDynamicResolutionPass(PassData::DynamicResolutionPassData& data, xiiRGBuilder& builder)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // Pass constants buffer.
  {
    xiiGALBufferCreationDescription description;
    description.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
    description.m_uiElementByteStride = 0U;
    description.m_uiSize              = sizeof(xiiDynamicResolutionPassConstants);
    description.m_Usage               = xiiGALResourceUsage::Dynamic;
    description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

    data.m_hPassConstantsBuffer = builder.DeclareBuffer("DynamicResolution_PassConstants", description);
  }

  // PID state buffer.
  if (!m_PersistentFrameResources.m_DynamicResolution.m_pResolutionStateBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(PersistentFrameResources::DynamicResolution::ResolutionStateData);
    description.m_uiSize              = description.m_uiElementByteStride;
    description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Mutable;

    PersistentFrameResources::DynamicResolution::ResolutionStateData bufferData = {};
    bufferData.m_fCurrentScale                                                  = 1.0f;
    bufferData.m_fSmoothedScale                                                 = 1.0f;
    bufferData.m_fErrorIntegral                                                 = 0.0f;
    bufferData.m_fPreviousError                                                 = 0.0f;

    xiiGALBufferData initialData;
    initialData.m_pData      = &bufferData;
    initialData.m_uiDataSize = sizeof(bufferData);

    m_PersistentFrameResources.m_DynamicResolution.m_pResolutionStateBuffer = pDevice->CreateBuffer(description, &initialData);

    // Import state buffer as UAV.
    data.m_hResolutionStateBuffer = builder.ImportBuffer("DynamicResolution_State", m_PersistentFrameResources.m_DynamicResolution.m_pResolutionStateBuffer, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_hResolutionStateBuffer = builder.WriteBuffer(data.m_hResolutionStateBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  }

  // CPU-side values.
  data.m_fFrameDeltaTimeMs   = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;
  data.m_fTargetFrameTimeMs  = cvar_RenderingDynamicResolutionTargetFrameTimeMs;
  data.m_fMinimumRenderScale = cvar_RenderingDynamicResolutionMinimumRenderScale;
  data.m_fMaximumRenderScale = cvar_RenderingDynamicResolutionMaximumRenderScale;

  data.m_fCurrentGpuTimeMs  = data.m_fTargetFrameTimeMs; // TODO.
  data.m_fSmoothedGpuTimeMs = data.m_fTargetFrameTimeMs; // TODO.

  if (!m_PersistentFrameResources.m_DynamicResolution.m_pComputePipeline)
  {
    xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Passes/DynamicResolution/DynamicResolution.xiiShader");

    xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables;
    m_PersistentFrameResources.m_DynamicResolution.m_hShaderPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, true);

    xiiResourceLock<xiiShaderPermutationResource> pPermutation(m_PersistentFrameResources.m_DynamicResolution.m_hShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    XII_ASSERT_DEV(pPermutation.IsValid(), "Failed to load shader permutation for dynamic resolution pass.");

    xiiSharedPtr<xiiGALShader>                    pComputeShader = pPermutation->GetGALShader(xiiGALShaderType::Compute);
    xiiSharedPtr<xiiGALPipelineResourceSignature> pSignature     = pPermutation->GetPipelineResourceSignature();

    xiiGALComputePipelineStateCreationDescription description;
    description.m_pComputeShader             = pComputeShader;
    description.m_pPipelineResourceSignature = pSignature;

    m_PersistentFrameResources.m_DynamicResolution.m_pComputePipeline = xiiGALPipelineCache::GetPipeline(description);

    XII_ASSERT_DEV(m_PersistentFrameResources.m_DynamicResolution.m_pComputePipeline, "Failed to create compute pipeline for dynamic resolution pass.");
  }

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiRenderWorldModule::ExecuteDynamicResolutionPass(const PassData::DynamicResolutionPassData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("Dynamic Resolution Scaling");
  {
    // Update pass constants.
    {
      xiiGALMapHelper<xiiDynamicResolutionPassConstants> pConstants(cmd, context.GetBuffer(data.m_hPassConstantsBuffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);

      pConstants->FrameDeltaTimeMs   = data.m_fFrameDeltaTimeMs;
      pConstants->TargetFrameTimeMs  = data.m_fTargetFrameTimeMs;
      pConstants->MinimumRenderScale = data.m_fMinimumRenderScale;
      pConstants->MaximumRenderScale = data.m_fMaximumRenderScale;
      pConstants->CurrentGpuTimeMs   = data.m_fCurrentGpuTimeMs;
      pConstants->SmoothedGpuTimeMs  = data.m_fSmoothedGpuTimeMs;
    }

    cmd.SetPipelineState(m_PersistentFrameResources.m_DynamicResolution.m_pComputePipeline);

    cmd.ResolveAndSetConstantBuffer(XII_PP_STRINGIFY(xiiDynamicResolutionPassConstants), context.GetBuffer(data.m_hPassConstantsBuffer), xiiGALShaderType::Compute);

    if (xiiGALBuffer* pState = context.GetBuffer(data.m_hResolutionStateBuffer))
    {
      cmd.ResolveAndSetUnorderedAccessBufferView("g_DynamicResolutionData", pState->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    }

    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

    cmd.DispatchCompute({1U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

void xiiRenderWorldModule::SetupPerFrameBufferUploadPass(PassData::PerFrameBufferUploadPassData& data, xiiRGBuilder& builder)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  if (!m_PersistentFrameResources.m_PerFrameBufferUpload.m_pGlobalConstantsBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(xiiPerFrameGlobalUploadData);
    description.m_uiSize              = description.m_uiElementByteStride;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Mutable;

    m_PersistentFrameResources.m_PerFrameBufferUpload.m_pGlobalConstantsBuffer = pDevice->CreateBuffer(description);
  }

  if (!m_PersistentFrameResources.m_PerFrameBufferUpload.m_pCameraConstantsBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(xiiPerFrameCameraUploadData);
    description.m_uiSize              = description.m_uiElementByteStride;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Mutable;

    m_PersistentFrameResources.m_PerFrameBufferUpload.m_pCameraConstantsBuffer = pDevice->CreateBuffer(description);
  }

  data.m_hCameraConstantsOutputBuffer = builder.ImportBuffer("PerFrame_CameraConstants", m_PersistentFrameResources.m_PerFrameBufferUpload.m_pCameraConstantsBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hCameraConstantsOutputBuffer = builder.WriteBuffer(data.m_hCameraConstantsOutputBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGlobalConstantsOutputBuffer = builder.ImportBuffer("PerFrame_GlobalConstants", m_PersistentFrameResources.m_PerFrameBufferUpload.m_pGlobalConstantsBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGlobalConstantsOutputBuffer = builder.WriteBuffer(data.m_hGlobalConstantsOutputBuffer, xiiGALResourceStateFlags::UnorderedAccess);

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiRenderWorldModule::ExecutePerFrameBufferUploadPass(const PassData::PerFrameBufferUploadPassData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("Per-Frame Buffer Upload");
  {
    // This pass is responsible for uploading per-frame constants that are used by multiple passes throughout the frame, such as camera matrices, light data, and global parameters.
    // The data is gathered and prepared on the CPU during the Execute phase of this pass, then written to GPU buffers that are accessible to other passes.
    {
      xiiGALMapHelper<xiiPerFrameGlobalUploadData> pGlobalConstants(cmd, m_PersistentFrameResources.m_PerFrameBufferUpload.m_pGlobalConstantsBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);

      // Wrap around to prevent floating point issues. A wrap around of 1000 allows all frequencies with 3 digits after the decimal.
      constexpr double fWrapAround        = 1000.0;
      pGlobalConstants->FrameIndex        = context.GetBlackboard().GetRef<xiiUInt32>("FrameIndex");
      pGlobalConstants->DeltaTime         = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds());
      pGlobalConstants->GlobalTime        = static_cast<float>(xiiMath::Mod(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), fWrapAround));
      pGlobalConstants->WorldTime         = static_cast<float>(xiiMath::Mod(GetWorld()->GetClock().GetAccumulatedTime().GetSeconds(), fWrapAround));
      pGlobalConstants->RenderScaleJitter = xiiVec4::MakeZero(); // TODO.
    }
    {
      xiiGALMapHelper<xiiPerFrameCameraUploadData> pCameraConstants(cmd, m_PersistentFrameResources.m_PerFrameBufferUpload.m_pCameraConstantsBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);

      const xiiViewData& viewData = context.GetView()->GetData();
      const xiiCamera*   pCamera  = context.GetView()->GetCamera();

      pCameraConstants->ViewProjectionMatrix[0]       = viewData.m_ViewProjectionMatrix[0];
      pCameraConstants->InverseProjectionMatrix[0]    = viewData.m_InverseProjectionMatrix[0];
      pCameraConstants->CameraDirectionAndFarPlane[0] = xiiVec4(pCamera->GetDirForwards(xiiCameraEye::Left), pCamera->GetFarPlane());
      pCameraConstants->CameraPositionAndNearPlane[0] = xiiVec4(pCamera->GetPosition(xiiCameraEye::Left), pCamera->GetNearPlane());

      pCameraConstants->ViewProjectionMatrix[1]       = viewData.m_ViewProjectionMatrix[1];
      pCameraConstants->InverseProjectionMatrix[1]    = viewData.m_InverseProjectionMatrix[1];
      pCameraConstants->CameraDirectionAndFarPlane[1] = xiiVec4(pCamera->GetDirForwards(xiiCameraEye::Right), pCamera->GetFarPlane());
      pCameraConstants->CameraPositionAndNearPlane[1] = xiiVec4(pCamera->GetPosition(xiiCameraEye::Right), pCamera->GetNearPlane());
    }
  }
  cmd.EndDebugGroup();
}
