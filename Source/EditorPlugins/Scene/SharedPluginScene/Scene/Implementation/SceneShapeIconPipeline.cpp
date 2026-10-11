/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

xiiResult xiiSceneShapeIconPass::GetPipelineDescriptor(xiiRenderPipelineResourceDescriptor& descriptor)
{
  xiiRenderPipelineResource::GetDefaultDescriptor(descriptor);
  xiiUInt32 debug = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < descriptor.m_Passes.GetCount(); ++i)
    if (descriptor.m_Passes[i].m_sName == "Debug")
      debug = i;
  if (debug == xiiInvalidIndex)
    return XII_FAILURE;
  const auto icons = descriptor.AddPass("Scene shape icons", GetStaticRTTI()->GetTypeName());
  bool       color = false, order = false, depth = false;
  // Snapshot because Connect can grow the connection array. Preserve the producer's pin identity.
  const auto connections = descriptor.m_Connections;
  for (const auto& connection : connections)
  {
    if (connection.m_uiTarget != debug)
      continue;
    if (connection.m_sInput == "DisplayLinearColor" || connection.m_sInput == "Before")
    {
      for (xiiUInt32 i = 0; i < descriptor.m_Connections.GetCount(); ++i)
      {
        if (descriptor.m_Connections[i].m_uiTarget == debug && descriptor.m_Connections[i].m_sInput == connection.m_sInput)
        {
          XII_SUCCEED_OR_RETURN(descriptor.Disconnect(i));
          break;
        }
      }
      const bool isColor = connection.m_sInput == "DisplayLinearColor";
      XII_SUCCEED_OR_RETURN(descriptor.Connect(connection.m_uiSource, connection.m_sOutput, icons, isColor ? "Color" : "Before"));
      XII_SUCCEED_OR_RETURN(descriptor.Connect(icons, isColor ? "Color" : "After", debug, connection.m_sInput));
      (isColor ? color : order) = true;
    }
    else if (connection.m_sInput == "SceneDepth")
    {
      XII_SUCCEED_OR_RETURN(descriptor.Connect(connection.m_uiSource, connection.m_sOutput, icons, "SceneDepth"));
      depth = true;
    }
  }
  return color && order && depth ? XII_SUCCESS : XII_FAILURE;
}

xiiRenderPipelineResourceHandle xiiSceneShapeIconPass::GetPipeline()
{
  const xiiStringView id       = "{ XII Editor Scene Pipeline }";
  auto                existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(id);
  if (existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor descriptor;
  if (GetPipelineDescriptor(descriptor).Failed())
    return {};
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(id, std::move(descriptor));
}
