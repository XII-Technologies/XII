/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/JSONWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>

xiiEvent<const xiiRenderGraphDebug::ExecutionEvent&, xiiMutex> xiiRenderGraphDebug::s_ExecutionEvents;

namespace
{
  void WriteId(xiiJSONWriter& writer, xiiStringView name, xiiUInt64 value)
  {
    xiiStringBuilder text;
    text.SetFormat("{}", value);
    writer.AddVariableString(name, text);
  }

  template <typename T>
  void WriteIndices(xiiJSONWriter& writer, xiiStringView name, const T& values)
  {
    writer.BeginArray(name);
    for (auto value : values)
      writer.WriteUInt32(value);
    writer.EndArray();
  }
} // namespace

xiiResult xiiRenderGraphDebug::WriteSnapshot(const ExecutionEvent& event, xiiStreamWriter& stream)
{
  const auto&           graph = event.m_Graph;
  xiiStandardJSONWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetWhitespaceMode(xiiJSONWriter::WhitespaceMode::None);
  writer.BeginObject();
  writer.AddVariableUInt32("version", 1);
  writer.AddVariableString("kind", "renderGraph");
  WriteId(writer, "id", graph.GetId().m_Data);
  WriteId(writer, "instance", graph.GetInstanceId());
  WriteId(writer, "frame", event.m_uiFrameIndex);
  writer.AddVariableString("name", graph.GetName());
  writer.AddVariableString("view", event.m_sViewName);
  writer.AddVariableBool("succeeded", event.m_bSucceeded);
  writer.AddVariableBool("compiled", graph.IsCompiled());
  writer.AddVariableString("error", event.m_sError);
  writer.AddVariableBool("gpuProfiling", event.m_pProfiler != nullptr);

  const auto& stats = graph.GetStatistics();
  writer.BeginObject("statistics");
  writer.AddVariableUInt32("registered", stats.m_uiRegisteredPassCount);
  writer.AddVariableUInt32("compiled", stats.m_uiCompiledPassCount);
  writer.AddVariableUInt32("culled", stats.m_uiCulledPassCount);
  writer.AddVariableUInt32("barriers", stats.m_uiTotalBarrierCount);
  writer.AddVariableUInt32("splitBarriers", stats.m_uiSplitBarrierCount);
  writer.AddVariableUInt32("mergeGroups", stats.m_uiMergeGroupCount);
  writer.AddVariableUInt32("submissions", stats.m_uiQueueSubmissionCount);
  writer.AddVariableUInt32("transientTextures", stats.m_uiTransientTextureCount);
  writer.AddVariableUInt32("transientBuffers", stats.m_uiTransientBufferCount);
  writer.AddVariableUInt32("aliasGroups", stats.m_uiAliasGroupCount);
  writer.AddVariableUInt32("aliasedResources", stats.m_uiAliasedResourceCount);
  writer.AddVariableBool("cached", stats.m_bUsedCachedCompile);
  WriteId(writer, "signature", stats.m_uiGraphSignature);
  writer.EndObject();

  writer.BeginArray("passes");
  for (const auto& pass : graph.GetCompiledPasses())
  {
    writer.BeginObject();
    WriteId(writer, "id", pass.m_Id.m_Data);
    writer.AddVariableString("name", pass.m_sName.GetView());
    writer.AddVariableUInt32("index", pass.m_uiPassIndex);
    writer.AddVariableUInt32("queue", pass.m_uiQueueIndex);
    writer.AddVariableUInt32("mergeGroup", pass.m_uiMergeGroupIndex);
    writer.AddVariableBool("culled", pass.m_bIsCulled);
    writer.AddVariableBool("sideEffects", pass.m_bHasSideEffects);
    writer.AddVariableBool("managed", pass.m_bManagedRenderPass);
    writer.AddVariableBool("allowMerge", pass.m_bAllowMerge);
    WriteIndices(writer, "dependencies", pass.m_DependencyPassIndices);
    WriteIndices(writer, "preBarriers", pass.m_PreBarrierIndices);
    WriteIndices(writer, "splitBegins", pass.m_PostBarrierBeginIndices);
    WriteIndices(writer, "postBarriers", pass.m_PostBarrierIndices);
    WriteIndices(writer, "acquire", pass.m_AcquireResourceIndices);
    WriteIndices(writer, "release", pass.m_ReleaseResourceIndices);
    float      duration  = 0;
    xiiUInt64  frame     = 0;
    const bool available = !pass.m_bIsCulled && event.m_pProfiler && event.m_pProfiler->GetPassTiming(graph.GetInstanceId(), pass.m_uiPassIndex, pass.m_sName.GetView(), duration, frame);
    writer.AddVariableBool("timingAvailable", available);
    if (available)
    {
      writer.AddVariableFloat("gpuMs", duration);
      WriteId(writer, "timingFrame", frame);
    }
    writer.EndObject();
  }
  writer.EndArray();

  writer.BeginArray("resources");
  for (const auto& resource : graph.GetResourceDescriptions())
  {
    writer.BeginObject();
    WriteId(writer, "id", resource.m_Id.m_Data);
    writer.AddVariableString("name", resource.m_sName.GetView());
    writer.AddVariableBool("texture", resource.m_bIsTexture);
    writer.AddVariableUInt32("flags", resource.m_Flags.GetValue());
    writer.AddVariableUInt32("first", resource.m_uiFirstUsePassIndex);
    writer.AddVariableUInt32("last", resource.m_uiLastUsePassIndex);
    writer.AddVariableUInt32("alias", resource.m_uiAliasGroup);
    if (resource.m_bIsTexture)
    {
      const auto& texture = resource.m_TextureDescription;
      writer.AddVariableUInt32("width", texture.m_Size.width);
      writer.AddVariableUInt32("height", texture.m_Size.height);
      writer.AddVariableUInt32("depthOrSlices", texture.m_uiArraySizeOrDepth);
      writer.AddVariableUInt32("mips", texture.m_uiMipLevels);
      xiiStringBuilder format;
      xiiReflectionUtils::EnumerationToString(texture.m_Format, format);
      writer.AddVariableString("format", format);
    }
    else
      WriteId(writer, "bufferBytes", resource.m_BufferDescription.m_uiSize);
    writer.EndObject();
  }
  writer.EndArray();

  writer.BeginArray("versions");
  for (const auto& version : graph.GetResourceVersions())
  {
    writer.BeginObject();
    WriteId(writer, "id", version.m_Id.m_Data);
    WriteId(writer, "resource", version.m_ResourceId.m_Data);
    WriteId(writer, "producer", version.m_ProducerPassId.m_Data);
    writer.AddVariableUInt32("version", version.m_uiVersion);
    writer.AddVariableUInt32("parent", version.m_uiParentVersion);
    writer.AddVariableUInt32("first", version.m_uiFirstUsePassIndex);
    writer.AddVariableUInt32("last", version.m_uiLastUsePassIndex);
    writer.AddVariableUInt32("requiredState", version.m_RequiredState.GetValue());
    writer.AddVariableUInt32("currentState", version.m_CurrentState.GetValue());
    writer.AddVariableBool("exported", version.m_bExported);
    writer.EndObject();
  }
  writer.EndArray();

  writer.BeginArray("barriers");
  for (const auto& barrier : graph.GetBarriers())
  {
    writer.BeginObject();
    writer.AddVariableUInt32("resource", barrier.m_uiResourceIndex);
    writer.AddVariableUInt32("before", barrier.m_BeforeState.GetValue());
    writer.AddVariableUInt32("after", barrier.m_AfterState.GetValue());
    xiiStringBuilder state;
    xiiReflectionUtils::BitflagsToString(barrier.m_BeforeState, state);
    writer.AddVariableString("beforeName", state);
    xiiReflectionUtils::BitflagsToString(barrier.m_AfterState, state);
    writer.AddVariableString("afterName", state);
    writer.AddVariableUInt32("type", barrier.m_TransitionType.GetValue());
    writer.AddVariableUInt32("flags", barrier.m_TransitionFlags.GetValue());
    writer.AddVariableUInt32("sourceQueue", barrier.m_uiSourceQueue);
    writer.AddVariableUInt32("targetQueue", barrier.m_uiTargetQueue);
    writer.AddVariableBool("ownershipTransfer", barrier.m_bQueueOwnershipTransfer);
    writer.AddVariableUInt32("firstMip", barrier.m_uiFirstMipLevel);
    writer.AddVariableUInt32("mips", barrier.m_uiMipLevelCount);
    writer.AddVariableUInt32("firstSlice", barrier.m_uiFirstArraySlice);
    writer.AddVariableUInt32("slices", barrier.m_uiArraySliceCount);
    writer.EndObject();
  }
  writer.EndArray();

  writer.BeginArray("submissions");
  for (const auto& submission : graph.GetQueueSubmissions())
  {
    writer.BeginObject();
    writer.AddVariableUInt32("queue", submission.m_uiQueueIndex);
    writer.AddVariableUInt32("flags", submission.m_QueueFlags.GetValue());
    WriteId(writer, "signal", submission.m_uiSignalValue);
    WriteIndices(writer, "passes", submission.m_PassOrder);
    WriteIndices(writer, "waits", submission.m_WaitSubmissionIndices);
    writer.BeginArray("waitValues");
    for (auto value : submission.m_WaitValues)
    {
      xiiStringBuilder text;
      text.SetFormat("{}", value);
      writer.WriteString(text);
    }
    writer.EndArray();
    writer.EndObject();
  }
  writer.EndArray();

  writer.BeginArray("mergeGroups");
  for (const auto& group : graph.GetMergeGroups())
  {
    writer.BeginObject();
    WriteIndices(writer, "passes", group.m_PassIndices);
    WriteIndices(writer, "attachments", group.m_AttachmentResourceIndices);
    writer.EndObject();
  }
  writer.EndArray();
  writer.EndObject();
  return writer.HadWriteError() ? XII_FAILURE : XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderGraphCapture);
