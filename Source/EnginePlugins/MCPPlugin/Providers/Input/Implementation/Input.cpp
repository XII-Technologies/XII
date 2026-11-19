/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Providers/RuntimeProviders.h>

#include <algorithm>

namespace
{
  bool ReadPage(const xiiVariantDictionary& arguments, xiiUInt64& out_uiOffset, xiiUInt64& out_uiLimit)
  {
    const auto* pOffset = arguments.GetValue("offset");
    const auto* pLimit  = arguments.GetValue("limit");
    out_uiOffset        = 0;
    out_uiLimit         = 32;
    return (pOffset == nullptr || xiiMCPJson::ReadUInt(*pOffset, out_uiOffset)) &&
      (pLimit == nullptr || xiiMCPJson::ReadUInt(*pLimit, out_uiLimit)) &&
      out_uiOffset <= xiiMath::MaxValue<xiiUInt32>() && xiiMath::IsInRange(out_uiLimit, xiiUInt64(1), xiiUInt64(64));
  }

  bool ReadActions(xiiStringView sSet, xiiDynamicArray<xiiString>& out_actions)
  {
    xiiDynamicArray<xiiString> sets;
    xiiInputManager::GetAllInputSets(sets);
    if (!sets.Contains(xiiString(sSet)))
    {
      return false;
    }

    xiiInputManager::GetAllInputActions(sSet, out_actions);
    return true;
  }

  const char* GetStateName(xiiKeyState::Enum state)
  {
    switch (state)
    {
      case xiiKeyState::Up:
        return "up";
      case xiiKeyState::Released:
        return "released";
      case xiiKeyState::Pressed:
        return "pressed";
      case xiiKeyState::Down:
        return "down";
    }
    return "unknown";
  }

  xiiVariantDictionary DescribeAction(xiiStringView sSet, xiiStringView sAction)
  {
    const auto config       = xiiInputManager::GetInputActionConfig(sSet, sAction);
    float      fValue       = 0.0f;
    xiiInt8    iAlternative = -1;
    const auto state        = xiiInputManager::GetInputActionState(sSet, sAction, &fValue, &iAlternative);

    xiiVariantArray bindings;
    for (xiiUInt32 uiIndex = 0; uiIndex < xiiInputActionConfig::MaxInputSlotAlternatives; ++uiIndex)
    {
      const auto& sSlot = config.m_sInputSlotTrigger[uiIndex];
      bindings.PushBack(sSlot.IsEmpty() ? xiiVariant() : xiiVariant(sSlot));
    }

    xiiVariantDictionary row;
    row["action"]     = xiiString(sAction);
    row["state"]      = xiiString(GetStateName(state));
    row["value"]      = xiiMath::IsFinite(fValue) ? xiiVariant(fValue) : xiiVariant();
    row["timeScaled"] = config.m_bApplyTimeScaling;
    row["bindings"]   = std::move(bindings);
    if (iAlternative >= 0)
    {
      row["triggeredAlternative"] = iAlternative;
    }
    return row;
  }

  xiiStatus InspectInput(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto sSet     = xiiMCPJson::GetString(arguments, "set");
    xiiUInt64  uiOffset = 0, uiLimit = 0;
    if (!ReadPage(arguments, uiOffset, uiLimit) || (arguments.Contains("set") && sSet.IsEmpty()))
    {
      return xiiStatus("Provide a valid page range and, when specifying a set, its name.");
    }

    xiiDynamicArray<xiiString> names;
    if (sSet.IsEmpty())
    {
      xiiInputManager::GetAllInputSets(names);
    }
    else if (!ReadActions(sSet, names))
    {
      return xiiStatus("Select an existing input set from runtime.input.");
    }

    xiiVariantArray rows;
    const auto      uiEnd = xiiMath::Min<xiiUInt64>(uiOffset + uiLimit, names.GetCount());
    for (xiiUInt64 uiIndex = uiOffset; uiIndex < uiEnd; ++uiIndex)
    {
      if (context.HasExpired())
      {
        return xiiStatus("Input discovery exceeded its deadline.");
      }
      const auto& sName = names[static_cast<xiiUInt32>(uiIndex)];
      if (sSet.IsEmpty())
      {
        rows.PushBack(sName);
      }
      else
      {
        rows.PushBack(DescribeAction(sSet, sName));
      }
    }

    out_result[sSet.IsEmpty() ? "sets" : "actions"] = std::move(rows);
    out_result["exclusiveSet"]                      = xiiString(xiiInputManager::GetExclusiveInputSet());
    if (!sSet.IsEmpty())
    {
      out_result["set"] = xiiString(sSet);
    }
    if (uiEnd < names.GetCount())
    {
      out_result["nextOffset"] = uiEnd;
    }
    return XII_SUCCESS;
  }

  xiiStatus ListInputSlots(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto sFilter  = xiiMCPJson::GetString(arguments, "filter");
    xiiUInt64  uiOffset = 0, uiLimit = 0;
    if (!ReadPage(arguments, uiOffset, uiLimit))
    {
      return xiiStatus("Provide a valid page range.");
    }

    xiiDynamicArray<xiiStringView> slots;
    xiiInputManager::RetrieveAllKnownInputSlots(slots);
    xiiVariantArray rows;
    xiiUInt64       uiMatched = 0;
    for (const auto sSlot : slots)
    {
      if (context.HasExpired())
      {
        return xiiStatus("Input slot discovery exceeded its deadline.");
      }
      if ((!sFilter.IsEmpty() && sSlot.FindSubString_NoCase(sFilter) == nullptr) || uiMatched++ < uiOffset)
      {
        continue;
      }
      if (rows.GetCount() == uiLimit)
      {
        out_result["nextOffset"] = uiOffset + uiLimit;
        break;
      }

      xiiVariantDictionary row;
      row["slot"]        = xiiString(sSlot);
      row["displayName"] = xiiString(xiiInputManager::GetInputSlotDisplayName(sSlot));
      rows.PushBack(std::move(row));
    }
    out_result["slots"] = std::move(rows);
    return XII_SUCCESS;
  }

  xiiStatus BindInput(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto  sSet         = xiiMCPJson::GetString(arguments, "set");
    const auto  sAction      = xiiMCPJson::GetString(arguments, "action");
    const auto* pAlternative = arguments.GetValue("alternative");
    const auto* pSlot        = arguments.GetValue("slot");
    xiiUInt64   uiIndex      = 0;
    if (sSet.IsEmpty() || sAction.IsEmpty() || pAlternative == nullptr || !xiiMCPJson::ReadUInt(*pAlternative, uiIndex) ||
        uiIndex >= xiiInputActionConfig::MaxInputSlotAlternatives || pSlot == nullptr || (pSlot->IsValid() && !pSlot->IsA<xiiString>()))
    {
      return xiiStatus("Provide an existing set and action, a binding index from 0 to 2, and a slot name or null.");
    }

    xiiDynamicArray<xiiString> actions;
    if (!ReadActions(sSet, actions) || !actions.Contains(xiiString(sAction)))
    {
      return xiiStatus("Select an existing input set and action from runtime.input.");
    }

    auto  config      = xiiInputManager::GetInputActionConfig(sSet, sAction);
    auto& ref_binding = config.m_sInputSlotTrigger[static_cast<xiiUInt32>(uiIndex)];
    if (pSlot->IsValid())
    {
      const auto&                    sSlot = pSlot->Get<xiiString>();
      xiiDynamicArray<xiiStringView> slots;
      xiiInputManager::RetrieveAllKnownInputSlots(slots);
      if (std::ranges::find(slots, sSlot.GetView()) == std::ranges::end(slots))
      {
        return xiiStatus("Use a registered slot from runtime.input_slots or null to clear the binding.");
      }
      ref_binding = sSlot;
    }
    else
    {
      ref_binding.Clear();
    }

    if (context.HasExpired())
    {
      return xiiStatus("The input binding change expired.");
    }
    xiiInputManager::SetInputActionConfig(sSet, sAction, config, false);
    out_result        = DescribeAction(sSet, sAction);
    out_result["set"] = xiiString(sSet);
    return XII_SUCCESS;
  }
} // namespace

void xiiMCPRuntimeProviders::RegisterInput(xiiMCPRegistry& ref_registry)
{
  ref_registry.RegisterTool("runtime.input", "List input sets or inspect the state and alternative bindings of actions in one set.",
                            R"({"type":"object","properties":{"set":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":64}}})",
                            xiiMCPPermission::Read, &InspectInput)
    .IgnoreResult();
  ref_registry.RegisterTool("runtime.input_slots", "List registered game input slot IDs and display names for binding. Filter by slot ID.",
                            R"({"type":"object","properties":{"filter":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":64}}})",
                            xiiMCPPermission::Read, &ListInputSlots)
    .IgnoreResult();
  ref_registry.RegisterTool("runtime.bind_input", "Change one alternative binding on an existing input action for this process. Pass null to clear it.",
                            R"({"type":"object","required":["set","action","alternative","slot"],"properties":{"set":{"type":"string"},"action":{"type":"string"},"alternative":{"type":"integer","minimum":0,"maximum":2},"slot":{"type":["string","null"]}}})",
                            xiiMCPPermission::Edit, &BindInput)
    .IgnoreResult();
}
