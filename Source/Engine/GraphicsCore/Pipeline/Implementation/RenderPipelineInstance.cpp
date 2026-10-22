/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>

xiiRenderPipelineInstance::xiiRenderPipelineInstance()
{
  static xiiAtomicInteger64 scope;
  m_uiInstanceScope = static_cast<xiiUInt64>(scope.Increment());
}
xiiRenderPipelineInstance::~xiiRenderPipelineInstance() { Clear(); }
void xiiRenderPipelineInstance::Clear()
{
  m_sBuildError.Clear();
  m_uiPreparedSetup = m_uiBuiltSetup = 0;
  m_pPreparedView                    = nullptr;
  m_pPreparedBlackboard              = nullptr;
  for (auto* pass : m_Passes)
    pass->GetDynamicRTTI()->GetAllocator()->Deallocate(pass);
  m_Passes.Clear();
  m_Bindings.Clear();
  m_pPipeline = nullptr;
  m_Parameters.Clear();
  m_Overrides.Clear();
  m_PassParameters.Clear();
  m_Blackboards.Clear();
  m_PassBlackboards.Clear();
  m_HistoryBindings.Clear();
  m_ExecutionScopes.Clear();
  m_sScopeParent.Clear();
}
xiiResult xiiRenderPipelineInstance::Initialize(xiiSharedPtr<const xiiRenderPipeline> pipeline)
{
  if (pipeline == m_pPipeline && pipeline)
    return XII_SUCCESS;
  Clear();
  if (!pipeline)
    return XII_FAILURE;
  m_pPipeline = std::move(pipeline);
  for (xiiUInt32 i = 0; i < m_pPipeline->m_Passes.GetCount(); ++i)
  {
    xiiRenderPipelinePass* pass = m_pPipeline->m_Passes[i].m_pType->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    m_Passes.PushBack(pass);
    auto properties = m_pPipeline->m_Descriptor.m_Passes[i].m_Properties;
    properties.Remove("PinConfigurations"); // Authored pin objects are lowered into immutable compiled contracts.
    if (pass->Configure(properties).Failed() || pass->InitializePinBindings(m_pPipeline->m_Passes[i].m_Pins).Failed())
    {
      Clear();
      return XII_FAILURE;
    }
  }
  m_Bindings.SetCount(m_pPipeline->m_uiBindingCount);
  m_HistoryBindings.SetCount(m_pPipeline->m_uiBindingCount);
  m_Parameters     = m_pPipeline->m_Descriptor.m_Parameters;
  m_PassParameters = m_pPipeline->m_PassParameters;
  for (const auto& scope : m_pPipeline->m_ResourceScopes)
  {
    xiiRenderGraphBlackboard* blackboard = nullptr;
    if (!scope.IsEmpty())
    {
      xiiUniquePtr<xiiRenderGraphBlackboard>* owned = nullptr;
      if (!m_Blackboards.TryGetValue(scope, owned))
      {
        m_Blackboards.Insert(scope, XII_DEFAULT_NEW(xiiRenderGraphBlackboard));
        m_Blackboards.TryGetValue(scope, owned);
      }
      blackboard = owned->Borrow();
    }
    m_PassBlackboards.PushBack(blackboard);
  }
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelineInstance::Synchronize(const xiiRenderPipelineResourceHandle& resource, bool* out_changed)
{
  if (out_changed)
    *out_changed = false;
  if (!resource.IsValid())
  {
    if (out_changed)
      *out_changed = m_pPipeline != nullptr;
    Clear();
    return XII_FAILURE;
  }
  xiiResourceLock<xiiRenderPipelineResource> acquired(resource, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
  auto                                       pipeline = acquired.IsValid() ? acquired->GetPipeline() : xiiSharedPtr<const xiiRenderPipeline>();
  if (out_changed)
    *out_changed = pipeline != m_pPipeline;
  return Initialize(std::move(pipeline));
}
xiiResult xiiRenderPipelineInstance::BeginFrame(xiiView* view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiVariantDictionary& overrides)
{
  if (!m_pPipeline)
    return XII_FAILURE;
  if (m_uiPreparedSetup != 0 && m_uiPreparedSetup == graph.GetSetupSerial())
  {
    if (m_pPreparedView != view || m_pPreparedBlackboard != &blackboard || m_Overrides != overrides)
    {
      m_sBuildError = "A prepared pipeline frame cannot change its view, blackboard or parameters within the same graph setup.";
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  }
  m_uiPreparedSetup = 0;
  m_sBuildError.Clear();
  const auto&                      settings = m_pPipeline->m_Descriptor.m_Settings;
  const auto                       device   = settings.m_RequiredDeviceFeatures.IsAnyFlagSet() && xiiGALDevice::HasDefaultDevice() ? xiiGALDevice::GetDefaultDevice() : xiiSharedPtr<xiiGALDevice>();
  xiiGALTextureCreationDescription output;
  const bool                       checkOutput = settings.m_OutputFormat != xiiGALResourceFormat::Unknown || settings.m_uiOutputSamples;
  const auto*                      target      = view && checkOutput ? view->GetRenderTargetView() : nullptr;
  bool                             hasOutput   = target != nullptr;
  if (target)
  {
    output = target->GetTexture()->GetDescription();
    if (target->GetDescription().m_Format != xiiGALResourceFormat::Unknown)
      output.m_Format = target->GetDescription().m_Format;
  }
  else if (view && checkOutput && view->GetSwapChain() && view->GetSwapChain()->GetBackBufferTexture())
  {
    output    = view->GetSwapChain()->GetBackBufferTexture()->GetDescription();
    hasOutput = true;
  }
  xiiStringBuilder error;
  if (settings.ValidateDevice(device ? &device->GetFeatures() : nullptr, &error).Failed() || settings.ValidateOutput(hasOutput ? &output : nullptr, &error).Failed())
  {
    xiiStringBuilder identity;
    xiiConversionUtils::ToString(m_pPipeline->m_Descriptor.m_Id, identity);
    xiiLog::Error("Render pipeline '{}' configuration: {}", identity, error);
    m_sBuildError = error;
    return XII_FAILURE;
  }
  if (graph.GetSetupSerial() == 0)
  {
    m_sBuildError = "A pipeline frame requires an active render graph setup.";
    return XII_FAILURE;
  }
  if (m_ExecutionScopes.IsEmpty() || m_sScopeParent != graph.GetResourceNamespace())
  {
    m_sScopeParent = graph.GetResourceNamespace();
    m_ExecutionScopes.SetCount(m_pPipeline->m_ResourceScopes.GetCount());
    for (xiiUInt32 i = 0; i < m_ExecutionScopes.GetCount(); ++i)
    {
      xiiStringBuilder scope;
      scope.SetFormat("PipelineInstance/{}/{}", m_uiInstanceScope, m_pPipeline->m_ResourceScopes[i]);
      if (!m_sScopeParent.IsEmpty())
        scope.Prepend(m_sScopeParent, "/");
      m_ExecutionScopes[i] = scope;
    }
  }
  const xiiString previousScope = graph.GetResourceNamespace();
  XII_SCOPE_EXIT(graph.SetResourceNamespace(previousScope));
  auto* previousBlackboard = graph.GetSetupBlackboard();
  XII_SCOPE_EXIT(graph.SetSetupBlackboard(previousBlackboard));
  for (auto it = m_Blackboards.GetIterator(); it.IsValid(); ++it)
    XII_SUCCEED_OR_RETURN(it.Value()->BeginFrameFrom(blackboard));
  if (m_Overrides != overrides)
  {
    m_Overrides  = overrides;
    m_Parameters = m_pPipeline->m_Descriptor.m_Parameters;
    for (auto it = overrides.GetIterator(); it.IsValid(); ++it)
      m_Parameters.Insert(it.Key(), it.Value());
    m_PassParameters = m_pPipeline->m_PassParameters;
    for (auto& parameters : m_PassParameters)
      for (auto it = overrides.GetIterator(); it.IsValid(); ++it)
        parameters.Insert(it.Key(), it.Value());
  }
  for (auto& binding : m_Bindings)
    binding = {};
  for (xiiUInt32 index = 0; index < m_pPipeline->m_Passes.GetCount(); ++index)
  {
    const auto& pass = m_pPipeline->m_Passes[index];
    if (!pass.m_bHasTemporalHistory)
      continue;
    graph.SetResourceNamespace(m_ExecutionScopes[index]);
    auto& localBlackboard = m_PassBlackboards[index] ? *m_PassBlackboards[index] : blackboard;
    graph.SetSetupBlackboard(&localBlackboard);
    xiiRenderPipelinePassContext history{view, graph, localBlackboard, m_PassParameters[index], pass.m_Pins, m_Bindings.GetArrayPtr().GetSubArray(pass.m_uiBindingOffset, pass.m_Pins.GetCount()), m_pPipeline->m_Descriptor.m_Passes[index].m_sName};
    history.m_sInstanceScope         = m_ExecutionScopes[index];
    history.m_ExtractionRequirements = m_pPipeline->m_GroupRequirements[index];
    history.m_RuntimeFeatures        = m_pPipeline->m_GroupFeatures[index];
    history.m_bTemporalHistoryPhase  = true;
    XII_SUCCEED_OR_RETURN(m_Passes[index]->BuildTemporalHistory(history));
    for (xiiUInt32 i = 0; i < pass.m_Pins.GetCount(); ++i)
    {
      const auto& pin     = pass.m_Pins[i];
      const auto& binding = history.m_Bindings[i];
      if (!pin.m_bTemporalHistoryOutput)
        continue;
      if (pin.m_Type == xiiRenderPipelinePin::Texture)
      {
        const auto* description = graph.GetTextureDescription(binding.m_hTexture);
        if ((binding.m_hTexture.IsValid() || !pin.m_bOptional) && (!description || !pin.Accepts(*description)))
          return XII_FAILURE;
      }
      else if (pin.m_Type == xiiRenderPipelinePin::Buffer && (binding.m_hBuffer.IsValid() || !pin.m_bOptional))
      {
        const auto* description = graph.GetBufferDescription(binding.m_hBuffer);
        if (!description || !pin.Accepts(*description))
          return XII_FAILURE;
      }
      m_HistoryBindings[pass.m_uiBindingOffset + i] = binding;
    }
  }
  m_uiPreparedSetup     = graph.GetSetupSerial();
  m_pPreparedView       = view;
  m_pPreparedBlackboard = &blackboard;
  return XII_SUCCESS;
}

xiiResult xiiRenderPipelineInstance::Build(xiiView* view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiVariantDictionary& overrides, xiiArrayPtr<const xiiRenderPipelineInputBinding> inputs)
{
  XII_SUCCEED_OR_RETURN(BeginFrame(view, graph, blackboard, overrides));
  if (m_uiBuiltSetup == graph.GetSetupSerial())
  {
    m_sBuildError = "A pipeline instance can build only once per graph setup; start a new setup after a failed or completed build.";
    return XII_FAILURE;
  }
  m_uiBuiltSetup = graph.GetSetupSerial();
  xiiHybridArray<xiiUInt8, 32> supplied;
  supplied.SetCount(m_pPipeline->m_Interface.GetCount(), 0);
  for (const auto& input : inputs)
  {
    if (input.m_uiSetupSerial != graph.GetSetupSerial() || input.m_uiInterface >= supplied.GetCount() || supplied[input.m_uiInterface] || !m_pPipeline->m_Interface[input.m_uiInterface].m_Pin.m_bInput)
    {
      m_sBuildError = "An external pipeline input belongs to another setup, is unknown, duplicated or refers to an output interface.";
      return XII_FAILURE;
    }
    supplied[input.m_uiInterface] = 1;
    const auto& external          = m_pPipeline->m_Interface[input.m_uiInterface];
    const auto& binding           = input.m_Binding;
    const auto* texture           = graph.GetTextureDescription(binding.m_hTexture);
    const auto* buffer            = graph.GetBufferDescription(binding.m_hBuffer);
    bool        valid             = false;
    switch (external.m_Pin.m_Type)
    {
      case xiiRenderPipelinePin::Texture:
        valid = !binding.m_pFrameData && !binding.m_hBuffer.IsValid() && binding.m_AccelerationStructure.IsEmpty() &&
          (texture ? external.m_Pin.Accepts(*texture) : external.m_Pin.m_bOptional && !binding.m_hTexture.IsValid());
        break;
      case xiiRenderPipelinePin::Buffer:
        valid = !binding.m_pFrameData && !binding.m_hTexture.IsValid() && binding.m_AccelerationStructure.IsEmpty() &&
          (buffer ? external.m_Pin.Accepts(*buffer) : external.m_Pin.m_bOptional && !binding.m_hBuffer.IsValid());
        break;
      case xiiRenderPipelinePin::AccelerationStructure:
        valid = !binding.m_pFrameData && !binding.m_hTexture.IsValid() && !binding.m_hBuffer.IsValid() &&
          (binding.m_AccelerationStructure.IsValid() ? graph.GetBufferDescription(binding.m_AccelerationStructure.m_Dependency) != nullptr : external.m_Pin.m_bOptional && binding.m_AccelerationStructure.IsEmpty());
        break;
      case xiiRenderPipelinePin::Order:
        valid = !binding.m_pFrameData && !binding.m_hTexture.IsValid() && !binding.m_hBuffer.IsValid() && binding.m_AccelerationStructure.IsEmpty();
        break;
      case xiiRenderPipelinePin::FrameData:
        valid = !binding.m_hTexture.IsValid() && !binding.m_hBuffer.IsValid() && binding.m_AccelerationStructure.IsEmpty() &&
          (binding.m_pFrameData ? external.m_Pin.m_pDataType && binding.m_pFrameData->GetSetupSerial() == graph.GetSetupSerial() && binding.m_pFrameData->GetDynamicRTTI()->IsDerivedFrom(external.m_Pin.m_pDataType) : external.m_Pin.m_bOptional);
        break;
    }
    if (!valid)
    {
      xiiStringBuilder message;
      message.SetFormat("External pipeline input '{}' is stale or violates its compiled resource contract.", external.m_Pin.m_sName);
      m_sBuildError = message;
      return XII_FAILURE;
    }
    for (auto slot : external.m_Bindings)
      m_Bindings[slot] = binding;
  }
  for (xiiUInt32 i = 0; i < supplied.GetCount(); ++i)
    if (m_pPipeline->m_Interface[i].m_Pin.m_bInput && !m_pPipeline->m_Interface[i].m_Pin.m_bOptional && !supplied[i])
    {
      xiiStringBuilder message;
      message.SetFormat("Required external pipeline input '{}' was not supplied.", m_pPipeline->m_Interface[i].m_Pin.m_sName);
      m_sBuildError = message;
      return XII_FAILURE;
    }
  const xiiString previousScope = graph.GetResourceNamespace();
  XII_SCOPE_EXIT(graph.SetResourceNamespace(previousScope));
  auto* previousBlackboard = graph.GetSetupBlackboard();
  XII_SCOPE_EXIT(graph.SetSetupBlackboard(previousBlackboard));
  xiiStringBuilder error;
  for (auto index : m_pPipeline->m_Order)
  {
    graph.SetResourceNamespace(m_ExecutionScopes[index]);
    auto& localBlackboard = m_PassBlackboards[index] ? *m_PassBlackboards[index] : blackboard;
    graph.SetSetupBlackboard(&localBlackboard);
    const auto& pass = m_pPipeline->m_Passes[index];
    for (xiiUInt32 i = 0; i < pass.m_uiConnectionCount; ++i)
    {
      const auto& connection                   = m_pPipeline->m_Connections[pass.m_uiConnectionOffset + i];
      m_Bindings[connection.m_uiTargetBinding] = m_Bindings[connection.m_uiSourceBinding];
    }
    xiiRenderPipelinePassContext context{view, graph, localBlackboard, m_PassParameters[index], pass.m_Pins, m_Bindings.GetArrayPtr().GetSubArray(pass.m_uiBindingOffset, pass.m_Pins.GetCount()), m_pPipeline->m_Descriptor.m_Passes[index].m_sName};
    context.m_sInstanceScope         = m_ExecutionScopes[index];
    context.m_ExtractionRequirements = m_pPipeline->m_GroupRequirements[index];
    context.m_RuntimeFeatures        = m_pPipeline->m_GroupFeatures[index];
    auto validateBindings            = [&](bool inputs) -> xiiResult {
      for (xiiUInt32 i = 0; i < pass.m_Pins.GetCount(); ++i)
      {
        const auto& pin = pass.m_Pins[i];
        if (pin.m_bInput != inputs)
          continue;
        const auto& binding = context.m_Bindings[i];
        if (binding.m_pFrameData && pin.m_Type != xiiRenderPipelinePin::FrameData)
          return context.ReportFailure("A non-data pin contains a frame-data binding.");
        if (pin.m_Type == xiiRenderPipelinePin::Texture)
        {
          const auto* description = graph.GetTextureDescription(binding.m_hTexture);
          if ((binding.m_hTexture.IsValid() || !pin.m_bOptional) && (!description || !pin.Accepts(*description)))
          {
            error.SetFormat("Pass '{}' (type '{}', instance '{}'), {} texture pin '{}': resource is missing or violates its compiled contract.", context.m_sPassName, pass.m_pType->GetTypeName(), context.m_sInstanceScope, inputs ? "input" : "output", pin.m_sName);
            m_sBuildError = error;
            return XII_FAILURE;
          }
        }
        else if (pin.m_Type == xiiRenderPipelinePin::Buffer && (binding.m_hBuffer.IsValid() || !pin.m_bOptional))
        {
          const auto* description = graph.GetBufferDescription(binding.m_hBuffer);
          if (!description || !pin.Accepts(*description))
          {
            error.SetFormat("Pass '{}' (instance '{}'), {} buffer pin '{}': resource is missing or violates its compiled contract.", context.m_sPassName, context.m_sInstanceScope, inputs ? "input" : "output", pin.m_sName);
            m_sBuildError = error;
            return XII_FAILURE;
          }
        }
        else if (pin.m_Type == xiiRenderPipelinePin::FrameData)
        {
          if (binding.m_hTexture.IsValid() || binding.m_hBuffer.IsValid() || !binding.m_AccelerationStructure.IsEmpty() || (binding.m_pFrameData ? !pin.m_pDataType || binding.m_pFrameData->GetSetupSerial() != graph.GetSetupSerial() || !binding.m_pFrameData->GetDynamicRTTI()->IsDerivedFrom(pin.m_pDataType) : !pin.m_bOptional))
          {
            error.SetFormat("Pass '{}' (type '{}', instance '{}'), {} frame-data pin '{}': snapshot is missing, stale or violates its native RTTI contract.", context.m_sPassName, pass.m_pType->GetTypeName(), context.m_sInstanceScope, inputs ? "input" : "output", pin.m_sName);
            m_sBuildError = error;
            return XII_FAILURE;
          }
        }
        else if (pin.m_Type == xiiRenderPipelinePin::AccelerationStructure)
        {
          const auto& structure = binding.m_AccelerationStructure;
          if ((!structure.IsEmpty() || !pin.m_bOptional) && (!structure.IsValid() || !graph.GetBufferDescription(structure.m_Dependency)))
          {
            error.SetFormat("Pass '{}' (type '{}', instance '{}'), {} acceleration-structure pin '{}': TLAS or graph build dependency is missing or stale.", context.m_sPassName, pass.m_pType->GetTypeName(), context.m_sInstanceScope, inputs ? "input" : "output", pin.m_sName);
            m_sBuildError = error;
            return XII_FAILURE;
          }
        }
      }
      return XII_SUCCESS;
    };
    XII_SUCCEED_OR_RETURN(validateBindings(true));
    error.Clear();
    context.m_pBuildError = &error;
    if (m_Passes[index]->Build(context).Failed())
    {
      const xiiString detail(error);
      error.SetFormat("Pass '{}' (instance '{}', type '{}') failed to build its operation. {}", context.m_sPassName, context.m_sInstanceScope, pass.m_pType->GetTypeName(), detail);
      m_sBuildError = error;
      return XII_FAILURE;
    }
    if (pass.m_bHasTemporalHistory)
      for (xiiUInt32 i = 0; i < pass.m_Pins.GetCount(); ++i)
      {
        if (!pass.m_Pins[i].m_bTemporalHistoryOutput)
          continue;
        const auto& expected = m_HistoryBindings[pass.m_uiBindingOffset + i];
        const auto& current  = context.m_Bindings[i];
        if (current.m_hTexture != expected.m_hTexture || current.m_hBuffer != expected.m_hBuffer)
          return XII_FAILURE;
      }
    XII_SUCCEED_OR_RETURN(validateBindings(false));
  }
  return XII_SUCCESS;
}

void xiiRenderPipelineInstance::InvalidateTemporalHistory()
{
  for (auto* pass : m_Passes)
    pass->InvalidateTemporalHistory();
  m_uiPreparedSetup = 0;
}

const xiiRenderPipelineBinding* xiiRenderPipelineInstance::GetOutputBinding(xiiUInt32 passIndex, xiiStringView name) const
{
  if (!m_pPipeline || passIndex >= m_pPipeline->m_Passes.GetCount())
    return nullptr;
  const auto& pass = m_pPipeline->m_Passes[passIndex];
  for (xiiUInt32 index = 0; index < pass.m_Pins.GetCount(); ++index)
    if (!pass.m_Pins[index].m_bInput && pass.m_Pins[index].m_sName == name)
      return &m_Bindings[pass.m_uiBindingOffset + index];
  return nullptr;
}

const xiiRenderPipelineBinding* xiiRenderPipelineInstance::GetInterfaceBinding(xiiUInt32 index) const
{
  if (!m_pPipeline || !m_uiPreparedSetup || index >= m_pPipeline->m_Interface.GetCount() || m_pPipeline->m_Interface[index].m_Bindings.IsEmpty())
    return nullptr;
  return &m_Bindings[m_pPipeline->m_Interface[index].m_Bindings[0]];
}
