#include <Core/CorePCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/World.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiWindWorldModuleInterface, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiWindStrength, 1)
  XII_ENUM_CONSTANTS(xiiWindStrength::Calm, xiiWindStrength::LightBreeze, xiiWindStrength::GentleBreeze, xiiWindStrength::ModerateBreeze, xiiWindStrength::StrongBreeze, xiiWindStrength::Storm)
  XII_ENUM_CONSTANTS(xiiWindStrength::WeakShockwave, xiiWindStrength::MediumShockwave, xiiWindStrength::StrongShockwave, xiiWindStrength::ExtremeShockwave)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

float xiiWindStrength::GetInMetersPerSecond(Enum strength)
{
  // inspired by the Beaufort scale
  // https://en.wikipedia.org/wiki/Beaufort_scale

  switch (strength)
  {
    case Calm:
      return 0.5f;

    case LightBreeze:
      return 2.0f;

    case GentleBreeze:
      return 5.0f;

    case ModerateBreeze:
      return 9.0f;

    case StrongBreeze:
      return 14.0f;

    case Storm:
      return 20.0f;

    case WeakShockwave:
      return 40.0f;

    case MediumShockwave:
      return 70.0f;

    case StrongShockwave:
      return 100.0f;

    case ExtremeShockwave:
      return 150.0f;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

xiiWindWorldModuleInterface::xiiWindWorldModuleInterface(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiVec3 xiiWindWorldModuleInterface::ComputeWindFlutter(const xiiVec3& vWind, const xiiVec3& vObjectDir, float fFlutterSpeed, xiiUInt32 uiFlutterRandomOffset) const
{
  if (vWind.IsZero(0.001f))
    return xiiVec3::ZeroVector();

  xiiVec3     windDir       = vWind;
  const float fWindStrength = windDir.GetLengthAndNormalize();

  if (fWindStrength <= 0.01f)
    return xiiVec3::ZeroVector();

  xiiVec3 mainDir = vObjectDir;
  mainDir.NormalizeIfNotZero(xiiVec3::UnitZAxis()).IgnoreResult();

  xiiVec3 flutterDir = windDir.CrossRH(mainDir);
  flutterDir.NormalizeIfNotZero(xiiVec3::UnitZAxis()).IgnoreResult();

  const float fFlutterOffset = (uiFlutterRandomOffset & 1023u) / 256.0f;

  const float fFlutter = xiiMath::Sin(xiiAngle::MakeFromRadian(fFlutterOffset + fFlutterSpeed * fWindStrength * GetWorld()->GetClock().GetAccumulatedTime().AsFloatInSeconds())) * fWindStrength;

  return flutterDir * fFlutter;
}

XII_STATICLINK_FILE(Core, Core_Interfaces_Implementation_WindWorldModule);
