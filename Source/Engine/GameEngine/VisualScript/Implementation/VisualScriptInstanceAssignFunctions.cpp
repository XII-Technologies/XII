#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Types/Variant.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

xiiMap<xiiVisualScriptInstance::AssignFuncKey, xiiVisualScriptDataPinAssignFunc> xiiVisualScriptInstance::s_DataPinAssignFunctions;

bool xiiVisualScriptAssignNumberNumber(const void* src, void* dst)
{
  const bool res                  = *reinterpret_cast<double*>(dst) != *reinterpret_cast<const double*>(src);
  *reinterpret_cast<double*>(dst) = *reinterpret_cast<const double*>(src);
  return res;
}

bool xiiVisualScriptAssignNumberBool(const void* src, void* dst)
{
  const bool res                = (*reinterpret_cast<bool*>(dst) != (*reinterpret_cast<const double*>(src) > 0.0));
  *reinterpret_cast<bool*>(dst) = *reinterpret_cast<const double*>(src) > 0.0;
  return res;
}

bool xiiVisualScriptAssignNumberVec3(const void* src, void* dst)
{
  const bool res                   = *reinterpret_cast<xiiVec3*>(dst) != xiiVec3(static_cast<float>(*reinterpret_cast<const double*>(src)));
  *reinterpret_cast<xiiVec3*>(dst) = xiiVec3(static_cast<float>(*reinterpret_cast<const double*>(src)));
  return res;
}

bool xiiVisualScriptAssignNumberString(const void* src, void* dst)
{
  double           newValue = *reinterpret_cast<const double*>(src);
  xiiStringBuilder sb;
  xiiConversionUtils::ToString(newValue, sb);

  const bool res                     = *reinterpret_cast<xiiString*>(dst) != sb;
  *reinterpret_cast<xiiString*>(dst) = sb;
  return res;
}

bool xiiVisualScriptAssignNumberVariant(const void* src, void* dst)
{
  xiiVariant newValue                 = *reinterpret_cast<const double*>(src);
  const bool res                      = *reinterpret_cast<xiiVariant*>(dst) != newValue;
  *reinterpret_cast<xiiVariant*>(dst) = newValue;
  return res;
}


bool xiiVisualScriptAssignBoolBool(const void* src, void* dst)
{
  const bool res                = *reinterpret_cast<bool*>(dst) != *reinterpret_cast<const bool*>(src);
  *reinterpret_cast<bool*>(dst) = *reinterpret_cast<const bool*>(src);
  return res;
}

bool xiiVisualScriptAssignBoolNumber(const void* src, void* dst)
{
  double     newValue             = *reinterpret_cast<const bool*>(src) ? 1.0 : 0.0;
  const bool res                  = *reinterpret_cast<double*>(dst) != newValue;
  *reinterpret_cast<double*>(dst) = newValue;
  return res;
}

bool xiiVisualScriptAssignBoolString(const void* src, void* dst)
{
  bool             newValue = *reinterpret_cast<const bool*>(src);
  xiiStringBuilder sb;
  xiiConversionUtils::ToString(newValue, sb);

  const bool res                     = *reinterpret_cast<xiiString*>(dst) != sb;
  *reinterpret_cast<xiiString*>(dst) = sb;
  return res;
}

bool xiiVisualScriptAssignBoolVariant(const void* src, void* dst)
{
  xiiVariant newValue                 = *reinterpret_cast<const bool*>(src);
  const bool res                      = *reinterpret_cast<xiiVariant*>(dst) != newValue;
  *reinterpret_cast<xiiVariant*>(dst) = newValue;
  return res;
}


bool xiiVisualScriptAssignVec3Vec3(const void* src, void* dst)
{
  const bool res                   = *reinterpret_cast<xiiVec3*>(dst) != *reinterpret_cast<const xiiVec3*>(src);
  *reinterpret_cast<xiiVec3*>(dst) = *reinterpret_cast<const xiiVec3*>(src);
  return res;
}

bool xiiVisualScriptAssignVec3Variant(const void* src, void* dst)
{
  xiiVariant newValue                 = *reinterpret_cast<const xiiVec3*>(src);
  const bool res                      = *reinterpret_cast<xiiVariant*>(dst) != newValue;
  *reinterpret_cast<xiiVariant*>(dst) = newValue;
  return res;
}


bool xiiVisualScriptAssignStringString(const void* src, void* dst)
{
  const bool res                     = *reinterpret_cast<xiiString*>(dst) != *reinterpret_cast<const xiiString*>(src);
  *reinterpret_cast<xiiString*>(dst) = *reinterpret_cast<const xiiString*>(src);
  return res;
}

bool xiiVisualScriptAssignStringVariant(const void* src, void* dst)
{
  xiiVariant newValue                 = *reinterpret_cast<const xiiString*>(src);
  const bool res                      = *reinterpret_cast<xiiVariant*>(dst) != newValue;
  *reinterpret_cast<xiiVariant*>(dst) = newValue;
  return res;
}


bool xiiVisualScriptAssignGameObject(const void* src, void* dst)
{
  const bool res                               = *reinterpret_cast<xiiGameObjectHandle*>(dst) != *reinterpret_cast<const xiiGameObjectHandle*>(src);
  *reinterpret_cast<xiiGameObjectHandle*>(dst) = *reinterpret_cast<const xiiGameObjectHandle*>(src);
  return res;
}

bool xiiVisualScriptAssignComponent(const void* src, void* dst)
{
  const bool res                              = *reinterpret_cast<xiiComponentHandle*>(dst) != *reinterpret_cast<const xiiComponentHandle*>(src);
  *reinterpret_cast<xiiComponentHandle*>(dst) = *reinterpret_cast<const xiiComponentHandle*>(src);
  return res;
}

bool xiiVisualScriptAssignVariantVariant(const void* src, void* dst)
{
  const bool res                      = *reinterpret_cast<xiiVariant*>(dst) != *reinterpret_cast<const xiiVariant*>(src);
  *reinterpret_cast<xiiVariant*>(dst) = *reinterpret_cast<const xiiVariant*>(src);
  return res;
}

void xiiVisualScriptInstance::SetupPinDataTypeConversions()
{
  static bool bDone = false;
  if (bDone)
    return;

  bDone = true;

  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Number, xiiVisualScriptDataPinType::Number, xiiVisualScriptAssignNumberNumber);
  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Number, xiiVisualScriptDataPinType::Boolean, xiiVisualScriptAssignNumberBool);
  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Number, xiiVisualScriptDataPinType::Vec3, xiiVisualScriptAssignNumberVec3);
  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Number, xiiVisualScriptDataPinType::String, xiiVisualScriptAssignNumberString);
  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Number, xiiVisualScriptDataPinType::Variant, xiiVisualScriptAssignNumberVariant);

  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Boolean, xiiVisualScriptDataPinType::Boolean, xiiVisualScriptAssignBoolBool);
  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Boolean, xiiVisualScriptDataPinType::Number, xiiVisualScriptAssignBoolNumber);
  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Boolean, xiiVisualScriptDataPinType::String, xiiVisualScriptAssignBoolString);
  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Boolean, xiiVisualScriptDataPinType::Variant, xiiVisualScriptAssignBoolVariant);

  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Vec3, xiiVisualScriptDataPinType::Vec3, xiiVisualScriptAssignVec3Vec3);
  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Vec3, xiiVisualScriptDataPinType::Variant, xiiVisualScriptAssignVec3Variant);

  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::String, xiiVisualScriptDataPinType::String, xiiVisualScriptAssignStringString);
  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::String, xiiVisualScriptDataPinType::Variant, xiiVisualScriptAssignStringVariant);

  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::GameObjectHandle, xiiVisualScriptDataPinType::GameObjectHandle, xiiVisualScriptAssignGameObject);
  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::ComponentHandle, xiiVisualScriptDataPinType::ComponentHandle, xiiVisualScriptAssignComponent);

  RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Variant, xiiVisualScriptDataPinType::Variant, xiiVisualScriptAssignVariantVariant);
}

void xiiVisualScriptInstance::RegisterDataPinAssignFunction(
  xiiVisualScriptDataPinType::Enum sourceType,
  xiiVisualScriptDataPinType::Enum dstType,
  xiiVisualScriptDataPinAssignFunc func)
{
  AssignFuncKey key;
  key.m_SourceType = sourceType;
  key.m_DstType    = dstType;

  s_DataPinAssignFunctions[key] = func;
}

xiiVisualScriptDataPinAssignFunc xiiVisualScriptInstance::FindDataPinAssignFunction(
  xiiVisualScriptDataPinType::Enum sourceType,
  xiiVisualScriptDataPinType::Enum dstType)
{
  AssignFuncKey key;
  key.m_SourceType = sourceType;
  key.m_DstType    = dstType;

  return s_DataPinAssignFunctions.GetValueOrDefault(key, nullptr);
}
