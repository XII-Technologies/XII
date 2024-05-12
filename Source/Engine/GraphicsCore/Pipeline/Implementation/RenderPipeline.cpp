#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <GraphicsCore/Components/AlwaysVisibleComponent.h>
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
#include <GraphicsFoundation/Profiling/Profiling.h>
#include <GraphicsFoundation/Resources/Texture.h>

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
xiiCVarBool xiiRenderPipeline::cvar_SpatialCullingVis("Spatial.Culling.Vis", false, xiiCVarFlags::Default, "Enables debug visualization of visibility culling");
xiiCVarBool cvar_SpatialCullingShowStats("Spatial.Culling.ShowStats", false, xiiCVarFlags::Default, "Display some stats of the visibility culling");
#endif

xiiCVarBool  cvar_SpatialCullingOcclusionEnable("Spatial.Occlusion.Enable", true, xiiCVarFlags::Default, "Use software rasterization for occlusion culling.");
xiiCVarBool  cvar_SpatialCullingOcclusionVisView("Spatial.Occlusion.VisView", false, xiiCVarFlags::Default, "Render the occlusion framebuffer as an overlay.");
xiiCVarFloat cvar_SpatialCullingOcclusionBoundsInlation("Spatial.Occlusion.BoundsInflation", 0.5f, xiiCVarFlags::Default, "How much to inflate bounds during occlusion check.");
xiiCVarFloat cvar_SpatialCullingOcclusionFarPlane("Spatial.Occlusion.FarPlane", 50.0f, xiiCVarFlags::Default, "Far plane distance for finding occluders.");

xiiRenderPipeline::xiiRenderPipeline()
{
  m_CurrentExtractThread  = (xiiThreadID)0;
  m_CurrentRenderThread   = (xiiThreadID)0;
  m_uiLastExtractionFrame = -1;
  m_uiLastRenderFrame     = -1;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_AverageCullingTime = xiiTime::Seconds(0.1f);
#endif
}

xiiRenderPipeline::~xiiRenderPipeline()
{
  if (!m_hOcclusionDebugViewTexture.IsInvalidated())
  {
    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
    pDevice->DestroyTexture(m_hOcclusionDebugViewTexture);
    m_hOcclusionDebugViewTexture.Invalidate();
  }

  m_Data[0].Clear();
  m_Data[1].Clear();

  ClearRenderPassGraphTextures();
  while (!m_Passes.IsEmpty())
  {
    RemovePass(m_Passes.PeekBack().Borrow());
  }
}

void xiiRenderPipeline::AddPass(xiiUniquePtr<xiiRenderPipelinePass>&& pPass)
{
  m_PipelineState    = PipelineState::Uninitialized;
  pPass->m_pPipeline = this;
  pPass->InitializePins();

  auto it = m_Connections.Insert(pPass.Borrow(), ConnectionData());
  it.Value().m_Inputs.SetCount(pPass->GetInputPins().GetCount());
  it.Value().m_Outputs.SetCount(pPass->GetOutputPins().GetCount());
  m_Passes.PushBack(std::move(pPass));
}

void xiiRenderPipeline::RemovePass(xiiRenderPipelinePass* pPass)
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

void xiiRenderPipeline::GetPasses(xiiDynamicArray<const xiiRenderPipelinePass*>& ref_passes) const
{
  ref_passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

void xiiRenderPipeline::GetPasses(xiiDynamicArray<xiiRenderPipelinePass*>& ref_passes)
{
  ref_passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

xiiRenderPipelinePass* xiiRenderPipeline::GetPassByName(const xiiStringView& sPassName)
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

xiiHashedString xiiRenderPipeline::GetViewName() const
{
  return m_sName;
}

bool xiiRenderPipeline::Connect(xiiRenderPipelinePass* pOutputNode, xiiStringView sOutputPinName, xiiRenderPipelinePass* pInputNode, xiiStringView sInputPinName)
{
  xiiHashedString sOutputPinNameHash;
  sOutputPinNameHash.Assign(sOutputPinName);
  xiiHashedString sInputPinNameHash;
  sInputPinNameHash.Assign(sInputPinName);
  return Connect(pOutputNode, sOutputPinNameHash, pInputNode, sInputPinNameHash);
}

bool xiiRenderPipeline::Connect(xiiRenderPipelinePass* pOutputNode, xiiHashedString sOutputPinName, xiiRenderPipelinePass* pInputNode, xiiHashedString sInputPinName)
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

  // Add at output
  xiiRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  if (pConnection == nullptr)
  {
    pConnection                                          = XII_DEFAULT_NEW(xiiRenderPipelinePassConnection);
    pConnection->m_pOutput                               = pPinSource;
    itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex] = pConnection;
  }
  else
  {
    // Check that only one passthrough is connected
    if (pPinTarget->m_Type == xiiRenderPipelineNodePin::Type::PassThrough)
    {
      for (const xiiRenderPipelineNodePin* pPin : pConnection->m_Inputs)
      {
        if (pPin->m_Type == xiiRenderPipelineNodePin::Type::PassThrough)
        {
          xiiLog::Error("A pass through pin is already connected to the '{0}' pin!", sOutputPinName);
          return false;
        }
      }
    }
  }

  // Add at input
  pConnection->m_Inputs.PushBack(pPinTarget);
  itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] = pConnection;
  m_PipelineState                                   = PipelineState::Uninitialized;
  return true;
}

bool xiiRenderPipeline::Disconnect(xiiRenderPipelinePass* pOutputNode, xiiHashedString sOutputPinName, xiiRenderPipelinePass* pInputNode, xiiHashedString sInputPinName)
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

const xiiRenderPipelinePassConnection* xiiRenderPipeline::GetInputConnection(const xiiRenderPipelinePass* pPass, xiiHashedString sInputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto&                           data = it.Value();
  const xiiRenderPipelineNodePin* pPin = pPass->GetPinByName(sInputPinName);
  if (!pPin || pPin->m_uiInputIndex == 0xFF)
    return nullptr;

  return data.m_Inputs[pPin->m_uiInputIndex];
}

const xiiRenderPipelinePassConnection* xiiRenderPipeline::GetOutputConnection(const xiiRenderPipelinePass* pPass, xiiHashedString sOutputPinName) const
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

  ClearRenderPassGraphTextures();

  bool bRes = RebuildInternal(view);
  if (!bRes)
  {
    ClearRenderPassGraphTextures();
  }
  else
  {
    // make sure the renderdata stores the updated view data
    UpdateViewData(view, xiiRenderWorld::GetDataIndexForRendering());
  }

  m_PipelineState = bRes ? PipelineState::Initialized : PipelineState::RebuildError;
  return m_PipelineState;
}

bool xiiRenderPipeline::RebuildInternal(const xiiView& view)
{
  if (!SortPasses())
    return false;
  if (!InitRenderTargetDescriptions(view))
    return false;
  if (!CreateRenderTargetUsage(view))
    return false;
  if (!InitRenderPipelinePasses())
    return false;

  SortExtractors();

  return true;
}

bool xiiRenderPipeline::SortPasses()
{
  xiiLogBlock                                b("Sort Passes");
  xiiHybridArray<xiiRenderPipelinePass*, 32> done;
  done.Reserve(m_Passes.GetCount());

  xiiHybridArray<xiiRenderPipelinePass*, 8> usable;     // Stack of passes with all connections setup, they can be asked for descriptions.
  xiiHybridArray<xiiRenderPipelinePass*, 8> candidates; // Not usable yet, but all input connections are available

  // Find all source passes from which we can start the output description propagation.
  for (auto& pPass : m_Passes)
  {
    // if (std::all_of(cbegin(it.Value().m_Inputs), cend(it.Value().m_Inputs), [](xiiRenderPipelinePassConnection* pConn){return pConn == nullptr; }))
    if (AreInputDescriptionsAvailable(pPass.Borrow(), done))
    {
      usable.PushBack(pPass.Borrow());
    }
  }

  // Via a depth first traversal, order the passes
  while (!usable.IsEmpty())
  {
    xiiRenderPipelinePass* pPass = usable.PeekBack();
    xiiLogBlock            b2("Traverse", pPass->GetName());

    usable.PopBack();
    ConnectionData& data = m_Connections[pPass];

    XII_ASSERT_DEBUG(data.m_Inputs.GetCount() == pPass->GetInputPins().GetCount(), "Input pin count missmatch!");
    XII_ASSERT_DEBUG(data.m_Outputs.GetCount() == pPass->GetOutputPins().GetCount(), "Output pin count missmatch!");

    // Check for new candidate passes. Can't be done in the previous loop as multiple connections may be required by a node.
    for (xiiUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        // Go through all inputs this connection is connected to and test the corresponding node for availability
        for (const xiiRenderPipelineNodePin* pPin : data.m_Outputs[i]->m_Inputs)
        {
          XII_ASSERT_DEBUG(pPin->m_pParent != nullptr, "Pass was not initialized!");
          xiiRenderPipelinePass* pTargetPass = static_cast<xiiRenderPipelinePass*>(pPin->m_pParent);
          if (done.Contains(pTargetPass))
          {
            xiiLog::Error("Loop detected, graph not supported!");
            return false;
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
    for (xiiInt32 i = (xiiInt32)candidates.GetCount() - 1; i >= 0; i--)
    {
      xiiRenderPipelinePass* pCandidatePass = candidates[i];
      if (AreInputDescriptionsAvailable(pCandidatePass, done) && ArePassThroughInputsDone(pCandidatePass, done))
      {
        usable.PushBack(pCandidatePass);
        candidates.RemoveAtAndCopy(i);
      }
    }
  }

  if (done.GetCount() < m_Passes.GetCount())
  {
    xiiLog::Error("Pipeline: Not all nodes could be initialized");
    for (auto& pass : m_Passes)
    {
      if (!done.Contains(pass.Borrow()))
      {
        xiiLog::Error("Failed to initialize node: {} - {}", pass->GetName(), pass->GetDynamicRTTI()->GetTypeName());
      }
    }
    return false;
  }

  struct xiiPipelineSorter
  {
    /// \brief Returns true if a is less than b
    XII_FORCE_INLINE bool Less(const xiiUniquePtr<xiiRenderPipelinePass>& a, const xiiUniquePtr<xiiRenderPipelinePass>& b) const { return m_pDone->IndexOf(a.Borrow()) < m_pDone->IndexOf(b.Borrow()); }

    /// \brief Returns true if a is equal to b
    XII_ALWAYS_INLINE bool Equal(const xiiUniquePtr<xiiRenderPipelinePass>& a, const xiiUniquePtr<xiiRenderPipelinePass>& b) const { return a.Borrow() == b.Borrow(); }

    xiiHybridArray<xiiRenderPipelinePass*, 32>* m_pDone;
  };

  xiiPipelineSorter sorter;
  sorter.m_pDone = &done;
  m_Passes.Sort(sorter);
  return true;
}

bool xiiRenderPipeline::InitRenderTargetDescriptions(const xiiView& view)
{
  xiiLogBlock                                           b("Init Render Target Descriptions");
  xiiHybridArray<xiiGALTextureCreationDescription*, 10> inputs;
  xiiHybridArray<xiiGALTextureCreationDescription, 10>  outputs;

  for (auto& pPass : m_Passes)
  {
    xiiLogBlock b2("InitPass", pPass->GetName());

    if (view.GetCamera()->IsStereoscopic() && !pPass->IsStereoAware())
    {
      xiiLog::Error("View '{0}' uses a stereoscopic camera, but the render pass '{1}' does not support stereo rendering!", view.GetName(), pPass->GetName());
    }

    ConnectionData& data = m_Connections[pPass.Borrow()];

    XII_ASSERT_DEBUG(data.m_Inputs.GetCount() == pPass->GetInputPins().GetCount(), "Input pin count missmatch!");
    XII_ASSERT_DEBUG(data.m_Outputs.GetCount() == pPass->GetOutputPins().GetCount(), "Output pin count missmatch!");

    inputs.SetCount(data.m_Inputs.GetCount());
    outputs.Clear();
    outputs.SetCount(data.m_Outputs.GetCount());
    // Fill inputs array
    for (xiiUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
    {
      if (data.m_Inputs[i] != nullptr)
      {
        inputs[i] = &data.m_Inputs[i]->m_Desc;
      }
      else
      {
        inputs[i] = nullptr;
      }
    }

    bool bRes = pPass->GetRenderTargetDescriptions(view, inputs, outputs);
    if (!bRes)
    {
      xiiLog::Error("The pass could not be successfully queried for render target descriptions.");
      return false;
    }

    // Copy queried outputs into the output connections.
    for (xiiUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        data.m_Outputs[i]->m_Desc = outputs[i];
      }
    }

    // Check pass-through consistency of input / output target desc.
    auto inputPins = pPass->GetInputPins();
    for (const xiiRenderPipelineNodePin* pPin : inputPins)
    {
      if (pPin->m_Type == xiiRenderPipelineNodePin::Type::PassThrough)
      {
        if (data.m_Outputs[pPin->m_uiOutputIndex] != nullptr)
        {
          if (data.m_Inputs[pPin->m_uiInputIndex] == nullptr)
          {
            // xiiLog::Error("The pass of type '{0}' has a pass through pin '{1}' that has an output but no input!", pPass->GetDynamicRTTI()->GetTypeName(), pPass->GetPinName(pPin));
            // return false;
          }
          else if (data.m_Outputs[pPin->m_uiOutputIndex]->m_Desc.CalculateHash() != data.m_Inputs[pPin->m_uiInputIndex]->m_Desc.CalculateHash())
          {
            xiiLog::Error("The pass has a pass through pin '{0}' that has different descriptors for input and output!", pPass->GetPinName(pPin));
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool xiiRenderPipeline::CreateRenderTargetUsage(const xiiView& view)
{
  xiiLogBlock b("Create Render Target Usage Data");
  XII_ASSERT_DEBUG(m_TextureUsage.IsEmpty(), "Need to call ClearRenderPassGraphTextures before re-creating the pipeline.");

  m_ConnectionToTextureIndex.Clear();

  // Gather all connections that share the same path-through texture and their first and last usage pass index.
  for (xiiUInt16 i = 0; i < static_cast<xiiUInt16>(m_Passes.GetCount()); i++)
  {
    const auto&     pPass = m_Passes[i].Borrow();
    ConnectionData& data  = m_Connections[pPass];
    for (xiiRenderPipelinePassConnection* pConn : data.m_Inputs)
    {
      if (pConn != nullptr)
      {
        xiiUInt32 uiDataIdx                        = m_ConnectionToTextureIndex[pConn];
        m_TextureUsage[uiDataIdx].m_uiLastUsageIdx = i;
      }
    }

    for (xiiRenderPipelinePassConnection* pConn : data.m_Outputs)
    {
      if (pConn != nullptr)
      {
        if (pConn->m_pOutput->m_Type == xiiRenderPipelineNodePin::Type::PassThrough && data.m_Inputs[pConn->m_pOutput->m_uiInputIndex] != nullptr)
        {
          xiiRenderPipelinePassConnection* pCorrespondingInputConn = data.m_Inputs[pConn->m_pOutput->m_uiInputIndex];
          XII_ASSERT_DEV(m_ConnectionToTextureIndex.Contains(pCorrespondingInputConn), "");
          xiiUInt32 uiDataIdx = m_ConnectionToTextureIndex[pCorrespondingInputConn];
          m_TextureUsage[uiDataIdx].m_UsedBy.PushBack(pConn);
          m_TextureUsage[uiDataIdx].m_uiLastUsageIdx = i;

          XII_ASSERT_DEV(!m_ConnectionToTextureIndex.Contains(pConn), "");
          m_ConnectionToTextureIndex[pConn] = uiDataIdx;
        }
        else
        {
          m_ConnectionToTextureIndex[pConn] = m_TextureUsage.GetCount();
          TextureUsageData& texData         = m_TextureUsage.ExpandAndGetRef();

          texData.m_iTargetTextureIndex = -1;
          texData.m_uiFirstUsageIdx     = i;
          texData.m_uiLastUsageIdx      = i;
          texData.m_UsedBy.PushBack(pConn);
        }
      }
    }
  }

  static xiiUInt32 defaultTextureDescHash = xiiGALTextureCreationDescription{}.CalculateHash();
  // Set view's render target textures to target pass connections.
  for (xiiUInt32 i = 0; i < m_Passes.GetCount(); i++)
  {
    const auto& pPass = m_Passes[i].Borrow();
    if (pPass->IsInstanceOf<xiiTargetPass>())
    {
      const xiiGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

      xiiTargetPass*  pTargetPass = static_cast<xiiTargetPass*>(pPass);
      ConnectionData& data        = m_Connections[pPass];
      for (xiiUInt32 j = 0; j < data.m_Inputs.GetCount(); j++)
      {
        xiiRenderPipelinePassConnection* pConn = data.m_Inputs[j];
        if (pConn != nullptr)
        {
          const xiiGALTextureHandle* hTexture = pTargetPass->GetTextureHandle(renderTargets, pPass->GetInputPins()[j]);
          XII_ASSERT_DEV(m_ConnectionToTextureIndex.Contains(pConn), "");

          xiiUInt32 uiDataIdx = m_ConnectionToTextureIndex[pConn];
          if (!hTexture)
          {
            m_TextureUsage[uiDataIdx].m_iTargetTextureIndex = -1;
            for (auto pUsedByConn : m_TextureUsage[uiDataIdx].m_UsedBy)
            {
              pUsedByConn->m_TextureHandle.Invalidate();
            }
          }
          else if (!hTexture->IsInvalidated() || pConn->m_Desc.CalculateHash() == defaultTextureDescHash)
          {
            m_TextureUsage[uiDataIdx].m_iTargetTextureIndex = static_cast<xiiInt32>(hTexture - reinterpret_cast<const xiiGALTextureHandle*>(&renderTargets));
            XII_ASSERT_DEV(reinterpret_cast<const xiiGALTextureHandle*>(&renderTargets)[m_TextureUsage[uiDataIdx].m_iTargetTextureIndex] == *hTexture, "Offset computation broken.");

            for (auto pUsedByConn : m_TextureUsage[uiDataIdx].m_UsedBy)
            {
              pUsedByConn->m_TextureHandle = *hTexture;
            }
          }
          else
          {
            // In this case, the xiiTargetPass does not provide a render target for the connection but the descriptor is set so we can instead use the pool to supplement the missing texture.
          }
        }
      }
    }
  }

  // Inconvenient loop to gather all TextureUsageData indices that are not view render target textures.
  for (xiiUInt32 i = 0; i < m_TextureUsage.GetCount(); i++)
  {
    TextureUsageData& data = m_TextureUsage[i];
    if (data.m_iTargetTextureIndex != -1)
      continue;

    m_TextureUsageIdxSortedByFirstUsage.PushBack((xiiUInt16)i);
    m_TextureUsageIdxSortedByLastUsage.PushBack((xiiUInt16)i);
  }

  // Sort first and last usage arrays, these will determine the lifetime of the pool textures.
  struct FirstUsageComparer
  {
    FirstUsageComparer(xiiDynamicArray<TextureUsageData>& ref_textureUsage) :
      m_TextureUsage(ref_textureUsage)
    {
    }

    XII_ALWAYS_INLINE bool Less(xiiUInt16 a, xiiUInt16 b) const { return m_TextureUsage[a].m_uiFirstUsageIdx < m_TextureUsage[b].m_uiFirstUsageIdx; }

    xiiDynamicArray<TextureUsageData>& m_TextureUsage;
  };

  struct LastUsageComparer
  {
    LastUsageComparer(xiiDynamicArray<TextureUsageData>& ref_textureUsage) :
      m_TextureUsage(ref_textureUsage)
    {
    }

    XII_ALWAYS_INLINE bool Less(xiiUInt16 a, xiiUInt16 b) const { return m_TextureUsage[a].m_uiLastUsageIdx < m_TextureUsage[b].m_uiLastUsageIdx; }

    xiiDynamicArray<TextureUsageData>& m_TextureUsage;
  };

  m_TextureUsageIdxSortedByFirstUsage.Sort(FirstUsageComparer(m_TextureUsage));
  m_TextureUsageIdxSortedByLastUsage.Sort(LastUsageComparer(m_TextureUsage));

  return true;
}

bool xiiRenderPipeline::InitRenderPipelinePasses()
{
  xiiLogBlock b("Init Render Pipeline Passes");
  // Init every pass now.
  for (auto& pPass : m_Passes)
  {
    ConnectionData& data = m_Connections[pPass.Borrow()];
    pPass->InitRenderPipelinePass(data.m_Inputs, data.m_Outputs);
  }

  return true;
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

    bool allDependenciesFound = true;
    for (auto& sDependency : extractor->m_DependsOn)
    {
      if (!Helper::FindDependency(sDependency, m_SortedExtractors))
      {
        allDependenciesFound = false;
        break;
      }
    }

    if (allDependenciesFound)
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

  XII_ASSERT_DEV(uiDataIndex <= 1, "Data index must be 0 or 1");
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

void xiiRenderPipeline::RemoveConnections(xiiRenderPipelinePass* pPass)
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return;

  ConnectionData& data = it.Value();
  for (xiiUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
  {
    xiiRenderPipelinePassConnection* pConn = data.m_Inputs[i];
    if (pConn != nullptr)
    {
      xiiRenderPipelinePass* pSource = static_cast<xiiRenderPipelinePass*>(pConn->m_pOutput->m_pParent);
      bool                   bRes    = Disconnect(pSource, pSource->GetPinName(pConn->m_pOutput), pPass, pPass->GetPinName(pPass->GetInputPins()[i]));
      XII_IGNORE_UNUSED(bRes);
      XII_ASSERT_DEBUG(bRes, "xiiRenderPipeline::RemoveConnections should not fail to disconnect pins!");
    }
  }
  for (xiiUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
  {
    xiiRenderPipelinePassConnection* pConn = data.m_Outputs[i];
    while (pConn != nullptr)
    {
      xiiRenderPipelinePass* pTarget = static_cast<xiiRenderPipelinePass*>(pConn->m_Inputs[0]->m_pParent);
      bool                   bRes    = Disconnect(pPass, pPass->GetPinName(pConn->m_pOutput), pTarget, pTarget->GetPinName(pConn->m_Inputs[0]));
      XII_IGNORE_UNUSED(bRes);
      XII_ASSERT_DEBUG(bRes, "xiiRenderPipeline::RemoveConnections should not fail to disconnect pins!");

      pConn = data.m_Outputs[i];
    }
  }
}

void xiiRenderPipeline::ClearRenderPassGraphTextures()
{
  m_TextureUsage.Clear();
  m_TextureUsageIdxSortedByFirstUsage.Clear();
  m_TextureUsageIdxSortedByLastUsage.Clear();

  // xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  for (auto it = m_Connections.GetIterator(); it.IsValid(); ++it)
  {
    auto& conn = it.Value();
    for (auto pConn : conn.m_Outputs)
    {
      if (pConn)
      {
        pConn->m_Desc = xiiGALTextureCreationDescription{.m_Type = xiiGALResourceDimension::Texture2D};
        if (!pConn->m_TextureHandle.IsInvalidated())
        {
          pConn->m_TextureHandle.Invalidate();
        }
      }
    }
  }
}

bool xiiRenderPipeline::AreInputDescriptionsAvailable(const xiiRenderPipelinePass* pPass, const xiiHybridArray<xiiRenderPipelinePass*, 32>& done) const
{
  auto                  it   = m_Connections.Find(pPass);
  const ConnectionData& data = it.Value();
  for (xiiUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
  {
    const xiiRenderPipelinePassConnection* pConn = data.m_Inputs[i];
    if (pConn != nullptr)
    {
      // If the connections source is not done yet, the connections output is undefined yet and the inputs can't be processed yet.
      if (!done.Contains(static_cast<xiiRenderPipelinePass*>(pConn->m_pOutput->m_pParent)))
      {
        return false;
      }
    }
  }

  return true;
}

bool xiiRenderPipeline::ArePassThroughInputsDone(const xiiRenderPipelinePass* pPass, const xiiHybridArray<xiiRenderPipelinePass*, 32>& done) const
{
  auto                  it     = m_Connections.Find(pPass);
  const ConnectionData& data   = it.Value();
  auto                  inputs = pPass->GetInputPins();
  for (xiiUInt32 i = 0; i < inputs.GetCount(); i++)
  {
    const xiiRenderPipelineNodePin* pPin = inputs[i];
    if (pPin->m_Type == xiiRenderPipelineNodePin::Type::PassThrough)
    {
      const xiiRenderPipelinePassConnection* pConn = data.m_Inputs[pPin->m_uiInputIndex];
      if (pConn != nullptr)
      {
        for (const xiiRenderPipelineNodePin* pInputPin : pConn->m_Inputs)
        {
          // Any input that is also connected to the source of pPin must be done before we can use the pass through input
          if (pInputPin != pPin && !done.Contains(static_cast<xiiRenderPipelinePass*>(pInputPin->m_pParent)))
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
      XII_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->Extract(view, m_VisibleObjects, data);
    }
  }

  data.SortAndBatch();

  for (auto& pExtractor : m_Extractors)
  {
    if (pExtractor->m_bActive)
    {
      XII_PROFILE_SCOPE(pExtractor->m_sName.GetData());

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
  queryParams.m_IncludeTags       = view.m_IncludeTags;
  queryParams.m_ExcludeTags       = view.m_ExcludeTags;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  queryParams.m_pStats = bRecordStats ? &stats : nullptr;
#endif

  xiiFrustum     limitedFrustum                               = frustum;
  const xiiPlane farPlane                                     = limitedFrustum.GetPlane(xiiFrustum::PlaneType::FarPlane);
  limitedFrustum.AccessPlane(xiiFrustum::PlaneType::FarPlane) = xiiPlane(farPlane.m_vNormal, view.GetCullingCamera()->GetCenterPosition() + farPlane.m_vNormal * cvar_SpatialCullingOcclusionFarPlane.GetValue()); // only use occluders closer than this

  xiiRasterizerView* pRasterizer = PrepareOcclusionCulling(limitedFrustum, view);
  XII_SCOPE_EXIT(g_pRasterizerViewPool->ReturnRasterizerView(pRasterizer));

  const xiiVisibilityState visType = bIsMainView ? xiiVisibilityState::Direct : xiiVisibilityState::Indirect;

  if (pRasterizer != nullptr && pRasterizer->HasRasterizedAnyOccluders())
  {
    XII_PROFILE_SCOPE("Occlusion::FindVisibleObjects");

    auto IsOccluded = [=](const xiiSimdBBox& aabb) {
      // grow the bbox by some percent to counter the lower precision of the occlusion buffer

      const xiiSimdVec4f c = aabb.GetCenter();
      const xiiSimdVec4f e = aabb.GetHalfExtents();

      xiiSimdBBox aabb2;
      aabb2.SetCenterAndHalfExtents(c, e.CompMul(xiiSimdVec4f(1.0f + cvar_SpatialCullingOcclusionBoundsInlation)));

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
  // XII_PROFILE_AND_MARKER(pRenderContext->GetCommandList(), m_sName.GetData());
  XII_PROFILE_SCOPE(m_sName.GetData());

  XII_ASSERT_DEV(m_PipelineState != PipelineState::Uninitialized, "Pipeline must be rebuild before rendering.");
  if (m_PipelineState == PipelineState::RebuildError)
  {
    return;
  }

  XII_ASSERT_DEV(m_CurrentRenderThread == (xiiThreadID)0, "Render must not be called from multiple threads.");
  m_CurrentRenderThread = xiiThreadUtils::GetCurrentThreadID();

  XII_ASSERT_DEV(m_uiLastRenderFrame != xiiRenderWorld::GetFrameCounter(), "Render must not be called multiple times per frame.");
  m_uiLastRenderFrame = xiiRenderWorld::GetFrameCounter();


  auto&              data       = m_Data[xiiRenderWorld::GetDataIndexForRendering()];
  const xiiCamera*   pCamera    = &data.GetCamera();
  const xiiCamera*   pLodCamera = &data.GetLodCamera();
  const xiiViewData* pViewData  = &data.GetViewData();

  auto& gc = pRenderContext->WriteGlobalConstants();
  for (xiiInt32 i = 0; i < 2; ++i)
  {
    gc.CameraToScreenMatrix[i] = pViewData->m_ProjectionMatrix[i];
    gc.ScreenToCameraMatrix[i] = pViewData->m_InverseProjectionMatrix[i];
    gc.WorldToCameraMatrix[i]  = pViewData->m_ViewMatrix[i];
    gc.CameraToWorldMatrix[i]  = pViewData->m_InverseViewMatrix[i];
    gc.WorldToScreenMatrix[i]  = pViewData->m_ViewProjectionMatrix[i];
    gc.ScreenToWorldMatrix[i]  = pViewData->m_InverseViewProjectionMatrix[i];
  }

  const xiiRectFloat& viewport = pViewData->m_ViewPortRect;
  gc.ViewportSize              = xiiVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);

  float fNear   = pCamera->GetNearPlane();
  float fFar    = pCamera->GetFarPlane();
  gc.ClipPlanes = xiiVec4(fNear, fFar, 1.0f / fFar, 0.0f);

  const bool bIsDirectionalLightShadow = pViewData->m_CameraUsageHint == xiiCameraUsageHint::Shadow && pCamera->IsOrthographic();
  gc.MaxZValue                         = bIsDirectionalLightShadow ? 0.0f : xiiMath::MinValue<float>();

  // Wrap around to prevent floating point issues. Wrap around is dividable by all whole numbers up to 11.
  gc.DeltaTime  = (float)xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds();
  gc.GlobalTime = (float)xiiMath::Mod(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 20790.0);
  gc.WorldTime  = (float)xiiMath::Mod(data.GetWorldTime().GetSeconds(), 20790.0);

  gc.Exposure   = pCamera->GetExposure();
  gc.RenderPass = xiiViewRenderMode::GetRenderPassForShader(pViewData->m_ViewRenderMode);

  xiiRenderViewContext renderViewContext;
  renderViewContext.m_pCamera            = pCamera;
  renderViewContext.m_pLodCamera         = pLodCamera;
  renderViewContext.m_pViewData          = pViewData;
  renderViewContext.m_pRenderContext     = pRenderContext;
  renderViewContext.m_pWorldDebugContext = &data.GetWorldDebugContext();
  renderViewContext.m_pViewDebugContext  = &data.GetViewDebugContext();

  // Set camera mode permutation variable here since it doesn't change throughout the frame
  static xiiHashedString sCameraMode  = xiiMakeHashedString("CAMERA_MODE");
  static xiiHashedString sOrtho       = xiiMakeHashedString("CAMERA_MODE_ORTHO");
  static xiiHashedString sPerspective = xiiMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static xiiHashedString sStereo      = xiiMakeHashedString("CAMERA_MODE_STEREO");

  static xiiHashedString sVSRTAI           = xiiMakeHashedString("VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX");
  static xiiHashedString sClipSpaceFlipped = xiiMakeHashedString("CLIP_SPACE_FLIPPED");
  static xiiHashedString sTrue             = xiiMakeHashedString("TRUE");
  static xiiHashedString sFalse            = xiiMakeHashedString("FALSE");

  if (pCamera->IsOrthographic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sOrtho);
  else if (pCamera->IsStereoscopic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sStereo);
  else
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sPerspective);

  /// \todo Check vertex shader render target array index.
  pRenderContext->SetShaderPermutationVariable(sVSRTAI, sTrue);

  pRenderContext->SetShaderPermutationVariable(sClipSpaceFlipped, xiiClipSpaceYMode::RenderToTextureDefault == xiiClipSpaceYMode::Flipped ? sTrue : sFalse);

  // Also set pipeline specific permutation vars
  for (auto& var : m_PermutationVars)
  {
    pRenderContext->SetShaderPermutationVariable(var.m_sName, var.m_sValue);
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

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  pDevice->BeginPipeline(m_sName, renderViewContext.m_pViewData->m_hSwapChain);

  if (const xiiGALSwapChain* pSwapChain = pDevice->GetSwapChain(renderViewContext.m_pViewData->m_hSwapChain))
  {
    xiiGALRenderTargets renderTargets;
    renderTargets.m_hRTs[0] = pSwapChain->GetBackBufferTexture();
    // Update target textures after the swap chain acquired new textures.
    for (xiiUInt32 i = 0; i < m_TextureUsage.GetCount(); i++)
    {
      TextureUsageData& textureUsageData = m_TextureUsage[i];
      if (textureUsageData.m_iTargetTextureIndex != -1)
      {
        xiiGALTextureHandle hTexture = reinterpret_cast<const xiiGALTextureHandle*>(&renderTargets)[textureUsageData.m_iTargetTextureIndex];
        for (auto pUsedByConn : textureUsageData.m_UsedBy)
        {
          pUsedByConn->m_TextureHandle = hTexture;
        }
      }
    }
  }

  xiiUInt32 uiCurrentFirstUsageIdx = 0;
  xiiUInt32 uiCurrentLastUsageIdx  = 0;
  for (xiiUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    auto& pPass = m_Passes[i];
    XII_PROFILE_SCOPE(pPass->GetName());
    xiiLogBlock passBlock("Render Pass", pPass->GetName());

    // Create pool textures
    for (; uiCurrentFirstUsageIdx < m_TextureUsageIdxSortedByFirstUsage.GetCount();)
    {
      xiiUInt16         uiCurrentUsageData = m_TextureUsageIdxSortedByFirstUsage[uiCurrentFirstUsageIdx];
      TextureUsageData& usageData          = m_TextureUsage[uiCurrentUsageData];
      if (usageData.m_uiFirstUsageIdx == i)
      {
        xiiGALTextureHandle hTexture = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(usageData.m_UsedBy[0]->m_Desc);
        XII_ASSERT_DEV(!hTexture.IsInvalidated(), "GPU pool returned an invalidated texture!");
        for (xiiRenderPipelinePassConnection* pConn : usageData.m_UsedBy)
        {
          pConn->m_TextureHandle = hTexture;
        }
        ++uiCurrentFirstUsageIdx;
      }
      else
      {
        // The current usage data blocks m_uiFirstUsageIdx isn't reached yet so wait.
        break;
      }
    }

    // Execute pass block
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

    // Release pool textures
    for (; uiCurrentLastUsageIdx < m_TextureUsageIdxSortedByLastUsage.GetCount();)
    {
      xiiUInt16         uiCurrentUsageData = m_TextureUsageIdxSortedByLastUsage[uiCurrentLastUsageIdx];
      TextureUsageData& usageData          = m_TextureUsage[uiCurrentUsageData];
      if (usageData.m_uiLastUsageIdx == i)
      {
        xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(usageData.m_UsedBy[0]->m_TextureHandle);
        for (xiiRenderPipelinePassConnection* pConn : usageData.m_UsedBy)
        {
          pConn->m_TextureHandle.Invalidate();
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
  XII_ASSERT_DEV(uiCurrentFirstUsageIdx == m_TextureUsageIdxSortedByFirstUsage.GetCount(), "Rendering all passes should have moved us through all texture usage blocks!");
  XII_ASSERT_DEV(uiCurrentLastUsageIdx == m_TextureUsageIdxSortedByLastUsage.GetCount(), "Rendering all passes should have moved us through all texture usage blocks!");

  pDevice->EndPipeline(renderViewContext.m_pViewData->m_hSwapChain);

  renderEvent.m_Type = xiiRenderWorldRenderEvent::Type::AfterPipelineExecution;
  {
    XII_PROFILE_SCOPE("AfterPipelineExecution");
    xiiRenderWorld::s_RenderEvent.Broadcast(renderEvent);
  }

  pRenderContext->ResetContextState();

  data.Clear();

  m_CurrentRenderThread = (xiiThreadID)0;
}

const xiiExtractedRenderData& xiiRenderPipeline::GetRenderData() const
{
  return m_Data[xiiRenderWorld::GetDataIndexForRendering()];
}

xiiRenderDataBatchList xiiRenderPipeline::GetRenderDataBatchesWithCategory(xiiRenderData::Category category, xiiRenderDataBatch::Filter filter) const
{
  auto& data = m_Data[xiiRenderWorld::GetDataIndexForRendering()];
  return data.GetRenderDataBatchesWithCategory(category, filter);
}

void xiiRenderPipeline::CreateDgmlGraph(xiiDGMLGraph& ref_graph)
{
  xiiStringBuilder                                      sTmp;
  xiiHashTable<const xiiRenderPipelineNode*, xiiUInt32> nodeMap;
  nodeMap.Reserve(m_Passes.GetCount() + m_TextureUsage.GetCount() * 3);
  for (xiiUInt32 p = 0; p < m_Passes.GetCount(); ++p)
  {
    const auto& pPass = m_Passes[p];
    sTmp.SetFormat("#{}: {}", p, pPass->GetName().IsEmpty() ? pPass->GetDynamicRTTI()->GetTypeName() : pPass->GetName());

    xiiDGMLGraph::NodeDesc nd;
    nd.m_Color            = xiiColor::Gray;
    nd.m_Shape            = xiiDGMLGraph::NodeShape::Rectangle;
    xiiUInt32 uiGraphNode = ref_graph.AddNode(sTmp, &nd);
    nodeMap.Insert(pPass.Borrow(), uiGraphNode);
  }

  for (xiiUInt32 i = 0; i < m_TextureUsage.GetCount(); ++i)
  {
    const TextureUsageData& data = m_TextureUsage[i];

    for (const xiiRenderPipelinePassConnection* pCon : data.m_UsedBy)
    {
      xiiDGMLGraph::NodeDesc nd;
      nd.m_Color = data.m_iTargetTextureIndex != -1 ? xiiColor::Black : xiiColorScheme::GetColor(static_cast<xiiColorScheme::Enum>(i % xiiColorScheme::Count), 4);
      nd.m_Shape = xiiDGMLGraph::NodeShape::RoundedRectangle;

      xiiStringBuilder sFormat;
      if (!xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALTextureFormat>(), pCon->m_Desc.m_Format, sFormat, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
      {
        sFormat.SetFormat("Unknown Format {}", (int)pCon->m_Desc.m_Format);
      }
      sTmp.SetFormat("{} #{}: {}x{}:{}, MSAA:{}, {}Format: {}", data.m_iTargetTextureIndex != -1 ? "RenderTarget" : "PoolTexture", i, pCon->m_Desc.m_Size.width, pCon->m_Desc.m_Size.height, pCon->m_Desc.m_uiArraySizeOrDepth, pCon->m_Desc.m_uiSampleCount, xiiGALTextureFormat::IsDepthFormat(pCon->m_Desc.m_Format) ? "Depth" : "Color", sFormat);
      xiiUInt32 uiTextureNode = ref_graph.AddNode(sTmp, &nd);

      xiiUInt32 uiOutputNode = *nodeMap.GetValue(pCon->m_pOutput->m_pParent);
      ref_graph.AddConnection(uiOutputNode, uiTextureNode, pCon->m_pOutput->m_pParent->GetPinName(pCon->m_pOutput));
      for (const xiiRenderPipelineNodePin* pInput : pCon->m_Inputs)
      {
        xiiUInt32 uiInputNode = *nodeMap.GetValue(pInput->m_pParent);
        ref_graph.AddConnection(uiTextureNode, uiInputNode, pInput->m_pParent->GetPinName(pInput));
      }
    }
  }
}

xiiRasterizerView* xiiRenderPipeline::PrepareOcclusionCulling(const xiiFrustum& frustum, const xiiView& view)
{
#if XII_ENABLED(XII_PLATFORM_ARCH_X86)
  if (!cvar_SpatialCullingOcclusionEnable)
    return nullptr;

  if (!xiiSystemInformation::Get().GetCpuFeatures().IsAvx1Available())
    return nullptr;

  xiiRasterizerView* pRasterizer = nullptr;

  // extract all occlusion geometry from the scene
  XII_PROFILE_SCOPE("Occlusion::RasterizeView");

  pRasterizer = g_pRasterizerViewPool->GetRasterizerView(static_cast<xiiUInt32>(view.GetViewport().width / 2), static_cast<xiiUInt32>(view.GetViewport().height / 2), (float)view.GetViewport().width / (float)view.GetViewport().height);
  pRasterizer->SetCamera(view.GetCullingCamera());

  {
    XII_PROFILE_SCOPE("Occlusion::FindOccluders");

    xiiSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = xiiDefaultSpatialDataCategories::OcclusionStatic.GetBitmask() | xiiDefaultSpatialDataCategories::OcclusionDynamic.GetBitmask();
    queryParams.m_IncludeTags       = view.m_IncludeTags;
    queryParams.m_ExcludeTags       = view.m_ExcludeTags;

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, xiiVisibilityState::Indirect);
  }

  pRasterizer->BeginScene();

  for (const xiiGameObject* pObj : m_VisibleObjects)
  {
    xiiMsgExtractOccluderData msg;
    pObj->SendMessage(msg);

    for (const auto& ed : msg.m_ExtractedOccluderData)
    {
      pRasterizer->AddObject(ed.m_pObject, ed.m_Transform);
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

  const xiiUInt32 uiImgWidth  = rasterizer.GetResolutionX();
  const xiiUInt32 uiImgHeight = rasterizer.GetResolutionY();

  // get the debug image from the rasterizer
  xiiDynamicArray<xiiColorLinearUB> fb;
  fb.SetCountUninitialized(uiImgWidth * uiImgHeight);
  rasterizer.ReadBackFrame(fb);

  const float  w            = (float)uiImgWidth;
  const float  h            = (float)uiImgHeight;
  xiiRectFloat rectInPixel1 = xiiRectFloat(5.0f, 5.0f, w + 10, h + 10);
  xiiRectFloat rectInPixel2 = xiiRectFloat(10.0f, 10.0f, w, h);

  xiiDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel1, 0.0f, xiiColor::MediumPurple);

  // TODO: it would be better to update a single texture every frame, however since this is a render pass, we currently can't create nested passes
  // so either this has to be done elsewhere, or nested passes have to be allowed
  if (false)
  {
    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

    // check whether we need to re-create the texture
    if (!m_hOcclusionDebugViewTexture.IsInvalidated())
    {
      const xiiGALTexture* pTexture = pDevice->GetTexture(m_hOcclusionDebugViewTexture);

      if (pTexture->GetDescription().m_Size.width != uiImgWidth || pTexture->GetDescription().m_Size.height != uiImgHeight)
      {
        pDevice->DestroyTexture(m_hOcclusionDebugViewTexture);
        m_hOcclusionDebugViewTexture.Invalidate();
      }
    }

    // create the texture
    if (m_hOcclusionDebugViewTexture.IsInvalidated())
    {
      xiiGALTextureCreationDescription desc;
      desc.m_Type           = xiiGALResourceDimension::Texture2D;
      desc.m_Size.width     = uiImgWidth;
      desc.m_Size.height    = uiImgHeight;
      desc.m_Format         = xiiGALTextureFormat::RGBA8UNormalized;
      desc.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      desc.m_BindFlags      = xiiGALBindFlags::ShaderResource;
      desc.m_Usage          = xiiGALResourceUsage::Default;

      m_hOcclusionDebugViewTexture = pDevice->CreateTexture(desc);
    }

    // upload the image to the texture
    {
      xiiGALCommandQueue* pGALCommandQueue = pDevice->GetDefaultCommandQueue();
      auto                pCommandList     = pGALCommandQueue->BeginCommandList("RasterizerDebugViewUpdate");

      xiiBoundingBoxU32 destBox;
      destBox.m_vMin.SetZero();
      destBox.m_vMax = xiiVec3U32(uiImgWidth, uiImgHeight, 1);

      xiiGALTextureSubResourceData sourceData;
      sourceData.m_pData    = fb.GetData();
      sourceData.m_uiStride = uiImgWidth * sizeof(xiiColorLinearUB);

      pCommandList->UpdateTextureExtended(m_hOcclusionDebugViewTexture, xiiGALTextureMipLevelData(), destBox, sourceData);

      pGALCommandQueue->Submit(pCommandList);
    }

    xiiDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel2, 0.0f, xiiColor::White, pDevice->GetTexture(m_hOcclusionDebugViewTexture)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiVec2(1, -1));
  }
  else
  {
    xiiTexture2DResourceDescriptor d;
    d.m_DescGAL.m_Size.width  = rasterizer.GetResolutionX();
    d.m_DescGAL.m_Size.height = rasterizer.GetResolutionY();
    d.m_DescGAL.m_Format      = xiiGALTextureFormat::RGBA8SNormalized;

    xiiGALTextureSubResourceData content[1];
    content[0].m_pData         = fb.GetData();
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
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipeline);
