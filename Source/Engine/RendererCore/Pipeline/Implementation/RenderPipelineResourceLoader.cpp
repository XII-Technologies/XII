#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Serialization/BinarySerializer.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>

////////////////////////////////////////////////////////////////////////
// xiiDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

struct RenderPipelineResourceLoaderConnectionInternal
{
  xiiUuid   m_Source;
  xiiUuid   m_Target;
  xiiString m_SourcePin;
  xiiString m_TargetPin;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, RenderPipelineResourceLoaderConnectionInternal);

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(RenderPipelineResourceLoaderConnectionInternal, xiiNoBase, 1, xiiRTTIDefaultAllocator<RenderPipelineResourceLoaderConnectionInternal>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Connection::Source", m_Source),
    XII_MEMBER_PROPERTY("Connection::Target", m_Target),
    XII_MEMBER_PROPERTY("Connection::SourcePin", m_SourcePin),    
    XII_MEMBER_PROPERTY("Connection::TargetPin", m_TargetPin),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiRenderPipelineRttiConverterContext::Clear()
{
  xiiRttiConverterContext::Clear();

  m_pRenderPipeline = nullptr;
}

xiiInternal::NewInstance<void> xiiRenderPipelineRttiConverterContext::CreateObject(const xiiUuid& guid, const xiiRTTI* pRtti)
{
  XII_ASSERT_DEBUG(pRtti != nullptr, "Object type is unknown");

  if (pRtti->IsDerivedFrom<xiiRenderPipelinePass>())
  {
    if (!pRtti->GetAllocator()->CanAllocate())
    {
      xiiLog::Error("Failed to create xiiRenderPipelinePass because '{0}' cannot allocate!", pRtti->GetTypeName());
      return nullptr;
    }

    auto pass = pRtti->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    m_pRenderPipeline->AddPass(pass);

    RegisterObject(guid, pRtti, pass);
    return pass;
  }
  else if (pRtti->IsDerivedFrom<xiiExtractor>())
  {
    if (!pRtti->GetAllocator()->CanAllocate())
    {
      xiiLog::Error("Failed to create xiiExtractor because '{0}' cannot allocate!", pRtti->GetTypeName());
      return nullptr;
    }

    auto extractor = pRtti->GetAllocator()->Allocate<xiiExtractor>();
    m_pRenderPipeline->AddExtractor(extractor);

    RegisterObject(guid, pRtti, extractor);
    return extractor;
  }
  else
  {
    return xiiRttiConverterContext::CreateObject(guid, pRtti);
  }
}

void xiiRenderPipelineRttiConverterContext::DeleteObject(const xiiUuid& guid)
{
  xiiRttiConverterObject object = GetObjectByGUID(guid);
  const xiiRTTI*         pRtti  = object.m_pType;
  XII_ASSERT_DEBUG(pRtti != nullptr, "Object does not exist!");
  if (pRtti->IsDerivedFrom<xiiRenderPipelinePass>())
  {
    xiiRenderPipelinePass* pPass = static_cast<xiiRenderPipelinePass*>(object.m_pObject);

    UnregisterObject(guid);
    m_pRenderPipeline->RemovePass(pPass);
  }
  else if (pRtti->IsDerivedFrom<xiiExtractor>())
  {
    xiiExtractor* pExtractor = static_cast<xiiExtractor*>(object.m_pObject);

    UnregisterObject(guid);
    m_pRenderPipeline->RemoveExtractor(pExtractor);
  }
  else
  {
    xiiRttiConverterContext::DeleteObject(guid);
  }
}

// static
xiiInternal::NewInstance<xiiRenderPipeline> xiiRenderPipelineResourceLoader::CreateRenderPipeline(const xiiRenderPipelineResourceDescriptor& desc)
{
  auto                                  pPipeline = XII_DEFAULT_NEW(xiiRenderPipeline);
  xiiRenderPipelineRttiConverterContext context;
  context.m_pRenderPipeline = pPipeline;

  xiiRawMemoryStreamReader memoryReader(desc.m_SerializedPipeline);

  xiiAbstractObjectGraph graph;
  xiiAbstractGraphBinarySerializer::Read(memoryReader, &graph);

  xiiRttiConverterReader rttiConverter(&graph, &context);

  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto     pNode = it.Value();
    xiiRTTI* pType = xiiRTTI::FindTypeByName(pNode->GetType());
    if (pType && pType->IsDerivedFrom<xiiRenderPipelinePass>())
    {
      auto pPass = rttiConverter.CreateObjectFromNode(pNode);
      if (!pPass)
      {
        xiiLog::Error("Failed to deserialize xiiRenderPipelinePass!");
      }
    }
    else if (pType && pType->IsDerivedFrom<xiiExtractor>())
    {
      auto pExtractor = rttiConverter.CreateObjectFromNode(pNode);
      if (!pExtractor)
      {
        xiiLog::Error("Failed to deserialize xiiExtractor!");
      }
    }
  }

  auto             pType = xiiGetStaticRTTI<RenderPipelineResourceLoaderConnectionInternal>();
  xiiStringBuilder tmp;

  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto*          pNode = it.Value();
    const xiiUuid& guid  = pNode->GetGuid();

    if (xiiStringUtils::IsEqual(pNode->GetNodeName(), "Connection") == false)
      continue;

    RenderPipelineResourceLoaderConnectionInternal data;
    rttiConverter.ApplyPropertiesToObject(pNode, pType, &data);

    auto objectSource = context.GetObjectByGUID(data.m_Source);
    if (objectSource.m_pObject == nullptr || !objectSource.m_pType->IsDerivedFrom<xiiRenderPipelinePass>())
    {
      xiiLog::Error("Failed to retrieve connection target '{0}' with pin '{1}'", xiiConversionUtils::ToString(guid, tmp), data.m_TargetPin);
      continue;
    }

    auto objectTarget = context.GetObjectByGUID(data.m_Target);
    if (objectTarget.m_pObject == nullptr || !objectTarget.m_pType->IsDerivedFrom<xiiRenderPipelinePass>())
    {
      xiiLog::Error("Failed to retrieve connection target '{0}' with pin '{1}'", xiiConversionUtils::ToString(guid, tmp), data.m_TargetPin);
      continue;
    }

    xiiRenderPipelinePass* pSource = static_cast<xiiRenderPipelinePass*>(objectSource.m_pObject);
    xiiRenderPipelinePass* pTarget = static_cast<xiiRenderPipelinePass*>(objectTarget.m_pObject);

    if (!pPipeline->Connect(pSource, data.m_SourcePin, pTarget, data.m_TargetPin))
    {
      xiiLog::Error("Failed to connect '{0}'::'{1}' to '{2}'::'{3}'!", pSource->GetName(), data.m_SourcePin, pTarget->GetName(), data.m_TargetPin);
    }
  }

  return pPipeline;
}

// static
void xiiRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(const xiiRenderPipeline* pPipeline, xiiRenderPipelineResourceDescriptor& ref_desc)
{
  xiiRenderPipelineRttiConverterContext context;

  xiiAbstractObjectGraph graph;

  xiiRttiConverterWriter rttiConverter(&graph, &context, false, true);

  xiiHybridArray<const xiiRenderPipelinePass*, 16> passes;
  pPipeline->GetPasses(passes);

  // Need to serialize all passes first so we have guids for each to be referenced in the connections.
  for (auto pPass : passes)
  {
    xiiUuid guid;
    guid.CreateNewUuid();
    context.RegisterObject(guid, pPass->GetDynamicRTTI(), const_cast<xiiRenderPipelinePass*>(pPass));
    rttiConverter.AddObjectToGraph(const_cast<xiiRenderPipelinePass*>(pPass));
  }
  xiiHybridArray<const xiiExtractor*, 16> extractors;
  pPipeline->GetExtractors(extractors);
  for (auto pExtractor : extractors)
  {
    xiiUuid guid;
    guid.CreateNewUuid();
    context.RegisterObject(guid, pExtractor->GetDynamicRTTI(), const_cast<xiiExtractor*>(pExtractor));
    rttiConverter.AddObjectToGraph(const_cast<xiiExtractor*>(pExtractor));
  }

  auto  pType = xiiGetStaticRTTI<RenderPipelineResourceLoaderConnectionInternal>();
  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode       = it.Value();
    auto  objectSoure = context.GetObjectByGUID(pNode->GetGuid());

    if (objectSoure.m_pObject == nullptr || !objectSoure.m_pType->IsDerivedFrom<xiiRenderPipelinePass>())
    {
      continue;
    }
    xiiRenderPipelinePass* pSource = static_cast<xiiRenderPipelinePass*>(objectSoure.m_pObject);

    RenderPipelineResourceLoaderConnectionInternal data;
    data.m_Source = pNode->GetGuid();

    auto outputs = pSource->GetOutputPins();
    for (const xiiRenderPipelineNodePin* pPinSource : outputs)
    {
      data.m_SourcePin = pSource->GetPinName(pPinSource).GetView();

      const xiiRenderPipelinePassConnection* pConnection = pPipeline->GetOutputConnection(pSource, pSource->GetPinName(pPinSource));
      if (!pConnection)
        continue;

      for (const xiiRenderPipelineNodePin* pPinTarget : pConnection->m_Inputs)
      {
        data.m_Target    = context.GetObjectGUID(pPinTarget->m_pParent->GetDynamicRTTI(), pPinTarget->m_pParent);
        data.m_TargetPin = pPinTarget->m_pParent->GetPinName(pPinTarget).GetView();

        xiiUuid connectionGuid;
        connectionGuid.CreateNewUuid();
        context.RegisterObject(connectionGuid, pType, &data);
        rttiConverter.AddObjectToGraph(pType, &data, "Connection");
      }
    }
  }

  xiiMemoryStreamContainerWrapperStorage<xiiDynamicArray<xiiUInt8>> storage(&ref_desc.m_SerializedPipeline);

  xiiMemoryStreamWriter memoryWriter(&storage);
  xiiAbstractGraphBinarySerializer::Write(memoryWriter, &graph);
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
