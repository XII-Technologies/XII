/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorPluginScene/Scene/SceneComponentPropertyMetaState.h>
#include <EditorPluginScene/Visualizers/ReflectionCaptureVisualizerAdapter.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Components/Lights/ReflectionCaptureComponent.h>
#include <GraphicsCore/Components/Lights/SpotLightComponent.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  class ReflectionAuthoringDocument : public xiiAssetDocument
  {
  public:
    ReflectionAuthoringDocument() : xiiAssetDocument("ReflectionAuthoringTest.xiiScene", XII_DEFAULT_NEW(xiiDocumentObjectManager), xiiAssetDocEngineConnection::None)
    {
      m_pDocumentInfo = CreateDocumentInfo();
      InitializeAfterLoading(false);
    }

    xiiTransformStatus InternalTransformAsset(xiiStreamWriter&, xiiStringView, const xiiPlatformProfile*, const xiiAssetFileHeader&, xiiBitflags<xiiTransformFlags>) override
    {
      return xiiTransformStatus("This test document does not export assets.");
    }

    xiiUInt32 GetSyncObjectCount() const { return m_SyncObjects.GetCount(); }
  };

  class TestReflectionVisualizer : public xiiReflectionCaptureVisualizerAdapter
  {
  public:
    const xiiEngineGizmoHandle& Box() const { return m_hBox; }
    const xiiEngineGizmoHandle& InnerBox() const { return m_hInnerBox; }
    const xiiEngineGizmoHandle& Sphere() const { return m_hSphere; }
    const xiiEngineGizmoHandle& InnerSphere() const { return m_hInnerSphere; }
    void                        SetPose(const xiiTransform& transform)
    {
      m_Transform = transform;
      UpdateGizmoTransform();
    }
    void SetHidden(bool hidden)
    {
      m_bVisualizerIsVisible = !hidden;
      Update();
    }

  protected:
    xiiTransform GetObjectTransform() const override { return m_Transform; }
    xiiTransform m_Transform = xiiTransform::MakeIdentity();
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorDocuments, ReflectionCaptureAuthoring)
{
  xiiEditorEngineProcessConnection connection;
  ReflectionAuthoringDocument      document;
  auto*                            accessor = document.GetObjectAccessor();
  accessor->StartTransaction("Reflection capture authoring");
  xiiUuid ownerGuid, probeGuid, cameraGuid;
  XII_TEST_STATUS(accessor->AddObjectByName(document.GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), ownerGuid));
  const auto* owner = accessor->GetObject(ownerGuid);
  XII_TEST_STATUS(accessor->AddObjectByName(owner, "Components", -1, xiiGetStaticRTTI<xiiReflectionCaptureComponent>(), probeGuid));
  XII_TEST_STATUS(accessor->AddObjectByName(owner, "Components", -1, xiiGetStaticRTTI<xiiCameraComponent>(), cameraGuid));
  const auto* probe  = accessor->GetObject(probeGuid);
  const auto* camera = accessor->GetObject(cameraGuid);
  accessor->FinishTransaction();

  const auto* attr = xiiGetStaticRTTI<xiiReflectionCaptureComponent>()->GetAttributeByType<xiiReflectionCaptureVisualizerAttribute>();
  if (!XII_TEST_BOOL(attr != nullptr && probe != nullptr && camera != nullptr))
    return;
  XII_TEST_INT(document.GetSyncObjectCount(), 0);
  {
    TestReflectionVisualizer visualizer;
    visualizer.SetVisualizer(attr, probe);
    XII_TEST_INT(document.GetSyncObjectCount(), 4);

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Box extents and full-weight blend boundary match native local dimensions")
    {
      accessor->StartTransaction("Box volume");
      XII_TEST_STATUS(accessor->SetValueByName(probe, "HalfExtents", xiiVec3(4, 5, 6)));
      XII_TEST_STATUS(accessor->SetValueByName(probe, "BlendDistance", 1.5f));
      accessor->FinishTransaction();
      XII_TEST_BOOL(visualizer.Box().IsVisible() && visualizer.InnerBox().IsVisible());
      XII_TEST_BOOL(!visualizer.Sphere().IsVisible() && !visualizer.InnerSphere().IsVisible());
      XII_TEST_VEC3(visualizer.Box().GetTransformation().m_vScale, xiiVec3(8, 10, 12), 0);
      XII_TEST_VEC3(visualizer.InnerBox().GetTransformation().m_vScale, xiiVec3(5, 7, 9), 0);
      auto pose        = xiiTransform::MakeIdentity();
      pose.m_vPosition = xiiVec3(3, 2, 1);
      pose.m_vScale    = xiiVec3(2, 3, 4);
      pose.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(35));
      visualizer.SetPose(pose);
      XII_TEST_VEC3(visualizer.Box().GetTransformation().m_vScale, xiiVec3(16, 30, 48), 0);
      XII_TEST_VEC3(visualizer.InnerBox().GetTransformation().m_vScale, xiiVec3(10, 21, 36), 0);
      XII_TEST_VEC3(visualizer.Box().GetTransformation().m_vPosition, pose.m_vPosition, 0);
      XII_TEST_BOOL(visualizer.Box().GetTransformation().m_qRotation.IsEqualRotation(pose.m_qRotation, xiiMath::DefaultEpsilon<float>()));
      visualizer.SetPose(xiiTransform::MakeIdentity());
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shape changes, undo and inward sphere blend")
    {
      accessor->StartTransaction("Sphere volume");
      XII_TEST_STATUS(accessor->SetValueByName(probe, "InfluenceShape", static_cast<xiiInt64>(xiiReflectionProbeInfluenceShape::Sphere)));
      XII_TEST_STATUS(accessor->SetValueByName(probe, "SphereRadius", 7.0f));
      accessor->FinishTransaction();
      XII_TEST_BOOL(!visualizer.Box().IsVisible() && !visualizer.InnerBox().IsVisible());
      XII_TEST_BOOL(visualizer.Sphere().IsVisible() && visualizer.InnerSphere().IsVisible());
      XII_TEST_VEC3(visualizer.Sphere().GetTransformation().m_vScale, xiiVec3(7), 0);
      XII_TEST_VEC3(visualizer.InnerSphere().GetTransformation().m_vScale, xiiVec3(5.5f), 0);
      XII_TEST_STATUS(document.GetCommandHistory()->Undo());
      XII_TEST_BOOL(visualizer.Box().IsVisible() && !visualizer.Sphere().IsVisible());
      XII_TEST_STATUS(document.GetCommandHistory()->Redo());
      XII_TEST_BOOL(visualizer.Sphere().IsVisible() && !visualizer.Box().IsVisible());
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Degenerate blend boundaries, hidden state and invalid settings")
    {
      auto setBlend = [&](float value) {
        accessor->StartTransaction("Blend distance");
        XII_TEST_STATUS(accessor->SetValueByName(probe, "BlendDistance", value));
        accessor->FinishTransaction();
      };
      setBlend(0);
      XII_TEST_BOOL(visualizer.Sphere().IsVisible() && !visualizer.InnerSphere().IsVisible());
      setBlend(7);
      XII_TEST_BOOL(visualizer.Sphere().IsVisible() && !visualizer.InnerSphere().IsVisible());
      setBlend(1);
      visualizer.SetHidden(true);
      XII_TEST_BOOL(!visualizer.Sphere().IsVisible() && !visualizer.InnerSphere().IsVisible());
      visualizer.SetHidden(false);
      XII_TEST_BOOL(visualizer.Sphere().IsVisible() && visualizer.InnerSphere().IsVisible());
      setBlend(xiiMath::NaN<float>());
      XII_TEST_BOOL(!visualizer.Sphere().IsVisible() && !visualizer.InnerSphere().IsVisible());
      setBlend(1);
      XII_TEST_BOOL(visualizer.Sphere().IsVisible() && visualizer.InnerSphere().IsVisible());
      accessor->StartTransaction("Unused box dimensions");
      XII_TEST_STATUS(accessor->SetValueByName(probe, "HalfExtents", xiiVec3(xiiMath::NaN<float>())));
      accessor->FinishTransaction();
      XII_TEST_BOOL(visualizer.Sphere().IsVisible());
      accessor->StartTransaction("Unused sphere dimensions");
      XII_TEST_STATUS(accessor->SetValueByName(probe, "HalfExtents", xiiVec3(4, 5, 6)));
      XII_TEST_STATUS(accessor->SetValueByName(probe, "InfluenceShape", static_cast<xiiInt64>(xiiReflectionProbeInfluenceShape::Box)));
      XII_TEST_STATUS(accessor->SetValueByName(probe, "SphereRadius", xiiMath::NaN<float>()));
      accessor->FinishTransaction();
      XII_TEST_BOOL(visualizer.Box().IsVisible());
      accessor->StartTransaction("Unknown influence shape");
      XII_TEST_STATUS(accessor->SetValueByName(probe, "InfluenceShape", 42));
      accessor->FinishTransaction();
      XII_TEST_BOOL(!visualizer.Box().IsVisible() && !visualizer.Sphere().IsVisible());
      accessor->StartTransaction("Restore sphere");
      XII_TEST_STATUS(accessor->SetValueByName(probe, "SphereRadius", 7.0f));
      XII_TEST_STATUS(accessor->SetValueByName(probe, "InfluenceShape", static_cast<xiiInt64>(xiiReflectionProbeInfluenceShape::Sphere)));
      accessor->FinishTransaction();
    }
  }
  XII_TEST_INT(document.GetSyncObjectCount(), 0);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native shape and camera usage property visibility")
  {
    xiiMap<xiiString, xiiPropertyUiState> states;
    xiiPropertyMetaStateEvent             event;
    event.m_pPropertyStates = &states;
    event.m_pObject         = probe;
    xiiReflectionCaptureComponent_PropertyMetaStateEventHandler(event);
    XII_TEST_BOOL(states["SphereRadius"].m_Visibility == xiiPropertyUiState::Default);
    XII_TEST_BOOL(states["HalfExtents"].m_Visibility == xiiPropertyUiState::Invisible);
    XII_TEST_BOOL(states["ParallaxCorrected"].m_Visibility == xiiPropertyUiState::Invisible);
    accessor->StartTransaction("Box influence");
    XII_TEST_STATUS(accessor->SetValueByName(probe, "InfluenceShape", static_cast<xiiInt64>(xiiReflectionProbeInfluenceShape::Box)));
    accessor->FinishTransaction();
    xiiReflectionCaptureComponent_PropertyMetaStateEventHandler(event);
    XII_TEST_BOOL(states["SphereRadius"].m_Visibility == xiiPropertyUiState::Invisible);
    XII_TEST_BOOL(states["HalfExtents"].m_Visibility == xiiPropertyUiState::Default);
    XII_TEST_BOOL(states["ParallaxCorrected"].m_Visibility == xiiPropertyUiState::Default);

    event.m_pObject = camera;
    for (auto usage : {xiiCameraUsageHint::CinematicView, xiiCameraUsageHint::RenderTarget, xiiCameraUsageHint::None})
    {
      accessor->StartTransaction("Camera usage");
      XII_TEST_STATUS(accessor->SetValueByName(camera, "UsageHint", static_cast<xiiInt64>(usage)));
      accessor->FinishTransaction();
      states.Clear();
      xiiCameraComponent_PropertyMetaStateEventHandler(event);
      const auto expected = usage == xiiCameraUsageHint::RenderTarget ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
      XII_TEST_BOOL(states["RenderTargetOffset"].m_Visibility == expected);
      XII_TEST_BOOL(states["RenderTargetSize"].m_Visibility == expected);
      XII_TEST_BOOL(!states.Contains("RenderTarget")); // The native component has no such property.
    }

    for (const auto* type : {xiiGetStaticRTTI<xiiPointLightComponent>(), xiiGetStaticRTTI<xiiSpotLightComponent>(), xiiGetStaticRTTI<xiiDirectionalLightComponent>()})
    {
      accessor->StartTransaction("Native light");
      xiiUuid lightGuid;
      XII_TEST_STATUS(accessor->AddObjectByName(owner, "Components", -1, type, lightGuid));
      accessor->FinishTransaction();
      event.m_pObject = accessor->GetObject(lightGuid);
      for (bool shadows : {false, true})
      {
        accessor->StartTransaction("Light shadows");
        XII_TEST_STATUS(accessor->SetValueByName(event.m_pObject, "CastShadows", shadows));
        accessor->FinishTransaction();
        states.Clear();
        xiiLightComponent_PropertyMetaStateEventHandler(event);
        if (type->FindPropertyByName("ShadowFadeOutRange") != nullptr)
        {
          XII_TEST_INT(states.GetCount(), 1);
          XII_TEST_BOOL(states["ShadowFadeOutRange"].m_Visibility == (shadows ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible));
        }
        else
        {
          XII_TEST_BOOL(states.IsEmpty());
        }
      }
    }
  }
}
