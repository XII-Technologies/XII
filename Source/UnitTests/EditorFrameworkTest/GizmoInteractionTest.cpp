/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <Foundation/Reflection/ReflectionUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(EditorViewportInteraction);
XII_CREATE_SIMPLE_TEST(EditorViewportInteraction, HandleFeedback)
{
  xiiTranslateGizmo    tool;
  xiiEngineGizmoHandle handle;
  handle.ConfigureHandle(&tool, xiiEngineGizmoHandleType::TranslationAxis, xiiColor::Red, xiiGizmoFlags::Pickable);
  const auto* type = handle.GetDynamicRTTI();
  const auto  read = [&](const char* name) {
    return xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(type->FindPropertyByName(name)), &handle);
  };
  auto transform        = xiiTransform::MakeIdentity();
  transform.m_vPosition = xiiVec3(2, 3, 4);
  tool.SetTransformation(transform);
  tool.ConfigureInteraction(&handle, nullptr, transform.m_vPosition, xiiVec2I32(800, 600));
  for (auto end : {xiiGizmoEvent::Type::EndInteractions, xiiGizmoEvent::Type::CancelInteractions})
  {
    tool.m_GizmoEvents.Broadcast({&tool, xiiGizmoEvent::Type::BeginInteractions});
    XII_TEST_INT(read("VisualState").Get<xiiInt32>(), static_cast<xiiInt32>(xiiGizmoVisualState::Active));
    XII_TEST_VEC3(read("DragOrigin").Get<xiiVec3>(), transform.m_vPosition, 0);
    handle.SetVisualParameters(1, 2);
    handle.SetInteractionText("12.50 units");
    XII_TEST_STRING(read("InteractionText").Get<xiiString>(), "12.50 units");
    tool.m_GizmoEvents.Broadcast({&tool, end});
    XII_TEST_INT(read("VisualState").Get<xiiInt32>(), static_cast<xiiInt32>(xiiGizmoVisualState::Normal));
    XII_TEST_VEC2(read("RotationArc").Get<xiiVec2>(), xiiVec2::MakeZero(), 0);
    XII_TEST_BOOL(read("InteractionText").Get<xiiString>().IsEmpty());
  }
}
