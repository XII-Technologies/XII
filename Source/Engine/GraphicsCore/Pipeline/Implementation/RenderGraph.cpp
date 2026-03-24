#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderGraph.h>

namespace
{
  static xiiResult BuildError(xiiStringBuilder* out_pErrorMessage, xiiStringView sMessage)
  {
    if (out_pErrorMessage != nullptr)
    {
      out_pErrorMessage->Set(sMessage);
    }

    return XII_FAILURE;
  }

  static xiiBitflags<xiiGALResourceStateFlags> ResolveRequiredState(const xiiRenderGraphResourceUsage& usage)
  {
    if (usage.m_RequiredState.GetValue() != 0U)
    {
      return usage.m_RequiredState;
    }

    xiiBitflags<xiiGALResourceStateFlags> states = xiiGALResourceStateFlags::Unknown;

    if (usage.m_AccessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::RenderTarget))
    {
      states.Add(xiiGALResourceStateFlags::RenderTarget);
    }

    if (usage.m_AccessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::DepthStencilWrite))
    {
      states.Add(xiiGALResourceStateFlags::DepthWrite);
    }

    if (usage.m_AccessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::DepthStencilReadOnly))
    {
      states.Add(xiiGALResourceStateFlags::DepthRead);
    }

    if (usage.m_AccessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::UnorderedAccess | xiiRenderGraphResourceAccessFlags::Write))
    {
      states.Add(xiiGALResourceStateFlags::UnorderedAccess);
    }

    if (usage.m_AccessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::Read))
    {
      states.Add(xiiGALResourceStateFlags::ShaderResource);
    }

    if (usage.m_AccessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::RayTracingStructure))
    {
      states.Add(xiiGALResourceStateFlags::BuildASRead | xiiGALResourceStateFlags::BuildASWrite | xiiGALResourceStateFlags::RayTracing);
    }

    if (states.GetValue() == 0U)
    {
      states = xiiGALResourceStateFlags::Common;
    }

    return states;
  }

  static void CollectPassResourceStates(const xiiRenderGraphPassDescription& passDescription, xiiHashTable<xiiHashedString, xiiBitflags<xiiGALResourceStateFlags>>& out_resourceStates)
  {
    out_resourceStates.Clear();

    for (const xiiRenderGraphResourceUsage& input : passDescription.m_Inputs)
    {
      const xiiBitflags<xiiGALResourceStateFlags> requiredState = ResolveRequiredState(input);

      xiiBitflags<xiiGALResourceStateFlags> existingState = xiiGALResourceStateFlags::Unknown;
      if (out_resourceStates.TryGetValue(input.m_sResourceName, existingState))
      {
        out_resourceStates.Insert(input.m_sResourceName, existingState | requiredState);
      }
      else
      {
        out_resourceStates.Insert(input.m_sResourceName, requiredState);
      }
    }

    for (const xiiRenderGraphResourceUsage& output : passDescription.m_Outputs)
    {
      const xiiBitflags<xiiGALResourceStateFlags> requiredState = ResolveRequiredState(output);

      xiiBitflags<xiiGALResourceStateFlags> existingState = xiiGALResourceStateFlags::Unknown;
      if (out_resourceStates.TryGetValue(output.m_sResourceName, existingState))
      {
        out_resourceStates.Insert(output.m_sResourceName, existingState | requiredState);
      }
      else
      {
        out_resourceStates.Insert(output.m_sResourceName, requiredState);
      }
    }
  }
} // namespace

void xiiRenderGraphCompiler::AddPass(const xiiRenderGraphPassBase* pPass)
{
  XII_ASSERT_DEV(pPass != nullptr, "Render graph pass must be valid.");

  m_Passes.PushBack(pPass);
}

void xiiRenderGraphCompiler::Reset()
{
  m_Passes.Clear();
}

xiiResult xiiRenderGraphCompiler::Compile(xiiDynamicArray<xiiRenderGraphCompiledPass>& out_compiledPasses, xiiStringBuilder* out_pErrorMessage) const
{
  xiiDynamicArray<xiiRenderGraphBarrier> unusedBarriers;
  return Compile(out_compiledPasses, unusedBarriers, out_pErrorMessage);
}

xiiResult xiiRenderGraphCompiler::Compile(xiiDynamicArray<xiiRenderGraphCompiledPass>& out_compiledPasses, xiiDynamicArray<xiiRenderGraphBarrier>& out_barriers, xiiStringBuilder* out_pErrorMessage) const
{
  out_compiledPasses.Clear();
  out_barriers.Clear();

  if (out_pErrorMessage != nullptr)
  {
    out_pErrorMessage->Clear();
  }

  out_compiledPasses.Reserve(m_Passes.GetCount());

  xiiHashTable<xiiHashedString, xiiUInt32> passNameToIndex;
  xiiHashTable<xiiHashedString, xiiUInt32> resourceProducer;

  xiiDynamicArray<xiiHybridArray<xiiUInt32, 8U>> dependencies;
  xiiDynamicArray<xiiHybridArray<xiiUInt32, 8U>> dependents;
  xiiDynamicArray<xiiUInt32>                    inDegree;

  dependencies.SetCount(m_Passes.GetCount());
  dependents.SetCount(m_Passes.GetCount());
  inDegree.SetCount(m_Passes.GetCount(), 0U);

  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < m_Passes.GetCount(); ++uiPassIndex)
  {
    const xiiRenderGraphPassBase* pPass = m_Passes[uiPassIndex];
    if (pPass == nullptr)
    {
      return BuildError(out_pErrorMessage, "Render graph contains a null pass.");
    }

    const xiiRenderGraphPassDescription& passDescription = pPass->GetDescription();
    XII_SUCCEED_OR_RETURN(ValidatePassDescription(passDescription, uiPassIndex, out_pErrorMessage));

    if (passNameToIndex.Contains(passDescription.m_sPassName))
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph contains duplicate pass name '{0}'.", passDescription.m_sPassName.GetView());
      return BuildError(out_pErrorMessage, sError.GetView());
    }
    passNameToIndex.Insert(passDescription.m_sPassName, uiPassIndex);

    for (const xiiRenderGraphResourceUsage& output : passDescription.m_Outputs)
    {
      if (resourceProducer.Contains(output.m_sResourceName))
      {
        xiiUInt32 uiProducerIndex = xiiInvalidIndex;
        XII_VERIFY(resourceProducer.TryGetValue(output.m_sResourceName, uiProducerIndex), "Producer lookup must succeed.");

        const xiiRenderGraphPassDescription& producerDescription = m_Passes[uiProducerIndex]->GetDescription();
        xiiStringBuilder sError;
        sError.SetFormat("Resource '{0}' is written by multiple passes ('{1}' and '{2}').", output.m_sResourceName.GetView(), producerDescription.m_sPassName.GetView(), passDescription.m_sPassName.GetView());
        return BuildError(out_pErrorMessage, sError.GetView());
      }

      resourceProducer.Insert(output.m_sResourceName, uiPassIndex);
    }
  }

  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < m_Passes.GetCount(); ++uiPassIndex)
  {
    const xiiRenderGraphPassDescription& passDescription = m_Passes[uiPassIndex]->GetDescription();
    for (const xiiRenderGraphResourceUsage& input : passDescription.m_Inputs)
    {
      xiiUInt32 uiProducerIndex = xiiInvalidIndex;
      if (!resourceProducer.TryGetValue(input.m_sResourceName, uiProducerIndex))
      {
        // Input can be an external resource imported by the runtime.
        continue;
      }

      if (uiProducerIndex == uiPassIndex)
      {
        continue;
      }

      if (dependencies[uiPassIndex].Contains(uiProducerIndex))
      {
        continue;
      }

      dependencies[uiPassIndex].PushBack(uiProducerIndex);
      dependents[uiProducerIndex].PushBack(uiPassIndex);
      ++inDegree[uiPassIndex];
    }
  }

  xiiDynamicArray<xiiUInt32> readyPasses;
  readyPasses.Reserve(m_Passes.GetCount());

  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < m_Passes.GetCount(); ++uiPassIndex)
  {
    if (inDegree[uiPassIndex] == 0U)
    {
      readyPasses.PushBack(uiPassIndex);
    }
  }

  xiiUInt32 uiReadyCursor = 0U;
  while (uiReadyCursor < readyPasses.GetCount())
  {
    const xiiUInt32 uiPassIndex           = readyPasses[uiReadyCursor++];
    xiiRenderGraphCompiledPass& compiledPass = out_compiledPasses.ExpandAndGetRef();
    compiledPass.m_pPass                  = m_Passes[uiPassIndex];
    compiledPass.m_uiPassIndex            = uiPassIndex;
    compiledPass.m_Dependencies           = dependencies[uiPassIndex];

    for (xiiUInt32 uiDependentPassIndex : dependents[uiPassIndex])
    {
      XII_ASSERT_DEV(inDegree[uiDependentPassIndex] > 0U, "In-degree must be greater than zero before decrement.");
      --inDegree[uiDependentPassIndex];

      if (inDegree[uiDependentPassIndex] == 0U)
      {
        readyPasses.PushBack(uiDependentPassIndex);
      }
    }
  }

  if (out_compiledPasses.GetCount() != m_Passes.GetCount())
  {
    xiiStringBuilder sError;
    sError.Set("Render graph contains a cycle. Topological scheduling failed.");
    return BuildError(out_pErrorMessage, sError.GetView());
  }

  xiiHashTable<xiiHashedString, xiiBitflags<xiiGALResourceStateFlags>> currentResourceStates;
  xiiHashTable<xiiHashedString, xiiUInt32>                              lastPassUsingResource;
  xiiHashTable<xiiHashedString, xiiBitflags<xiiGALResourceStateFlags>> passResourceStates;

  for (const xiiRenderGraphCompiledPass& compiledPass : out_compiledPasses)
  {
    const xiiRenderGraphPassDescription& passDescription = compiledPass.m_pPass->GetDescription();
    CollectPassResourceStates(passDescription, passResourceStates);

    for (auto it = passResourceStates.GetIterator(); it.IsValid(); ++it)
    {
      const xiiHashedString& resourceName                                = it.Key();
      const xiiBitflags<xiiGALResourceStateFlags> requiredState          = it.Value();

      xiiBitflags<xiiGALResourceStateFlags> currentState                 = xiiGALResourceStateFlags::Unknown;
      const bool bHasCurrentState = currentResourceStates.TryGetValue(resourceName, currentState);

      xiiUInt32 uiPreviousPassIndex = xiiInvalidIndex;
      XII_IGNORE_UNUSED(lastPassUsingResource.TryGetValue(resourceName, uiPreviousPassIndex));

      if (bHasCurrentState && currentState.GetValue() != requiredState.GetValue())
      {
        xiiRenderGraphBarrier& barrier = out_barriers.ExpandAndGetRef();
        barrier.m_sResourceName        = resourceName;
        barrier.m_BeforeState          = currentState;
        barrier.m_AfterState           = requiredState;
        barrier.m_uiFromPassIndex      = uiPreviousPassIndex;
        barrier.m_uiToPassIndex        = compiledPass.m_uiPassIndex;
      }

      currentResourceStates.Insert(resourceName, requiredState);
      lastPassUsingResource.Insert(resourceName, compiledPass.m_uiPassIndex);
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiRenderGraphCompiler::ValidatePassDescription(const xiiRenderGraphPassDescription& passDescription, xiiUInt32 uiPassIndex, xiiStringBuilder* out_pErrorMessage) const
{
  XII_IGNORE_UNUSED(uiPassIndex);

  if (passDescription.m_sPassName.GetString().IsEmpty())
  {
    return BuildError(out_pErrorMessage, "Render graph pass has no name.");
  }

  if (!passDescription.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute | xiiGALCommandQueueFlags::Transfer))
  {
    xiiStringBuilder sError;
    sError.SetFormat("Render graph pass '{0}' does not select a valid command queue.", passDescription.m_sPassName.GetView());
    return BuildError(out_pErrorMessage, sError.GetView());
  }

  for (const xiiRenderGraphResourceUsage& input : passDescription.m_Inputs)
  {
    if (input.m_sResourceName.GetString().IsEmpty())
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph pass '{0}' contains an unnamed input resource.", passDescription.m_sPassName.GetView());
      return BuildError(out_pErrorMessage, sError.GetView());
    }

    if (input.m_AccessFlags.IsNoFlagSet())
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph pass '{0}' input '{1}' must declare at least one access flag.", passDescription.m_sPassName.GetView(), input.m_sResourceName.GetView());
      return BuildError(out_pErrorMessage, sError.GetView());
    }

    const xiiBitflags<xiiGALResourceStateFlags> requiredState = input.m_RequiredState.IsNoFlagSet() ? DeriveRequiredState(input.m_AccessFlags) : input.m_RequiredState;
    if (requiredState.GetValue() == 0U)
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph pass '{0}' input '{1}' could not derive a valid resource state.", passDescription.m_sPassName.GetView(), input.m_sResourceName.GetView());
      return BuildError(out_pErrorMessage, sError.GetView());
    }
  }

  for (const xiiRenderGraphResourceUsage& output : passDescription.m_Outputs)
  {
    if (output.m_sResourceName.GetString().IsEmpty())
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph pass '{0}' contains an unnamed output resource.", passDescription.m_sPassName.GetView());
      return BuildError(out_pErrorMessage, sError.GetView());
    }

    if (output.m_AccessFlags.IsNoFlagSet())
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph pass '{0}' output '{1}' must declare at least one access flag.", passDescription.m_sPassName.GetView(), output.m_sResourceName.GetView());
      return BuildError(out_pErrorMessage, sError.GetView());
    }

    const xiiBitflags<xiiGALResourceStateFlags> requiredState = output.m_RequiredState.IsNoFlagSet() ? DeriveRequiredState(output.m_AccessFlags) : output.m_RequiredState;
    if (requiredState.GetValue() == 0U)
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph pass '{0}' output '{1}' could not derive a valid resource state.", passDescription.m_sPassName.GetView(), output.m_sResourceName.GetView());
      return BuildError(out_pErrorMessage, sError.GetView());
    }
  }

  return XII_SUCCESS;
}

xiiBitflags<xiiGALResourceStateFlags> xiiRenderGraphCompiler::DeriveRequiredState(xiiBitflags<xiiRenderGraphResourceAccessFlags> accessFlags)
{
  xiiRenderGraphResourceUsage usage;
  usage.m_AccessFlags = accessFlags;
  return ResolveRequiredState(usage);
}

xiiResult xiiRenderGraphExecutor::Execute(const xiiArrayPtr<const xiiRenderGraphCompiledPass> compiledPasses, const xiiArrayPtr<const xiiRenderGraphBarrier> barriers, const xiiRenderGraphPassExecutionContext& executionContext, const xiiRenderGraphResourceResolver* pResourceResolver, xiiStringBuilder* out_pErrorMessage) const
{
  if (out_pErrorMessage != nullptr)
  {
    out_pErrorMessage->Clear();
  }

  if (executionContext.m_pCommandList == nullptr)
  {
    return BuildError(out_pErrorMessage, "Render graph execution requires a valid command list.");
  }

  if (!barriers.IsEmpty() && pResourceResolver == nullptr)
  {
    return BuildError(out_pErrorMessage, "Render graph execution requires a resource resolver when barriers are present.");
  }

  xiiHashTable<xiiUInt32, xiiHybridArray<const xiiRenderGraphBarrier*, 4U>> barriersByTargetPass;
  barriersByTargetPass.Reserve(barriers.GetCount());

  for (const xiiRenderGraphBarrier& barrier : barriers)
  {
    if (barrier.m_uiToPassIndex == xiiInvalidIndex)
    {
      continue;
    }

    xiiHybridArray<const xiiRenderGraphBarrier*, 4U> passBarriers;
    if (!barriersByTargetPass.TryGetValue(barrier.m_uiToPassIndex, passBarriers))
    {
      passBarriers.PushBack(&barrier);
      barriersByTargetPass.Insert(barrier.m_uiToPassIndex, passBarriers);
      continue;
    }

    passBarriers.PushBack(&barrier);
    barriersByTargetPass.Insert(barrier.m_uiToPassIndex, passBarriers);
  }

  xiiHybridArray<xiiGALStateTransitionDescription, 16U> stateTransitions;

  for (const xiiRenderGraphCompiledPass& compiledPass : compiledPasses)
  {
    stateTransitions.Clear();

    xiiHybridArray<const xiiRenderGraphBarrier*, 4U> passBarriers;
    if (barriersByTargetPass.TryGetValue(compiledPass.m_uiPassIndex, passBarriers))
    {
      for (const xiiRenderGraphBarrier* pBarrier : passBarriers)
      {
        XII_ASSERT_DEV(pBarrier != nullptr, "Barrier pointer must be valid.");

        xiiGALStateTransitionDescription& transition = stateTransitions.ExpandAndGetRef();
        transition.m_pResource                      = pResourceResolver->ResolveResource(pBarrier->m_sResourceName);
        if (transition.m_pResource == nullptr)
        {
          xiiStringBuilder sError;
          sError.SetFormat("Render graph failed to resolve resource '{0}' for pass index {1}.", pBarrier->m_sResourceName.GetView(), compiledPass.m_uiPassIndex);
          return BuildError(out_pErrorMessage, sError.GetView());
        }

        transition.m_OldState                       = pBarrier->m_BeforeState;
        transition.m_NewState                       = pBarrier->m_AfterState;
        transition.m_TransitionType                 = xiiGALStateTransitionType::Immediate;
        transition.m_TransitionFlags                = xiiGALStateTransitionFlags::UpdateState;
      }
    }

    if (!stateTransitions.IsEmpty())
    {
      executionContext.m_pCommandList->TransitionResourceStates(stateTransitions);
    }

    if (compiledPass.m_pPass == nullptr)
    {
      return BuildError(out_pErrorMessage, "Render graph execution encountered a null compiled pass.");
    }

    compiledPass.m_pPass->RecordCommands(executionContext);
  }

  return XII_SUCCESS;
}

void xiiRenderGraphRuntime::AddPass(const xiiRenderGraphPassBase* pPass)
{
  XII_ASSERT_DEV(pPass != nullptr, "Render graph runtime pass must be valid.");

  m_Passes.PushBack(pPass);
  m_bIsCompiled = false;
}

void xiiRenderGraphRuntime::ClearPasses()
{
  m_Passes.Clear();
  m_CompiledPasses.Clear();
  m_Barriers.Clear();
  m_bIsCompiled = false;
}

void xiiRenderGraphRuntime::SetExternalResourceResolver(const xiiRenderGraphResourceResolver* pResourceResolver)
{
  m_pExternalResourceResolver = pResourceResolver;
}

void xiiRenderGraphRuntime::RemoveResource(xiiHashedString sResourceName)
{
  m_LocalResources.RemoveResource(sResourceName);
}

void xiiRenderGraphRuntime::ClearResources()
{
  m_LocalResources.ClearResources();
}

xiiResult xiiRenderGraphRuntime::Compile(xiiStringBuilder* out_pErrorMessage)
{
  m_Compiler.Reset();

  for (const xiiRenderGraphPassBase* pPass : m_Passes)
  {
    m_Compiler.AddPass(pPass);
  }

  XII_SUCCEED_OR_RETURN(m_Compiler.Compile(m_CompiledPasses, m_Barriers, out_pErrorMessage));

  m_bIsCompiled = true;
  return XII_SUCCESS;
}

xiiResult xiiRenderGraphRuntime::Execute(const xiiRenderGraphPassExecutionContext& executionContext, xiiStringBuilder* out_pErrorMessage) const
{
  if (!m_bIsCompiled)
  {
    return BuildError(out_pErrorMessage, "Render graph runtime is not compiled. Call Compile() before Execute().");
  }

  const xiiRenderGraphResourceResolver* pResolver = m_pExternalResourceResolver;
  if (pResolver == nullptr)
  {
    pResolver = &m_LocalResources;
  }

  return m_Executor.Execute(m_CompiledPasses, m_Barriers, executionContext, pResolver, out_pErrorMessage);
}
