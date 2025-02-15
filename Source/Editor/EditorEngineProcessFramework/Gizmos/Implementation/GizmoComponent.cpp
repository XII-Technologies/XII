#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>

xiiGizmoComponentManager::xiiGizmoComponentManager(xiiWorld* pWorld) :
  xiiComponentManager(pWorld)
{
}

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGizmoRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiGizmoComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiHiddenAttribute(),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiGizmoComponent::xiiGizmoComponent()  = default;
xiiGizmoComponent::~xiiGizmoComponent() = default;

xiiMeshRenderData* xiiGizmoComponent::CreateRenderData() const
{
  xiiColor color = m_GizmoColor;

  auto pManager = static_cast<const xiiGizmoComponentManager*>(GetOwningManager());
  if (GetUniqueID() == pManager->m_uiHighlightID)
  {
    color = xiiColor(0.9f, 0.9f, 0.1f, color.a);
  }

  xiiGizmoRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiGizmoRenderData>(GetOwner());
  pRenderData->m_GizmoColor       = color;
  pRenderData->m_bIsPickable      = m_bIsPickable;

  return pRenderData;
}

xiiResult xiiGizmoComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  xiiResult r = SUPER::GetLocalBounds(bounds, bAlwaysVisible, msg);

  // Adjust the bounds to be mirrored and pretty large, to combat the constant size and face camera modes that are implemented by the shader
  // bounds.m_vBoxHalfExtents = (bounds.m_vCenter + bounds.m_vBoxHalfExtents) * 3.0f;
  // bounds.m_vCenter.SetZero();
  // bounds.m_fSphereRadius = xiiMath::Max(bounds.m_vBoxHalfExtents.x, bounds.m_vBoxHalfExtents.y, bounds.m_vBoxHalfExtents.z);

  // since there is always only a single gizmo on screen, there's no harm in making it always visible
  bAlwaysVisible = true;
  return r;
}
