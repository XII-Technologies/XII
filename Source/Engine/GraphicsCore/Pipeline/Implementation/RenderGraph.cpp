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
  out_compiledPasses.Clear();

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
  inDegree.SetCount(m_Passes.GetCount());
  inDegree.Fill(0U);

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
    if (requiredState.IsNoFlagSet() || requiredState == xiiGALResourceStateFlags::Unknown)
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
    if (requiredState.IsNoFlagSet() || requiredState == xiiGALResourceStateFlags::Unknown)
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
  xiiBitflags<xiiGALResourceStateFlags> states = xiiGALResourceStateFlags::Unknown;

  if (accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::RenderTarget))
  {
    states.Add(xiiGALResourceStateFlags::RenderTarget);
  }

  if (accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::DepthStencilWrite))
  {
    states.Add(xiiGALResourceStateFlags::DepthWrite);
  }

  if (accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::DepthStencilReadOnly))
  {
    states.Add(xiiGALResourceStateFlags::DepthRead);
  }

  if (accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::UnorderedAccess | xiiRenderGraphResourceAccessFlags::Write))
  {
    states.Add(xiiGALResourceStateFlags::UnorderedAccess);
  }

  if (accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::Read))
  {
    states.Add(xiiGALResourceStateFlags::ShaderResource);
  }

  if (accessFlags.IsAnySet(xiiRenderGraphResourceAccessFlags::RayTracingStructure))
  {
    states.Add(xiiGALResourceStateFlags::BuildASRead | xiiGALResourceStateFlags::BuildASWrite | xiiGALResourceStateFlags::RayTracing);
  }

  if (states == xiiGALResourceStateFlags::Unknown)
  {
    states = xiiGALResourceStateFlags::Common;
  }

  return states;
}
