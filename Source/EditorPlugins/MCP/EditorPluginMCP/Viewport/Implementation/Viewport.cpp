/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginMCP/EditorMCP.h>
#include <EditorPluginMCP/EditorMCPArguments.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Reflection.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <QByteArray>

namespace
{
  constexpr xiiUInt32     MaximumRetainedCaptures = 8;
  const xiiTime           CaptureTimeout          = xiiTime::MakeFromSeconds(8);
  const xiiTime           CaptureRetention        = xiiTime::MakeFromSeconds(30);
  constexpr xiiStringView CaptureUriPrefix        = "xii-editor://viewport-captures/";

  bool IsViewportRenderModeAvailable(xiiViewRenderMode::Enum mode)
  {
    const auto* pEngine = xiiEditorEngineProcessConnection::GetSingleton();
    return xiiViewRenderMode::IsSupportedVisualizationMode(mode) &&
      (!xiiViewRenderMode::IsRayTracingVisualizationMode(mode) || (pEngine != nullptr && pEngine->SupportsRayTracingModes()));
  }

  bool ReadViewportVector(const xiiVariantDictionary& object, xiiStringView sKey, xiiVec3& out_vValue)
  {
    const auto* pValue = object.GetValue(sKey);
    float       values[3];
    if (pValue == nullptr || !xiiMCPReflection::ReadFloatArray(*pValue, values))
    {
      return false;
    }
    out_vValue.Set(values[0], values[1], values[2]);
    return true;
  }

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
      xiiDynamicArray<xiiReflectionUtils::EnumKeyValuePair> entries;
      xiiReflectionUtils::GetEnumKeysAndValues(xiiGetStaticRTTI<xiiViewRenderMode>(), entries);
      xiiVariantArray modes;
      for (const auto& entry : entries)
      {
        if (IsViewportRenderModeAvailable(static_cast<xiiViewRenderMode::Enum>(entry.m_iValue)))
        {
          modes.PushBack(entry.m_sKey);
        }
      }
      result["availableRenderModes"] = std::move(modes);

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

xiiStatus xiiEditorMCP::ConfigureViewport(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  using xiiEditorMCPArguments::ReadUnsigned;

  xiiDocument* pDocument = nullptr;
  XII_SUCCEED_OR_RETURN(ValidateEdit(arguments, pDocument));
  xiiUInt64 uiView = 0;
  if (!ReadUnsigned(arguments, "view", uiView) || uiView > xiiMath::MaxValue<xiiUInt32>())
  {
    return xiiStatus("Use a view ID returned by editor.viewport.");
  }

  auto* pWindow = qobject_cast<xiiQtEngineDocumentWindow*>(xiiQtDocumentWindow::FindWindowByDocument(pDocument));
  auto* pView   = pWindow != nullptr ? pWindow->GetViewWidgetByID(static_cast<xiiUInt32>(uiView)) : nullptr;
  if (pView == nullptr || pView->m_pViewConfig == nullptr)
  {
    return xiiStatus("The requested view no longer belongs to this document.");
  }

  const auto* pMode  = arguments.GetValue("renderMode");
  const auto* pStats = arguments.GetValue("frameStatistics");
  const auto* pVSync = arguments.GetValue("verticalSync");
  const auto* pPose  = arguments.GetValue("camera");
  if (pMode == nullptr && pStats == nullptr && pVSync == nullptr && pPose == nullptr)
  {
    return xiiStatus("Provide a render mode, frame statistics, vertical sync or a camera pose.");
  }
  if ((pStats != nullptr && !pStats->IsA<bool>()) || (pVSync != nullptr && !pVSync->IsA<bool>()))
  {
    return xiiStatus("Frame statistics and vertical sync must be booleans.");
  }

  auto& config = *pView->m_pViewConfig;
  auto  mode   = config.m_RenderMode;
  if (pMode != nullptr)
  {
    xiiInt64 iMode = 0;
    if (!pMode->IsA<xiiString>() || !xiiReflectionUtils::StringToEnumeration(xiiGetStaticRTTI<xiiViewRenderMode>(), pMode->Get<xiiString>(), iMode) ||
        !IsViewportRenderModeAvailable(static_cast<xiiViewRenderMode::Enum>(iMode)))
    {
      return xiiStatus("Choose a render mode from the view's availableRenderModes.");
    }
    mode = static_cast<xiiViewRenderMode::Enum>(iMode);
  }

  xiiVec3 vPosition, vTarget, vUp;
  if (pPose != nullptr)
  {
    if (!config.m_Camera.IsPerspective() || config.m_Camera.IsStereoscopic())
    {
      return xiiStatus("Camera poses require a monoscopic perspective viewport.");
    }
    if (const auto* pInput = xiiEditorInputContext::GetActiveInputContext(); pInput != nullptr && pInput->GetOwnerView() == pView)
    {
      return xiiStatus("Wait for the current viewport interaction to finish before moving its camera.");
    }
    const auto* pCamera = xiiMCPJson::GetObject(arguments, "camera");
    if (pCamera == nullptr || pCamera->GetCount() != 3 || !ReadViewportVector(*pCamera, "position", vPosition) ||
        !ReadViewportVector(*pCamera, "target", vTarget) || !ReadViewportVector(*pCamera, "up", vUp))
    {
      return xiiStatus("Provide camera position, target and up as three-number arrays.");
    }
    auto vForward = vTarget - vPosition;
    if (vForward.NormalizeIfNotZero().Failed() || vUp.NormalizeIfNotZero().Failed() || xiiMath::Abs(vForward.Dot(vUp)) > 0.9999f)
    {
      return xiiStatus("Camera target must differ from position, and up must be nonzero and independent of the viewing direction.");
    }
  }

  if (context.HasExpired())
  {
    return xiiStatus("The viewport configuration request expired.");
  }
  config.m_RenderMode = mode;
  if (pStats != nullptr)
  {
    config.m_bShowFrameStats = pStats->Get<bool>();
  }
  if (pVSync != nullptr)
  {
    config.m_bVSync = pVSync->Get<bool>();
  }
  if (pPose != nullptr)
  {
    pView->CancelCameraInterpolation();
    config.m_Camera.LookAt(vPosition, vTarget, vUp);
  }
  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
  out_result["view"] = DescribeViewport(*pView, pWindow->GetFocusedViewWidget() == pView, false);
  return XII_SUCCESS;
}

bool xiiEditorMCP::IsViewportCaptureAuthorized() const
{
  return m_bAllowViewportCapture && xiiToolsProject::IsProjectOpen() &&
    xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID == m_AllowedProject;
}

const xiiEditorMCP::ViewportCapture* xiiEditorMCP::FindViewportCapture(xiiStringView sKey) const
{
  if (!IsViewportCaptureAuthorized() || sKey.GetElementCount() != 36)
  {
    return nullptr;
  }
  const xiiStringBuilder sUuid("{ ", sKey, " }");
  if (!xiiConversionUtils::IsStringUuid(sUuid))
  {
    return nullptr;
  }
  const auto* pCapture = m_ViewportCaptures.GetValue(xiiConversionUtils::ConvertStringToUuid(sUuid));
  return pCapture != nullptr && xiiTime::Now() - pCapture->m_Started < CaptureRetention ? pCapture : nullptr;
}

void xiiEditorMCP::UpdateViewportCaptures()
{
  if (m_ViewportCaptures.IsEmpty())
  {
    return;
  }
  if (!IsViewportCaptureAuthorized())
  {
    m_ViewportCaptures.Clear();
    return;
  }

  const auto  now     = xiiTime::Now();
  const auto* pEngine = xiiEditorEngineProcessConnection::GetSingleton();
  for (auto it = m_ViewportCaptures.GetIterator(); it.IsValid();)
  {
    auto&      capture = it.Value();
    const auto elapsed = now - capture.m_Started;
    if (elapsed >= CaptureRetention)
    {
      it = m_ViewportCaptures.Remove(it);
      continue;
    }
    if (capture.IsPending())
    {
      if (pEngine == nullptr || pEngine->IsProcessCrashed() || !pEngine->IsEngineSetup())
      {
        capture.m_sFailure = "The Editor engine process disconnected during capture.";
      }
      else if (elapsed >= CaptureTimeout)
      {
        capture.m_sFailure = "No rendered viewport capture arrived before the deadline.";
      }
    }
    ++it;
  }
}

void xiiEditorMCP::OnViewportCapture(const xiiViewportCaptureMsgToEditor& message)
{
  auto* pCapture = m_ViewportCaptures.GetValue(message.m_RequestId);
  if (pCapture == nullptr || !pCapture->IsPending() || !IsViewportCaptureAuthorized() || xiiTime::Now() - pCapture->m_Started >= CaptureTimeout)
  {
    return;
  }
  if (!message.m_sFailure.IsEmpty())
  {
    pCapture->m_sFailure = message.m_sFailure;
    return;
  }
  if (message.m_Png.IsEmpty() || message.m_Png.GetCount() > xiiViewportCaptureMsgToEditor::MaximumPngBytes || message.m_uiWidth == 0 || message.m_uiHeight == 0)
  {
    pCapture->m_sFailure = "The Editor engine process returned an invalid viewport capture.";
    return;
  }
  const auto blob      = QByteArray::fromRawData(reinterpret_cast<const char*>(message.m_Png.GetData()), message.m_Png.GetCount()).toBase64();
  pCapture->m_sBlob    = xiiStringView(blob.constData(), static_cast<xiiUInt32>(blob.size()));
  pCapture->m_uiWidth  = message.m_uiWidth;
  pCapture->m_uiHeight = message.m_uiHeight;
}

xiiStatus xiiEditorMCP::CaptureViewport(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  using namespace xiiEditorMCPArguments;

  xiiUInt64 uiGeneration = 0;
  auto*     pDocument    = FindDocument(arguments);
  if (!IsViewportCaptureAuthorized() || pDocument == nullptr || !ReadUnsigned(arguments, "generation", uiGeneration) ||
      !xiiToolsProject::IsProjectGenerationCurrent(uiGeneration) || ReadGuid(arguments, "session") != GetDocumentSession(*pDocument))
  {
    return xiiStatus("Viewport capture requires an authorized project and the current document session and project generation.");
  }
  xiiUInt64 uiView = 0;
  if (!ReadUnsigned(arguments, "view", uiView) || uiView > xiiMath::MaxValue<xiiUInt32>())
  {
    return xiiStatus("Use a view ID returned by editor.viewport.");
  }
  auto* pWindow = qobject_cast<xiiQtEngineDocumentWindow*>(xiiQtDocumentWindow::FindWindowByDocument(pDocument));
  auto* pView   = pWindow != nullptr ? pWindow->GetViewWidgetByID(static_cast<xiiUInt32>(uiView)) : nullptr;
  if (pView == nullptr || !pView->isVisible() || !pView->GetRenderResolution().HasNonZeroArea())
  {
    return xiiStatus("Choose a visible viewport that has presented a frame.");
  }
  auto* pEngine = xiiEditorEngineProcessConnection::GetSingleton();
  if (pEngine == nullptr || !pEngine->IsEngineSetup() || pEngine->IsProcessCrashed() || pWindow->GetDocument()->GetEngineStatus() != xiiAssetDocument::EngineStatus::Loaded)
  {
    return xiiStatus("The document's engine view is not ready for capture.");
  }

  auto oldest = m_ViewportCaptures.GetIterator();
  for (auto it = m_ViewportCaptures.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().IsPending() && it.Value().m_uiView == uiView)
    {
      return xiiStatus("This viewport already has a pending capture.");
    }
    if (!it.Value().IsPending() && (oldest.Value().IsPending() || it.Value().m_Started < oldest.Value().m_Started))
    {
      oldest = it;
    }
  }
  const bool bFull = m_ViewportCaptures.GetCount() >= MaximumRetainedCaptures;
  if (bFull && oldest.Value().IsPending())
  {
    return xiiStatus("Wait for a pending viewport capture to complete before starting another.");
  }

  xiiCaptureViewportMsgToEngine request;
  request.m_RequestId    = xiiUuid::MakeUuid();
  request.m_DocumentGuid = pDocument->GetGuid();
  request.m_uiViewID     = static_cast<xiiUInt32>(uiView);
  // Capture the selected local render target. The optional remote preview has its own camera and output.
  if (context.HasExpired() || !pEngine->GetCommunicationChannel().SendMessage(&request))
  {
    return xiiStatus("Could not send the viewport capture request to the Editor engine process.");
  }
  if (bFull)
  {
    m_ViewportCaptures.Remove(oldest);
  }
  ViewportCapture capture;
  capture.m_Started = xiiTime::Now();
  capture.m_uiView  = request.m_uiViewID;
  m_ViewportCaptures.Insert(request.m_RequestId, std::move(capture));
  const auto sGuid      = FormatGuid(request.m_RequestId);
  out_result["capture"] = xiiString(sGuid.GetView().GetSubString(2, 36));
  out_result["state"]   = xiiString("pending");
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::GetViewportCapture(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  const auto  sKey     = xiiMCPJson::GetString(arguments, "capture");
  const auto* pCapture = FindViewportCapture(sKey);
  if (pCapture == nullptr)
  {
    return xiiStatus("The viewport capture is unavailable or has expired.");
  }
  out_result["capture"] = xiiString(sKey);
  out_result["view"]    = pCapture->m_uiView;
  if (pCapture->IsPending())
  {
    out_result["state"] = xiiString("pending");
  }
  else if (!pCapture->m_sFailure.IsEmpty())
  {
    out_result["state"] = xiiString("failed");
    out_result["error"] = pCapture->m_sFailure;
  }
  else
  {
    out_result["state"]    = xiiString("completed");
    out_result["width"]    = pCapture->m_uiWidth;
    out_result["height"]   = pCapture->m_uiHeight;
    out_result["mimeType"] = xiiString("image/png");
  }
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::ReadViewportCapture(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  const auto* pCapture = FindViewportCapture(xiiMCPJson::GetString(arguments, "capture"));
  if (pCapture == nullptr || pCapture->m_sBlob.IsEmpty())
  {
    return xiiStatus("The completed viewport capture is unavailable. Check editor.viewport_capture.");
  }
  xiiVariantDictionary image;
  image["uri"]      = xiiString(xiiMCPJson::GetString(arguments, "uri"));
  image["mimeType"] = xiiString("image/png");
  image["blob"]     = pCapture->m_sBlob;
  xiiVariantArray contents;
  contents.PushBack(std::move(image));
  out_result["contents"] = std::move(contents);
  return XII_SUCCESS;
}

xiiResult xiiEditorMCP::RegisterViewport()
{
  const auto capture = xiiMCPPermission::Capture;
  XII_SUCCEED_OR_RETURN(m_Registry.RegisterTool("editor.capture_viewport", "Queue a PNG of the selected visible document viewport's next rendered frame.",
                                                R"({"type":"object","required":["document","session","generation","view"],"properties":{"document":{"type":"string"},"session":{"type":"string"},"generation":{"type":"integer"},"view":{"type":"integer","minimum":0}}})",
                                                capture, xiiMakeDelegate(&xiiEditorMCP::CaptureViewport, this)));
  XII_SUCCEED_OR_RETURN(m_Registry.RegisterTool("editor.viewport_capture", "Read the state of a requested viewport capture. When complete, substitute its ID for {capture} in the viewport capture resource template.",
                                                R"({"type":"object","required":["capture"],"properties":{"capture":{"type":"string"}}})",
                                                capture, xiiMakeDelegate(&xiiEditorMCP::GetViewportCapture, this)));
  xiiVariantDictionary resource;
  resource["name"]        = xiiString("Editor viewport capture");
  resource["mimeType"]    = xiiString("image/png");
  resource["description"] = xiiString("PNG for a completed editor.viewport_capture ID.");
  XII_SUCCEED_OR_RETURN(m_Registry.Register(xiiMCPCapability::ResourceTemplate, xiiStringBuilder(CaptureUriPrefix, "{capture}"), std::move(resource),
                                            capture, xiiMakeDelegate(&xiiEditorMCP::ReadViewportCapture, this)));
  XII_SUCCEED_OR_RETURN(m_Registry.RegisterTool("editor.configure_viewport", "Set a document view's render mode, frame statistics, vertical sync or perspective camera pose.",
                                                R"({"type":"object","required":["document","session","generation","revision","view"],"properties":{"document":{"type":"string"},"session":{"type":"string"},"generation":{"type":"integer"},"revision":{"type":"integer"},"view":{"type":"integer","minimum":0},"renderMode":{"type":"string"},"frameStatistics":{"type":"boolean"},"verticalSync":{"type":"boolean"},"camera":{"type":"object","required":["position","target","up"],"additionalProperties":false,"properties":{"position":{"type":"array","minItems":3,"maxItems":3,"items":{"type":"number"}},"target":{"type":"array","minItems":3,"maxItems":3,"items":{"type":"number"}},"up":{"type":"array","minItems":3,"maxItems":3,"items":{"type":"number"}}}}}})",
                                                xiiMCPPermission::Edit, xiiMakeDelegate(&xiiEditorMCP::ConfigureViewport, this)));
  return m_Registry.RegisterTool("editor.viewport", "Inspect the active or selected document's viewports, camera, resolution and render mode. Select a view ID to read its effective render settings.",
                                 R"({"type":"object","properties":{"document":{"type":"string"},"view":{"type":"integer","minimum":0}}})",
                                 xiiMCPPermission::Read, xiiMakeDelegate(&xiiEditorMCP::Viewport, this));
}
