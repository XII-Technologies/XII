/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>


#include <Core/World/World.h>
#include <Foundation/Algorithm/HashingUtils.h>

#include <Foundation/Types/ScopeExit.h>



#include <SharedPluginScene/Scene/Implementation/SceneMeshPickingState.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneMeshPickingPass, 3, xiiRTTIDefaultAllocator<xiiSceneMeshPickingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Before", m_Before)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("After", m_After)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SurfaceInput", m_InputSurface)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Surface")),
    XII_MEMBER_PROPERTY("SurfaceOutput", m_OutputSurface)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Surface")),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSceneMeshPickingPass::xiiSceneMeshPickingPass()  = default;
xiiSceneMeshPickingPass::~xiiSceneMeshPickingPass() = default;



void xiiSceneMeshPickingPass::InvalidateTemporalHistory()
{
  if (m_pState)
    m_pState->Invalidate();
}

xiiResult xiiSceneMeshPickingPass::Build(xiiRenderPipelinePassContext& context)
{
  bool valid = false;
  XII_SCOPE_EXIT(if (!valid) InvalidateTemporalHistory());
  const auto surface = context.GetInputTexture(m_InputSurface);
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputSurface, surface));
  bool enabled = m_bEnabled;
  if (const auto* parameter = context.m_Parameters.GetValue("Editor.Picking"))
  {
    xiiResult  converted = XII_FAILURE;
    const bool requested = parameter->ConvertTo<bool>(&converted);
    enabled              = enabled && requested;
    XII_SUCCEED_OR_RETURN(converted);
  }
  if (!enabled)
    return XII_SUCCESS;
  if (!context.m_pView || !context.m_pView->GetGpuInstanceData())
    return context.ReportFailure("Surface capture requires world GPU identity metadata.");
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(xiiSceneMeshPickingState);
  auto&           state     = *m_pState;
  const auto      revision  = context.m_pView->GetGpuInstanceData()->GetDataRevision();
  const auto      selection = context.m_pView->GetGpuInstanceSelection();
  const xiiUInt64 signature = selection.IsEmpty() ? 0 : xiiHashingUtils::xxHash64(selection.GetPtr(), selection.GetCount() * sizeof(xiiUInt32));
  const auto      policy    = [&](xiiStringView name) {
    const auto* value = context.m_Parameters.GetValue(name);
    return value ? *value : xiiVariant();
  };
  const auto selectedObjects = policy("Editor.SelectedObjects"), pickSelected = policy("Editor.PickSelected"), transparent = policy("Editor.PickTransparent");
  if (revision != state.m_uiObservedDataRevision || signature != state.m_uiViewSelection || selectedObjects != state.m_SelectionPolicy || pickSelected != state.m_PickSelectedPolicy || transparent != state.m_TransparentPolicy)
  {
    state.Invalidate();
    state.m_uiObservedDataRevision = revision;
    state.m_uiViewSelection        = signature;
    state.m_SelectionPolicy        = selectedObjects;
    state.m_PickSelectedPolicy     = pickSelected;
    state.m_TransparentPolicy      = transparent;
  }
  XII_SUCCEED_OR_RETURN(state.Poll());
  XII_SUCCEED_OR_RETURN(state.Capture(context, surface));
  valid = true;
  return XII_SUCCESS;
}

xiiResult xiiSceneMeshPickingPass::Pick(const xiiView& view, xiiUInt32 x, xiiUInt32 y, PickingResult& out_hit) const
{
  out_hit = {};
  return m_pState ? m_pState->Pick(view, x, y, out_hit) : XII_FAILURE;
}

xiiResult xiiSceneMeshPickingPass::PickObjectsInRect(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiGameObjectHandle>& out_objects) const
{
  out_objects.Clear();
  return m_pState ? m_pState->Marquee(view, rectangle, out_objects) : XII_FAILURE;
}
