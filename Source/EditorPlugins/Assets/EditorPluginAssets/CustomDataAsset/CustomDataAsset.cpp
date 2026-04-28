/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCustomDataAssetProperties, 1, xiiRTTIDefaultAllocator<xiiCustomDataAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_pType)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCustomDataAssetDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCustomDataAssetDocument::xiiCustomDataAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiCustomDataAssetProperties>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiCustomDataAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  xiiAbstractObjectGraph           abstractObjectGraph;
  xiiDocumentObjectConverterWriter objectWriter(&abstractObjectGraph, GetObjectManager());

  xiiDocumentObject* pObject = GetPropertyObject();

  xiiVariant type = pObject->GetTypeAccessor().GetValue("Type");
  XII_ASSERT_DEV(type.IsA<xiiUuid>(), "Implementation error");

  if (xiiDocumentObject* pDataObject = pObject->GetChild(type.Get<xiiUuid>()))
  {
    xiiAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(pDataObject, "root");
  }

  xiiAbstractGraphBinarySerializer::Write(stream, &abstractObjectGraph);
  return XII_SUCCESS;
}

void xiiCustomDataAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const xiiDocumentObject* pObject = GetPropertyObject();

  const xiiUuid typeGuid = GetObjectAccessor()->GetByName<xiiUuid>(pObject, "Type");
  if (const xiiDocumentObject* pDataObject = GetObjectAccessor()->GetObject(typeGuid))
  {
    const xiiRTTI* pRtti = pDataObject->GetType();

    xiiStringBuilder tags(";");

    while (pRtti && pRtti != xiiGetStaticRTTI<xiiCustomData>())
    {
      tags.Append(pRtti->GetTypeName(), ";");

      pRtti = pRtti->GetParentType();
    }

    pInfo->m_sAssetsDocumentTags = tags;
  }
}
