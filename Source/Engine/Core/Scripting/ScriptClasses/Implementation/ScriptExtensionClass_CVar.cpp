#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_CVar.h>

#include <Foundation/Configuration/CVar.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptExtensionClass_CVar, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GetValue, In, "Name")->AddFlags(xiiPropertyFlags::Const),
    XII_SCRIPT_FUNCTION_PROPERTY(GetBoolValue, In, "Name")->AddFlags(xiiPropertyFlags::Const),
    XII_SCRIPT_FUNCTION_PROPERTY(GetIntValue, In, "Name")->AddFlags(xiiPropertyFlags::Const),
    XII_SCRIPT_FUNCTION_PROPERTY(GetFloatValue, In, "Name")->AddFlags(xiiPropertyFlags::Const),
    XII_SCRIPT_FUNCTION_PROPERTY(GetDoubleValue, In, "Name")->AddFlags(xiiPropertyFlags::Const),
    XII_SCRIPT_FUNCTION_PROPERTY(GetStringValue, In, "Name")->AddFlags(xiiPropertyFlags::Const),
    XII_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Name", In, "Value"),
    XII_SCRIPT_FUNCTION_PROPERTY(SetBoolValue, In, "Name", In, "Value"),
    XII_SCRIPT_FUNCTION_PROPERTY(SetIntValue, In, "Name", In, "Value"),
    XII_SCRIPT_FUNCTION_PROPERTY(SetFloatValue, In, "Name", In, "Value"),
    XII_SCRIPT_FUNCTION_PROPERTY(SetDoubleValue, In, "Name", In, "Value"),
    XII_SCRIPT_FUNCTION_PROPERTY(SetStringValue, In, "Name", In, "Value"),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiScriptExtensionAttribute("CVar"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

static xiiHashTable<xiiTempHashedString, xiiCVar*> s_CachedCVars;

static xiiCVar* FindCVarByNameCached(xiiStringView sName)
{
  xiiTempHashedString sNameHashed(sName);

  xiiCVar* pCVar = nullptr;
  if (!s_CachedCVars.TryGetValue(sNameHashed, pCVar))
  {
    pCVar = xiiCVar::FindCVarByName(sName);

    s_CachedCVars.Insert(sNameHashed, pCVar);
  }

  xiiCVar::s_AllCVarEvents.AddEventHandler(
    [&](const xiiCVarEvent& e) {
      if (e.m_EventType == xiiCVarEvent::Type::ListOfVarsChanged)
      {
        s_CachedCVars.Clear();
      }
    });

  return pCVar;
}

// static
xiiVariant xiiScriptExtensionClass_CVar::GetValue(xiiStringView sName)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr)
  {
    return {};
  }

  switch (pCVar->GetType())
  {
    case xiiCVarType::Bool:
      return static_cast<xiiCVarBool*>(pCVar)->GetValue();
    case xiiCVarType::Int:
      return static_cast<xiiCVarInt*>(pCVar)->GetValue();
    case xiiCVarType::Float:
      return static_cast<xiiCVarFloat*>(pCVar)->GetValue();
    case xiiCVarType::Double:
      return static_cast<xiiCVarDouble*>(pCVar)->GetValue();
    case xiiCVarType::String:
      return static_cast<xiiCVarString*>(pCVar)->GetValue();

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return {};
}

// static
bool xiiScriptExtensionClass_CVar::GetBoolValue(xiiStringView sName)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != xiiCVarType::Bool)
  {
    xiiLog::Error("CVar '{}' does not exist or is not of type bool.", sName);
    return false;
  }

  return static_cast<xiiCVarBool*>(pCVar)->GetValue();
}

// static
int xiiScriptExtensionClass_CVar::GetIntValue(xiiStringView sName)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != xiiCVarType::Int)
  {
    xiiLog::Error("CVar '{}' does not exist or is not of type int.", sName);
    return 0;
  }

  return static_cast<xiiCVarInt*>(pCVar)->GetValue();
}

// static
float xiiScriptExtensionClass_CVar::GetFloatValue(xiiStringView sName)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != xiiCVarType::Float)
  {
    xiiLog::Error("CVar '{}' does not exist or is not of type float.", sName);
    return 0;
  }

  return static_cast<xiiCVarFloat*>(pCVar)->GetValue();
}

double xiiScriptExtensionClass_CVar::GetDoubleValue(xiiStringView sName)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != xiiCVarType::Double)
  {
    xiiLog::Error("CVar '{}' does not exist or is not of type double.", sName);
    return 0;
  }

  return static_cast<xiiCVarDouble*>(pCVar)->GetValue();
}

// static
xiiString xiiScriptExtensionClass_CVar::GetStringValue(xiiStringView sName)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != xiiCVarType::String)
  {
    xiiLog::Error("CVar '{}' does not exist or is not of type string.", sName);
    return "";
  }

  return static_cast<xiiCVarString*>(pCVar)->GetValue();
}

// static
void xiiScriptExtensionClass_CVar::SetValue(xiiStringView sName, const xiiVariant& value)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr)
  {
    xiiLog::Error("CVar '{}' does not exist.", sName);
    return;
  }

  switch (pCVar->GetType())
  {
    case xiiCVarType::Bool:
    {
      xiiCVarBool* pVar = static_cast<xiiCVarBool*>(pCVar);
      *pVar             = value.ConvertTo<bool>();
    }
    break;
    case xiiCVarType::Int:
    {
      xiiCVarInt* pVar = static_cast<xiiCVarInt*>(pCVar);
      *pVar            = value.ConvertTo<int>();
    }
    break;
    case xiiCVarType::Float:
    {
      xiiCVarFloat* pVar = static_cast<xiiCVarFloat*>(pCVar);
      *pVar              = value.ConvertTo<float>();
    }
    break;
    case xiiCVarType::Double:
    {
      xiiCVarDouble* pVar = static_cast<xiiCVarDouble*>(pCVar);
      *pVar               = value.ConvertTo<double>();
    }
    break;
    case xiiCVarType::String:
    {
      xiiCVarString* pVar = static_cast<xiiCVarString*>(pCVar);
      *pVar               = value.ConvertTo<xiiString>();
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

// static
void xiiScriptExtensionClass_CVar::SetBoolValue(xiiStringView sName, bool bValue)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != xiiCVarType::Bool)
  {
    xiiLog::Error("CVar '{}' does not exist or is not of type bool.", sName);
    return;
  }

  xiiCVarBool* pVar = static_cast<xiiCVarBool*>(pCVar);
  *pVar             = bValue;
}

// static
void xiiScriptExtensionClass_CVar::SetIntValue(xiiStringView sName, int iValue)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != xiiCVarType::Int)
  {
    xiiLog::Error("CVar '{}' does not exist or is not of type int.", sName);
    return;
  }

  xiiCVarInt* pVar = static_cast<xiiCVarInt*>(pCVar);
  *pVar            = iValue;
}

// static
void xiiScriptExtensionClass_CVar::SetFloatValue(xiiStringView sName, float fValue)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != xiiCVarType::Float)
  {
    xiiLog::Error("CVar '{}' does not exist or is not of type float.", sName);
    return;
  }

  xiiCVarFloat* pVar = static_cast<xiiCVarFloat*>(pCVar);
  *pVar              = fValue;
}

void xiiScriptExtensionClass_CVar::SetDoubleValue(xiiStringView sName, double fValue)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != xiiCVarType::Double)
  {
    xiiLog::Error("CVar '{}' does not exist or is not of type double.", sName);
    return;
  }

  xiiCVarDouble* pVar = static_cast<xiiCVarDouble*>(pCVar);
  *pVar               = fValue;
}

// static
void xiiScriptExtensionClass_CVar::SetStringValue(xiiStringView sName, const xiiString& sValue)
{
  xiiCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != xiiCVarType::String)
  {
    xiiLog::Error("CVar '{}' does not exist or is not of type string.", sName);
    return;
  }

  xiiCVarString* pVar = static_cast<xiiCVarString*>(pCVar);
  *pVar               = sValue;
}
