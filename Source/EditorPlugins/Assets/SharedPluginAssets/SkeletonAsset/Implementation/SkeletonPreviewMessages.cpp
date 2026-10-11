/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <SharedPluginAssets/SkeletonAsset/SkeletonPreviewMessages.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineSkeletonPreviewMsg, 1, xiiRTTIDefaultAllocator<xiiEditorEngineSkeletonPreviewMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Data", m_Data),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiEditorEngineSkeletonPreviewMsg::SetSkeleton(const xiiEditableSkeleton& skeleton)
{
  m_Data.Clear();
  xiiSkeletonResourceDescriptor descriptor;
  XII_SUCCEED_OR_RETURN(skeleton.FillResourceDescriptor(descriptor));
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  xiiReflectionSerializer::WriteObjectToDDL(writer, skeleton.GetDynamicRTTI(), &skeleton);
  m_Data = xiiArrayPtr<const xiiUInt8>(storage.GetData(), storage.GetStorageSize32());
  return XII_SUCCESS;
}

xiiResult xiiEditorEngineSkeletonPreviewMsg::GetSkeleton(xiiUniquePtr<xiiEditableSkeleton>& out_skeleton) const
{
  out_skeleton.Clear();
  if (m_Data.IsEmpty())
    return XII_FAILURE;
  xiiRawMemoryStreamReader stream(m_Data.GetData(), m_Data.GetCount());
  xiiAbstractObjectGraph   graph;
  XII_SUCCEED_OR_RETURN(xiiAbstractGraphDdlSerializer::Read(stream, &graph));
  const auto* pRoot = graph.GetNodeByName("root");
  if (pRoot == nullptr || pRoot->GetType() != xiiGetStaticRTTI<xiiEditableSkeleton>()->GetTypeName())
    return XII_FAILURE;

  xiiUniquePtr<xiiEditableSkeleton> skeleton = XII_DEFAULT_NEW(xiiEditableSkeleton);
  xiiRttiConverterContext           context;
  context.RegisterObject(pRoot->GetGuid(), skeleton->GetDynamicRTTI(), skeleton.Borrow());
  xiiRttiConverterReader reader(&graph, &context);
  reader.ApplyPropertiesToObject(pRoot, skeleton->GetDynamicRTTI(), skeleton.Borrow());
  xiiSkeletonResourceDescriptor descriptor;
  XII_SUCCEED_OR_RETURN(skeleton->FillResourceDescriptor(descriptor));
  out_skeleton = std::move(skeleton);
  return XII_SUCCESS;
}
