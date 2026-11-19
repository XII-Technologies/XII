/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginMCP/EditorMCP.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Reflection.h>
#include <ToolsFoundation/Project/ToolsProject.h>

namespace
{
  template <typename Enum>
  xiiString ViewportEnumName(typename Enum::Enum value)
  {
    xiiStringBuilder sName;
    return xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<Enum>(), value, sName) ? xiiString(sName) : xiiString();
  }

  xiiVariantDictionary DescribeViewport(const xiiQtEngineViewWidget& view, bool bFocused, bool bIncludeSettings)
  {
    xiiVariantDictionary result;
    result["id"]          = view.GetViewID();
    result["focused"]     = bFocused;
    result["visible"]     = view.isVisible();
    result["width"]       = view.GetRenderResolution().width;
    result["height"]      = view.GetRenderResolution().height;
    result["renderError"] = xiiString(view.GetRenderConfigurationError());

    const auto& config        = *view.m_pViewConfig;
    result["renderMode"]      = ViewportEnumName<xiiViewRenderMode>(config.m_RenderMode);
    result["perspective"]     = ViewportEnumName<xiiSceneViewPerspective>(config.m_Perspective);
    result["frameStatistics"] = config.m_bShowFrameStats;
    result["verticalSync"]    = config.m_bVSync;

    xiiVariantDictionary camera;
    camera["mode"]     = ViewportEnumName<xiiCameraMode>(config.m_Camera.GetCameraMode());
    camera["near"]     = config.m_Camera.GetNearPlane();
    camera["far"]      = config.m_Camera.GetFarPlane();
    camera["fovOrDim"] = config.m_Camera.GetFovOrDim();
    XII_VERIFY(xiiMCPReflection::ConvertValue(config.m_Camera.GetCenterPosition(), camera["position"]), "Camera position must be JSON-compatible.");
    XII_VERIFY(xiiMCPReflection::ConvertValue(config.m_Camera.GetCenterDirForwards(), camera["forward"]), "Camera direction must be JSON-compatible.");
    XII_VERIFY(xiiMCPReflection::ConvertValue(config.m_Camera.GetCenterDirUp(), camera["up"]), "Camera up vector must be JSON-compatible.");
    result["camera"] = std::move(camera);

    if (bIncludeSettings)
    {
      const auto&     effective = view.GetEffectiveRenderConfiguration();
      xiiVariantArray settings;
      for (const auto* pSubsystem : effective.m_Subsystems)
      {
        if (pSubsystem == nullptr)
        {
          continue;
        }
        if (settings.GetCount() == 16)
        {
          result["settingsTruncated"] = true;
          break;
        }
        const auto&          type = *pSubsystem->GetDynamicRTTI();
        xiiVariantDictionary row;
        row["type"]       = xiiString(type.GetTypeName());
        row["properties"] = xiiMCPReflection::ReadMembers(type, pSubsystem);
        settings.PushBack(std::move(row));
      }
      result["renderSettings"] = std::move(settings);
    }
    return result;
  }
} // namespace

xiiStatus xiiEditorMCP::Viewport(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  const auto* pViewArg = arguments.GetValue("view");
  xiiUInt64   uiView   = 0;
  if (pViewArg != nullptr && (!xiiMCPJson::ReadUInt(*pViewArg, uiView) || uiView > xiiMath::MaxValue<xiiUInt32>()))
  {
    return xiiStatus("Use a view ID returned by editor.viewport.");
  }

  const auto* pContainer = xiiQtContainerWindow::GetContainerWindow();
  auto*       pActive    = pContainer != nullptr ? pContainer->GetFocusedDocumentWindow() : nullptr;
  auto*       pDocument  = arguments.Contains("document") ? FindDocument(arguments) : (pActive != nullptr ? pActive->GetDocument() : nullptr);
  if (pDocument == nullptr || !xiiToolsProject::IsProjectOpen() || !xiiToolsProject::GetSingleton()->IsDocumentInAllowedRoot(pDocument->GetDocumentPath()))
  {
    return xiiStatus("Select an open project document or focus one in the Editor.");
  }

  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
  auto* pWindow        = xiiQtDocumentWindow::FindWindowByDocument(pDocument);
  out_result["active"] = pWindow != nullptr && pWindow == pActive;
  xiiVariantArray views;
  if (const auto* pEngineWindow = qobject_cast<const xiiQtEngineDocumentWindow*>(pWindow))
  {
    const auto* pFocused = pEngineWindow->GetFocusedViewWidget();
    for (const auto* pView : pEngineWindow->GetViewWidgets())
    {
      if (pView != nullptr && pView->m_pViewConfig != nullptr && (pViewArg == nullptr || pView->GetViewID() == uiView))
      {
        views.PushBack(DescribeViewport(*pView, pView == pFocused, pViewArg != nullptr));
      }
    }
  }
  if (pViewArg != nullptr && views.IsEmpty())
  {
    return xiiStatus("The requested view no longer belongs to this document.");
  }
  out_result["views"] = std::move(views);
  return XII_SUCCESS;
}

xiiResult xiiEditorMCP::RegisterViewport()
{
  return m_Registry.RegisterTool("editor.viewport", "Inspect the active or selected document's viewports, camera, resolution and render mode. Select a view ID to read its effective render settings.",
                                 R"({"type":"object","properties":{"document":{"type":"string"},"view":{"type":"integer","minimum":0}}})",
                                 xiiMCPPermission::Read, xiiMakeDelegate(&xiiEditorMCP::Viewport, this));
}
