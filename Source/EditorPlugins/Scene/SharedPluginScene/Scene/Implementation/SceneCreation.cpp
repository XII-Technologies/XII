/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <SharedPluginScene/Scene/SceneCreation.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiStatus xiiCreateDefaultScene(xiiObjectAccessorBase& ref_accessor)
{
  ref_accessor.StartTransaction("Initial Scene Setup");
  bool bFinished = false;
  XII_SCOPE_EXIT(if (!bFinished) ref_accessor.CancelTransaction());

  auto addObject = [&](const xiiDocumentObject* pParent, xiiStringView sProperty, const xiiRTTI* pType, const xiiDocumentObject*& out_pObject) -> xiiStatus {
    xiiUuid guid;
    XII_SUCCEED_OR_RETURN(ref_accessor.AddObjectByName(pParent, sProperty, -1, pType, guid));
    out_pObject = ref_accessor.GetObject(guid);
    return out_pObject != nullptr ? xiiStatus(XII_SUCCESS) : xiiStatus("The scene accessor did not return the newly created object.");
  };

  const xiiDocumentObject* pRoot         = ref_accessor.GetObjectManager()->GetRootObject();
  const xiiDocumentObject* pCameraObject = nullptr;
  const xiiDocumentObject* pCamera       = nullptr;
  XII_SUCCEED_OR_RETURN(addObject(pRoot, "Children", xiiGetStaticRTTI<xiiGameObject>(), pCameraObject));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pCameraObject, "Name", "Scene Thumbnail Camera"));
  XII_SUCCEED_OR_RETURN(addObject(pCameraObject, "Components", xiiGetStaticRTTI<xiiCameraComponent>(), pCamera));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pCamera, "EditorShortcut", 1));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pCamera, "UsageHint", static_cast<xiiInt64>(xiiCameraUsageHint::Thumbnail)));

  const xiiDocumentObject* pMeshObject = nullptr;
  const xiiDocumentObject* pMesh       = nullptr;
  XII_SUCCEED_OR_RETURN(addObject(pRoot, "Children", xiiGetStaticRTTI<xiiGameObject>(), pMeshObject));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pMeshObject, "Name", "Sphere"));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pMeshObject, "LocalPosition", xiiVec3(3, 0, 0)));
  XII_SUCCEED_OR_RETURN(addObject(pMeshObject, "Components", xiiGetStaticRTTI<xiiMeshComponent>(), pMesh));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pMesh, "Mesh", "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }"));

  const xiiDocumentObject* pSkyObject = nullptr;
  const xiiDocumentObject* pSky       = nullptr;
  XII_SUCCEED_OR_RETURN(addObject(pRoot, "Children", xiiGetStaticRTTI<xiiGameObject>(), pSkyObject));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pSkyObject, "Name", "Sky"));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pSkyObject, "LocalPosition", xiiVec3(0, 0, 1)));
  XII_SUCCEED_OR_RETURN(ref_accessor.ClearByName(pSkyObject, "Tags"));
  // Retain the established scene tag so existing view filters remain meaningful.
  XII_SUCCEED_OR_RETURN(ref_accessor.InsertValueByName(pSkyObject, "Tags", "SkyLight", -1));
  XII_SUCCEED_OR_RETURN(addObject(pSkyObject, "Components", xiiGetStaticRTTI<xiiSkyAtmosphereComponent>(), pSky));

  const xiiDocumentObject* pLightObject = nullptr;
  const xiiDocumentObject* pLight       = nullptr;
  XII_SUCCEED_OR_RETURN(addObject(pRoot, "Children", xiiGetStaticRTTI<xiiGameObject>(), pLightObject));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pLightObject, "Name", "Directional Light"));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pLightObject, "LocalPosition", xiiVec3(0, 0, 2)));
  const auto rotation = xiiQuat::MakeFromEulerAngles(xiiAngle::MakeZero(), xiiAngle::MakeFromDegree(55), xiiAngle::MakeFromDegree(90));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pLightObject, "LocalRotation", rotation));
  XII_SUCCEED_OR_RETURN(addObject(pLightObject, "Components", xiiGetStaticRTTI<xiiDirectionalLightComponent>(), pLight));
  // Reflected enum defaults belong to the base light type; author the directional quantity explicitly.
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pLight, "IntensityUnit", static_cast<xiiInt64>(xiiPhotometricUnit::Lux)));
  XII_SUCCEED_OR_RETURN(ref_accessor.SetValueByName(pLight, "Intensity", 10.0f));

  ref_accessor.FinishTransaction();
  bFinished = true;
  return xiiStatus(XII_SUCCESS);
}
