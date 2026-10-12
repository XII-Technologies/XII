/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/World/World.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <SharedPluginScene/Scene/ScenePreviewLighting.h>

namespace
{
  template <typename Component>
  Component* FindHelper(xiiWorld& world, xiiGameObjectHandle& inout_hObject)
  {
    xiiGameObject* pObject    = nullptr;
    Component*     pComponent = nullptr;
    if (world.TryGetObject(inout_hObject, pObject))
    {
      if (pObject->TryGetComponentOfBaseType(pComponent))
        return pComponent;

      // An externally removed component must not leave an orphan helper behind.
      world.DeleteObjectNow(inout_hObject, false);
    }
    inout_hObject.Invalidate();
    return nullptr;
  }

  void SetEditorTag(xiiGameObject& object, bool bSetEditorTag)
  {
    const xiiTag& tag = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
    if (bSetEditorTag)
      object.SetTag(tag);
    else
      object.RemoveTag(tag);
  }
} // namespace

xiiScenePreviewLighting::xiiScenePreviewLighting(xiiWorld& world) : m_World(world) {}

xiiScenePreviewLighting::~xiiScenePreviewLighting()
{
  Clear();
}

xiiResult xiiScenePreviewLighting::EnsureCreated(bool bSetEditorTag, bool bForce)
{
  XII_LOCK(m_World.GetWriteMarker());

  auto* pSky   = FindHelper<xiiSkyAtmosphereComponent>(m_World, m_hAtmosphere);
  auto* pLight = FindHelper<xiiDirectionalLightComponent>(m_World, m_hDirectionalLight);
  if (!bForce && m_World.GetObjectCount() == 0)
    return XII_SUCCESS;

  if (pSky == nullptr)
  {
    xiiGameObjectDescription description;
    description.m_sName.Assign("Editor Preview Sky");
    xiiGameObject* pObject = nullptr;
    m_hAtmosphere          = m_World.CreateObject(description, pObject);
    if (pObject == nullptr || xiiSkyAtmosphereComponent::CreateComponent(pObject, pSky).IsInvalidated())
    {
      Clear();
      return XII_FAILURE;
    }
    pSky->SetPriority(xiiMath::MinValue<xiiInt32>());
  }

  if (pLight == nullptr)
  {
    xiiGameObjectDescription description;
    description.m_sName.Assign("Editor Preview Light");
    description.m_LocalRotation = xiiQuat::MakeFromEulerAngles(xiiAngle::MakeFromDegree(-14.510815f), xiiAngle::MakeFromDegree(43.07951f), xiiAngle::MakeFromDegree(93.223808f));
    xiiGameObject* pObject      = nullptr;
    m_hDirectionalLight         = m_World.CreateObject(description, pObject);
    if (pObject == nullptr || xiiDirectionalLightComponent::CreateComponent(pObject, pLight).IsInvalidated())
    {
      Clear();
      return XII_FAILURE;
    }
    pLight->SetIntensityUnit(xiiPhotometricUnit::Lux);
    pLight->SetIntensity(10.0f);
  }

  SetEditorTag(*pSky->GetOwner(), bSetEditorTag);
  SetEditorTag(*pLight->GetOwner(), bSetEditorTag);
  return XII_SUCCESS;
}

void xiiScenePreviewLighting::Clear()
{
  XII_LOCK(m_World.GetWriteMarker());
  m_World.DeleteObjectNow(m_hAtmosphere, false);
  m_World.DeleteObjectNow(m_hDirectionalLight, false);
  m_hAtmosphere.Invalidate();
  m_hDirectionalLight.Invalidate();
}
