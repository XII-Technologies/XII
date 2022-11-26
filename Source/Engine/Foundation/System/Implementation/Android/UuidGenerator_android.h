#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Android/AndroidJni.h>
#include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#include <android_native_app_glue.h>

void xiiUuid::CreateNewUuid()
{
  xiiJniAttachment attachment;

  xiiJniClass uuidClass("java/util/UUID");
  XII_ASSERT_DEBUG(!uuidClass.IsNull(), "UUID class not found.");
  xiiJniObject javaUuid         = uuidClass.CallStatic<xiiJniObject>("randomUUID");
  jlong        mostSignificant  = javaUuid.Call<jlong>("getMostSignificantBits");
  jlong        leastSignificant = javaUuid.Call<jlong>("getLeastSignificantBits");

  m_uiHigh = mostSignificant;
  m_uiLow  = leastSignificant;

  //#TODO maybe faster to read /proc/sys/kernel/random/uuid, but that can't be done via xiiOSFile
  // see https://stackoverflow.com/questions/11888055/include-uuid-h-into-android-ndk-project
}
