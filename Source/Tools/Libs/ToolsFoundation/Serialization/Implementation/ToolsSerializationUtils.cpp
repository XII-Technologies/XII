#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

void xiiToolsSerializationUtils::SerializeTypes(const xiiSet<const xiiRTTI*>& types, xiiAbstractObjectGraph& ref_typesGraph)
{
  xiiRttiConverterContext context;
  xiiRttiConverterWriter  rttiConverter(&ref_typesGraph, &context, true, true);
  for (const xiiRTTI* pType : types)
  {
    xiiReflectedTypeDescriptor desc;
    if (pType->GetTypeFlags().IsSet(xiiTypeFlags::Phantom))
    {
      xiiToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pType, desc);
    }
    else
    {
      xiiToolsReflectionUtils::GetMinimalReflectedTypeDescriptorFromRtti(pType, desc);
    }

    context.RegisterObject(xiiUuid::StableUuidForString(pType->GetTypeName()), xiiGetStaticRTTI<xiiReflectedTypeDescriptor>(), &desc);
    rttiConverter.AddObjectToGraph(xiiGetStaticRTTI<xiiReflectedTypeDescriptor>(), &desc);
  }
}

void xiiToolsSerializationUtils::CopyProperties(const xiiDocumentObject* pSource, const xiiDocumentObjectManager* pSourceManager, void* pTarget, const xiiRTTI* pTargetType, FilterFunction propertFilter)
{
  xiiAbstractObjectGraph           graph;
  xiiDocumentObjectConverterWriter writer(&graph, pSourceManager, [](const xiiDocumentObject*, const xiiAbstractProperty* p) { return p->GetAttributeByType<xiiHiddenAttribute>() == nullptr; });
  xiiAbstractObjectNode*           pAbstractObj = writer.AddObjectToGraph(pSource);

  xiiRttiConverterContext context;
  xiiRttiConverterReader  reader(&graph, &context);

  reader.ApplyPropertiesToObject(pAbstractObj, pTargetType, pTarget);
}
