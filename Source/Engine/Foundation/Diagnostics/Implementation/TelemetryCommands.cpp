/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Diagnostics/TelemetryCommands.h>
#include <Foundation/Network/Reflection/NetworkAttributes.h>

namespace
{
  xiiTelemetryCommandResponse Error(xiiNetworkDataErrorCode code)
  {
    xiiTelemetryCommandResponse response;
    response.m_uiError = static_cast<xiiUInt8>(code);
    return response;
  }

  xiiNetworkRpcAttribute* Command(xiiUInt64 id)
  {
    auto* attribute                     = new xiiNetworkRpcAttribute(id);
    attribute->m_bAllowFireAndForget    = false;
    attribute->m_uiMaximumArgumentBytes = 8192;
    attribute->m_uiTimeoutMilliseconds  = 5000;
    return attribute;
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTelemetryCommandResponse, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTelemetryCommandResponse>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Error", m_uiError)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Output", m_sOutput)->AddAttributes(new xiiNetworkAttribute(2)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494432000007ULL) } XII_END_ATTRIBUTES;
} XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTelemetryCommands, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTelemetryCommands>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(SetCVar)->AddAttributes(Command(1)),
    XII_FUNCTION_PROPERTY(Execute)->AddAttributes(Command(2)),
    XII_FUNCTION_PROPERTY(Complete)->AddAttributes(Command(3)),
  } XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494432000008ULL) } XII_END_ATTRIBUTES;
} XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiTelemetryCommandResponse xiiTelemetryCommands::SetCVar(const xiiString& sName, const xiiVariant& value)
{
  if (sName.IsEmpty() || sName.GetElementCount() > 512)
    return Error(xiiNetworkDataErrorCode::InvalidArgument);
  for (auto* variable = xiiCVar::GetFirstInstance(); variable; variable = variable->GetNextInstance())
  {
    if (variable->GetName() != sName)
      continue;
    switch (variable->GetType())
    {
      case xiiCVarType::Bool:
        if (!value.IsA<bool>())
          break;
        *static_cast<xiiCVarBool*>(variable) = value.Get<bool>();
        return {};
      case xiiCVarType::Int:
        if (!value.IsA<xiiInt32>())
          break;
        *static_cast<xiiCVarInt*>(variable) = value.Get<xiiInt32>();
        return {};
      case xiiCVarType::Float:
        if (!value.IsA<float>() || !xiiMath::IsFinite(value.Get<float>()))
          break;
        *static_cast<xiiCVarFloat*>(variable) = value.Get<float>();
        return {};
      case xiiCVarType::Double:
        if (!value.IsA<double>() || !xiiMath::IsFinite(value.Get<double>()))
          break;
        *static_cast<xiiCVarDouble*>(variable) = value.Get<double>();
        return {};
      case xiiCVarType::String:
        if (!value.IsA<xiiString>() || value.Get<xiiString>().GetElementCount() > 4096)
          break;
        *static_cast<xiiCVarString*>(variable) = value.Get<xiiString>();
        return {};
      default:
        break;
    }
    return Error(xiiNetworkDataErrorCode::InvalidArgument);
  }
  return Error(xiiNetworkDataErrorCode::UnknownType);
}

xiiTelemetryCommandResponse xiiTelemetryCommands::Execute(const xiiString& sCommand)
{
  if (sCommand.GetElementCount() > 4096)
    return Error(xiiNetworkDataErrorCode::SizeLimit);
  return m_Console.IsValid() ? m_Console(sCommand, false) : Error(xiiNetworkDataErrorCode::UnknownRpc);
}

xiiTelemetryCommandResponse xiiTelemetryCommands::Complete(const xiiString& sCommand)
{
  if (sCommand.GetElementCount() > 4096)
    return Error(xiiNetworkDataErrorCode::SizeLimit);
  return m_Console.IsValid() ? m_Console(sCommand, true) : Error(xiiNetworkDataErrorCode::UnknownRpc);
}

XII_STATICLINK_FILE(Foundation, Foundation_Diagnostics_Implementation_TelemetryCommands);
