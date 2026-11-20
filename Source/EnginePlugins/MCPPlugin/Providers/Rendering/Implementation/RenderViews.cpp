/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#ifdef XII_MCP_HAS_RENDERING
#  include <Core/Graphics/Camera.h>
#  include <Core/World/World.h>
#  include <Foundation/Reflection/ReflectionUtils.h>
#  include <Foundation/Utilities/ConversionUtils.h>
#  include <GraphicsCore/Debug/DebugRenderSettings.h>
#  include <GraphicsCore/Pipeline/RenderConfiguration.h>
#  include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#  include <GraphicsCore/Pipeline/RenderWorldModule.h>
#  include <GraphicsCore/Pipeline/View.h>
#  include <MCPPlugin/Json.h>
#  include <MCPPlugin/Providers/RuntimeProviders.h>
#  include <MCPPlugin/Providers/World/World.h>
#  include <MCPPlugin/Reflection.h>

namespace
{
  using xiiMCPRuntimeProviders::FindWorld;
  using xiiMCPRuntimeProviders::FormatHandle;

  constexpr xiiStringView ViewsTool    = "runtime.render_views";
  constexpr xiiStringView ViewTool     = "runtime.render_view";
  constexpr xiiStringView EditViewTool = "runtime.edit_render_view";

  xiiView* FindRenderView(const xiiWorld& world, const xiiVariantDictionary& arguments)
  {
    xiiUInt64 uiId = 0;
    if (xiiConversionUtils::StringToUInt64Strict(xiiMCPJson::GetString(arguments, "view"), uiId).Failed() ||
        uiId > xiiMath::MaxValue<xiiUInt32>())
    {
      return nullptr;
    }

    xiiViewId id;
    id.m_Data           = static_cast<xiiUInt32>(uiId);
    const auto* pRender = world.GetModuleReadOnly<xiiRenderWorldModule>();
    xiiView*    pView   = nullptr;
    return pRender != nullptr && pRender->TryGetView(xiiViewHandle(id), pView) ? pView : nullptr;
  }

  xiiVariantDictionary DescribeRenderView(const xiiView& view)
  {
    const auto&          viewport = view.GetViewport();
    xiiVariantDictionary row;
    row["id"]              = FormatHandle(view.GetHandle());
    row["name"]            = xiiString(view.GetName());
    row["usageHint"]       = static_cast<xiiUInt32>(view.GetCameraUsageHint().GetValue());
    row["viewportWidth"]   = viewport.width;
    row["viewportHeight"]  = viewport.height;
    row["renderWidth"]     = view.GetRenderResolutionWidth();
    row["renderHeight"]    = view.GetRenderResolutionHeight();
    row["renderScale"]     = view.GetRenderScale();
    row["resolutionScale"] = view.GetRenderResolutionScale();
    row["renderMode"]      = view.GetViewRenderMode().GetValue();
    row["renderPath"]      = view.GetRenderPath().GetValue();
    if (view.GetRenderPipelineResource().IsValid())
    {
      row["pipeline"] = xiiString(view.GetRenderPipelineResource().GetResourceID());
    }
    if (const auto* pCamera = view.GetCamera())
    {
      if (xiiVariant position; xiiMCPReflection::ConvertValue(pCamera->GetPosition(), position))
      {
        row["cameraPosition"] = std::move(position);
      }
      if (xiiVariant forward; xiiMCPReflection::ConvertValue(pCamera->GetDirForwards(), forward))
      {
        row["cameraForward"] = std::move(forward);
      }
    }
    return row;
  }

  xiiStatus ListRenderViews(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto* pWorld   = FindWorld(arguments);
    const auto* pOffset  = arguments.GetValue("offset");
    const auto* pLimit   = arguments.GetValue("limit");
    xiiUInt64   uiOffset = 0, uiLimit = 32;
    if (pWorld == nullptr || (pOffset != nullptr && !xiiMCPJson::ReadUInt(*pOffset, uiOffset)) ||
        (pLimit != nullptr && !xiiMCPJson::ReadUInt(*pLimit, uiLimit)) || uiOffset > xiiMath::MaxValue<xiiUInt32>() ||
        uiLimit < 1 || uiLimit > 64)
    {
      return xiiStatus("Provide a current world ID from runtime.worlds and a valid view page range.");
    }

    XII_LOCK(pWorld->GetReadMarker());
    const auto* pRender = pWorld->GetModuleReadOnly<xiiRenderWorldModule>();
    if (pRender == nullptr)
    {
      return xiiStatus("The selected world has no rendering module.");
    }

    xiiDynamicArray<xiiViewHandle> handles;
    pRender->GetViewHandles(handles);
    const auto      uiEnd = xiiMath::Min<xiiUInt64>(uiOffset + uiLimit, handles.GetCount());
    xiiVariantArray rows;
    for (xiiUInt64 uiIndex = uiOffset; uiIndex < uiEnd; ++uiIndex)
    {
      if (context.HasExpired())
      {
        return xiiStatus("Render view inspection exceeded its deadline.");
      }
      xiiView* pView = nullptr;
      if (pRender->TryGetView(handles[static_cast<xiiUInt32>(uiIndex)], pView))
      {
        rows.PushBack(DescribeRenderView(*pView));
      }
    }
    out_result["views"] = std::move(rows);
    if (uiEnd < handles.GetCount())
    {
      out_result["nextOffset"] = uiEnd;
    }
    return XII_SUCCESS;
  }

  bool CanEditSetting(const xiiRTTI& type, const xiiAbstractProperty& property)
  {
    // Debug controls retain state independently of the resolved viewport override.
    return !type.IsDerivedFrom<xiiDebugRenderSettings>() && !property.GetFlags().IsSet(xiiPropertyFlags::Bitflags) &&
      property.GetAttributeByType<xiiReadOnlyAttribute>() == nullptr && xiiMCPReflection::SupportsArgument(property);
  }

  bool ReadRenderSetting(const xiiRenderSubsystemSettings& settings, const xiiRenderSettingProperty& property, xiiVariant& out_value)
  {
    xiiVariant native;
    if (xiiRenderSubsystemSettings::ReadProperty(settings, property.m_sPath, native).Failed())
    {
      return false;
    }
    if (property.m_pProperty->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
    {
      xiiStringBuilder sValue;
      if (!xiiReflectionUtils::EnumerationToString(property.m_pProperty->GetSpecificType(), native.ConvertTo<xiiInt64>(), sValue))
      {
        return false;
      }
      out_value = xiiString(sValue);
      return true;
    }
    return xiiMCPReflection::ConvertValue(native, out_value);
  }

  xiiStatus InspectRenderView(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto* pWorld = FindWorld(arguments);
    if (pWorld == nullptr)
    {
      return xiiStatus("Select a current world from runtime.worlds.");
    }
    XII_LOCK(pWorld->GetReadMarker());
    const auto* pView = FindRenderView(*pWorld, arguments);
    if (pView == nullptr)
    {
      return xiiStatus("Select a current view from runtime.render_views.");
    }

    const auto effective = pView->GetEffectiveRenderConfiguration();
    const auto sType     = xiiMCPJson::GetString(arguments, "subsystem");
    out_result           = DescribeRenderView(*pView);
    if (const auto sError = pView->GetRenderPipelineInstance().GetLastBuildError(); !sError.IsEmpty())
    {
      out_result["pipelineError"] = xiiString(sError.GetSubString(0, 512));
    }
    if (sType.IsEmpty() && !arguments.Contains("subsystem"))
    {
      xiiVariantArray types;
      for (const auto* pSettings : effective.m_Subsystems)
      {
        if (pSettings != nullptr && types.GetCount() < 64)
        {
          types.PushBack(xiiString(pSettings->GetDynamicRTTI()->GetTypeName()));
        }
      }
      out_result["subsystems"] = std::move(types);
      return XII_SUCCESS;
    }

    const auto* pType     = xiiRTTI::FindTypeByName(sType);
    const auto* pSettings = pType != nullptr ? effective.FindSubsystem(pType) : nullptr;
    if (pSettings == nullptr)
    {
      return xiiStatus("The subsystem is unavailable on the selected render view.");
    }

    const auto*                               pAuthored = pView->GetRenderConfiguration().FindSubsystem(pType);
    xiiDynamicArray<xiiRenderSettingProperty> properties;
    pSettings->GetProperties(properties);
    xiiVariantDictionary values;
    for (const auto& property : properties)
    {
      xiiVariant value;
      if (values.GetCount() >= 128 || !ReadRenderSetting(*pSettings, property, value))
      {
        continue;
      }
      xiiVariantDictionary row;
      row["value"]             = std::move(value);
      row["scope"]             = static_cast<xiiUInt32>(effective.GetSource(pType, property.m_sPath));
      row["viewOverride"]      = pAuthored != nullptr && pAuthored->IsOverridden(property.m_sPath);
      row["editable"]          = CanEditSetting(*pType, *property.m_pProperty);
      values[property.m_sPath] = std::move(row);
    }
    out_result["subsystem"] = xiiString(sType);
    out_result["settings"]  = std::move(values);
    out_result["schema"]    = xiiMCPReflection::Describe(*pType);
    return XII_SUCCESS;
  }

  xiiStatus EditRenderView(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto* pWorld = FindWorld(arguments);
    if (pWorld == nullptr)
    {
      return xiiStatus("Select a current world from runtime.worlds.");
    }
    const auto  sAction   = xiiMCPJson::GetString(arguments, "action");
    const auto  sProperty = xiiMCPJson::GetString(arguments, "property");
    const auto* pValue    = arguments.GetValue("value");
    if (sProperty.IsEmpty() || (sAction != "clear" && (sAction != "set" || pValue == nullptr)))
    {
      return xiiStatus("Provide a setting path and a set or clear action. Set requires a value.");
    }

    XII_LOCK(pWorld->GetWriteMarker());
    auto*       pView     = FindRenderView(*pWorld, arguments);
    const auto* pType     = xiiRTTI::FindTypeByName(xiiMCPJson::GetString(arguments, "subsystem"));
    const auto* pSettings = pView != nullptr && pType != nullptr ? pView->GetRenderSubsystemSettings(pType) : nullptr;
    if (pSettings == nullptr || context.HasExpired())
    {
      return xiiStatus("The render view or subsystem is unavailable, or the request expired.");
    }

    xiiDynamicArray<xiiRenderSettingProperty> properties;
    pSettings->GetProperties(properties);
    const xiiAbstractProperty* pProperty = nullptr;
    for (const auto& property : properties)
    {
      if (property.m_sPath == sProperty)
      {
        pProperty = property.m_pProperty;
        break;
      }
    }
    if (pProperty == nullptr || !CanEditSetting(*pType, *pProperty))
    {
      return xiiStatus("Use an editable setting path from runtime.render_view.");
    }

    auto candidate = pView->GetRenderConfiguration();
    if (sAction == "set")
    {
      xiiVariant value;
      if (!xiiMCPReflection::ConvertArgument(*pProperty, *pValue, value) || candidate.SetOverride(pType, sProperty, value).Failed())
      {
        return xiiStatus("The value failed reflected type or render setting validation.");
      }
    }
    else
    {
      candidate.RemoveOverride(pType, sProperty);
    }
    if (context.HasExpired() || pView->SetRenderConfiguration(candidate).Failed())
    {
      return xiiStatus("The render configuration was rejected or the request expired.");
    }

    out_result["view"]         = FormatHandle(pView->GetHandle());
    out_result["subsystem"]    = xiiString(pType->GetTypeName());
    out_result["property"]     = xiiString(sProperty);
    out_result["viewOverride"] = sAction == "set";
    return XII_SUCCESS;
  }
} // namespace

void xiiMCPRuntimeProviders::RegisterRenderViews(xiiMCPRegistry& ref_registry)
{
  const auto read = xiiMCPPermission::Read;
  ref_registry.RegisterTool(ViewsTool, "Page through a world's render views, their current IDs, cameras, pipelines, modes and render resolutions.",
                            R"({"type":"object","required":["world"],"properties":{"world":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":64}}})",
                            read, &ListRenderViews)
    .IgnoreResult();
  ref_registry.RegisterTool(ViewTool, "Inspect one render view and discover its effective render subsystems. Provide a subsystem name to inspect its settings and view overrides.",
                            R"({"type":"object","required":["world","view"],"properties":{"world":{"type":"string"},"view":{"type":"string"},"subsystem":{"type":"string"}}})",
                            read, &InspectRenderView)
    .IgnoreResult();
  ref_registry.RegisterTool(EditViewTool, "Set or clear a validated render setting override on one view. Effective settings update at the next frame boundary.",
                            R"({"type":"object","required":["world","view","subsystem","property","action"],"properties":{"world":{"type":"string"},"view":{"type":"string"},"subsystem":{"type":"string"},"property":{"type":"string"},"action":{"enum":["set","clear"]},"value":{}}})",
                            xiiMCPPermission::Edit, &EditRenderView)
    .IgnoreResult();
}

void xiiMCPRuntimeProviders::RemoveRenderViews(xiiMCPRegistry& ref_registry)
{
  ref_registry.Remove(xiiMCPCapability::Tool, EditViewTool);
  ref_registry.Remove(xiiMCPCapability::Tool, ViewTool);
  ref_registry.Remove(xiiMCPCapability::Tool, ViewsTool);
}
#endif
