#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
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

  static xiiResult ValidateRayTracingAccessFlags(const xiiRenderGraphPassDescription& passDescription, const xiiRenderGraphResourceUsage& usage, bool bIsInput, xiiStringBuilder* out_pErrorMessage)
  {
    const xiiBitflags<xiiRenderGraphResourceAccessFlags> accessFlags = usage.m_AccessFlags;

    if (accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::RayTracing) && accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::Write))
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph pass '{0}' {1} resource '{2}' cannot combine RayTracing with Write access.",
                       passDescription.m_sPassName.GetView(), bIsInput ? "input" : "output", usage.m_sResourceName.GetView());
      return BuildError(out_pErrorMessage, sError.GetView());
    }

    if (accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::BuildASRead) && accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::Write))
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph pass '{0}' {1} resource '{2}' cannot combine BuildASRead with Write access.",
                       passDescription.m_sPassName.GetView(), bIsInput ? "input" : "output", usage.m_sResourceName.GetView());
      return BuildError(out_pErrorMessage, sError.GetView());
    }

    if (accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::BuildASWrite) && !accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::Write))
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph pass '{0}' {1} resource '{2}' uses BuildASWrite but is missing Write access.",
                       passDescription.m_sPassName.GetView(), bIsInput ? "input" : "output", usage.m_sResourceName.GetView());
      return BuildError(out_pErrorMessage, sError.GetView());
    }

    return XII_SUCCESS;
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

    if (usage.m_AccessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::BuildASRead))
    {
      states.Add(xiiGALResourceStateFlags::BuildASRead);
    }

    if (usage.m_AccessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::BuildASWrite))
    {
      states.Add(xiiGALResourceStateFlags::BuildASWrite);
    }

    if (usage.m_AccessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::RayTracing))
    {
      states.Add(xiiGALResourceStateFlags::RayTracing);
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

  static void AppendHash(xiiUInt64& inout_uiSeed, const void* pData, xiiUInt64 uiNumBytes)
  {
    inout_uiSeed = xiiHashingUtils::xxHash64(pData, static_cast<size_t>(uiNumBytes), inout_uiSeed);
  }

  template <typename T>
  static void AppendHashValue(xiiUInt64& inout_uiSeed, const T& value)
  {
    AppendHash(inout_uiSeed, &value, sizeof(T));
  }

  static xiiUInt64 ComputeRenderGraphSignature(const xiiArrayPtr<const xiiRenderGraphPassBase* const>& passes, const xiiRenderGraphCompileSettings& compileSettings)
  {
    xiiUInt64 uiSeed = 0x9E3779B97F4A7C15ULL;

    AppendHashValue(uiSeed, compileSettings.m_bEnablePassCulling);
    AppendHashValue(uiSeed, compileSettings.m_bEnableCompileCache);
    AppendHashValue(uiSeed, compileSettings.m_uiCacheSalt);

    const xiiUInt32 uiPassCount = passes.GetCount();
    AppendHashValue(uiSeed, uiPassCount);

    for (const xiiRenderGraphPassBase* pPass : passes)
    {
      if (pPass == nullptr)
      {
        const xiiUInt32 uiNullPassMarker = 0xFFFFFFFFU;
        AppendHashValue(uiSeed, uiNullPassMarker);
        continue;
      }

      const xiiRenderGraphPassDescription& passDescription = pPass->GetDescription();
      const xiiUInt64                      uiPassNameHash  = passDescription.m_sPassName.GetHash();
      const xiiUInt32                      uiQueueFlags    = passDescription.m_QueueFlags.GetValue();

      AppendHashValue(uiSeed, uiPassNameHash);
      AppendHashValue(uiSeed, uiQueueFlags);
      AppendHashValue(uiSeed, passDescription.m_bHasSideEffects);

      const xiiUInt32 uiInputCount  = passDescription.m_Inputs.GetCount();
      const xiiUInt32 uiOutputCount = passDescription.m_Outputs.GetCount();

      AppendHashValue(uiSeed, uiInputCount);
      AppendHashValue(uiSeed, uiOutputCount);

      for (const xiiRenderGraphResourceUsage& usage : passDescription.m_Inputs)
      {
        const xiiUInt64 uiResourceNameHash = usage.m_sResourceName.GetHash();
        const xiiUInt16 uiAccessFlags      = usage.m_AccessFlags.GetValue();
        const xiiUInt32 uiRequiredState    = usage.m_RequiredState.GetValue();

        AppendHashValue(uiSeed, uiResourceNameHash);
        AppendHashValue(uiSeed, uiAccessFlags);
        AppendHashValue(uiSeed, uiRequiredState);
      }

      for (const xiiRenderGraphResourceUsage& usage : passDescription.m_Outputs)
      {
        const xiiUInt64 uiResourceNameHash = usage.m_sResourceName.GetHash();
        const xiiUInt16 uiAccessFlags      = usage.m_AccessFlags.GetValue();
        const xiiUInt32 uiRequiredState    = usage.m_RequiredState.GetValue();

        AppendHashValue(uiSeed, uiResourceNameHash);
        AppendHashValue(uiSeed, uiAccessFlags);
        AppendHashValue(uiSeed, uiRequiredState);
      }
    }

    return uiSeed;
  }

  static void BuildLivePassMask(const xiiArrayPtr<const xiiRenderGraphPassBase* const>& passes, const xiiArrayPtr<const xiiHybridArray<xiiUInt32, 8U>>& dependencies, const xiiRenderGraphCompileSettings& compileSettings, xiiDynamicArray<bool>& out_livePassMask)
  {
    out_livePassMask.SetCount(passes.GetCount(), true);

    if (!compileSettings.m_bEnablePassCulling)
    {
      return;
    }

    xiiDynamicArray<bool> livePasses;
    livePasses.SetCount(passes.GetCount(), false);

    xiiDynamicArray<xiiUInt32> livePassStack;
    livePassStack.Reserve(passes.GetCount());

    for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < passes.GetCount(); ++uiPassIndex)
    {
      const xiiRenderGraphPassBase* pPass = passes[uiPassIndex];
      if (pPass != nullptr && pPass->GetDescription().m_bHasSideEffects)
      {
        livePasses[uiPassIndex] = true;
        livePassStack.PushBack(uiPassIndex);
      }
    }

    if (livePassStack.IsEmpty())
    {
      // Keep current behavior if no terminal passes were explicitly tagged as side-effectful.
      return;
    }

    while (!livePassStack.IsEmpty())
    {
      const xiiUInt32 uiPassIndex = livePassStack.PeekBack();
      livePassStack.PopBack();

      for (xiiUInt32 uiDependencyPassIndex : dependencies[uiPassIndex])
      {
        if (livePasses[uiDependencyPassIndex])
        {
          continue;
        }

        livePasses[uiDependencyPassIndex] = true;
        livePassStack.PushBack(uiDependencyPassIndex);
      }
    }

    out_livePassMask = livePasses;
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

xiiResult xiiRenderGraphCompiler::Compile(xiiDynamicArray<xiiRenderGraphCompiledPass>& out_compiledPasses, xiiDynamicArray<xiiRenderGraphBarrier>& out_barriers, const xiiRenderGraphCompileSettings& compileSettings, xiiRenderGraphStatistics* out_pStatistics, xiiStringBuilder* out_pErrorMessage) const
{
  out_compiledPasses.Clear();
  out_barriers.Clear();

  if (out_pErrorMessage != nullptr)
  {
    out_pErrorMessage->Clear();
  }

  if (out_pStatistics != nullptr)
  {
    out_pStatistics->m_uiRegisteredPassCount = m_Passes.GetCount();
    out_pStatistics->m_uiCompiledPassCount   = 0U;
    out_pStatistics->m_uiCulledPassCount     = 0U;
    out_pStatistics->m_uiBarrierCount        = 0U;
  }

  out_compiledPasses.Reserve(m_Passes.GetCount());

  xiiHashTable<xiiHashedString, xiiUInt32> passNameToIndex;
  xiiHashTable<xiiHashedString, xiiUInt32> resourceProducer;

  xiiDynamicArray<xiiHybridArray<xiiUInt32, 8U>> dependencies;
  xiiDynamicArray<xiiHybridArray<xiiUInt32, 8U>> dependents;
  xiiDynamicArray<xiiUInt32>                     inDegree;

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
        xiiStringBuilder                     sError;
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

  xiiDynamicArray<bool> livePassMask;
  BuildLivePassMask(m_Passes, dependencies, compileSettings, livePassMask);

  xiiUInt32 uiIncludedPassCount = 0U;
  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < livePassMask.GetCount(); ++uiPassIndex)
  {
    if (livePassMask[uiPassIndex])
    {
      ++uiIncludedPassCount;
      continue;
    }

    inDegree[uiPassIndex] = 0U;
  }

  xiiDynamicArray<xiiUInt32> readyPasses;
  readyPasses.Reserve(m_Passes.GetCount());

  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < m_Passes.GetCount(); ++uiPassIndex)
  {
    if (!livePassMask[uiPassIndex])
    {
      continue;
    }

    if (inDegree[uiPassIndex] == 0U)
    {
      readyPasses.PushBack(uiPassIndex);
    }
  }

  xiiUInt32 uiReadyCursor = 0U;
  while (uiReadyCursor < readyPasses.GetCount())
  {
    const xiiUInt32             uiPassIndex  = readyPasses[uiReadyCursor++];
    xiiRenderGraphCompiledPass& compiledPass = out_compiledPasses.ExpandAndGetRef();
    compiledPass.m_pPass                     = m_Passes[uiPassIndex];
    compiledPass.m_uiPassIndex               = uiPassIndex;
    compiledPass.m_Dependencies              = dependencies[uiPassIndex];

    for (xiiUInt32 uiDependentPassIndex : dependents[uiPassIndex])
    {
      if (!livePassMask[uiDependentPassIndex])
      {
        continue;
      }

      XII_ASSERT_DEV(inDegree[uiDependentPassIndex] > 0U, "In-degree must be greater than zero before decrement.");
      --inDegree[uiDependentPassIndex];

      if (inDegree[uiDependentPassIndex] == 0U)
      {
        readyPasses.PushBack(uiDependentPassIndex);
      }
    }
  }

  if (out_compiledPasses.GetCount() != uiIncludedPassCount)
  {
    xiiStringBuilder sError;
    sError.Set("Render graph contains a cycle. Topological scheduling failed.");
    return BuildError(out_pErrorMessage, sError.GetView());
  }

  xiiHashTable<xiiHashedString, xiiBitflags<xiiGALResourceStateFlags>> currentResourceStates;
  xiiHashTable<xiiHashedString, xiiUInt32>                             lastPassUsingResource;
  xiiHashTable<xiiHashedString, xiiBitflags<xiiGALResourceStateFlags>> passResourceStates;

  for (const xiiRenderGraphCompiledPass& compiledPass : out_compiledPasses)
  {
    const xiiRenderGraphPassDescription& passDescription = compiledPass.m_pPass->GetDescription();
    CollectPassResourceStates(passDescription, passResourceStates);

    for (auto it = passResourceStates.GetIterator(); it.IsValid(); ++it)
    {
      const xiiHashedString&                      resourceName  = it.Key();
      const xiiBitflags<xiiGALResourceStateFlags> requiredState = it.Value();

      xiiBitflags<xiiGALResourceStateFlags> currentState     = xiiGALResourceStateFlags::Unknown;
      const bool                            bHasCurrentState = currentResourceStates.TryGetValue(resourceName, currentState);

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

  if (out_pStatistics != nullptr)
  {
    out_pStatistics->m_uiCompiledPassCount = out_compiledPasses.GetCount();
    out_pStatistics->m_uiCulledPassCount   = m_Passes.GetCount() - out_compiledPasses.GetCount();
    out_pStatistics->m_uiBarrierCount      = out_barriers.GetCount();
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

    XII_SUCCEED_OR_RETURN(ValidateRayTracingAccessFlags(passDescription, input, true, out_pErrorMessage));

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

    XII_SUCCEED_OR_RETURN(ValidateRayTracingAccessFlags(passDescription, output, false, out_pErrorMessage));

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

  xiiUInt32 uiMaxPassIndex = 0U;
  for (const xiiRenderGraphCompiledPass& compiledPass : compiledPasses)
  {
    uiMaxPassIndex = xiiMath::Max(uiMaxPassIndex, compiledPass.m_uiPassIndex);
  }

  xiiDynamicArray<xiiHybridArray<const xiiRenderGraphBarrier*, 4U>> barriersByTargetPass;
  barriersByTargetPass.SetCount(uiMaxPassIndex + 1U);

  for (const xiiRenderGraphBarrier& barrier : barriers)
  {
    if (barrier.m_uiToPassIndex == xiiInvalidIndex || barrier.m_uiToPassIndex >= barriersByTargetPass.GetCount())
    {
      continue;
    }

    barriersByTargetPass[barrier.m_uiToPassIndex].PushBack(&barrier);
  }

  xiiHybridArray<xiiGALStateTransitionDescription, 16U> stateTransitions;

  for (const xiiRenderGraphCompiledPass& compiledPass : compiledPasses)
  {
    stateTransitions.Clear();

    const xiiRenderGraphPassDescription&       passDescription       = compiledPass.m_pPass->GetDescription();
    const xiiBitflags<xiiGALCommandQueueFlags> commandListQueueFlags = executionContext.m_pCommandList->GetDescription().m_QueueFlags;
    if (!commandListQueueFlags.IsAnySet(passDescription.m_QueueFlags))
    {
      xiiStringBuilder sError;
      sError.SetFormat("Render graph pass '{0}' requires queue flags {1}, but command list only supports {2}.",
                       passDescription.m_sPassName.GetView(), passDescription.m_QueueFlags.GetValue(), commandListQueueFlags.GetValue());
      return BuildError(out_pErrorMessage, sError.GetView());
    }

    if (compiledPass.m_uiPassIndex < barriersByTargetPass.GetCount())
    {
      const xiiHybridArray<const xiiRenderGraphBarrier*, 4U>& passBarriers = barriersByTargetPass[compiledPass.m_uiPassIndex];

      for (const xiiRenderGraphBarrier* pBarrier : passBarriers)
      {
        XII_ASSERT_DEV(pBarrier != nullptr, "Barrier pointer must be valid.");

        xiiGALStateTransitionDescription& transition = stateTransitions.ExpandAndGetRef();
        transition.m_pResource                       = pResourceResolver->ResolveResource(pBarrier->m_sResourceName);
        if (transition.m_pResource == nullptr)
        {
          xiiStringBuilder sError;
          sError.SetFormat("Render graph failed to resolve resource '{0}' for pass index {1}.", pBarrier->m_sResourceName.GetView(), compiledPass.m_uiPassIndex);
          return BuildError(out_pErrorMessage, sError.GetView());
        }

        transition.m_OldState        = pBarrier->m_BeforeState;
        transition.m_NewState        = pBarrier->m_AfterState;
        transition.m_TransitionType  = xiiGALStateTransitionType::Immediate;
        transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
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
  m_bIsCompiled                     = false;
  m_Statistics.m_bUsedCachedCompile = false;
}

void xiiRenderGraphRuntime::ClearPasses()
{
  m_Passes.Clear();
  m_CompiledPasses.Clear();
  m_Barriers.Clear();
  m_Statistics             = xiiRenderGraphStatistics();
  m_uiLastCompileSignature = 0ULL;
  m_bIsCompiled            = false;
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
  const xiiUInt64 uiGraphSignature = ComputeRenderGraphSignature(m_Passes, m_CompileSettings);

  m_Statistics.m_uiRegisteredPassCount = m_Passes.GetCount();
  m_Statistics.m_uiGraphSignature      = uiGraphSignature;

  if (m_CompileSettings.m_bEnableCompileCache && m_bIsCompiled && uiGraphSignature == m_uiLastCompileSignature)
  {
    m_Statistics.m_uiCompiledPassCount = m_CompiledPasses.GetCount();
    m_Statistics.m_uiCulledPassCount   = m_Passes.GetCount() - m_CompiledPasses.GetCount();
    m_Statistics.m_uiBarrierCount      = m_Barriers.GetCount();
    m_Statistics.m_bUsedCachedCompile  = true;
    return XII_SUCCESS;
  }

  m_Compiler.Reset();

  for (const xiiRenderGraphPassBase* pPass : m_Passes)
  {
    m_Compiler.AddPass(pPass);
  }

  XII_SUCCEED_OR_RETURN(m_Compiler.Compile(m_CompiledPasses, m_Barriers, m_CompileSettings, &m_Statistics, out_pErrorMessage));

  m_Statistics.m_uiGraphSignature   = uiGraphSignature;
  m_Statistics.m_bUsedCachedCompile = false;
  m_uiLastCompileSignature          = uiGraphSignature;
  m_bIsCompiled                     = true;
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
