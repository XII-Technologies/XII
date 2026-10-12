/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <SharedPluginScene/Scene/SceneCreation.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiStatus xiiCreateSceneBox(xiiObjectAccessorBase& accessor, const xiiBoundingBox& bounds, const xiiUuid& material, xiiUuid& out_object)
{
  out_object = xiiUuid::MakeInvalid();
  if (!bounds.IsValid() || !bounds.m_vMin.IsValid() || !bounds.m_vMax.IsValid() || !bounds.GetCenter().IsValid() || !bounds.GetExtents().IsValid() ||
      bounds.GetExtents().x <= 0 || bounds.GetExtents().y <= 0 || bounds.GetExtents().z <= 0)
    return xiiStatus("A scene box requires finite, positive dimensions.");

  accessor.StartTransaction("Add Box");
  bool finished = false;
  XII_SCOPE_EXIT(if (!finished) accessor.CancelTransaction());
  xiiUuid objectId, meshId;
  XII_SUCCEED_OR_RETURN(accessor.AddObjectByName(accessor.GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), objectId));
  const auto* object = accessor.GetObject(objectId);
  if (!object)
    return xiiStatus("The scene accessor did not return the newly created box.");
  XII_SUCCEED_OR_RETURN(accessor.SetValueByName(object, "Name", "Box"));
  XII_SUCCEED_OR_RETURN(accessor.SetValueByName(object, "LocalPosition", bounds.GetCenter()));
  XII_SUCCEED_OR_RETURN(accessor.SetValueByName(object, "LocalScaling", bounds.GetExtents()));
  XII_SUCCEED_OR_RETURN(accessor.AddObjectByName(object, "Components", -1, xiiGetStaticRTTI<xiiMeshComponent>(), meshId));
  const auto* mesh = accessor.GetObject(meshId);
  if (!mesh)
    return xiiStatus("The scene accessor did not return the newly created mesh component.");
  XII_SUCCEED_OR_RETURN(accessor.SetValueByName(mesh, "Mesh", "{ 9a013e54-324d-47d0-899c-1ec97551bf06 }"));
  XII_SUCCEED_OR_RETURN(accessor.SetValueByName(mesh, "SectionIndex", xiiUInt32(0)));
  if (material.IsValid())
  {
    xiiStringBuilder id;
    XII_SUCCEED_OR_RETURN(accessor.InsertValueByName(mesh, "MaterialOverrides", xiiConversionUtils::ToString(material, id).GetView(), -1));
  }
  accessor.FinishTransaction();
  finished   = true;
  out_object = objectId;
  return XII_SUCCESS;
}
