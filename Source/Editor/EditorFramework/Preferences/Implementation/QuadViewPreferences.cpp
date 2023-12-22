#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <Foundation/Serialization/GraphPatch.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiEngineViewPreferences, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiEngineViewPreferences>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CamPos", m_vCamPos),
    XII_MEMBER_PROPERTY("CamDir", m_vCamDir),
    XII_MEMBER_PROPERTY("CamUp", m_vCamUp),
    XII_ENUM_MEMBER_PROPERTY("Perspective", xiiSceneViewPerspective, m_PerspectiveMode),
    XII_ENUM_MEMBER_PROPERTY("RenderMode", xiiViewRenderMode, m_RenderMode),
    XII_MEMBER_PROPERTY("FOV", m_fFov),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Patch class
  class xiiSceneViewPreferencesPatch_1_2 : public xiiGraphPatch
  {
  public:
    xiiSceneViewPreferencesPatch_1_2() :
      xiiGraphPatch("xiiSceneViewPreferences", 2)
    {
    }
    virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
    {
      ref_context.RenameClass("xiiEngineViewPreferences");
    }
  };
  xiiSceneViewPreferencesPatch_1_2 g_xiiSceneViewPreferencesPatch_1_2;
} // namespace

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiQuadViewPreferencesUser, 1, xiiRTTIDefaultAllocator<xiiQuadViewPreferencesUser>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("QuadView", m_bQuadView)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ViewSingle", m_ViewSingle)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ViewQuad0", m_ViewQuad0)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ViewQuad1", m_ViewQuad1)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ViewQuad2", m_ViewQuad2)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ViewQuad3", m_ViewQuad3)->AddAttributes(new xiiHiddenAttribute()),
    XII_ARRAY_ACCESSOR_PROPERTY("FavoriteCams", FavCams_GetCount, FavCams_GetCam, FavCams_SetCam, FavCams_Insert, FavCams_Remove)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiQuadViewPreferencesUser::xiiQuadViewPreferencesUser() :
  xiiPreferences(Domain::Document, "View")
{
  m_bQuadView = false;

  m_ViewSingle.m_vCamPos.Set(-3, 0, 2);
  m_ViewSingle.m_vCamDir.Set(1, 0, -0.5f);
  m_ViewSingle.m_vCamDir.Normalize();
  m_ViewSingle.m_vCamUp          = m_ViewSingle.m_vCamDir.CrossRH(xiiVec3(0, 1, 0)).GetNormalized();
  m_ViewSingle.m_PerspectiveMode = xiiSceneViewPerspective::Perspective;
  m_ViewSingle.m_RenderMode      = xiiViewRenderMode::Default;
  m_ViewSingle.m_fFov            = 70.0f;

  // Top Left: Top Down
  m_ViewQuad0.m_vCamPos.SetZero();
  m_ViewQuad0.m_vCamDir.Set(0, 0, -1);
  m_ViewQuad0.m_vCamUp.Set(1, 0, 0);
  m_ViewQuad0.m_PerspectiveMode = xiiSceneViewPerspective::Orthogonal_Top;
  m_ViewQuad0.m_RenderMode      = xiiViewRenderMode::WireframeMonochrome;
  m_ViewQuad0.m_fFov            = 20.0f;

  // Top Right: Perspective
  m_ViewQuad1.m_vCamPos         = m_ViewSingle.m_vCamPos;
  m_ViewQuad1.m_vCamDir         = m_ViewSingle.m_vCamDir;
  m_ViewQuad1.m_vCamUp          = m_ViewSingle.m_vCamUp;
  m_ViewQuad1.m_PerspectiveMode = xiiSceneViewPerspective::Perspective;
  m_ViewQuad1.m_RenderMode      = xiiViewRenderMode::Default;
  m_ViewQuad1.m_fFov            = 70.0f;

  // Bottom Left: Front to Back
  m_ViewQuad2.m_vCamPos.SetZero();
  m_ViewQuad2.m_vCamDir.Set(-1, 0, 0);
  m_ViewQuad2.m_vCamUp.Set(0, 0, 1);
  m_ViewQuad2.m_PerspectiveMode = xiiSceneViewPerspective::Orthogonal_Front;
  m_ViewQuad2.m_RenderMode      = xiiViewRenderMode::WireframeMonochrome;
  m_ViewQuad2.m_fFov            = 20.0f;

  // Bottom Right: Right to Left
  m_ViewQuad3.m_vCamPos.SetZero();
  m_ViewQuad3.m_vCamDir.Set(0, -1, 0);
  m_ViewQuad3.m_vCamUp.Set(0, 0, 1);
  m_ViewQuad3.m_PerspectiveMode = xiiSceneViewPerspective::Orthogonal_Right;
  m_ViewQuad3.m_RenderMode      = xiiViewRenderMode::WireframeMonochrome;
  m_ViewQuad3.m_fFov            = 20.0f;
}
