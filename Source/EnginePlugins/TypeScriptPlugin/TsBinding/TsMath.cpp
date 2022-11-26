#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

//////////////////////////////////////////////////////////////////////////

void xiiTypeScriptBinding::PushVec2(duk_context* pDuk, const xiiVec2& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                    // [ global ]
  XII_VERIFY(duk.PushLocalObject("__Vec2").Succeeded(), ""); // [ global __Vec2 ]
  duk_get_prop_string(duk, -1, "Vec2");                      // [ global __Vec2 Vec2 ]
  duk_push_number(duk, value.x);                             // [ global __Vec2 Vec2 x ]
  duk_push_number(duk, value.y);                             // [ global __Vec2 Vec2 x y ]
  duk_new(duk, 2);                                           // [ global __Vec2 result ]
  duk_remove(duk, -2);                                       // [ global result ]
  duk_remove(duk, -2);                                       // [ result ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void xiiTypeScriptBinding::SetVec2(duk_context* pDuk, xiiInt32 iObjIdx, const xiiVec2& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("x", value.x, iObjIdx);
  duk.SetNumberProperty("y", value.y, iObjIdx);

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void xiiTypeScriptBinding::SetVec2Property(duk_context* pDuk, const char* szPropertyName, xiiInt32 iObjIdx, const xiiVec2& value)
{
  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetVec2(pDuk, -1, value);
  duk.PopStack();

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

xiiVec2 xiiTypeScriptBinding::GetVec2(duk_context* pDuk, xiiInt32 iObjIdx, const xiiVec2& fallback /*= xiiVec2::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  xiiVec2 res;

  XII_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.x));
  duk_pop(pDuk);
  XII_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.y));
  duk_pop(pDuk);

  return res;
}

xiiVec2 xiiTypeScriptBinding::GetVec2Property(
  duk_context*   pDuk,
  const char*    szPropertyName,
  xiiInt32       iObjIdx,
  const xiiVec2& fallback /*= xiiVec2::ZeroVector()*/)
{
  xiiDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const xiiVec2 res = GetVec2(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void xiiTypeScriptBinding::PushVec3(duk_context* pDuk, const xiiVec3& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                    // [ global ]
  XII_VERIFY(duk.PushLocalObject("__Vec3").Succeeded(), ""); // [ global __Vec3 ]
  duk_get_prop_string(duk, -1, "Vec3");                      // [ global __Vec3 Vec3 ]
  duk_push_number(duk, value.x);                             // [ global __Vec3 Vec3 x ]
  duk_push_number(duk, value.y);                             // [ global __Vec3 Vec3 x y ]
  duk_push_number(duk, value.z);                             // [ global __Vec3 Vec3 x y z ]
  duk_new(duk, 3);                                           // [ global __Vec3 result ]
  duk_remove(duk, -2);                                       // [ global result ]
  duk_remove(duk, -2);                                       // [ result ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void xiiTypeScriptBinding::SetVec3(duk_context* pDuk, xiiInt32 iObjIdx, const xiiVec3& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("x", value.x, iObjIdx);
  duk.SetNumberProperty("y", value.y, iObjIdx);
  duk.SetNumberProperty("z", value.z, iObjIdx);
}

void xiiTypeScriptBinding::SetVec3Property(duk_context* pDuk, const char* szPropertyName, xiiInt32 iObjIdx, const xiiVec3& value)
{
  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetVec3(pDuk, -1, value);
  duk.PopStack();

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

xiiVec3 xiiTypeScriptBinding::GetVec3(duk_context* pDuk, xiiInt32 iObjIdx, const xiiVec3& fallback /*= xiiVec3::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  xiiVec3 res;

  XII_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.x));
  duk_pop(pDuk);
  XII_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.y));
  duk_pop(pDuk);
  XII_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "z"), "");
  res.z = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.z));
  duk_pop(pDuk);

  return res;
}

xiiVec3 xiiTypeScriptBinding::GetVec3Property(
  duk_context*   pDuk,
  const char*    szPropertyName,
  xiiInt32       iObjIdx,
  const xiiVec3& fallback /*= xiiVec3::ZeroVector()*/)
{
  xiiDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const xiiVec3 res = GetVec3(pDuk, -1, fallback);
  duk.PopStack(); // [ ]

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void xiiTypeScriptBinding::PushMat3(duk_context* pDuk, const xiiMat3& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                    // [ global ]
  XII_VERIFY(duk.PushLocalObject("__Mat3").Succeeded(), ""); // [ global __Mat3 ]
  duk_get_prop_string(duk, -1, "Mat3");                      // [ global __Mat3 Mat3 ]

  float rm[9];
  value.GetAsArray(rm, xiiMatrixLayout::RowMajor);

  for (xiiUInt32 i = 0; i < 9; ++i)
  {
    duk_push_number(duk, rm[i]); // [ global __Mat3 Mat3 9params ]
  }

  duk_new(duk, 9);     // [ global __Mat3 result ]
  duk_remove(duk, -2); // [ global result ]
  duk_remove(duk, -2); // [ result ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void xiiTypeScriptBinding::SetMat3(duk_context* pDuk, xiiInt32 iObjIdx, const xiiMat3& value)
{
  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");
  duk.SetNumberProperty("0", value.m_fElementsCM[0], -1);
  duk.SetNumberProperty("1", value.m_fElementsCM[1], -1);
  duk.SetNumberProperty("2", value.m_fElementsCM[2], -1);
  duk.SetNumberProperty("3", value.m_fElementsCM[3], -1);
  duk.SetNumberProperty("4", value.m_fElementsCM[4], -1);
  duk.SetNumberProperty("5", value.m_fElementsCM[5], -1);
  duk.SetNumberProperty("6", value.m_fElementsCM[6], -1);
  duk.SetNumberProperty("7", value.m_fElementsCM[7], -1);
  duk.SetNumberProperty("8", value.m_fElementsCM[8], -1);
  duk.PopStack();

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void xiiTypeScriptBinding::SetMat3Property(duk_context* pDuk, const char* szPropertyName, xiiInt32 iObjIdx, const xiiMat3& value)
{
  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetMat3(pDuk, -1, value);
  duk.PopStack();

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

xiiMat3 xiiTypeScriptBinding::GetMat3(duk_context* pDuk, xiiInt32 iObjIdx, const xiiMat3& fallback /*= xiiMat3::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  xiiMat3 res;

  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");

  res.m_fElementsCM[0] = duk.GetFloatProperty("0", 0.0f);
  res.m_fElementsCM[1] = duk.GetFloatProperty("1", 0.0f);
  res.m_fElementsCM[2] = duk.GetFloatProperty("2", 0.0f);
  res.m_fElementsCM[3] = duk.GetFloatProperty("3", 0.0f);
  res.m_fElementsCM[4] = duk.GetFloatProperty("4", 0.0f);
  res.m_fElementsCM[5] = duk.GetFloatProperty("5", 0.0f);
  res.m_fElementsCM[6] = duk.GetFloatProperty("6", 0.0f);
  res.m_fElementsCM[7] = duk.GetFloatProperty("7", 0.0f);
  res.m_fElementsCM[8] = duk.GetFloatProperty("8", 0.0f);

  duk.PopStack();

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

xiiMat3 xiiTypeScriptBinding::GetMat3Property(
  duk_context*   pDuk,
  const char*    szPropertyName,
  xiiInt32       iObjIdx,
  const xiiMat3& fallback /*= xiiMat3::ZeroVector()*/)
{
  xiiDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const xiiMat3 res = GetMat3(pDuk, -1, fallback);
  duk.PopStack(); // [ ]

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void xiiTypeScriptBinding::PushMat4(duk_context* pDuk, const xiiMat4& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                    // [ global ]
  XII_VERIFY(duk.PushLocalObject("__Mat4").Succeeded(), ""); // [ global __Mat4 ]
  duk_get_prop_string(duk, -1, "Mat4");                      // [ global __Mat4 Mat4 ]

  float rm[16];
  value.GetAsArray(rm, xiiMatrixLayout::RowMajor);

  for (xiiUInt32 i = 0; i < 16; ++i)
  {
    duk_push_number(duk, rm[i]); // [ global __Mat4 Mat4 16params ]
  }

  duk_new(duk, 16);    // [ global __Mat4 result ]
  duk_remove(duk, -2); // [ global result ]
  duk_remove(duk, -2); // [ result ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void xiiTypeScriptBinding::SetMat4(duk_context* pDuk, xiiInt32 iObjIdx, const xiiMat4& value)
{
  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");
  duk.SetNumberProperty("0", value.m_fElementsCM[0], -1);
  duk.SetNumberProperty("1", value.m_fElementsCM[1], -1);
  duk.SetNumberProperty("2", value.m_fElementsCM[2], -1);
  duk.SetNumberProperty("3", value.m_fElementsCM[3], -1);
  duk.SetNumberProperty("4", value.m_fElementsCM[4], -1);
  duk.SetNumberProperty("5", value.m_fElementsCM[5], -1);
  duk.SetNumberProperty("6", value.m_fElementsCM[6], -1);
  duk.SetNumberProperty("7", value.m_fElementsCM[7], -1);
  duk.SetNumberProperty("8", value.m_fElementsCM[8], -1);
  duk.SetNumberProperty("9", value.m_fElementsCM[9], -1);
  duk.SetNumberProperty("10", value.m_fElementsCM[10], -1);
  duk.SetNumberProperty("11", value.m_fElementsCM[11], -1);
  duk.SetNumberProperty("12", value.m_fElementsCM[12], -1);
  duk.SetNumberProperty("13", value.m_fElementsCM[13], -1);
  duk.SetNumberProperty("14", value.m_fElementsCM[14], -1);
  duk.SetNumberProperty("15", value.m_fElementsCM[15], -1);
  duk.PopStack();

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void xiiTypeScriptBinding::SetMat4Property(duk_context* pDuk, const char* szPropertyName, xiiInt32 iObjIdx, const xiiMat4& value)
{
  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetMat4(pDuk, -1, value);
  duk.PopStack();

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

xiiMat4 xiiTypeScriptBinding::GetMat4(duk_context* pDuk, xiiInt32 iObjIdx, const xiiMat4& fallback /*= xiiMat4::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  xiiMat4 res;

  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");

  res.m_fElementsCM[0]  = duk.GetFloatProperty("0", 0.0f);
  res.m_fElementsCM[1]  = duk.GetFloatProperty("1", 0.0f);
  res.m_fElementsCM[2]  = duk.GetFloatProperty("2", 0.0f);
  res.m_fElementsCM[3]  = duk.GetFloatProperty("3", 0.0f);
  res.m_fElementsCM[4]  = duk.GetFloatProperty("4", 0.0f);
  res.m_fElementsCM[5]  = duk.GetFloatProperty("5", 0.0f);
  res.m_fElementsCM[6]  = duk.GetFloatProperty("6", 0.0f);
  res.m_fElementsCM[7]  = duk.GetFloatProperty("7", 0.0f);
  res.m_fElementsCM[8]  = duk.GetFloatProperty("8", 0.0f);
  res.m_fElementsCM[9]  = duk.GetFloatProperty("9", 0.0f);
  res.m_fElementsCM[10] = duk.GetFloatProperty("10", 0.0f);
  res.m_fElementsCM[11] = duk.GetFloatProperty("11", 0.0f);
  res.m_fElementsCM[12] = duk.GetFloatProperty("12", 0.0f);
  res.m_fElementsCM[13] = duk.GetFloatProperty("13", 0.0f);
  res.m_fElementsCM[14] = duk.GetFloatProperty("14", 0.0f);
  res.m_fElementsCM[15] = duk.GetFloatProperty("15", 0.0f);

  duk.PopStack();

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

xiiMat4 xiiTypeScriptBinding::GetMat4Property(
  duk_context*   pDuk,
  const char*    szPropertyName,
  xiiInt32       iObjIdx,
  const xiiMat4& fallback /*= xiiMat4::ZeroVector()*/)
{
  xiiDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const xiiMat4 res = GetMat4(pDuk, -1, fallback);
  duk.PopStack(); // [ ]

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void xiiTypeScriptBinding::PushQuat(duk_context* pDuk, const xiiQuat& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                    // [ global ]
  XII_VERIFY(duk.PushLocalObject("__Quat").Succeeded(), ""); // [ global __Quat ]
  duk_get_prop_string(duk, -1, "Quat");                      // [ global __Quat Quat ]
  duk_push_number(duk, value.v.x);                           // [ global __Quat Quat x ]
  duk_push_number(duk, value.v.y);                           // [ global __Quat Quat x y ]
  duk_push_number(duk, value.v.z);                           // [ global __Quat Quat x y z ]
  duk_push_number(duk, value.w);                             // [ global __Quat Quat x y z w ]
  duk_new(duk, 4);                                           // [ global __Quat result ]
  duk_remove(duk, -2);                                       // [ global result ]
  duk_remove(duk, -2);                                       // [ result ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void xiiTypeScriptBinding::SetQuat(duk_context* pDuk, xiiInt32 iObjIdx, const xiiQuat& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("x", value.v.x, iObjIdx);
  duk.SetNumberProperty("y", value.v.y, iObjIdx);
  duk.SetNumberProperty("z", value.v.z, iObjIdx);
  duk.SetNumberProperty("w", value.w, iObjIdx);
}

void xiiTypeScriptBinding::SetQuatProperty(duk_context* pDuk, const char* szPropertyName, xiiInt32 iObjIdx, const xiiQuat& value)
{
  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetQuat(pDuk, -1, value);
  duk.PopStack();

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

xiiQuat xiiTypeScriptBinding::GetQuat(duk_context* pDuk, xiiInt32 iObjIdx, xiiQuat fallback /*= xiiQuat::IdentityQuaternion()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  xiiQuat res;

  XII_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.v.x = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.v.x));
  duk_pop(pDuk);
  XII_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.v.y = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.v.y));
  duk_pop(pDuk);
  XII_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "z"), "");
  res.v.z = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.v.z));
  duk_pop(pDuk);
  XII_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "w"), "");
  res.w = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.w));
  duk_pop(pDuk);

  return res;
}

xiiQuat xiiTypeScriptBinding::GetQuatProperty(
  duk_context* pDuk,
  const char*  szPropertyName,
  xiiInt32     iObjIdx,
  xiiQuat      fallback /*= xiiQuat::IdentityQuaternion()*/)
{
  xiiDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const xiiQuat res = GetQuat(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void xiiTypeScriptBinding::PushColor(duk_context* pDuk, const xiiColor& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                     // [ global ]
  XII_VERIFY(duk.PushLocalObject("__Color").Succeeded(), ""); // [ global __Color ]
  duk_get_prop_string(duk, -1, "Color");                      // [ global __Color Color ]
  duk_push_number(duk, value.r);                              // [ global __Color Color r ]
  duk_push_number(duk, value.g);                              // [ global __Color Color r g ]
  duk_push_number(duk, value.b);                              // [ global __Color Color r g b ]
  duk_push_number(duk, value.a);                              // [ global __Color Color r g b a ]
  duk_new(duk, 4);                                            // [ global __Color result ]
  duk_remove(duk, -2);                                        // [ global result ]
  duk_remove(duk, -2);                                        // [ result ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void xiiTypeScriptBinding::SetColor(duk_context* pDuk, xiiInt32 iObjIdx, const xiiColor& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("r", value.r, iObjIdx);
  duk.SetNumberProperty("g", value.g, iObjIdx);
  duk.SetNumberProperty("b", value.b, iObjIdx);
  duk.SetNumberProperty("a", value.a, iObjIdx);

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void xiiTypeScriptBinding::SetColorProperty(duk_context* pDuk, const char* szPropertyName, xiiInt32 iObjIdx, const xiiColor& value)
{
  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetColor(pDuk, -1, value);
  duk.PopStack();

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

xiiColor xiiTypeScriptBinding::GetColor(duk_context* pDuk, xiiInt32 iObjIdx, const xiiColor& fallback /*= xiiColor::White*/)
{
  xiiDuktapeHelper duk(pDuk);

  xiiColor res;
  res.r = duk.GetFloatProperty("r", fallback.r, iObjIdx);
  res.g = duk.GetFloatProperty("g", fallback.g, iObjIdx);
  res.b = duk.GetFloatProperty("b", fallback.b, iObjIdx);
  res.a = duk.GetFloatProperty("a", fallback.a, iObjIdx);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

xiiColor xiiTypeScriptBinding::GetColorProperty(
  duk_context*    pDuk,
  const char*     szPropertyName,
  xiiInt32        iObjIdx,
  const xiiColor& fallback /*= xiiColor::White*/)
{
  xiiDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const xiiColor res = GetColor(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void xiiTypeScriptBinding::PushTransform(duk_context* pDuk, const xiiTransform& value)
{
  xiiDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                         // [ global ]
  XII_VERIFY(duk.PushLocalObject("__Transform").Succeeded(), ""); // [ global __Transform ]
  duk_get_prop_string(duk, -1, "Transform");                      // [ global __Transform Transform ]
  duk_new(duk, 0);                                                // [ global __Transform object ]
  SetVec3Property(pDuk, "position", -1, value.m_vPosition);       // [ global __Transform object ]
  SetQuatProperty(pDuk, "rotation", -1, value.m_qRotation);       // [ global __Transform object ]
  SetVec3Property(pDuk, "scale", -1, value.m_vScale);             // [ global __Transform object ]
  duk_remove(duk, -2);                                            // [ global object ]
  duk_remove(duk, -2);                                            // [ object ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void xiiTypeScriptBinding::SetTransform(duk_context* pDuk, xiiInt32 iObjIdx, const xiiTransform& value)
{
  SetVec3Property(pDuk, "position", iObjIdx, value.m_vPosition);
  SetQuatProperty(pDuk, "rotation", iObjIdx, value.m_qRotation);
  SetVec3Property(pDuk, "scale", iObjIdx, value.m_vScale);
}

void xiiTypeScriptBinding::SetTransformProperty(duk_context* pDuk, const char* szPropertyName, xiiInt32 iObjIdx, const xiiTransform& value)
{
  xiiDuktapeHelper duk(pDuk);

  XII_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetTransform(pDuk, -1, value);
  duk.PopStack();

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

xiiTransform xiiTypeScriptBinding::GetTransform(duk_context* pDuk, xiiInt32 iObjIdx, const xiiTransform& fallback /*= xiiTransform::IdentityTransform()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  xiiTransform res;

  res.m_vPosition = GetVec3Property(pDuk, "position", iObjIdx, fallback.m_vPosition);
  res.m_qRotation = GetQuatProperty(pDuk, "rotation", iObjIdx, fallback.m_qRotation);
  res.m_vScale    = GetVec3Property(pDuk, "scale", iObjIdx, fallback.m_vScale);

  return res;
}

xiiTransform xiiTypeScriptBinding::GetTransformProperty(
  duk_context*        pDuk,
  const char*         szPropertyName,
  xiiInt32            iObjIdx,
  const xiiTransform& fallback /*= xiiTransform::IdentityTransform()*/)
{
  xiiDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const xiiTransform res = GetTransform(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void xiiTypeScriptBinding::PushVariant(duk_context* pDuk, const xiiVariant& value)
{
  xiiDuktapeHelper duk(pDuk);

  switch (value.GetType())
  {
    case xiiVariant::Type::Angle:
      duk.PushNumber(value.Get<xiiAngle>().GetRadian());
      break;

    case xiiVariant::Type::Time:
      duk.PushNumber(value.Get<xiiTime>().GetSeconds());
      break;

    case xiiVariant::Type::Bool:
      duk.PushBool(value.Get<bool>());
      break;

    case xiiVariant::Type::Int8:
    case xiiVariant::Type::UInt8:
    case xiiVariant::Type::Int16:
    case xiiVariant::Type::UInt16:
    case xiiVariant::Type::Int32:
    case xiiVariant::Type::UInt32:
    case xiiVariant::Type::Int64:
    case xiiVariant::Type::UInt64:
    case xiiVariant::Type::Float:
    case xiiVariant::Type::Double:
      duk.PushNumber(value.ConvertTo<double>());
      break;

    case xiiVariant::Type::Color:
    case xiiVariant::Type::ColorGamma:
      PushColor(duk, value.ConvertTo<xiiColor>());
      break;

    case xiiVariant::Type::Vector2:
      PushVec2(duk, value.Get<xiiVec2>());
      break;

    case xiiVariant::Type::Vector3:
      PushVec3(duk, value.Get<xiiVec3>());
      break;

    case xiiVariant::Type::Quaternion:
      PushQuat(duk, value.Get<xiiQuat>());
      break;

    case xiiVariant::Type::Transform:
      PushTransform(duk, value.Get<xiiTransform>());
      break;

    case xiiVariant::Type::String:
      duk.PushString(value.Get<xiiString>());
      break;

    case xiiVariant::Type::StringView:
      duk.PushString(value.Get<xiiStringView>());
      break;

    case xiiVariant::Type::Vector2I:
    {
      const xiiVec2I32 v = value.Get<xiiVec2I32>();
      PushVec2(duk, xiiVec2(static_cast<float>(v.x), static_cast<float>(v.y)));
      break;
    }

    case xiiVariant::Type::Vector2U:
    {
      const xiiVec2U32 v = value.Get<xiiVec2U32>();
      PushVec2(duk, xiiVec2(static_cast<float>(v.x), static_cast<float>(v.y)));
      break;
    }

    case xiiVariant::Type::Vector3I:
    {
      const xiiVec3I32 v = value.Get<xiiVec3I32>();
      PushVec3(duk, xiiVec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)));
      break;
    }

    case xiiVariant::Type::Vector3U:
    {
      const xiiVec3U32 v = value.Get<xiiVec3U32>();
      PushVec3(duk, xiiVec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)));
      break;
    }

    case xiiVariant::Type::Matrix3:
      PushMat3(duk, value.Get<xiiMat3>());
      break;

    case xiiVariant::Type::Matrix4:
      PushMat4(duk, value.Get<xiiMat4>());
      break;

      // TODO: implement these types
      // case xiiVariant::Type::Vector4:
      // case xiiVariant::Type::Vector4I:
      // case xiiVariant::Type::Vector4U:

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      duk.PushUndefined();
      break;
  }

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void xiiTypeScriptBinding::SetVariantProperty(duk_context* pDuk, const char* szPropertyName, xiiInt32 iObjIdx, const xiiVariant& value)
{
  xiiDuktapeHelper duk(pDuk);

  PushVariant(pDuk, value);
  duk.SetCustomProperty(szPropertyName, iObjIdx);

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

xiiVariant xiiTypeScriptBinding::GetVariant(duk_context* pDuk, xiiInt32 iObjIdx, const xiiRTTI* pType)
{
  xiiDuktapeHelper duk(pDuk);

  if (pType->IsDerivedFrom<xiiEnumBase>() || pType->IsDerivedFrom<xiiBitflagsBase>())
  {
    return duk.GetIntValue(iObjIdx);
  }

  switch (pType->GetVariantType())
  {
    case xiiVariant::Type::Invalid:
    {
      if (xiiStringUtils::IsEqual(pType->GetTypeName(), "xiiVariant"))
      {
        switch (duk_get_type(duk.GetContext(), iObjIdx))
        {
          case DUK_TYPE_BOOLEAN:
            return duk.GetBoolValue(iObjIdx);
          case DUK_TYPE_NUMBER:
            return duk.GetFloatValue(iObjIdx);
          case DUK_TYPE_STRING:
            return duk.GetStringValue(iObjIdx);

          default:
            return xiiVariant();
        }
      }

      return xiiVariant();
    }

    case xiiVariant::Type::Bool:
      return duk.GetBoolValue(iObjIdx);

    case xiiVariant::Type::Angle:
      return xiiAngle::Radian(duk.GetFloatValue(iObjIdx));

    case xiiVariant::Type::Time:
      return xiiTime::Seconds(duk.GetFloatValue(iObjIdx));

    case xiiVariant::Type::Int8:
    case xiiVariant::Type::Int16:
    case xiiVariant::Type::Int32:
    case xiiVariant::Type::Int64:
      return duk.GetIntValue(iObjIdx);

    case xiiVariant::Type::UInt8:
    case xiiVariant::Type::UInt16:
    case xiiVariant::Type::UInt32:
    case xiiVariant::Type::UInt64:
      return duk.GetUIntValue(iObjIdx);

    case xiiVariant::Type::Float:
      return duk.GetFloatValue(iObjIdx);

    case xiiVariant::Type::Double:
      return duk.GetNumberValue(iObjIdx);

    case xiiVariant::Type::String:
      return duk.GetStringValue(iObjIdx);

    case xiiVariant::Type::StringView:
      return xiiStringView(duk.GetStringValue(iObjIdx));

    case xiiVariant::Type::Vector2:
      return xiiTypeScriptBinding::GetVec2(duk, iObjIdx);

    case xiiVariant::Type::Vector3:
      return xiiTypeScriptBinding::GetVec3(duk, iObjIdx);

    case xiiVariant::Type::Quaternion:
      return xiiTypeScriptBinding::GetQuat(duk, iObjIdx);

    case xiiVariant::Type::Transform:
      return xiiTypeScriptBinding::GetTransform(duk, iObjIdx);

    case xiiVariant::Type::Color:
      return xiiTypeScriptBinding::GetColor(duk, iObjIdx);

    case xiiVariant::Type::ColorGamma:
      return xiiColorGammaUB(xiiTypeScriptBinding::GetColor(duk, iObjIdx));

    case xiiVariant::Type::Vector2I:
    {
      const xiiVec2 v = xiiTypeScriptBinding::GetVec2(duk, iObjIdx);
      return xiiVec2I32(static_cast<xiiInt32>(v.x), static_cast<xiiInt32>(v.y));
    }

    case xiiVariant::Type::Vector3I:
    {
      const xiiVec3 v = xiiTypeScriptBinding::GetVec3(duk, iObjIdx);
      return xiiVec3I32(static_cast<xiiInt32>(v.x), static_cast<xiiInt32>(v.y), static_cast<xiiInt32>(v.z));
    }

    case xiiVariant::Type::Vector2U:
    {
      const xiiVec2 v = xiiTypeScriptBinding::GetVec2(duk, iObjIdx);
      return xiiVec2U32(static_cast<xiiUInt32>(v.x), static_cast<xiiUInt32>(v.y));
    }

    case xiiVariant::Type::Vector3U:
    {
      const xiiVec3 v = xiiTypeScriptBinding::GetVec3(duk, iObjIdx);
      return xiiVec3U32(static_cast<xiiUInt32>(v.x), static_cast<xiiUInt32>(v.y), static_cast<xiiUInt32>(v.z));
    }

    case xiiVariant::Type::Matrix3:
      return xiiTypeScriptBinding::GetMat3(duk, iObjIdx);

    case xiiVariant::Type::Matrix4:
      return xiiTypeScriptBinding::GetMat4(duk, iObjIdx);

      // case xiiVariant::Type::Vector4:
      //  break;
      // case xiiVariant::Type::Vector4I:
      //  break;
      // case xiiVariant::Type::Vector4U:
      //  break;

      // case xiiVariant::Type::Uuid:
      //  break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return xiiVariant();
}

xiiVariant xiiTypeScriptBinding::GetVariantProperty(duk_context* pDuk, const char* szPropertyName, xiiInt32 iObjIdx, const xiiRTTI* pType)
{
  xiiDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    return xiiVariant();
  }

  const xiiVariant res = GetVariant(pDuk, -1, pType);
  duk.PopStack(); // [ ]

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}
