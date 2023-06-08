#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Types/Variant.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

xiiMap<xiiVisualScriptInstance::AssignFuncKey, xiiVisualScriptDataPinAssignFunc> xiiVisualScriptInstance::s_DataPinAssignFunctions;

bool xiiVisualScriptAssignNumberNumber(const void* pSrc, void* pDst)
{
  const bool res                   = *reinterpret_cast<double*>(pDst) != *reinterpret_cast<const double*>(pSrc);
  *reinterpret_cast<double*>(pDst) = *reinterpret_cast<const double*>(pSrc);
  return res;
}

bool xiiVisualScriptAssignNumberBool(const void* pSrc, void* pDst)
{
  const bool res                 = (*reinterpret_cast<bool*>(pDst) != (*reinterpret_cast<const double*>(pSrc) > 0.0));
  *reinterpret_cast<bool*>(pDst) = *reinterpret_cast<const double*>(pSrc) > 0.0;
  return res;
}

bool xiiVisualScriptAssignNumberVec3(const void* pSrc, void* pDst)
{
  const bool res                    = *reinterpret_cast<xiiVec3*>(pDst) != xiiVec3(static_cast<float>(*reinterpret_cast<const double*>(pSrc)));
  *reinterpret_cast<xiiVec3*>(pDst) = xiiVec3(static_cast<float>(*reinterpret_cast<const double*>(pSrc)));
  return res;
}

bool xiiVisualScriptAssignNumberString(const void* pSrc, void* pDst)
{
  double           newValue = *reinterpret_cast<const double*>(pSrc);
  xiiStringBuilder sb;
  xiiConversionUtils::ToString(newValue, sb);

  const bool res                      = *reinterpret_cast<xiiString*>(pDst) != sb;
  *reinterpret_cast<xiiString*>(pDst) = sb;
  return res;
}

bool xiiVisualScriptAssignNumberVariant(const void* pSrc, void* pDst)
{
  xiiVariant newValue                  = *reinterpret_cast<const double*>(pSrc);
  const bool res                       = *reinterpret_cast<xiiVariant*>(pDst) != newValue;
  *reinterpret_cast<xiiVariant*>(pDst) = newValue;
  return res;
}


bool xiiVisualScriptAssignBoolBool(const void* pSrc, void* pDst)
{
  const bool res                 = *reinterpret_cast<bool*>(pDst) != *reinterpret_cast<const bool*>(pSrc);
  *reinterpret_cast<bool*>(pDst) = *reinterpret_cast<const bool*>(pSrc);
  return res;
}

bool xiiVisualScriptAssignBoolNumber(const void* pSrc, void* pDst)
{
  double     newValue              = *reinterpret_cast<const bool*>(pSrc) ? 1.0 : 0.0;
  const bool res                   = *reinterpret_cast<double*>(pDst) != newValue;
  *reinterpret_cast<double*>(pDst) = newValue;
  return res;
}

bool xiiVisualScriptAssignBoolString(const void* pSrc, void* pDst)
{
  bool             newValue = *reinterpret_cast<const bool*>(pSrc);
  xiiStringBuilder sb;
  xiiConversionUtils::ToString(newValue, sb);

  const bool res                      = *reinterpret_cast<xiiString*>(pDst) != sb;
  *reinterpret_cast<xiiString*>(pDst) = sb;
  return res;
}

bool xiiVisualScriptAssignBoolVariant(const void* pSrc, void* pDst)
{
  xiiVariant newValue                  = *reinterpret_cast<const bool*>(pSrc);
  const bool res                       = *reinterpret_cast<xiiVariant*>(pDst) != newValue;
  *reinterpret_cast<xiiVariant*>(pDst) = newValue;
  return res;
}


bool xiiVisualScriptAssignVec3Vec3(const void* pSrc, void* pDst)
{
  const bool res                    = *reinterpret_cast<xiiVec3*>(pDst) != *reinterpret_cast<const xiiVec3*>(pSrc);
  *reinterpret_cast<xiiVec3*>(pDst) = *reinterpret_cast<const xiiVec3*>(pSrc);
  return res;
}

bool xiiVisualScriptAssignVec3Variant(const void* pSrc, void* pDst)
{
  xiiVariant newValue                  = *reinterpret_cast<const xiiVec3*>(pSrc);
  const bool res                       = *reinterpret_cast<xiiVariant*>(pDst) != newValue;
  *reinterpret_cast<xiiVariant*>(pDst) = newValue;
  return res;
}


bool xiiVisualScriptAssignStringString(const void* pSrc, void* pDst)
{
  const bool res                      = *reinterpret_cast<xiiString*>(pDst) != *reinterpret_cast<const xiiString*>(pSrc);
  *reinterpret_cast<xiiString*>(pDst) = *reinterpret_cast<const xiiString*>(pSrc);
  return res;
}

bool xiiVisualScriptAssignStringVariant(const void* pSrc, void* pDst)
{
  xiiVariant newValue                  = *reinterpret_cast<const xiiString*>(pSrc);
  const bool res                       = *reinterpret_cast<xiiVariant*>(pDst) != newValue;
  *reinterpret_cast<xiiVariant*>(pDst) = newValue;
  return res;
}


bool xiiVisualScriptAssignGameObject(const void* pSrc, void* pDst)
{
  const bool res                                = *reinterpret_cast<xiiGameObjectHandle*>(pDst) != *reinterpret_cast<const xiiGameObjectHandle*>(pSrc);
  *reinterpret_cast<xiiGameObjectHandle*>(pDst) = *reinterpret_cast<const xiiGameObjectHandle*>(pSrc);
  return res;
}

bool xiiVisualScriptAssignComponent(const void* pSrc, void* pDst)
{
  const bool res                               = *reinterpret_cast<xiiComponentHandle*>(pDst) != *reinterpret_cast<const xiiComponentHandle*>(pSrc);
  *reinterpret_cast<xiiComponentHandle*>(pDst) = *reinterpret_cast<const xiiComponentHandle*>(pSrc);
  return res;
}

bool xiiVisualScriptAssignVariantVariant(const void* pSrc, void* pDst)
{
  const bool res                       = *reinterpret_cast<xiiVariant*>(pDst) != *reinterpret_cast<const xiiVariant*>(pSrc);
  *reinterpret_cast<xiiVariant*>(pDst) = *reinterpret_cast<const xiiVariant*>(pSrc);
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


XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptInstanceAssignFunctions);
