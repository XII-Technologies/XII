#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/SerializationContext.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <GraphicsCore/Pipeline/Extractor.h>
#include <GraphicsCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

////////////////////////////////////////////////////////////////////////
// xiiDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineResourceLoaderConnection, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineResourceLoaderConnection>)
{
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiRenderPipelineResourceLoaderConnection::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_uiSource;
  inout_stream << m_uiTarget;
  inout_stream << m_sSourcePin;
  inout_stream << m_sTargetPin;

  return XII_SUCCESS;
}

xiiResult xiiRenderPipelineResourceLoaderConnection::Deserialize(xiiStreamReader& inout_stream)
{
  XII_VERIFY(xiiTypeVersionReadContext::GetContext()->GetTypeVersion(xiiGetStaticRTTI<xiiRenderPipelineResourceLoaderConnection>()) == 1, "Unknown version");

  inout_stream >> m_uiSource;
  inout_stream >> m_uiTarget;
  inout_stream >> m_sSourcePin;
  inout_stream >> m_sTargetPin;

  return XII_SUCCESS;
}

constexpr xiiTypeVersion s_RenderPipelineDescriptorVersion = 1;

// static
xiiInternal::NewInstance<xiiRenderPipeline> xiiRenderPipelineResourceLoader::CreateRenderPipeline(const xiiRenderPipelineResourceDescriptor& desc)
{
  auto pPipeline = XII_DEFAULT_NEW(xiiRenderPipeline);

  xiiRawMemoryStreamReader inout_stream(desc.m_SerializedPipeline);

  const auto uiVersion = inout_stream.ReadVersion(s_RenderPipelineDescriptorVersion);
  XII_IGNORE_UNUSED(uiVersion);

  xiiStringDeduplicationReadContext stringDeduplicationReadContext(inout_stream);
  xiiTypeVersionReadContext         typeVersionReadContext(inout_stream);

  xiiStringBuilder sTypeName;

  xiiHybridArray<xiiRenderPipelinePass*, 16> passes;

  // Passes
  {
    xiiUInt32 uiNumPasses = 0;
    inout_stream >> uiNumPasses;

    for (xiiUInt32 i = 0; i < uiNumPasses; ++i)
    {
      inout_stream >> sTypeName;
      if (const xiiRTTI* pType = xiiRTTI::FindTypeByName(sTypeName))
      {
        xiiUniquePtr<xiiRenderPipelinePass> pPass = pType->GetAllocator()->Allocate<xiiRenderPipelinePass>();
        pPass->Deserialize(inout_stream).AssertSuccess("");
        passes.PushBack(pPass.Borrow());
        pPipeline->AddPass(std::move(pPass));
      }
      else
      {
        xiiLog::Error("Unknown render pipeline pass type '{}'", sTypeName);
        return nullptr;
      }
    }
  }

  // Extractors
  {
    xiiUInt32 uiNumExtractors = 0;
    inout_stream >> uiNumExtractors;

    for (xiiUInt32 i = 0; i < uiNumExtractors; ++i)
    {
      inout_stream >> sTypeName;
      if (const xiiRTTI* pType = xiiRTTI::FindTypeByName(sTypeName))
      {
        xiiUniquePtr<xiiExtractor> pExtractor = pType->GetAllocator()->Allocate<xiiExtractor>();
        pExtractor->Deserialize(inout_stream).AssertSuccess("");
        pPipeline->AddExtractor(std::move(pExtractor));
      }
      else
      {
        xiiLog::Error("Unknown render pipeline extractor type '{}'", sTypeName);
        return nullptr;
      }
    }
  }

  // Connections
  {
    xiiUInt32 uiNumConnections = 0;
    inout_stream >> uiNumConnections;

    for (xiiUInt32 i = 0; i < uiNumConnections; ++i)
    {
      xiiRenderPipelineResourceLoaderConnection data;
      data.Deserialize(inout_stream).AssertSuccess("Failed to deserialize render pipeline connection");

      xiiRenderPipelinePass* pSource = passes[data.m_uiSource];
      xiiRenderPipelinePass* pTarget = passes[data.m_uiTarget];

      if (!pPipeline->Connect(pSource, data.m_sSourcePin, pTarget, data.m_sTargetPin))
      {
        xiiLog::Error("Failed to connect '{0}'::'{1}' to '{2}'::'{3}'!", pSource->GetName(), data.m_sSourcePin, pTarget->GetName(), data.m_sTargetPin);
      }
    }
  }
  return pPipeline;
}

// static
void xiiRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(const xiiRenderPipeline* pPipeline, xiiRenderPipelineResourceDescriptor& ref_desc)
{
  xiiHybridArray<const xiiRenderPipelinePass*, 16>              passes;
  xiiHybridArray<const xiiExtractor*, 16>                       extractors;
  xiiHybridArray<xiiRenderPipelineResourceLoaderConnection, 16> connections;

  xiiHashTable<const xiiRenderPipelineNode*, xiiUInt32> passToIndex;
  pPipeline->GetPasses(passes);
  pPipeline->GetExtractors(extractors);

  passToIndex.Reserve(passes.GetCount());
  for (xiiUInt32 i = 0; i < passes.GetCount(); i++)
  {
    passToIndex.Insert(passes[i], i);
  }


  for (xiiUInt32 i = 0; i < passes.GetCount(); i++)
  {
    const xiiRenderPipelinePass* pSource = passes[i];

    xiiRenderPipelineResourceLoaderConnection data;
    data.m_uiSource = i;

    auto outputs = pSource->GetOutputPins();
    for (const xiiRenderPipelineNodePin* pPinSource : outputs)
    {
      data.m_sSourcePin = pSource->GetPinName(pPinSource).GetView();

      const xiiRenderPipelinePassConnection* pConnection = pPipeline->GetOutputConnection(pSource, pSource->GetPinName(pPinSource));
      if (!pConnection)
        continue;

      for (const xiiRenderPipelineNodePin* pPinTarget : pConnection->m_Inputs)
      {
        XII_VERIFY(passToIndex.TryGetValue(pPinTarget->m_pParent, data.m_uiTarget), "Failed to resolve render pass to index");
        data.m_sTargetPin = pPinTarget->m_pParent->GetPinName(pPinTarget).GetView();

        connections.PushBack(data);
      }
    }
  }

  xiiMemoryStreamContainerWrapperStorage<xiiDynamicArray<xiiUInt8>> storage(&ref_desc.m_SerializedPipeline);
  xiiMemoryStreamWriter                                             memoryWriter(&storage);
  ExportPipeline(passes.GetArrayPtr(), extractors.GetArrayPtr(), connections.GetArrayPtr(), memoryWriter).AssertSuccess("Failed to serialize pipeline");
}

xiiResult xiiRenderPipelineResourceLoader::ExportPipeline(xiiArrayPtr<const xiiRenderPipelinePass* const> passes, xiiArrayPtr<const xiiExtractor* const> extractors, xiiArrayPtr<const xiiRenderPipelineResourceLoaderConnection> connections, xiiStreamWriter& ref_streamWriter)
{
  ref_streamWriter.WriteVersion(s_RenderPipelineDescriptorVersion);

  xiiStringDeduplicationWriteContext stringDeduplicationWriteContext(ref_streamWriter);
  xiiTypeVersionWriteContext         typeVersionWriteContext;
  auto&                              stream = typeVersionWriteContext.Begin(stringDeduplicationWriteContext.Begin());

  // passes
  {
    const xiiUInt32 uiNumPasses = passes.GetCount();
    stream << uiNumPasses;

    for (auto& pass : passes)
    {
      auto pPassType = pass->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pPassType);

      stream << pPassType->GetTypeName();
      XII_SUCCEED_OR_RETURN(pass->Serialize(stream));
    }
  }

  // extractors
  {
    const xiiUInt32 uiNumExtractors = extractors.GetCount();
    stream << uiNumExtractors;

    for (auto& extractor : extractors)
    {
      auto pExtractorType = extractor->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pExtractorType);

      stream << pExtractorType->GetTypeName();
      XII_SUCCEED_OR_RETURN(extractor->Serialize(stream));
    }
  }

  // Connections
  {
    const xiiUInt32 uiNumConnections = connections.GetCount();
    stream << uiNumConnections;

    typeVersionWriteContext.AddType(xiiGetStaticRTTI<xiiRenderPipelineResourceLoaderConnection>());

    for (auto& connection : connections)
    {
      XII_SUCCEED_OR_RETURN(connection.Serialize(stream));
    }
  }

  XII_SUCCEED_OR_RETURN(typeVersionWriteContext.End());
  XII_SUCCEED_OR_RETURN(stringDeduplicationWriteContext.End());

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelineResourceLoader);
