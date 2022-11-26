#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)

#  include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#  include <Foundation/Types/Uuid.h>
#  include <Windows.Foundation.numerics.h>

xiiMat4 xiiUwpUtils::ConvertMat4(const ABI::Windows::Foundation::Numerics::Matrix4x4& in)
{
  return xiiMat4(in.M11, in.M21, in.M31, in.M41, in.M12, in.M22, in.M32, in.M42, in.M13, in.M23, in.M33, in.M43, in.M14, in.M24, in.M34, in.M44);
}

xiiVec3 xiiUwpUtils::ConvertVec3(const ABI::Windows::Foundation::Numerics::Vector3& in)
{
  return xiiVec3(in.X, in.Y, in.Z);
}

void xiiUwpUtils::ConvertVec3(const xiiVec3& in, ABI::Windows::Foundation::Numerics::Vector3& out)
{
  out.X = in.x;
  out.Y = in.y;
  out.Z = in.z;
}

xiiQuat xiiUwpUtils::ConvertQuat(const ABI::Windows::Foundation::Numerics::Quaternion& in)
{
  return xiiQuat(in.X, in.Y, in.Z, in.W);
}

void xiiUwpUtils::ConvertQuat(const xiiQuat& in, ABI::Windows::Foundation::Numerics::Quaternion& out)
{
  out.X = in.v.x;
  out.Y = in.v.y;
  out.Z = in.v.z;
  out.W = in.w;
}

xiiUuid xiiUwpUtils::ConvertGuid(const GUID& in)
{
  return *reinterpret_cast<const xiiUuid*>(&in);
}

void xiiUwpUtils::ConvertGuid(const xiiUuid& in, GUID& out)
{
  xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt32*>(&out), reinterpret_cast<const xiiUInt32*>(&in), 4);
}


#endif



XII_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_uwp_UWPUtils);
