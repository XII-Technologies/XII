#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidJni.h>
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

thread_local JNIEnv*          xiiJniAttachment::s_env;
thread_local bool             xiiJniAttachment::s_ownsEnv;
thread_local int              xiiJniAttachment::s_attachCount;
thread_local xiiJniErrorState xiiJniAttachment::s_lastError;

xiiJniAttachment::xiiJniAttachment()
{
  if (s_attachCount > 0)
  {
    s_env->PushLocalFrame(16);
  }
  else
  {
    JNIEnv* env       = nullptr;
    jint    envStatus = xiiAndroidUtils::GetAndroidJavaVM()->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    bool    ownsEnv   = (envStatus != JNI_OK);
    if (ownsEnv)
    {
      // Assign name to attachment since ART complains about it not being set.
      JavaVMAttachArgs args = {JNI_VERSION_1_6, "XII JNI", nullptr};
      xiiAndroidUtils::GetAndroidJavaVM()->AttachCurrentThread(&env, &args);
    }
    else
    {
      // Assume already existing JNI environment will be alive as long as this object exists.
      XII_ASSERT_DEV(env != nullptr, "");
      env->PushLocalFrame(16);
    }

    s_env     = env;
    s_ownsEnv = ownsEnv;
  }

  s_attachCount++;
}

xiiJniAttachment::~xiiJniAttachment()
{
  s_attachCount--;

  if (s_attachCount == 0)
  {
    ClearLastError();

    if (s_ownsEnv)
    {
      xiiAndroidUtils::GetAndroidJavaVM()->DetachCurrentThread();
    }
    else
    {
      s_env->PopLocalFrame(nullptr);
    }

    s_env     = nullptr;
    s_ownsEnv = false;
  }
  else
  {
    s_env->PopLocalFrame(nullptr);
  }
}

xiiJniObject xiiJniAttachment::GetActivity()
{
  return xiiJniObject(xiiAndroidUtils::GetNativeAndroidApp()->activity->clazz, xiiJniOwnerShip::BORROW);
}

JNIEnv* xiiJniAttachment::GetEnv()
{
  XII_ASSERT_DEV(s_env != nullptr, "Thread not attached to the JVM - you forgot to create an instance of xiiJniAttachment in the current scope.");

#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  void* unused;
  XII_ASSERT_DEBUG(xiiAndroidUtils::GetAndroidJavaVM()->GetEnv(&unused, JNI_VERSION_1_6) == JNI_OK,
                   "Current thread has lost its attachment to the JVM - some OS calls can cause this to happen. Try to reduce the attachment to a smaller scope.");
#  endif

  return s_env;
}

xiiJniErrorState xiiJniAttachment::GetLastError()
{
  xiiJniErrorState state = s_lastError;
  return state;
}

void xiiJniAttachment::ClearLastError()
{
  s_lastError = xiiJniErrorState::SUCCESS;
}

void xiiJniAttachment::SetLastError(xiiJniErrorState state)
{
  s_lastError = state;
}

bool xiiJniAttachment::HasPendingException()
{
  return GetEnv()->ExceptionCheck();
}

void xiiJniAttachment::ClearPendingException()
{
  return GetEnv()->ExceptionClear();
}

xiiJniObject xiiJniAttachment::GetPendingException()
{
  return xiiJniObject(GetEnv()->ExceptionOccurred(), xiiJniOwnerShip::OWN);
}

bool xiiJniAttachment::FailOnPendingErrorOrException()
{
  if (xiiJniAttachment::GetLastError() != xiiJniErrorState::SUCCESS)
  {
    xiiLog::Error("Aborting call because the previous error state was not cleared.");
    return true;
  }

  if (xiiJniAttachment::HasPendingException())
  {
    xiiLog::Error("Aborting call because a Java exception is still pending.");
    xiiJniAttachment::SetLastError(xiiJniErrorState::PENDING_EXCEPTION);
    return true;
  }

  return false;
}

void xiiJniObject::DumpTypes(const xiiJniClass* inputTypes, int N, const xiiJniClass* returnType)
{
  if (returnType != nullptr)
  {
    xiiLog::Error("  With requested return type '{}'", returnType->ToString().GetData());
  }

  for (int paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    xiiLog::Error("  With passed param type #{} '{}'", paramIdx, inputTypes[paramIdx].IsNull() ? "(null)" : inputTypes[paramIdx].ToString().GetData());
  }
}

int xiiJniObject::CompareMethodSpecificity(const xiiJniObject& method1, const xiiJniObject& method2)
{
  xiiJniClass returnType1 = method1.UnsafeCall<xiiJniClass>("getReturnType", "()Ljava/lang/Class;");
  xiiJniClass returnType2 = method2.UnsafeCall<xiiJniClass>("getReturnType", "()Ljava/lang/Class;");

  xiiJniObject paramTypes1 = method1.UnsafeCall<xiiJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  xiiJniObject paramTypes2 = method2.UnsafeCall<xiiJniObject>("getParameterTypes", "()[Ljava/lang/Class;");

  jsize N = xiiJniAttachment::GetEnv()->GetArrayLength(jarray(paramTypes1.m_object));

  int decision = returnType1.IsAssignableFrom(returnType2) - returnType2.IsAssignableFrom(returnType1);

  for (jsize paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    xiiJniClass paramType1(
      jclass(xiiJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes1.m_object), paramIdx)), xiiJniOwnerShip::OWN);
    xiiJniClass paramType2(
      jclass(xiiJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes2.m_object), paramIdx)), xiiJniOwnerShip::OWN);

    int paramDecision = paramType1.IsAssignableFrom(paramType2) - paramType2.IsAssignableFrom(paramType1);

    if (decision == 0)
    {
      // No method is more specific yet
      decision = paramDecision;
    }
    else if (paramDecision != 0 && decision != paramDecision)
    {
      // There is no clear specificity ordering - one type is more specific, but the other less so
      return 0;
    }
  }

  return decision;
}

bool xiiJniObject::IsMethodViable(bool bStatic, const xiiJniObject& candidateMethod, const xiiJniClass& returnType, xiiJniClass* inputTypes, int N)
{
  // Check if staticness matches
  if (xiiJniClass("java/lang/reflect/Modifier").UnsafeCallStatic<bool>("isStatic", "(I)Z", candidateMethod.UnsafeCall<int>("getModifiers", "()I")) !=
      bStatic)
  {
    return false;
  }

  // Check if return type is assignable to the requested type
  xiiJniClass candidateReturnType = candidateMethod.UnsafeCall<xiiJniClass>("getReturnType", "()Ljava/lang/Class;");
  if (!returnType.IsAssignableFrom(candidateReturnType))
  {
    return false;
  }

  // Check number of parameters
  xiiJniObject parameterTypes     = candidateMethod.UnsafeCall<xiiJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  jsize        numCandidateParams = xiiJniAttachment::GetEnv()->GetArrayLength(jarray(parameterTypes.m_object));
  if (numCandidateParams != N)
  {
    return false;
  }

  // Check if input parameter types are assignable to the actual parameter types
  for (jsize paramIdx = 0; paramIdx < numCandidateParams; ++paramIdx)
  {
    xiiJniClass paramType(
      jclass(xiiJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(parameterTypes.m_object), paramIdx)), xiiJniOwnerShip::OWN);

    if (inputTypes[paramIdx].IsNull())
    {
      if (paramType.IsPrimitive())
      {
        return false;
      }
    }
    else
    {
      if (!paramType.IsAssignableFrom(inputTypes[paramIdx]))
      {
        return false;
      }
    }
  }

  return true;
}

xiiJniObject xiiJniObject::FindMethod(
  bool               bStatic,
  const char*        name,
  const xiiJniClass& searchClass,
  const xiiJniClass& returnType,
  xiiJniClass*       inputTypes,
  int                N)
{
  if (searchClass.IsNull())
  {
    xiiLog::Error("Attempting to find constructor for null type.");
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return xiiJniObject();
  }

  xiiHybridArray<xiiJniObject, 32> bestCandidates;

  // In case of no parameters, fetch the method directly.
  if (N == 0)
  {
    xiiJniObject candidateMethod = searchClass.UnsafeCall<xiiJniObject>(
      "getMethod", "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;", xiiJniString(name), xiiJniObject());

    if (!xiiJniAttachment::GetEnv()->ExceptionCheck() && IsMethodViable(bStatic, candidateMethod, returnType, inputTypes, N))
    {
      bestCandidates.PushBack(candidateMethod);
    }
    else
    {
      xiiJniAttachment::GetEnv()->ExceptionClear();
    }
  }
  else
  {
    // For methods with parameters, loop over all methods to find one with the correct name and matching parameter types

    xiiJniObject methodArray = searchClass.UnsafeCall<xiiJniObject>("getMethods", "()[Ljava/lang/reflect/Method;");

    jsize numMethods = xiiJniAttachment::GetEnv()->GetArrayLength(jarray(methodArray.m_object));
    for (jsize methodIdx = 0; methodIdx < numMethods; ++methodIdx)
    {
      xiiJniObject candidateMethod(
        xiiJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(methodArray.m_object), methodIdx), xiiJniOwnerShip::OWN);

      xiiJniString methodName = candidateMethod.UnsafeCall<xiiJniString>("getName", "()Ljava/lang/String;");

      if (strcmp(name, methodName.GetData()) != 0)
      {
        continue;
      }

      if (!IsMethodViable(bStatic, candidateMethod, returnType, inputTypes, N))
      {
        continue;
      }

      bool isMoreSpecific = true;
      for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
      {
        int comparison = CompareMethodSpecificity(bestCandidates[candidateIdx], candidateMethod);

        if (comparison == 1)
        {
          // Remove less specific candidate and continue looping
          bestCandidates.RemoveAtAndSwap(candidateIdx);
          candidateIdx--;
        }
        else if (comparison == -1)
        {
          // We're less specific, so by transitivity there are no other methods less specific than ours that we could throw out,
          // and we can abort the loop
          isMoreSpecific = false;
          break;
        }
        else
        {
          // No relation, so do nothing
        }
      }

      if (isMoreSpecific)
      {
        bestCandidates.PushBack(candidateMethod);
      }
    }
  }

  if (bestCandidates.GetCount() == 1)
  {
    return bestCandidates[0];
  }
  else if (bestCandidates.GetCount() == 0)
  {
    xiiLog::Error("Overload resolution failed: No method '{}' in class '{}' matches the requested return and parameter types.", name,
                  searchClass.ToString().GetData());
    DumpTypes(inputTypes, N, &returnType);
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_METHOD);
    return xiiJniObject();
  }
  else
  {
    xiiLog::Error("Overload resolution failed: Call to '{}' in class '{}' is ambiguous. Cannot decide between the following candidates:", name,
                  searchClass.ToString().GetData());
    for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
    {
      xiiLog::Error("  Candidate #{}: '{}'", candidateIdx, bestCandidates[candidateIdx].ToString().GetData());
    }
    DumpTypes(inputTypes, N, &returnType);
    xiiJniAttachment::SetLastError(xiiJniErrorState::AMBIGUOUS_CALL);
    return xiiJniObject();
  }
}

int xiiJniObject::CompareConstructorSpecificity(const xiiJniObject& method1, const xiiJniObject& method2)
{
  xiiJniObject paramTypes1 = method1.UnsafeCall<xiiJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  xiiJniObject paramTypes2 = method2.UnsafeCall<xiiJniObject>("getParameterTypes", "()[Ljava/lang/Class;");

  jsize N = xiiJniAttachment::GetEnv()->GetArrayLength(jarray(paramTypes1.m_object));

  int decision = 0;

  for (jsize paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    xiiJniClass paramType1(
      jclass(xiiJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes1.m_object), paramIdx)), xiiJniOwnerShip::OWN);
    xiiJniClass paramType2(
      jclass(xiiJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes2.m_object), paramIdx)), xiiJniOwnerShip::OWN);

    int paramDecision = paramType1.IsAssignableFrom(paramType2) - paramType2.IsAssignableFrom(paramType1);

    if (decision == 0)
    {
      // No method is more specific yet
      decision = paramDecision;
    }
    else if (paramDecision != 0 && decision != paramDecision)
    {
      // There is no clear specificity ordering - one type is more specific, but the other less so
      return 0;
    }
  }

  return decision;
}

bool xiiJniObject::IsConstructorViable(const xiiJniObject& candidateMethod, xiiJniClass* inputTypes, int N)
{
  // Check number of parameters
  xiiJniObject parameterTypes     = candidateMethod.UnsafeCall<xiiJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  jsize        numCandidateParams = xiiJniAttachment::GetEnv()->GetArrayLength(jarray(parameterTypes.m_object));
  if (numCandidateParams != N)
  {
    return false;
  }

  // Check if input parameter types are assignable to the actual parameter types
  for (jsize paramIdx = 0; paramIdx < numCandidateParams; ++paramIdx)
  {
    xiiJniClass paramType(
      jclass(xiiJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(parameterTypes.m_object), paramIdx)), xiiJniOwnerShip::OWN);

    if (inputTypes[paramIdx].IsNull())
    {
      if (paramType.IsPrimitive())
      {
        return false;
      }
    }
    else
    {
      if (!paramType.IsAssignableFrom(inputTypes[paramIdx]))
      {
        return false;
      }
    }
  }

  return true;
}

xiiJniObject xiiJniObject::FindConstructor(const xiiJniClass& type, xiiJniClass* inputTypes, int N)
{
  if (type.IsNull())
  {
    xiiLog::Error("Attempting to find constructor for null type.");
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return xiiJniObject();
  }

  xiiHybridArray<xiiJniObject, 32> bestCandidates;

  // In case of no parameters, fetch the method directly.
  if (N == 0)
  {
    xiiJniObject candidateMethod =
      type.UnsafeCall<xiiJniObject>("getConstructor", "([Ljava/lang/Class;)Ljava/lang/reflect/Constructor;", xiiJniObject());

    if (!xiiJniAttachment::GetEnv()->ExceptionCheck() && IsConstructorViable(candidateMethod, inputTypes, N))
    {
      bestCandidates.PushBack(candidateMethod);
    }
    else
    {
      xiiJniAttachment::GetEnv()->ExceptionClear();
    }
  }
  else
  {
    // For methods with parameters, loop over all methods to find one with the correct name and matching parameter types

    xiiJniObject methodArray = type.UnsafeCall<xiiJniObject>("getConstructors", "()[Ljava/lang/reflect/Constructor;");

    jsize numMethods = xiiJniAttachment::GetEnv()->GetArrayLength(jarray(methodArray.m_object));
    for (jsize methodIdx = 0; methodIdx < numMethods; ++methodIdx)
    {
      xiiJniObject candidateMethod(
        xiiJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(methodArray.m_object), methodIdx), xiiJniOwnerShip::OWN);

      if (!IsConstructorViable(candidateMethod, inputTypes, N))
      {
        continue;
      }

      bool isMoreSpecific = true;
      for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
      {
        int comparison = CompareConstructorSpecificity(bestCandidates[candidateIdx], candidateMethod);

        if (comparison == 1)
        {
          // Remove less specific candidate and continue looping
          bestCandidates.RemoveAtAndSwap(candidateIdx);
          candidateIdx--;
        }
        else if (comparison == -1)
        {
          // We're less specific, so by transitivity there are no other methods less specific than ours that we could throw out,
          // and we can abort the loop
          isMoreSpecific = false;
          break;
        }
        else
        {
          // No relation, so do nothing
        }
      }

      if (isMoreSpecific)
      {
        bestCandidates.PushBack(candidateMethod);
      }
    }
  }

  if (bestCandidates.GetCount() == 1)
  {
    return bestCandidates[0];
  }
  else if (bestCandidates.GetCount() == 0)
  {
    xiiLog::Error("Overload resolution failed: No constructor in class '{}' matches the requested parameter types.", type.ToString().GetData());
    DumpTypes(inputTypes, N, nullptr);
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_METHOD);
    return xiiJniObject();
  }
  else
  {
    xiiLog::Error("Overload resolution failed: Call to constructor in class '{}' is ambiguous. Cannot decide between the following candidates:",
                  type.ToString().GetData());
    for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
    {
      xiiLog::Error("  Candidate #{}: '{}'", candidateIdx, bestCandidates[candidateIdx].ToString().GetData());
    }
    DumpTypes(inputTypes, N, nullptr);
    xiiJniAttachment::SetLastError(xiiJniErrorState::AMBIGUOUS_CALL);
    return xiiJniObject();
  }
}

xiiJniObject::xiiJniObject() :
  m_object(nullptr), m_class(nullptr), m_own(false)
{
}

jobject xiiJniObject::GetHandle() const
{
  return m_object;
}

xiiJniClass xiiJniObject::GetClass() const
{
  if (!m_object)
  {
    return xiiJniClass();
  }

  if (!m_class)
  {
    const_cast<xiiJniObject*>(this)->m_class = xiiJniAttachment::GetEnv()->GetObjectClass(m_object);
  }

  return xiiJniClass(m_class, xiiJniOwnerShip::BORROW);
}

xiiJniString xiiJniObject::ToString() const
{
  if (xiiJniAttachment::FailOnPendingErrorOrException())
  {
    return xiiJniString();
  }

  // Implement ToString without UnsafeCall, since UnsafeCall requires ToString for diagnostic output.
  if (IsNull())
  {
    xiiLog::Error("Attempting to call method 'toString' on null object.");
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return xiiJniString();
  }

  jmethodID method = xiiJniAttachment::GetEnv()->GetMethodID(jclass(GetClass().m_object), "toString", "()Ljava/lang/String;");
  XII_ASSERT_DEV(method, "Could not find JNI method toString()");

  return xiiJniTraits<xiiJniString>::CallInstanceMethod(m_object, method);
}

bool xiiJniObject::IsInstanceOf(const xiiJniClass& clazz) const
{
  if (IsNull())
  {
    return false;
  }

  return clazz.IsAssignableFrom(GetClass());
}

xiiJniString::xiiJniString() :
  xiiJniObject(), m_utf(nullptr)
{
}

xiiJniString::xiiJniString(const char* str) :
  xiiJniObject(xiiJniAttachment::GetEnv()->NewStringUTF(str), xiiJniOwnerShip::OWN), m_utf(nullptr)
{
}

xiiJniString::xiiJniString(jstring string, xiiJniOwnerShip ownerShip) :
  xiiJniObject(string, ownerShip), m_utf(nullptr)
{
}

xiiJniString::xiiJniString(const xiiJniString& other) :
  xiiJniObject(other), m_utf(nullptr)
{
}

xiiJniString::xiiJniString(xiiJniString&& other) :
  xiiJniObject(other), m_utf(nullptr)
{
  m_utf       = other.m_utf;
  other.m_utf = nullptr;
}

xiiJniString& xiiJniString::operator=(const xiiJniString& other)
{
  if (m_utf)
  {
    xiiJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }

  xiiJniObject::operator=(other);

  return *this;
}

xiiJniString& xiiJniString::operator=(xiiJniString&& other)
{
  if (m_utf)
  {
    xiiJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }

  xiiJniObject::operator=(other);

  m_utf       = other.m_utf;
  other.m_utf = nullptr;

  return *this;
}

xiiJniString::~xiiJniString()
{
  if (m_utf)
  {
    xiiJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }
}

const char* xiiJniString::GetData() const
{
  if (IsNull())
  {
    xiiLog::Error("Calling AsChar() on null Java String");
    return "<null>";
  }

  if (!m_utf)
  {
    const_cast<xiiJniString*>(this)->m_utf = xiiJniAttachment::GetEnv()->GetStringUTFChars(jstring(GetJObject()), nullptr);
  }

  return m_utf;
}


xiiJniClass::xiiJniClass() :
  xiiJniObject()
{
}

xiiJniClass::xiiJniClass(const char* className) :
  xiiJniObject(xiiJniAttachment::GetEnv()->FindClass(className), xiiJniOwnerShip::OWN)
{
  if (IsNull())
  {
    xiiLog::Error("Class '{}' not found.", className);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CLASS_NOT_FOUND);
  }
}

xiiJniClass::xiiJniClass(jclass clazz, xiiJniOwnerShip ownerShip) :
  xiiJniObject(clazz, ownerShip)
{
}

xiiJniClass::xiiJniClass(const xiiJniClass& other) :
  xiiJniObject(static_cast<const xiiJniObject&>(other))
{
}

xiiJniClass::xiiJniClass(xiiJniClass&& other) :
  xiiJniObject(other)
{
}

xiiJniClass& xiiJniClass::operator=(const xiiJniClass& other)
{
  xiiJniObject::operator=(other);
  return *this;
}

xiiJniClass& xiiJniClass::operator=(xiiJniClass&& other)
{
  xiiJniObject::operator=(other);
  return *this;
}

jclass xiiJniClass::GetHandle() const
{
  return static_cast<jclass>(GetJObject());
}

bool xiiJniClass::IsAssignableFrom(const xiiJniClass& other) const
{
  static bool checkedApiOrder = false;
  static bool reverseArgs     = false;

  JNIEnv* env = xiiJniAttachment::GetEnv();

  // Guard against JNI bug reversing order of arguments - fixed in
  // https://android.googlesource.com/platform/art/+/1268b742c8cff7318dc0b5b283cbaeabfe0725ba
  if (!checkedApiOrder)
  {
    xiiJniClass objectClass("java/lang/Object");
    xiiJniClass stringClass("java/lang/String");

    if (env->IsAssignableFrom(jclass(objectClass.GetJObject()), jclass(stringClass.GetJObject())))
    {
      reverseArgs = true;
    }
    checkedApiOrder = true;
  }

  if (!reverseArgs)
  {
    return env->IsAssignableFrom(jclass(other.GetJObject()), jclass(GetJObject()));
  }
  else
  {
    return env->IsAssignableFrom(jclass(GetJObject()), jclass(other.GetJObject()));
  }
}

bool xiiJniClass::IsPrimitive()
{
  return UnsafeCall<bool>("isPrimitive", "()Z");
}
#endif


XII_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Android_AndroidJni);
