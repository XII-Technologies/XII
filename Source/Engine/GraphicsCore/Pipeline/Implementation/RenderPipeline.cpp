#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/Extractor.h>
#include <GraphicsCore/Pipeline/FrameDataProvider.h>
#include <GraphicsCore/Pipeline/Passes/TargetPass.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Rasterizer/RasterizerView.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
xiiCVarBool xiiRenderPipeline::cvar_SpatialCullingVis("Spatial.Culling.Vis", false, xiiCVarFlags::Default, "Enables debug visualization of visibility culling");
xiiCVarBool cvar_SpatialCullingShowStats("Spatial.Culling.ShowStats", false, xiiCVarFlags::Default, "Display some stats of the visibility culling");
#endif

xiiCVarBool  cvar_SpatialCullingOcclusionEnable("Spatial.Occlusion.Enable", true, xiiCVarFlags::Default, "Use software rasterization for occlusion culling.");
xiiCVarBool  cvar_SpatialCullingOcclusionVisView("Spatial.Occlusion.VisView", false, xiiCVarFlags::Default, "Render the occlusion framebuffer as an overlay.");
xiiCVarFloat cvar_SpatialCullingOcclusionBoundsInflation("Spatial.Occlusion.BoundsInflation", 0.5f, xiiCVarFlags::Default, "How much to inflate bounds during occlusion check.");
xiiCVarFloat cvar_SpatialCullingOcclusionFarPlane("Spatial.Occlusion.FarPlane", 50.0f, xiiCVarFlags::Default, "Far plane distance for finding occluders.");

xiiRenderPipeline::xiiRenderPipeline()
{
  m_CurrentExtractThread  = (xiiThreadID)0;
  m_CurrentRenderThread   = (xiiThreadID)0;
  m_uiLastExtractionFrame = -1;
  m_uiLastRenderFrame     = -1;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_AverageCullingTime = xiiTime::MakeFromSeconds(0.1f);
#endif
}

xiiRenderPipeline::~xiiRenderPipeline()
{
  m_Data[0].Clear();
  m_Data[1].Clear();

  ClearRenderPassGraphResources();

  while (!m_Passes.IsEmpty())
  {
    RemovePass(m_Passes.PeekBack().Borrow());
  }
}

void xiiRenderPipeline::AddPass(xiiUniquePtr<xiiRenderPipelinePassBase>&& pPass)
{
  m_PipelineState    = PipelineState::Uninitialized;
  pPass->m_pPipeline = this;
  pPass->InitializePins();

  auto it = m_Connections.Insert(pPass.Borrow(), ConnectionData());
  it.Value().m_Inputs.SetCount(pPass->GetInputPins().GetCount());
  it.Value().m_Outputs.SetCount(pPass->GetOutputPins().GetCount());

  m_Passes.PushBack(std::move(pPass));
}

void xiiRenderPipeline::RemovePass(xiiRenderPipelinePassBase* pPass)
{
  for (xiiUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    if (m_Passes[i].Borrow() == pPass)
    {
      m_PipelineState = PipelineState::Uninitialized;

      RemoveConnections(pPass);
      m_Connections.Remove(pPass);
      pPass->m_pPipeline = nullptr;

      m_Passes.RemoveAtAndCopy(i);
      break;
    }
  }
}

void xiiRenderPipeline::GetPasses(xiiDynamicArray<const xiiRenderPipelinePassBase*>& ref_passes) const
{
  ref_passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

void xiiRenderPipeline::GetPasses(xiiDynamicArray<xiiRenderPipelinePassBase*>& ref_passes)
{
  ref_passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

xiiRenderPipelinePassBase* xiiRenderPipeline::GetPassByName(const xiiStringView& sPassName)
{
  for (auto& pPass : m_Passes)
  {
    if (sPassName.IsEqual(pPass->GetName()))
    {
      return pPass.Borrow();
    }
  }
  return nullptr;
}

bool xiiRenderPipeline::Connect(xiiRenderPipelinePassBase* pOutputNode, xiiStringView sOutputPinName, xiiRenderPipelinePassBase* pInputNode, xiiStringView sInputPinName)
{
  xiiHashedString sOutputPinNameHash;
  sOutputPinNameHash.Assign(sOutputPinName);

  xiiHashedString sInputPinNameHash;
  sInputPinNameHash.Assign(sInputPinName);

  return Connect(pOutputNode, sOutputPinNameHash, pInputNode, sInputPinNameHash);
}

bool xiiRenderPipeline::Connect(xiiRenderPipelinePassBase* pOutputNode, xiiHashedString sOutputPinName, xiiRenderPipelinePassBase* pInputNode, xiiHashedString sInputPinName)
{
  xiiLogBlock b("xiiRenderPipeline::Connect");

  auto itOut = m_Connections.Find(pOutputNode);
  if (!itOut.IsValid())
  {
    xiiLog::Error("Output node '{0}' not added to pipeline!", pOutputNode->GetName());
    return false;
  }
  auto itIn = m_Connections.Find(pInputNode);
  if (!itIn.IsValid())
  {
    xiiLog::Error("Input node '{0}' not added to pipeline!", pInputNode->GetName());
    return false;
  }
  const xiiRenderPipelineNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    xiiLog::Error("Source pin '{0}::{1}' does not exist!", pOutputNode->GetName(), sOutputPinName);
    return false;
  }
  const xiiRenderPipelineNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    xiiLog::Error("Target pin '{0}::{1}' does not exist!", pInputNode->GetName(), sInputPinName);
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != nullptr)
  {
    xiiLog::Error("Pins already connected: '{0}::{1}' -> '{2}::{3}'!", pOutputNode->GetName(), sOutputPinName, pInputNode->GetName(), sInputPinName);
    return false;
  }

  // Add at output.
  xiiRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  if (pConnection == nullptr)
  {
    pConnection                                          = XII_DEFAULT_NEW(xiiRenderPipelinePassConnection);
    pConnection->m_pOutput                               = pPinSource;
    itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex] = pConnection;
  }
  else
  {
    // Check that only one passthrough is connected.
    if (pPinTarget->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::PassThrough))
    {
      for (const xiiRenderPipelineNodePin* pPin : pConnection->m_Inputs)
      {
        if (pPin->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::PassThrough))
        {
          xiiLog::Error("A pass through pin is already connected to the '{0}' pin!", sOutputPinName);
          return false;
        }
      }
    }
  }

  // Add at input.
  pConnection->m_Inputs.PushBack(pPinTarget);
  itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] = pConnection;
  m_PipelineState                                   = PipelineState::Uninitialized;
  return true;
}

bool xiiRenderPipeline::Disconnect(xiiRenderPipelinePassBase* pOutputNode, xiiHashedString sOutputPinName, xiiRenderPipelinePassBase* pInputNode, xiiHashedString sInputPinName)
{
  xiiLogBlock b("xiiRenderPipeline::Connect");

  auto itOut = m_Connections.Find(pOutputNode);
  if (!itOut.IsValid())
  {
    xiiLog::Error("Output node '{0}' not added to pipeline!", pOutputNode->GetName());
    return false;
  }
  auto itIn = m_Connections.Find(pInputNode);
  if (!itIn.IsValid())
  {
    xiiLog::Error("Input node '{0}' not added to pipeline!", pInputNode->GetName());
    return false;
  }
  const xiiRenderPipelineNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    xiiLog::Error("Source pin '{0}::{1}' does not exist!", pOutputNode->GetName(), sOutputPinName);
    return false;
  }
  const xiiRenderPipelineNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    xiiLog::Error("Target pin '{0}::{1}' does not exist!", pInputNode->GetName(), sInputPinName);
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] == nullptr || itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex])
  {
    xiiLog::Error("Pins not connected: '{0}::{1}' -> '{2}::{3}'!", pOutputNode->GetName(), sOutputPinName, pInputNode->GetName(), sInputPinName);
    return false;
  }

  // Remove at input
  xiiRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  pConnection->m_Inputs.RemoveAndCopy(pPinTarget);
  itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] = nullptr;

  if (pConnection->m_Inputs.IsEmpty())
  {
    // Remove at output
    itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex] = nullptr;
    XII_DEFAULT_DELETE(pConnection);
  }

  m_PipelineState = PipelineState::Uninitialized;
  return true;
}

const xiiRenderPipelinePassConnection* xiiRenderPipeline::GetInputConnection(const xiiRenderPipelinePassBase* pPass, xiiHashedString sInputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto&                           data = it.Value();
  const xiiRenderPipelineNodePin* pPin = pPass->GetPinByName(sInputPinName);
  if (!pPin || pPin->m_uiInputIndex == 0xFFU)
    return nullptr;

  return data.m_Inputs[pPin->m_uiInputIndex];
}

const xiiRenderPipelinePassConnection* xiiRenderPipeline::GetOutputConnection(const xiiRenderPipelinePassBase* pPass, xiiHashedString sOutputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto&                           data = it.Value();
  const xiiRenderPipelineNodePin* pPin = pPass->GetPinByName(sOutputPinName);
  if (!pPin)
    return nullptr;

  return data.m_Outputs[pPin->m_uiOutputIndex];
}

xiiRenderPipeline::PipelineState xiiRenderPipeline::Rebuild(const xiiView& view)
{
  xiiLogBlock b("xiiRenderPipeline::Rebuild");

  ClearRenderPassGraphResources();

  xiiResult result = RebuildInternal(view);

  if (result.Failed())
  {
    ClearRenderPassGraphResources();
  }
  else
  {
    // Ensure the render data stores the updated view data.
    UpdateViewData(view, xiiRenderWorld::GetDataIndexForRendering());
  }

  m_PipelineState = result.Succeeded() ? PipelineState::Initialized : PipelineState::RebuildError;
  return m_PipelineState;
}

xiiResult xiiRenderPipeline::RebuildInternal(const xiiView& view)
{
  XII_SUCCEED_OR_RETURN(SortPasses());
  XII_SUCCEED_OR_RETURN(InitializePassResourceDescriptions(view));
  XII_SUCCEED_OR_RETURN(CreatePassResourceUsage(view));
  XII_SUCCEED_OR_RETURN(InitializeRenderPipelinePasses(view));

  SortExtractors();

  return XII_SUCCESS;
}

xiiResult xiiRenderPipeline::SortPasses()
{
  xiiLogBlock                                    b("Sort Passes");
  xiiHybridArray<xiiRenderPipelinePassBase*, 32> done;
  done.Reserve(m_Passes.GetCount());

  xiiHybridArray<xiiRenderPipelinePassBase*, 8> usable;     // Stack of passes with all connections setup, they can be asked for descriptions.
  xiiHybridArray<xiiRenderPipelinePassBase*, 8> candidates; // Not usable yet, but all input connections are available.

  // Find all source passes from which we can start the output description propagation.
  for (auto& pPass : m_Passes)
  {
    if (AreInputDescriptionsAvailable(pPass.Borrow(), done))
    {
      usable.PushBack(pPass.Borrow());
    }
  }

  // Via a depth first traversal, order the passes.
  while (!usable.IsEmpty())
  {
    xiiRenderPipelinePassBase* pPass = usable.PeekBack();
    xiiLogBlock                b2("Traverse", pPass->GetName());

    usable.PopBack();
    ConnectionData& data = m_Connections[pPass];

    XII_ASSERT_DEBUG(data.m_Inputs.GetCount() == pPass->GetInputPins().GetCount(), "Input pin count mismatch!");
    XII_ASSERT_DEBUG(data.m_Outputs.GetCount() == pPass->GetOutputPins().GetCount(), "Output pin count mismatch!");

    // Check for new candidate passes. This step cannot be achieved in the previous loop as multiple connections may be required by a node.
    for (xiiUInt32 i = 0; i < data.m_Outputs.GetCount(); ++i)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        // Iterate all inputs this connection is connected to and test the corresponding node for availability.
        for (const xiiRenderPipelineNodePin* pPin : data.m_Outputs[i]->m_Inputs)
        {
          XII_ASSERT_DEBUG(pPin->m_pParent != nullptr, "Pass was not initialized!");

          xiiRenderPipelinePassBase* pTargetPass = static_cast<xiiRenderPipelinePassBase*>(pPin->m_pParent);
          if (done.Contains(pTargetPass))
          {
            xiiLog::Error("Loop detected, graph not supported!");
            return XII_FAILURE;
          }

          if (!usable.Contains(pTargetPass) && !candidates.Contains(pTargetPass))
          {
            candidates.PushBack(pTargetPass);
          }
        }
      }
    }

    done.PushBack(pPass);

    // Check for usable candidates. Reverse order for depth first traversal.
    for (xiiInt32 i = (xiiInt32)candidates.GetCount() - 1; i >= 0; --i)
    {
      xiiRenderPipelinePassBase* pCandidatePass = candidates[i];
      if (AreInputDescriptionsAvailable(pCandidatePass, done) && ArePassThroughInputsDone(pCandidatePass, done))
      {
        usable.PushBack(pCandidatePass);
        candidates.RemoveAtAndCopy(i);
      }
    }
  }

  if (done.GetCount() < m_Passes.GetCount())
  {
    xiiLog::Error("Pipeline: Not all nodes could be initialized!");

    for (auto& pPass : m_Passes)
    {
      if (!done.Contains(pPass.Borrow()))
      {
        xiiLog::Error("Failed to initialize node: {} - {}", pPass->GetName(), pPass->GetDynamicRTTI()->GetTypeName());
      }
    }
    return XII_FAILURE;
  }

  struct xiiPipelineSorter
  {
    /// \brief Returns true if a is less than b.
    XII_FORCE_INLINE bool Less(const xiiUniquePtr<xiiRenderPipelinePassBase>& a, const xiiUniquePtr<xiiRenderPipelinePassBase>& b) const { return m_pDone->IndexOf(a.Borrow()) < m_pDone->IndexOf(b.Borrow()); }

    /// \brief Returns true if a is equal to b.
    XII_ALWAYS_INLINE bool Equal(const xiiUniquePtr<xiiRenderPipelinePassBase>& a, const xiiUniquePtr<xiiRenderPipelinePassBase>& b) const { return a.Borrow() == b.Borrow(); }

    xiiHybridArray<xiiRenderPipelinePassBase*, 32U>* m_pDone;
  };

  xiiPipelineSorter pipelineSorter;
  pipelineSorter.m_pDone = &done;
  m_Passes.Sort(pipelineSorter);
  return XII_SUCCESS;
}

xiiResult xiiRenderPipeline::InitializePassResourceDescriptions(const xiiView& view)
{
  xiiLogBlock                                        b("Initialize Pass Resource Descriptions");
  xiiHybridArray<xiiRenderPipelinePassResource*, 10> inputs;
  xiiHybridArray<xiiRenderPipelinePassResource, 10>  outputs;

  for (auto& pPass : m_Passes)
  {
    xiiLogBlock b2("InitializePass", pPass->GetName());

    if (view.GetCamera()->IsStereoscopic() && !pPass->IsStereoAware())
    {
      xiiLog::Error("View '{0}' uses a stereoscopic camera, but the render pass '{1}' does not support stereo rendering!", view.GetName(), pPass->GetName());
    }

    ConnectionData& data = m_Connections[pPass.Borrow()];

    XII_ASSERT_DEBUG(data.m_Inputs.GetCount() == pPass->GetInputPins().GetCount(), "Input pin count mismatch!");
    XII_ASSERT_DEBUG(data.m_Outputs.GetCount() == pPass->GetOutputPins().GetCount(), "Output pin count mismatch!");

    inputs.SetCount(data.m_Inputs.GetCount());
    outputs.SetCount(data.m_Outputs.GetCount());

    // Fill inputs array.
    for (xiiUInt32 i = 0; i < data.m_Inputs.GetCount(); ++i)
    {
      if (data.m_Inputs[i] != nullptr)
      {
        inputs[i] = &data.m_Inputs[i]->m_Resource;
      }
      else
      {
        inputs[i] = nullptr;
      }
    }

    if (pPass->GetResourceDescriptions(view, inputs, outputs).Failed())
    {
      xiiLog::Error("The pass ('{}') could not be successfully queried for resource descriptions.", pPass->GetName());
      return XII_FAILURE;
    }

    // Copy queried outputs into the output connections.
    for (xiiUInt32 i = 0; i < data.m_Outputs.GetCount(); ++i)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        data.m_Outputs[i]->m_Resource = outputs[i];
      }
    }

    // Check pass-through consistency of input / output target descriptions.
    auto pInputPins = pPass->GetInputPins();
    for (const xiiRenderPipelineNodePin* pPin : pInputPins)
    {
      if (pPin->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::PassThrough))
      {
        if (data.m_Outputs[pPin->m_uiOutputIndex] != nullptr)
        {
          if (data.m_Inputs[pPin->m_uiInputIndex] == nullptr)
          {
            xiiLog::Error("The pass of type '{0}' has a pass through pin '{1}' that has an output but no input!", pPass->GetDynamicRTTI()->GetTypeName(), pPass->GetPinName(pPin));
            return XII_FAILURE;
          }
          else if (data.m_Outputs[pPin->m_uiOutputIndex]->m_Resource.m_Type != data.m_Inputs[pPin->m_uiInputIndex]->m_Resource.m_Type)
          {
            xiiLog::Error("The pass has a pass through pin '{0}' that has different resource types for input and output!", pPass->GetPinName(pPin));
            return XII_FAILURE;
          }
          else if (data.m_Outputs[pPin->m_uiOutputIndex]->m_Resource.CalculateDescriptorHash() != data.m_Inputs[pPin->m_uiInputIndex]->m_Resource.CalculateDescriptorHash())
          {
            xiiLog::Error("The pass has a pass through pin '{0}' that has different descriptors for input and output!", pPass->GetPinName(pPin));
            return XII_FAILURE;
          }
        }
      }
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiRenderPipeline::CreatePassResourceUsage(const xiiView& view)
{
  xiiLogBlock b("Create Render Target Usage Data");
  XII_ASSERT_DEBUG(m_ResourceUsage.IsEmpty(), "Need to call ClearRenderPassGraphResources before re-creating the pipeline.");

  m_ConnectionToResourceIndex.Clear();

  // Gather all connections that share the same path-through resource and their first and last usage pass index.
  for (xiiUInt16 i = 0; i < static_cast<xiiUInt16>(m_Passes.GetCount()); ++i)
  {
    const xiiRenderPipelinePassBase* pPass = m_Passes[i].Borrow();
    ConnectionData&                  data  = m_Connections[pPass];

    for (xiiRenderPipelinePassConnection* pConnection : data.m_Inputs)
    {
      if (pConnection != nullptr)
      {
        xiiUInt32 uiDataIdx                         = m_ConnectionToResourceIndex[pConnection];
        m_ResourceUsage[uiDataIdx].m_uiLastUsageIdx = i;
      }
    }

    for (xiiRenderPipelinePassConnection* pConnection : data.m_Outputs)
    {
      if (pConnection != nullptr)
      {
        if (pConnection->m_pOutput->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::PassThrough) && data.m_Inputs[pConnection->m_pOutput->m_uiInputIndex] != nullptr)
        {
          xiiRenderPipelinePassConnection* pCorrespondingInputConnection = data.m_Inputs[pConnection->m_pOutput->m_uiInputIndex];
          XII_ASSERT_DEV(m_ConnectionToResourceIndex.Contains(pCorrespondingInputConnection), "");

          xiiUInt32 uiDataIdx = m_ConnectionToResourceIndex[pCorrespondingInputConnection];
          m_ResourceUsage[uiDataIdx].m_UsedBy.PushBack(pConnection);
          m_ResourceUsage[uiDataIdx].m_uiLastUsageIdx = i;

          XII_ASSERT_DEV(!m_ConnectionToResourceIndex.Contains(pConnection), "");
          m_ConnectionToResourceIndex[pConnection] = uiDataIdx;
        }
        else
        {
          m_ConnectionToResourceIndex[pConnection] = m_ResourceUsage.GetCount();
          ResourceUsageData& resourceData          = m_ResourceUsage.ExpandAndGetRef();

          resourceData.m_uiFirstUsageIdx = i;
          resourceData.m_uiLastUsageIdx  = i;
          resourceData.m_UsedBy.PushBack(pConnection);
        }
      }
    }
  }

  // Find pins that provide resources into the pipeline, e.g. xiiTargetPass pins.
  // There can only be up to one provider pin connected to a resource usage block or there would be an ambiguity which of them provides the resource.
  for (xiiUInt32 i = 0; i < m_ResourceUsage.GetCount(); ++i)
  {
    ResourceUsageData&              resourceUsageData = m_ResourceUsage[i];
    const xiiRenderPipelineNodePin* pResourceProvider = nullptr;
    auto                            CheckForProvider  = [&](const xiiRenderPipelineNodePin* pPin) -> bool {
      if (!pPin->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::ResourceProvider))
        return true;

      if (!pResourceProvider)
      {
        pResourceProvider = pPin;
        return true;
      }

      auto pPinOwner      = static_cast<xiiRenderPipelinePassBase*>(pPin->m_pParent);
      auto pProviderOwner = static_cast<xiiRenderPipelinePassBase*>(pResourceProvider->m_pParent);
      xiiLog::Error("Two provider pins are connected to the same resource either directly or via passthrough pins: {}.{} and {}.{}", pProviderOwner->GetName().IsEmpty() ? pProviderOwner->GetDynamicRTTI()->GetTypeName() : pProviderOwner->GetName(), pProviderOwner->GetPinName(pResourceProvider).GetView(), pPinOwner->GetName().IsEmpty() ? pPinOwner->GetDynamicRTTI()->GetTypeName() : pPinOwner->GetName(), pPinOwner->GetPinName(pPin).GetView());
      return false;
    };

    for (xiiRenderPipelinePassConnection* pUsedByConnection : resourceUsageData.m_UsedBy)
    {
      if (!CheckForProvider(pUsedByConnection->m_pOutput))
        return XII_FAILURE;

      for (const xiiRenderPipelineNodePin* pPin : pUsedByConnection->m_Inputs)
      {
        if (!CheckForProvider(pPin))
          return XII_FAILURE;
      }
    }

    if (pResourceProvider)
    {
      auto                             pPass         = xiiDynamicCast<xiiRenderPipelinePassBase*>(pResourceProvider->m_pParent);
      xiiSharedPtr<xiiGALDeviceObject> pDeviceObject = pPass->QueryResourceProvider(pResourceProvider, xiiRenderPipelineResourceRequest(resourceUsageData.m_UsedBy[0]->m_Resource));
      if (!pDeviceObject)
      {
        // In this case, e.g. xiiTargetPass does not provide a render target for the connection but if the descriptor is set, we can instead use the pool to supplement the missing resource later.
        resourceUsageData.m_pResourceProvider = nullptr;

        for (xiiRenderPipelinePassConnection* pUsedByConnection : resourceUsageData.m_UsedBy)
        {
          if (pUsedByConnection->m_Resource.IsBuffer())
          {
            pUsedByConnection->m_Resource.m_Buffer.m_pBuffer = nullptr;
          }
          else if (pUsedByConnection->m_Resource.IsTexture())
          {
            pUsedByConnection->m_Resource.m_Texture.m_pTexture = nullptr;
          }
          else if (pUsedByConnection->m_Resource.IsSampler())
          {
            pUsedByConnection->m_Resource.m_Sampler.m_pSampler = nullptr;
          }
        }
      }
      else
      {
        resourceUsageData.m_pResourceProvider = pResourceProvider;

        for (auto pUsedByConnection : resourceUsageData.m_UsedBy)
        {
          if (pUsedByConnection->m_Resource.IsBuffer())
          {
            pUsedByConnection->m_Resource.m_Buffer.m_pBuffer = pDeviceObject.Downcast<xiiGALBuffer>();

            XII_ASSERT_DEBUG(pUsedByConnection->m_Resource.m_Buffer.m_Description == pUsedByConnection->m_Resource.m_Buffer.m_pBuffer->GetDescription(), "Invalid buffer provided.");
          }
          else if (pUsedByConnection->m_Resource.IsTexture())
          {
            pUsedByConnection->m_Resource.m_Texture.m_pTexture = pDeviceObject.Downcast<xiiGALTexture>();

            // XII_ASSERT_DEBUG(pUsedByConnection->m_Resource.m_Texture.m_Description == pUsedByConnection->m_Resource.m_Texture.m_pTexture->GetDescription(), "Invalid texture provided.");
          }
          else if (pUsedByConnection->m_Resource.IsSampler())
          {
            pUsedByConnection->m_Resource.m_Sampler.m_pSampler = pDeviceObject.Downcast<xiiGALSampler>();

            XII_ASSERT_DEBUG(pUsedByConnection->m_Resource.m_Sampler.m_Description == pUsedByConnection->m_Resource.m_Sampler.m_pSampler->GetDescription(), "Invalid sampler provided.");
          }
        }
      }
    }
  }

  // If a resource descriptor has this hash, it is uninitialized and no resource will be created at runtime.
  static xiiUInt32 uiDefaultTextureHash = xiiGALTextureCreationDescription().CalculateHash();
  static xiiUInt32 uiDefaultBufferHash  = xiiGALBufferCreationDescription().CalculateHash();
  static xiiUInt32 uiDefaultSamplerHash = xiiGALSamplerCreationDescription().CalculateHash();

  // Inconvenient loop to gather all ResourceUsageData indices that are not provider resources and valid.
  for (xiiUInt32 i = 0; i < m_ResourceUsage.GetCount(); ++i)
  {
    ResourceUsageData& data = m_ResourceUsage[i];

    if (data.m_pResourceProvider || data.m_UsedBy[0]->m_Resource.m_Type == xiiRenderPipelineNodePinResourceType::Unknown)
      continue;

    if (data.m_UsedBy[0]->m_Resource.IsBuffer() && data.m_UsedBy[0]->m_Resource.m_Buffer.m_Description.CalculateHash() == uiDefaultBufferHash)
      continue;

    if (data.m_UsedBy[0]->m_Resource.IsTexture() && data.m_UsedBy[0]->m_Resource.m_Texture.m_Description.CalculateHash() == uiDefaultTextureHash)
      continue;

    if (data.m_UsedBy[0]->m_Resource.IsSampler() && data.m_UsedBy[0]->m_Resource.m_Sampler.m_Description.CalculateHash() == uiDefaultSamplerHash)
      continue;

    m_ResourceUsageIdxSortedByFirstUsage.PushBack((xiiUInt16)i);
    m_ResourceUsageIdxSortedByLastUsage.PushBack((xiiUInt16)i);
  }

  // Sort first and last usage arrays, these will determine the lifetime of the pool resources.
  struct FirstUsageComparer
  {
    FirstUsageComparer(xiiDynamicArray<ResourceUsageData>& ref_resourceUsage) :
      m_ResourceUsage(ref_resourceUsage)
    {
    }

    XII_ALWAYS_INLINE bool Less(xiiUInt16 a, xiiUInt16 b) const { return m_ResourceUsage[a].m_uiFirstUsageIdx < m_ResourceUsage[b].m_uiFirstUsageIdx; }

    xiiDynamicArray<ResourceUsageData>& m_ResourceUsage;
  };

  struct LastUsageComparer
  {
    LastUsageComparer(xiiDynamicArray<ResourceUsageData>& ref_resourceUsage) :
      m_ResourceUsage(ref_resourceUsage)
    {
    }

    XII_ALWAYS_INLINE bool Less(xiiUInt16 a, xiiUInt16 b) const { return m_ResourceUsage[a].m_uiLastUsageIdx < m_ResourceUsage[b].m_uiLastUsageIdx; }

    xiiDynamicArray<ResourceUsageData>& m_ResourceUsage;
  };

  m_ResourceUsageIdxSortedByFirstUsage.Sort(FirstUsageComparer(m_ResourceUsage));
  m_ResourceUsageIdxSortedByLastUsage.Sort(LastUsageComparer(m_ResourceUsage));

  return XII_SUCCESS;
}

xiiResult xiiRenderPipeline::InitializeRenderPipelinePasses(const xiiView& view)
{
  xiiLogBlock b("Initialize Render Pipeline Passes");

  // Initialize every pass now.
  for (xiiUniquePtr<xiiRenderPipelinePassBase>& pPass : m_Passes)
  {
    ConnectionData& data = m_Connections[pPass.Borrow()];

    XII_SUCCEED_OR_RETURN(pPass->InitializeRenderPipelinePass(view, data.m_Inputs, data.m_Outputs));
  }

  return XII_SUCCESS;
}

void xiiRenderPipeline::SortExtractors()
{
  struct Helper
  {
    static bool FindDependency(const xiiHashedString& sDependency, xiiArrayPtr<xiiUniquePtr<xiiExtractor>> container)
    {
      for (auto& extractor : container)
      {
        if (sDependency == xiiTempHashedString(extractor->GetDynamicRTTI()->GetTypeNameHash()))
        {
          return true;
        }
      }
      return false;
    }
  };

  m_SortedExtractors.Clear();
  m_SortedExtractors.Reserve(m_Extractors.GetCount());

  xiiUInt32 uiIndex = 0;
  while (!m_Extractors.IsEmpty())
  {
    xiiUniquePtr<xiiExtractor>& extractor = m_Extractors[uiIndex];

    bool bAllDependenciesFound = true;
    for (auto& sDependency : extractor->m_DependsOn)
    {
      if (!Helper::FindDependency(sDependency, m_SortedExtractors))
      {
        bAllDependenciesFound = false;
        break;
      }
    }

    if (bAllDependenciesFound)
    {
      m_SortedExtractors.PushBack(std::move(extractor));
      m_Extractors.RemoveAtAndCopy(uiIndex);
    }
    else
    {
      ++uiIndex;
    }

    if (uiIndex >= m_Extractors.GetCount())
    {
      uiIndex = 0;
    }
  }

  m_Extractors.Swap(m_SortedExtractors);
}

void xiiRenderPipeline::UpdateViewData(const xiiView& view, xiiUInt32 uiDataIndex)
{
  if (!view.IsValid())
    return;

  if (uiDataIndex == xiiRenderWorld::GetDataIndexForExtraction() && m_CurrentExtractThread != (xiiThreadID)0)
    return;

  XII_ASSERT_DEV(uiDataIndex <= 1, "Data index must be 0 or 1.");
  auto& data = m_Data[uiDataIndex];

  data.SetCamera(*view.GetCamera());
  data.SetViewData(view.GetData());
}

void xiiRenderPipeline::AddExtractor(xiiUniquePtr<xiiExtractor>&& pExtractor)
{
  m_Extractors.PushBack(std::move(pExtractor));
}

void xiiRenderPipeline::RemoveExtractor(xiiExtractor* pExtractor)
{
  for (xiiUInt32 i = 0; i < m_Extractors.GetCount(); ++i)
  {
    if (m_Extractors[i].Borrow() == pExtractor)
    {
      m_Extractors.RemoveAtAndCopy(i);
      break;
    }
  }
}

void xiiRenderPipeline::GetExtractors(xiiDynamicArray<const xiiExtractor*>& ref_extractors) const
{
  ref_extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}

void xiiRenderPipeline::GetExtractors(xiiDynamicArray<xiiExtractor*>& ref_extractors)
{
  ref_extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}

xiiExtractor* xiiRenderPipeline::GetExtractorByName(const xiiStringView& sExtractorName)
{
  for (auto& pExtractor : m_Extractors)
  {
    if (sExtractorName.IsEqual(pExtractor->GetName()))
    {
      return pExtractor.Borrow();
    }
  }
  return nullptr;
}

void xiiRenderPipeline::RemoveConnections(xiiRenderPipelinePassBase* pPass)
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return;

  ConnectionData& data = it.Value();
  for (xiiUInt32 i = 0; i < data.m_Inputs.GetCount(); ++i)
  {
    xiiRenderPipelinePassConnection* pConnection = data.m_Inputs[i];
    if (pConnection != nullptr)
    {
      xiiRenderPipelinePassBase* pSource = static_cast<xiiRenderPipelinePassBase*>(pConnection->m_pOutput->m_pParent);
      bool                       bResult = Disconnect(pSource, pSource->GetPinName(pConnection->m_pOutput), pPass, pPass->GetPinName(pPass->GetInputPins()[i]));
      XII_IGNORE_UNUSED(bResult);
      XII_ASSERT_DEBUG(bResult, "xiiRenderPipeline::RemoveConnections should not fail to disconnect pins!");
    }
  }
  for (xiiUInt32 i = 0; i < data.m_Outputs.GetCount(); ++i)
  {
    xiiRenderPipelinePassConnection* pConnection = data.m_Outputs[i];
    while (pConnection != nullptr)
    {
      xiiRenderPipelinePassBase* pTarget = static_cast<xiiRenderPipelinePassBase*>(pConnection->m_Inputs[0]->m_pParent);
      bool                       bResult = Disconnect(pPass, pPass->GetPinName(pConnection->m_pOutput), pTarget, pTarget->GetPinName(pConnection->m_Inputs[0]));
      XII_IGNORE_UNUSED(bResult);
      XII_ASSERT_DEBUG(bResult, "xiiRenderPipeline::RemoveConnections should not fail to disconnect pins!");

      pConnection = data.m_Outputs[i];
    }
  }
}

void xiiRenderPipeline::ClearRenderPassGraphResources()
{
  m_ResourceUsage.Clear();
  m_ResourceUsageIdxSortedByFirstUsage.Clear();
  m_ResourceUsageIdxSortedByLastUsage.Clear();

  for (auto it = m_Connections.GetIterator(); it.IsValid(); ++it)
  {
    auto& connection = it.Value();

    for (auto pConnection : connection.m_Outputs)
    {
      if (pConnection)
      {
        pConnection->m_Resource = xiiRenderPipelinePassResource();
      }
    }
  }
}

bool xiiRenderPipeline::AreInputDescriptionsAvailable(const xiiRenderPipelinePassBase* pPass, const xiiHybridArray<xiiRenderPipelinePassBase*, 32>& done) const
{
  auto                  it   = m_Connections.Find(pPass);
  const ConnectionData& data = it.Value();
  for (xiiUInt32 i = 0; i < data.m_Inputs.GetCount(); ++i)
  {
    const xiiRenderPipelinePassConnection* pConnection = data.m_Inputs[i];

    if (pConnection != nullptr)
    {
      // If the connections source is not done yet, the connections output is undefined yet and the inputs can't be processed yet.
      if (!done.Contains(static_cast<xiiRenderPipelinePassBase*>(pConnection->m_pOutput->m_pParent)))
      {
        return false;
      }
    }
  }

  return true;
}

bool xiiRenderPipeline::ArePassThroughInputsDone(const xiiRenderPipelinePassBase* pPass, const xiiHybridArray<xiiRenderPipelinePassBase*, 32>& done) const
{
  auto                  it     = m_Connections.Find(pPass);
  const ConnectionData& data   = it.Value();
  auto                  inputs = pPass->GetInputPins();
  for (xiiUInt32 i = 0; i < inputs.GetCount(); ++i)
  {
    const xiiRenderPipelineNodePin* pPin = inputs[i];

    if (pPin->m_Flags.IsSet(xiiRenderPipelineNodePinFlags::PassThrough))
    {
      const xiiRenderPipelinePassConnection* pConnection = data.m_Inputs[pPin->m_uiInputIndex];

      if (pConnection != nullptr)
      {
        for (const xiiRenderPipelineNodePin* pInputPin : pConnection->m_Inputs)
        {
          // Any input that is also connected to the source of pPin must be done before we can use the pass through input.
          if (pInputPin != pPin && !done.Contains(static_cast<xiiRenderPipelinePassBase*>(pInputPin->m_pParent)))
          {
            return false;
          }
        }
      }
    }
  }
  return true;
}

xiiFrameDataProviderBase* xiiRenderPipeline::GetFrameDataProvider(const xiiRTTI* pRtti) const
{
  xiiUInt32 uiIndex = 0;
  if (m_TypeToDataProviderIndex.TryGetValue(pRtti, uiIndex))
  {
    return m_DataProviders[uiIndex].Borrow();
  }

  xiiUniquePtr<xiiFrameDataProviderBase> pNewDataProvider = pRtti->GetAllocator()->Allocate<xiiFrameDataProviderBase>();
  xiiFrameDataProviderBase*              pResult          = pNewDataProvider.Borrow();
  pResult->m_pOwnerPipeline                               = this;

  m_TypeToDataProviderIndex.Insert(pRtti, m_DataProviders.GetCount());
  m_DataProviders.PushBack(std::move(pNewDataProvider));

  return pResult;
}

void xiiRenderPipeline::ExtractData(const xiiView& view)
{
  XII_ASSERT_DEV(m_CurrentExtractThread == (xiiThreadID)0, "Extract must not be called from multiple threads.");
  m_CurrentExtractThread = xiiThreadUtils::GetCurrentThreadID();

  // Is this view already extracted?
  if (m_uiLastExtractionFrame == xiiRenderWorld::GetFrameCounter())
  {
    XII_REPORT_FAILURE("View '{0}' is extracted multiple times", view.GetName());
    return;
  }

  m_uiLastExtractionFrame = xiiRenderWorld::GetFrameCounter();

  // Determine visible objects
  FindVisibleObjects(view);

  // Extract and sort data
  auto& data = m_Data[xiiRenderWorld::GetDataIndexForExtraction()];

  // Usually clear is not needed, only if the multithreading flag is switched during runtime.
  data.Clear();

  // Store camera and viewdata
  data.SetCamera(*view.GetCamera());
  data.SetLodCamera(*view.GetLodCamera());
  data.SetViewData(view.GetData());
  data.SetWorldTime(view.GetWorld()->GetClock().GetAccumulatedTime());
  data.SetWorldDebugContext(view.GetWorld());
  data.SetViewDebugContext(view.GetHandle());

  // Extract object render data
  for (auto& pExtractor : m_Extractors)
  {
    if (pExtractor->m_bActive)
    {
      XII_PROFILE_SCOPE(pExtractor->m_sName.GetView());

      pExtractor->Extract(view, m_VisibleObjects, data);
    }
  }

  for (auto& processor : m_RenderDataProcessors)
  {
    processor(data);
  }

  data.SortAndBatch();

  for (auto& pExtractor : m_Extractors)
  {
    if (pExtractor->m_bActive)
    {
      XII_PROFILE_SCOPE(pExtractor->m_sName.GetView());

      pExtractor->PostSortAndBatch(view, m_VisibleObjects, data);
    }
  }

  m_CurrentExtractThread = (xiiThreadID)0;
}

xiiUniquePtr<xiiRasterizerViewPool> g_pRasterizerViewPool;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, SwRasterizer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    g_pRasterizerViewPool = XII_DEFAULT_NEW(xiiRasterizerViewPool);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    g_pRasterizerViewPool.Clear();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

void xiiRenderPipeline::FindVisibleObjects(const xiiView& view)
{
  XII_PROFILE_SCOPE("Visibility Culling");

  xiiFrustum frustum;
  view.ComputeCullingFrustum(frustum);

  XII_LOCK(view.GetWorld()->GetReadMarker());

  const bool bIsMainView = (view.GetCameraUsageHint() == xiiCameraUsageHint::MainView || view.GetCameraUsageHint() == xiiCameraUsageHint::EditorView);
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const bool                   bRecordStats = cvar_SpatialCullingShowStats && bIsMainView;
  xiiSpatialSystem::QueryStats stats;
#endif

  xiiSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = xiiDefaultSpatialDataCategories::RenderStatic.GetBitmask() | xiiDefaultSpatialDataCategories::RenderDynamic.GetBitmask();
  queryParams.m_pIncludeTags      = &view.m_IncludeTags;
  queryParams.m_pExcludeTags      = &view.m_ExcludeTags;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  queryParams.m_pStats = bRecordStats ? &stats : nullptr;
#endif

  xiiFrustum     limitedFrustum                               = frustum;
  const xiiPlane farPlane                                     = limitedFrustum.GetPlane(xiiFrustum::PlaneType::FarPlane);
  limitedFrustum.AccessPlane(xiiFrustum::PlaneType::FarPlane) = xiiPlane::MakeFromNormalAndPoint(farPlane.m_vNormal, view.GetCullingCamera()->GetCenterPosition() + farPlane.m_vNormal * cvar_SpatialCullingOcclusionFarPlane.GetValue()); // only use occluders closer than this

  xiiRasterizerView* pRasterizer = PrepareOcclusionCulling(limitedFrustum, view);
  XII_SCOPE_EXIT(g_pRasterizerViewPool->ReturnRasterizerView(pRasterizer));

  const xiiVisibilityState::Enum visType = bIsMainView ? xiiVisibilityState::Direct : xiiVisibilityState::Indirect;

  if (pRasterizer != nullptr && pRasterizer->HasRasterizedAnyOccluders())
  {
    auto IsOccluded = [=](const xiiSimdBBox& aabb) {
      // grow the bbox by some percent to counter the lower precision of the occlusion buffer

      const xiiSimdVec4f c     = aabb.GetCenter();
      const xiiSimdVec4f e     = aabb.GetHalfExtents();
      const xiiSimdBBox  aabb2 = xiiSimdBBox::MakeFromCenterAndHalfExtents(c, e.CompMul(xiiSimdVec4f(1.0f + cvar_SpatialCullingOcclusionBoundsInflation)));

      return !pRasterizer->IsVisible(aabb2);
    };

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, IsOccluded, visType);
  }
  else
  {
    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, visType);
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (pRasterizer)
  {
    if (view.GetCameraUsageHint() == xiiCameraUsageHint::EditorView || view.GetCameraUsageHint() == xiiCameraUsageHint::MainView)
    {
      PreviewOcclusionBuffer(*pRasterizer, view);
    }
  }
#endif

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiViewHandle hView = view.GetHandle();

  if (cvar_SpatialCullingVis && bIsMainView)
  {
    xiiDebugRenderer::DrawLineFrustum(view.GetWorld(), frustum, xiiColor::LimeGreen, false);
  }

  if (bRecordStats)
  {
    xiiStringBuilder sb;

    xiiDebugRenderer::DrawInfoText(hView, xiiDebugTextPlacement::TopLeft, "VisCulling", "Visibility Culling Stats", xiiColor::LimeGreen);

    sb.SetFormat("Total Num Objects: {0}", stats.m_uiTotalNumObjects);
    xiiDebugRenderer::DrawInfoText(hView, xiiDebugTextPlacement::TopLeft, "VisCulling", sb, xiiColor::LimeGreen);

    sb.SetFormat("Num Objects Tested: {0}", stats.m_uiNumObjectsTested);
    xiiDebugRenderer::DrawInfoText(hView, xiiDebugTextPlacement::TopLeft, "VisCulling", sb, xiiColor::LimeGreen);

    sb.SetFormat("Num Objects Passed: {0}", stats.m_uiNumObjectsPassed);
    xiiDebugRenderer::DrawInfoText(hView, xiiDebugTextPlacement::TopLeft, "VisCulling", sb, xiiColor::LimeGreen);

    // Exponential moving average for better readability.
    m_AverageCullingTime = xiiMath::Lerp(m_AverageCullingTime, stats.m_TimeTaken, 0.05f);

    sb.SetFormat("Time Taken: {0}ms", m_AverageCullingTime.GetMilliseconds());
    xiiDebugRenderer::DrawInfoText(hView, xiiDebugTextPlacement::TopLeft, "VisCulling", sb, xiiColor::LimeGreen);

    view.GetWorld()->GetSpatialSystem()->GetInternalStats(sb);
    xiiDebugRenderer::DrawInfoText(hView, xiiDebugTextPlacement::TopLeft, "VisCulling", sb, xiiColor::AntiqueWhite);
  }
#endif
}

void xiiRenderPipeline::Render(xiiRenderContext* pRenderContext)
{
  XII_PROFILE_SCOPE(m_sName.GetView());

  XII_ASSERT_DEV(m_PipelineState != PipelineState::Uninitialized, "Pipeline must be rebuild before rendering.");
  if (m_PipelineState == PipelineState::RebuildError)
    return;

  XII_ASSERT_DEV(m_CurrentRenderThread == (xiiThreadID)0, "Render must not be called from multiple threads.");
  m_CurrentRenderThread = xiiThreadUtils::GetCurrentThreadID();

  XII_ASSERT_DEV(m_uiLastRenderFrame != xiiRenderWorld::GetFrameCounter(), "Render must not be called multiple times per frame.");
  m_uiLastRenderFrame = xiiRenderWorld::GetFrameCounter();

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  auto&                      data    = m_Data[xiiRenderWorld::GetDataIndexForRendering()];

  xiiRenderViewContext renderViewContext;
  renderViewContext.m_pCamera            = &data.GetCamera();
  renderViewContext.m_pViewData          = &data.GetViewData();
  renderViewContext.m_pRenderContext     = pRenderContext;
  renderViewContext.m_pWorldDebugContext = &data.GetWorldDebugContext();
  renderViewContext.m_pViewDebugContext  = &data.GetViewDebugContext();

  {
    xiiGlobalConstants* pGlobalConstants = pRenderContext->GetGlobalConstants();

    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      pGlobalConstants->CameraToScreenMatrix[i] = renderViewContext.m_pViewData->m_ProjectionMatrix[i];
      pGlobalConstants->ScreenToCameraMatrix[i] = renderViewContext.m_pViewData->m_InverseProjectionMatrix[i];
      pGlobalConstants->WorldToCameraMatrix[i]  = renderViewContext.m_pViewData->m_ViewMatrix[i];
      pGlobalConstants->CameraToWorldMatrix[i]  = renderViewContext.m_pViewData->m_InverseViewMatrix[i];
      pGlobalConstants->WorldToScreenMatrix[i]  = renderViewContext.m_pViewData->m_ViewProjectionMatrix[i];
      pGlobalConstants->ScreenToWorldMatrix[i]  = renderViewContext.m_pViewData->m_InverseViewProjectionMatrix[i];
    }

    const xiiRectFloat& viewport   = renderViewContext.m_pViewData->m_ViewPortRect;
    pGlobalConstants->ViewportSize = xiiVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);

    float fNear                  = renderViewContext.m_pCamera->GetNearPlane();
    float fFar                   = renderViewContext.m_pCamera->GetFarPlane();
    pGlobalConstants->ClipPlanes = xiiVec4(fNear, fFar, 1.0f / fFar, 0.0f);

    const bool bIsDirectionalLightShadow = renderViewContext.m_pViewData->m_CameraUsageHint == xiiCameraUsageHint::Shadow && renderViewContext.m_pCamera->IsOrthographic();
    pGlobalConstants->MaxZValue          = bIsDirectionalLightShadow ? 0.0f : xiiMath::MinValue<float>();

    pGlobalConstants->Exposure   = renderViewContext.m_pCamera->GetExposure();
    pGlobalConstants->RenderPass = xiiViewRenderMode::GetRenderPassForShader(renderViewContext.m_pViewData->m_ViewRenderMode);

    pRenderContext->SetGlobalAndWorldTimeConstants(data.GetWorldTime());
  }

  // Set camera mode permutation variable here since it doesn't change throughout the frame.
  static xiiHashedString sCameraMode  = xiiMakeHashedString("CAMERA_MODE");
  static xiiHashedString sOrtho       = xiiMakeHashedString("CAMERA_MODE_ORTHO");
  static xiiHashedString sPerspective = xiiMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static xiiHashedString sStereo      = xiiMakeHashedString("CAMERA_MODE_STEREO");

  static xiiHashedString sClipSpaceFlipped = xiiMakeHashedString("CLIP_SPACE_FLIPPED");
  static xiiHashedString sTrue             = xiiMakeHashedString("TRUE");
  static xiiHashedString sFalse            = xiiMakeHashedString("FALSE");

  if (renderViewContext.m_pCamera->IsOrthographic())
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable(sCameraMode, sOrtho);
  else if (renderViewContext.m_pCamera->IsStereoscopic())
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable(sCameraMode, sStereo);
  else
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable(sCameraMode, sPerspective);

  XII_ASSERT_DEV(pDevice->GetFeatures().m_VertexShaderRenderTargetArrayIndex == xiiGALDeviceFeatureState::Enabled, "Vertex shader render target index must be supported for stereo rendering.");
  
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable(sClipSpaceFlipped, xiiClipSpaceYMode::RenderToTextureDefault == xiiClipSpaceYMode::Flipped ? sTrue : sFalse);

  // Also set pipeline specific permutation variables.
  for (xiiGALPermutationVariable& permutationVariable : m_PermutationVariables)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable(permutationVariable.m_sName, permutationVariable.m_sValue);
  }

  xiiRenderWorldRenderEvent renderEvent;
  renderEvent.m_Type               = xiiRenderWorldRenderEvent::Type::BeforePipelineExecution;
  renderEvent.m_pPipeline          = this;
  renderEvent.m_pRenderViewContext = &renderViewContext;
  renderEvent.m_uiFrameCounter     = xiiRenderWorld::GetFrameCounter();
  {
    XII_PROFILE_SCOPE("BeforePipelineExecution");
    xiiRenderWorld::s_RenderEvent.Broadcast(renderEvent);
  }

  {
    // Update resources from resource providers as these can change every frame (e.g. swap chain textures).
    for (ResourceUsageData& resourceUsageData : m_ResourceUsage)
    {
      if (!resourceUsageData.m_pResourceProvider)
        continue;

      auto                             pPass         = static_cast<xiiRenderPipelinePassBase*>(resourceUsageData.m_pResourceProvider->m_pParent);
      xiiSharedPtr<xiiGALDeviceObject> pDeviceObject = pPass->QueryResourceProvider(resourceUsageData.m_pResourceProvider, xiiRenderPipelineResourceRequest(resourceUsageData.m_UsedBy[0]->m_Resource));
      for (xiiRenderPipelinePassConnection* pUsedByConnection : resourceUsageData.m_UsedBy)
      {
        for (auto pUsedByConnection : resourceUsageData.m_UsedBy)
        {
          if (pUsedByConnection->m_Resource.IsBuffer())
          {
            pUsedByConnection->m_Resource.m_Buffer.m_pBuffer = pDeviceObject.Downcast<xiiGALBuffer>();

            XII_ASSERT_DEBUG(pUsedByConnection->m_Resource.m_Buffer.m_Description == pUsedByConnection->m_Resource.m_Buffer.m_pBuffer->GetDescription(), "Buffer mismatch or invalid.");
          }
          else if (pUsedByConnection->m_Resource.IsTexture())
          {
            pUsedByConnection->m_Resource.m_Texture.m_pTexture = pDeviceObject.Downcast<xiiGALTexture>();

            // XII_ASSERT_DEBUG(pUsedByConnection->m_Resource.m_Texture.m_Description == pUsedByConnection->m_Resource.m_Texture.m_pTexture->GetDescription(), "Texture mismatch or invalid.");
          }
          else if (pUsedByConnection->m_Resource.IsSampler())
          {
            pUsedByConnection->m_Resource.m_Sampler.m_pSampler = pDeviceObject.Downcast<xiiGALSampler>();

            XII_ASSERT_DEBUG(pUsedByConnection->m_Resource.m_Sampler.m_Description == pUsedByConnection->m_Resource.m_Sampler.m_pSampler->GetDescription(), "Sampler mismatch or invalid.");
          }
        }
      }
    }

    xiiUInt32 uiCurrentFirstUsageIdx = 0;
    xiiUInt32 uiCurrentLastUsageIdx  = 0;
    for (xiiUInt32 i = 0; i < m_Passes.GetCount(); ++i)
    {
      xiiUniquePtr<xiiRenderPipelinePassBase>& pPass = m_Passes[i];

      XII_PROFILE_SCOPE(pPass->GetName());
      xiiLogBlock passBlock("Render Pass", pPass->GetName());

      // Create pool resources.
      for (; uiCurrentFirstUsageIdx < m_ResourceUsageIdxSortedByFirstUsage.GetCount();)
      {
        xiiUInt16          uiCurrentUsageData = m_ResourceUsageIdxSortedByFirstUsage[uiCurrentFirstUsageIdx];
        ResourceUsageData& usageData          = m_ResourceUsage[uiCurrentUsageData];

        if (usageData.m_uiFirstUsageIdx == i)
        {
          xiiSharedPtr<xiiGALDeviceObject> pDeviceObject;

          if (usageData.m_UsedBy[0]->m_Resource.IsBuffer())
          {
            pDeviceObject = xiiGPUResourcePool::GetDefaultInstance()->GetBuffer(usageData.m_UsedBy[0]->m_Resource.m_Buffer.m_Description);

            XII_ASSERT_DEBUG(pDeviceObject.Downcast<xiiGALBuffer>()->GetDescription() == usageData.m_UsedBy[0]->m_Resource.m_Buffer.m_Description, "GPU pool returned a buffer with invalid description!");
          }
          else if (usageData.m_UsedBy[0]->m_Resource.IsTexture())
          {
            pDeviceObject = xiiGPUResourcePool::GetDefaultInstance()->GetTexture(usageData.m_UsedBy[0]->m_Resource.m_Texture.m_Description);

            XII_ASSERT_DEBUG(pDeviceObject.Downcast<xiiGALTexture>()->GetDescription() == usageData.m_UsedBy[0]->m_Resource.m_Texture.m_Description, "GPU pool returned a texture with invalid description!");
          }
          else if (usageData.m_UsedBy[0]->m_Resource.IsSampler())
          {
            pDeviceObject = xiiGPUResourcePool::GetDefaultInstance()->GetSampler(usageData.m_UsedBy[0]->m_Resource.m_Sampler.m_Description);

            XII_ASSERT_DEBUG(pDeviceObject.Downcast<xiiGALSampler>()->GetDescription() == usageData.m_UsedBy[0]->m_Resource.m_Sampler.m_Description, "GPU pool returned a sampler with invalid description!");
          }

          XII_ASSERT_DEV(pDeviceObject != nullptr, "GPU pool returned an invalidated resource!");

          for (xiiRenderPipelinePassConnection* pUsedByConnection : usageData.m_UsedBy)
          {
            if (pUsedByConnection->m_Resource.IsBuffer())
            {
              pUsedByConnection->m_Resource.m_Buffer.m_pBuffer = pDeviceObject.Downcast<xiiGALBuffer>();
            }
            else if (pUsedByConnection->m_Resource.IsTexture())
            {
              pUsedByConnection->m_Resource.m_Texture.m_pTexture = pDeviceObject.Downcast<xiiGALTexture>();
            }
            else if (pUsedByConnection->m_Resource.IsSampler())
            {
              pUsedByConnection->m_Resource.m_Sampler.m_pSampler = pDeviceObject.Downcast<xiiGALSampler>();
            }
          }
          ++uiCurrentFirstUsageIdx;
        }
        else
        {
          // The current usage data blocks m_uiFirstUsageIdx isn't reached yet so wait.
          break;
        }
      }

      // Execute pass block.
      {
        ConnectionData& connectionData = m_Connections[pPass.Borrow()];

        if (pPass->m_bActive)
        {
          pPass->Execute(renderViewContext, connectionData.m_Inputs, connectionData.m_Outputs);
        }
        else
        {
          pPass->ExecuteInactive(renderViewContext, connectionData.m_Inputs, connectionData.m_Outputs);
        }
      }

      // Release pool textures.
      for (; uiCurrentLastUsageIdx < m_ResourceUsageIdxSortedByLastUsage.GetCount();)
      {
        xiiUInt16          uiCurrentUsageData = m_ResourceUsageIdxSortedByLastUsage[uiCurrentLastUsageIdx];
        ResourceUsageData& usageData          = m_ResourceUsage[uiCurrentUsageData];
        if (usageData.m_uiLastUsageIdx == i)
        {
          if (usageData.m_UsedBy[0]->m_Resource.IsBuffer())
          {
            xiiGPUResourcePool::GetDefaultInstance()->ReturnBuffer(std::move(usageData.m_UsedBy[0]->m_Resource.m_Buffer.m_pBuffer));
          }
          else if (usageData.m_UsedBy[0]->m_Resource.IsTexture())
          {
            xiiGPUResourcePool::GetDefaultInstance()->ReturnTexture(std::move(usageData.m_UsedBy[0]->m_Resource.m_Texture.m_pTexture));
          }
          else if (usageData.m_UsedBy[0]->m_Resource.IsSampler())
          {
            xiiGPUResourcePool::GetDefaultInstance()->ReturnSampler(std::move(usageData.m_UsedBy[0]->m_Resource.m_Sampler.m_pSampler));
          }

          for (xiiRenderPipelinePassConnection* pUsedByConnection : usageData.m_UsedBy)
          {
            if (pUsedByConnection->m_Resource.IsBuffer())
            {
              pUsedByConnection->m_Resource.m_Buffer.m_pBuffer.Clear();
            }
            else if (pUsedByConnection->m_Resource.IsTexture())
            {
              pUsedByConnection->m_Resource.m_Texture.m_pTexture.Clear();
            }
            else if (pUsedByConnection->m_Resource.IsSampler())
            {
              pUsedByConnection->m_Resource.m_Sampler.m_pSampler.Clear();
            }
          }

          ++uiCurrentLastUsageIdx;
        }
        else
        {
          // The current usage data blocks m_uiLastUsageIdx isn't reached yet so wait.
          break;
        }
      }
    }

    XII_ASSERT_DEV(uiCurrentFirstUsageIdx == m_ResourceUsageIdxSortedByFirstUsage.GetCount(), "Rendering all passes should have moved us through all texture usage blocks!");
    XII_ASSERT_DEV(uiCurrentLastUsageIdx == m_ResourceUsageIdxSortedByLastUsage.GetCount(), "Rendering all passes should have moved us through all texture usage blocks!");
  }

  renderEvent.m_Type = xiiRenderWorldRenderEvent::Type::AfterPipelineExecution;
  {
    XII_PROFILE_SCOPE("AfterPipelineExecution");
    xiiRenderWorld::s_RenderEvent.Broadcast(renderEvent);
  }

  data.Clear();

  m_CurrentRenderThread = (xiiThreadID)0;
}

const xiiExtractedRenderData& xiiRenderPipeline::GetRenderData() const
{
  return m_Data[xiiRenderWorld::GetDataIndexForRendering()];
}

xiiRenderDataBatchList xiiRenderPipeline::GetRenderDataBatchesWithCategory(xiiRenderData::Category category) const
{
  auto& data = m_Data[xiiRenderWorld::GetDataIndexForRendering()];
  return data.GetRenderDataBatchesWithCategory(category);
}

xiiUInt32 xiiRenderPipeline::AddRenderDataProcessor(RenderDataProcessor processor)
{
  xiiUInt32 uiIndex = m_RenderDataProcessors.GetCount();
  m_RenderDataProcessors.PushBack(processor);
  return uiIndex;
}

void xiiRenderPipeline::CreateDgmlGraph(xiiDGMLGraph& ref_graph)
{
  xiiStringBuilder                                      sTemp;
  xiiHashTable<const xiiRenderPipelineNode*, xiiUInt32> nodeMap;
  nodeMap.Reserve(m_Passes.GetCount() + m_ResourceUsage.GetCount() * 3);

  xiiStringBuilder sTmp;
  for (xiiUInt32 p = 0; p < m_Passes.GetCount(); ++p)
  {
    const auto& pPass = m_Passes[p];
    sTemp.SetFormat("#{}: {}", p, pPass->GetName().IsEmpty() ? pPass->GetDynamicRTTI()->GetTypeName() : pPass->GetName());

    xiiDGMLGraph::NodeDesc nd;
    nd.m_Color            = xiiColor::Gray;
    nd.m_Shape            = xiiDGMLGraph::NodeShape::Rectangle;
    xiiUInt32 uiGraphNode = ref_graph.AddNode(sTemp, &nd);
    nodeMap.Insert(pPass.Borrow(), uiGraphNode);
  }

  for (xiiUInt32 i = 0; i < m_ResourceUsage.GetCount(); ++i)
  {
    const ResourceUsageData& data = m_ResourceUsage[i];

    for (const xiiRenderPipelinePassConnection* pConnection : data.m_UsedBy)
    {
      xiiDGMLGraph::NodeDesc nd;
      nd.m_Color = data.m_pResourceProvider ? xiiColor::Black : xiiColorScheme::GetColor(static_cast<xiiColorScheme::Enum>(i % xiiColorScheme::Count), 4);
      nd.m_Shape = xiiDGMLGraph::NodeShape::RoundedRectangle;

      xiiStringBuilder sFormat;

      if (pConnection->m_Resource.IsBuffer())
      {
        sFormat.AppendFormat("[{} bytes", pConnection->m_Resource.m_Buffer.m_Description.m_uiSize);

        if (pConnection->m_Resource.m_Buffer.m_Description.m_uiElementByteStride > 0)
        {
          sFormat.AppendFormat(", Stride: {}", pConnection->m_Resource.m_Buffer.m_Description.m_uiElementByteStride);
        }

        if (xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALBufferMode>(), pConnection->m_Resource.m_Buffer.m_Description.m_Mode.GetValue(), sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Mode: {}", sTmp);
        }
        else
        {
          sFormat.AppendFormat(", Mode: {}", pConnection->m_Resource.m_Buffer.m_Description.m_Mode.GetValue());
        }

        if (pConnection->m_Resource.m_Buffer.m_Description.m_BindFlags != xiiGALBindFlags::None && xiiReflectionUtils::BitflagsToString(pConnection->m_Resource.m_Buffer.m_Description.m_BindFlags, sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Bind: {}", sTmp);
        }
        else if (pConnection->m_Resource.m_Buffer.m_Description.m_BindFlags != xiiGALBindFlags::None)
        {
          sFormat.AppendFormat(", Bind: 0x{:X}", pConnection->m_Resource.m_Buffer.m_Description.m_BindFlags.GetValue());
        }

        if (pConnection->m_Resource.m_Buffer.m_Description.m_Usage != xiiGALResourceUsage::Mutable && xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALResourceUsage>(), pConnection->m_Resource.m_Buffer.m_Description.m_Usage.GetValue(), sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Usage: {}", sTmp);
        }
        else if (pConnection->m_Resource.m_Buffer.m_Description.m_Usage != xiiGALResourceUsage::Mutable)
        {
          sFormat.AppendFormat(", Usage: {}", pConnection->m_Resource.m_Buffer.m_Description.m_Usage.GetValue());
        }

        if (pConnection->m_Resource.m_Buffer.m_Description.m_CPUAccessFlags != xiiGALCPUAccessFlag::None && xiiReflectionUtils::BitflagsToString(pConnection->m_Resource.m_Buffer.m_Description.m_CPUAccessFlags, sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", CPU Access: {}", sTmp);
        }
        else if (pConnection->m_Resource.m_Buffer.m_Description.m_CPUAccessFlags != xiiGALCPUAccessFlag::None)
        {
          sFormat.AppendFormat(", CPU Access: 0x{:X}", pConnection->m_Resource.m_Buffer.m_Description.m_CPUAccessFlags.GetValue());
        }

        if (pConnection->m_Resource.m_Buffer.m_Description.m_MiscFlags != xiiGALMiscBufferFlags::None && xiiReflectionUtils::BitflagsToString(pConnection->m_Resource.m_Buffer.m_Description.m_MiscFlags, sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Misc: {}", sTmp);
        }
        else if (pConnection->m_Resource.m_Buffer.m_Description.m_MiscFlags != xiiGALMiscBufferFlags::None)
        {
          sFormat.AppendFormat(", Misc: 0x{:X}", pConnection->m_Resource.m_Buffer.m_Description.m_MiscFlags.GetValue());
        }

        sFormat.Append("]");

        sTemp.SetFormat("{}Buffer #{}: {}", data.m_pResourceProvider ? "External" : "Pooled", i, sFormat);
      }
      else if (pConnection->m_Resource.IsTexture())
      {
        sFormat.AppendFormat("[{}x{}", pConnection->m_Resource.m_Texture.m_Description.m_Size.width, pConnection->m_Resource.m_Texture.m_Description.GetHeight());

        const xiiUInt32 uiArrayOrDepthSize = pConnection->m_Resource.m_Texture.m_Description.GetArraySize();
        if (uiArrayOrDepthSize > 1)
        {
          sFormat.AppendFormat(":{}", uiArrayOrDepthSize);
        }

        if (xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALResourceFormat>(), pConnection->m_Resource.m_Texture.m_Description.m_Format.GetValue(), sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Format: {}", sTmp);
        }
        else
        {
          sFormat.AppendFormat(", Format: {}", pConnection->m_Resource.m_Texture.m_Description.m_Format.GetValue());
        }

        if (pConnection->m_Resource.m_Texture.m_Description.m_uiMipLevels > 1)
        {
          sFormat.AppendFormat(", Mips: {}", pConnection->m_Resource.m_Texture.m_Description.m_uiMipLevels);
        }

        if (pConnection->m_Resource.m_Texture.m_Description.m_uiSampleCount > 1)
        {
          sFormat.AppendFormat(", MSAAx{}", pConnection->m_Resource.m_Texture.m_Description.m_uiSampleCount);
        }

        if (xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALResourceDimension>(), pConnection->m_Resource.m_Texture.m_Description.m_Type.GetValue(), sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Type: {}", sTmp);
        }
        else
        {
          sFormat.AppendFormat(", Type: {}", pConnection->m_Resource.m_Texture.m_Description.m_Type.GetValue());
        }

        if (pConnection->m_Resource.m_Texture.m_Description.m_BindFlags != xiiGALBindFlags::None && xiiReflectionUtils::BitflagsToString(pConnection->m_Resource.m_Texture.m_Description.m_BindFlags, sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Bind: {}", sTmp);
        }
        else if (pConnection->m_Resource.m_Texture.m_Description.m_BindFlags != xiiGALBindFlags::None)
        {
          sFormat.AppendFormat(", Bind: 0x{:X}", pConnection->m_Resource.m_Texture.m_Description.m_BindFlags.GetValue());
        }

        if (pConnection->m_Resource.m_Texture.m_Description.m_Usage != xiiGALResourceUsage::Mutable && xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALResourceUsage>(), pConnection->m_Resource.m_Texture.m_Description.m_Usage.GetValue(), sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Usage: {}", sTmp);
        }
        else if (pConnection->m_Resource.m_Texture.m_Description.m_Usage != xiiGALResourceUsage::Mutable)
        {
          sFormat.AppendFormat(", Usage: {}", pConnection->m_Resource.m_Texture.m_Description.m_Usage.GetValue());
        }

        if (pConnection->m_Resource.m_Texture.m_Description.m_CPUAccessFlags != xiiGALCPUAccessFlag::None && xiiReflectionUtils::BitflagsToString(pConnection->m_Resource.m_Texture.m_Description.m_CPUAccessFlags, sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", CPU Access: {}", sTmp);
        }
        else if (pConnection->m_Resource.m_Texture.m_Description.m_CPUAccessFlags != xiiGALCPUAccessFlag::None)
        {
          sFormat.AppendFormat(", CPU Access: 0x{:X}", pConnection->m_Resource.m_Texture.m_Description.m_CPUAccessFlags.GetValue());
        }

        if (pConnection->m_Resource.m_Texture.m_Description.m_MiscFlags != xiiGALMiscTextureFlags::None && xiiReflectionUtils::BitflagsToString(pConnection->m_Resource.m_Texture.m_Description.m_MiscFlags, sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Misc: {}", sTmp);
        }
        else if (pConnection->m_Resource.m_Texture.m_Description.m_MiscFlags != xiiGALMiscTextureFlags::None)
        {
          sFormat.AppendFormat(", Misc: 0x{:X}", pConnection->m_Resource.m_Texture.m_Description.m_MiscFlags.GetValue());
        }

        sFormat.Append("]");

        sTemp.SetFormat("{}Texture #{}: {}", data.m_pResourceProvider ? "External" : "Pooled", i, sFormat);
      }
      else if (pConnection->m_Resource.IsSampler())
      {
        sFormat.Append("[");

        xiiStringBuilder sMin, sMag, sMip;
        if (xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALFilterType>(), pConnection->m_Resource.m_Sampler.m_Description.m_MinFilter.GetValue(), sMin, xiiReflectionUtils::EnumConversionMode::ValueNameOnly) && xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALFilterType>(), pConnection->m_Resource.m_Sampler.m_Description.m_MagFilter.GetValue(), sMag, xiiReflectionUtils::EnumConversionMode::ValueNameOnly) && xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALFilterType>(), pConnection->m_Resource.m_Sampler.m_Description.m_MipFilter.GetValue(), sMip, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat("Filter: {}/{}/{}", sMin, sMag, sMip);
        }

        xiiStringBuilder sU, sV, sW;
        if (xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALTextureAddressMode>(), pConnection->m_Resource.m_Sampler.m_Description.m_AddressU.GetValue(), sU, xiiReflectionUtils::EnumConversionMode::ValueNameOnly) && xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALTextureAddressMode>(), pConnection->m_Resource.m_Sampler.m_Description.m_AddressV.GetValue(), sV, xiiReflectionUtils::EnumConversionMode::ValueNameOnly) && xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALTextureAddressMode>(), pConnection->m_Resource.m_Sampler.m_Description.m_AddressW.GetValue(), sW, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Address: {}/{}/{}", sU, sV, sW);
        }

        if (pConnection->m_Resource.m_Sampler.m_Description.m_uiMaxAnisotropy > 0)
        {
          sFormat.AppendFormat(", Aniso: {}", pConnection->m_Resource.m_Sampler.m_Description.m_uiMaxAnisotropy);
        }

        if (pConnection->m_Resource.m_Sampler.m_Description.m_ComparisonFunction != xiiGALComparisonFunction::Never && xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALComparisonFunction>(), pConnection->m_Resource.m_Sampler.m_Description.m_ComparisonFunction.GetValue(), sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Compare: {}", sTmp);
        }
        else if (pConnection->m_Resource.m_Sampler.m_Description.m_ComparisonFunction != xiiGALComparisonFunction::Never)
        {
          sFormat.AppendFormat(", Compare: {}", pConnection->m_Resource.m_Sampler.m_Description.m_ComparisonFunction.GetValue());
        }

        if (pConnection->m_Resource.m_Sampler.m_Description.m_Flags != xiiGALSamplerFlags::None && xiiReflectionUtils::BitflagsToString(pConnection->m_Resource.m_Sampler.m_Description.m_Flags, sTmp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          sFormat.AppendFormat(", Flags: {}", sTmp);
        }
        else if (pConnection->m_Resource.m_Sampler.m_Description.m_Flags != xiiGALSamplerFlags::None)
        {
          sFormat.AppendFormat(", Flags: 0x{:X}", pConnection->m_Resource.m_Sampler.m_Description.m_Flags.GetValue());
        }

        if (pConnection->m_Resource.m_Sampler.m_Description.m_fMipLODBias != 0.0f)
        {
          sFormat.AppendFormat(", LOD Bias: {:.2f}", pConnection->m_Resource.m_Sampler.m_Description.m_fMipLODBias);
        }

        if (pConnection->m_Resource.m_Sampler.m_Description.m_fMinLOD != 0.0f)
        {
          sFormat.AppendFormat(", Min LOD: {:.2f}", pConnection->m_Resource.m_Sampler.m_Description.m_fMinLOD);
        }

        if (pConnection->m_Resource.m_Sampler.m_Description.m_fMaxLOD != xiiMath::MaxValue<float>())
        {
          sFormat.AppendFormat(", Max LOD: {:.2f}", pConnection->m_Resource.m_Sampler.m_Description.m_fMaxLOD);
        }

        if (pConnection->m_Resource.m_Sampler.m_Description.m_bUnormalizedCoords)
        {
          sFormat.Append(", UnnormalizedCoords");
        }

        sFormat.Append("]");

        sTemp.SetFormat("{}Sampler #{}: {}", data.m_pResourceProvider ? "External" : "Pooled", i, sFormat);
      }

      xiiUInt32 uiResourceNode = ref_graph.AddNode(sTemp, &nd);

      xiiUInt32 uiOutputNode = *nodeMap.GetValue(pConnection->m_pOutput->m_pParent);
      ref_graph.AddConnection(uiOutputNode, uiResourceNode, pConnection->m_pOutput->m_pParent->GetPinName(pConnection->m_pOutput));

      for (const xiiRenderPipelineNodePin* pInput : pConnection->m_Inputs)
      {
        xiiUInt32 uiInputNode = *nodeMap.GetValue(pInput->m_pParent);
        ref_graph.AddConnection(uiResourceNode, uiInputNode, pInput->m_pParent->GetPinName(pInput));
      }
    }
  }
}

xiiRasterizerView* xiiRenderPipeline::PrepareOcclusionCulling(const xiiFrustum& frustum, const xiiView& view)
{
#if XII_ENABLED(XII_PLATFORM_ARCH_X86)
  if (!cvar_SpatialCullingOcclusionEnable)
    return nullptr;

  auto& cpuFeatures = xiiSystemInformation::Get().GetCpuFeatures();
  if (!cpuFeatures.IsAvx1Available() || !cpuFeatures.HW_FMA3)
    return nullptr;

  xiiRasterizerView* pRasterizer = nullptr;

  // Extract all occlusion geometry from the scene.
  XII_PROFILE_SCOPE("PrepareOcclusionCulling");

  pRasterizer = g_pRasterizerViewPool->GetRasterizerView(static_cast<xiiUInt32>(view.GetViewport().width / 2), static_cast<xiiUInt32>(view.GetViewport().height / 2), (float)view.GetViewport().width / (float)view.GetViewport().height);
  pRasterizer->SetCamera(view.GetCullingCamera());

  {
    XII_PROFILE_SCOPE("FindOccluders");

    xiiSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = xiiDefaultSpatialDataCategories::OcclusionStatic.GetBitmask() | xiiDefaultSpatialDataCategories::OcclusionDynamic.GetBitmask();
    queryParams.m_pIncludeTags      = &view.m_IncludeTags;
    queryParams.m_pExcludeTags      = &view.m_ExcludeTags;

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, xiiVisibilityState::Indirect);
  }

  pRasterizer->BeginScene();

  {
    XII_PROFILE_SCOPE("ExtractOccluders");

    for (const xiiGameObject* pObj : m_VisibleObjects)
    {
      xiiMsgExtractOccluderData msg;
      pObj->SendMessage(msg);

      for (const auto& ed : msg.m_ExtractedOccluderData)
      {
        pRasterizer->AddObject(ed.m_pObject, ed.m_Transform);
      }
    }
  }

  pRasterizer->EndScene();

  return pRasterizer;
#else
  return nullptr;
#endif
}

void xiiRenderPipeline::PreviewOcclusionBuffer(const xiiRasterizerView& rasterizer, const xiiView& view)
{
  if (!cvar_SpatialCullingOcclusionVisView || !rasterizer.HasRasterizedAnyOccluders())
    return;

  XII_PROFILE_SCOPE("Occlusion::DebugPreview");

  const xiiUInt32 uiImageWidth  = rasterizer.GetResolutionX();
  const xiiUInt32 uiImageHeight = rasterizer.GetResolutionY();

  // get the debug image from the rasterizer
  xiiDynamicArray<xiiColorLinearUB> fb;
  fb.SetCountUninitialized(uiImageWidth * uiImageHeight);
  rasterizer.ReadBackFrame(fb);

  const float  w            = (float)uiImageWidth;
  const float  h            = (float)uiImageHeight;
  xiiRectFloat rectInPixel1 = xiiRectFloat(5.0f, 5.0f, w + 10, h + 10);
  xiiRectFloat rectInPixel2 = xiiRectFloat(10.0f, 10.0f, w, h);

  xiiDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel1, 0.0f, xiiColor::MediumPurple);

  xiiTexture2DResourceDescriptor d;
  d.m_DescGAL.m_Type        = xiiGALResourceDimension::Texture2D;
  d.m_DescGAL.m_Size.width  = rasterizer.GetResolutionX();
  d.m_DescGAL.m_Size.height = rasterizer.GetResolutionY();
  d.m_DescGAL.m_Format      = xiiGALResourceFormat::RGBA8SNormalized;

  xiiGALTextureSubResourceData content[1];
  content[0].m_pData         = fb.GetByteArrayPtr();
  content[0].m_uiStride      = sizeof(xiiColorLinearUB) * d.m_DescGAL.m_Size.width;
  content[0].m_uiDepthStride = content[0].m_uiStride * d.m_DescGAL.m_Size.height;
  d.m_InitialContent         = content;

  static xiiAtomicInteger32 name = 0;
  name.Increment();

  xiiStringBuilder sName;
  sName.SetFormat("RasterizerPreview-{}", name);

  xiiTexture2DResourceHandle hDebug = xiiResourceManager::CreateResource<xiiTexture2DResource>(sName, std::move(d));

  xiiDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel2, 0.0f, xiiColor::White, hDebug, xiiVec2(1, -1));
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipeline);
