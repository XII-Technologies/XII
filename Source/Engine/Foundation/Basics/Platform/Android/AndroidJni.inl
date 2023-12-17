struct xiiJniModifiers
{
  enum Enum
  {
    PUBLIC       = 1,
    PRIVATE      = 2,
    PROTECTED    = 4,
    STATIC       = 8,
    FINAL        = 16,
    SYNCHRONIZED = 32,
    VOLATILE     = 64,
    TRANSIENT    = 128,
    NATIVE       = 256,
    INTERFACE    = 512,
    ABSTRACT     = 1024,
    STRICT       = 2048,
  };
};

xiiJniObject::xiiJniObject(jobject object, xiiJniOwnerShip ownerShip) :
  m_class(nullptr)
{
  switch (ownerShip)
  {
    case xiiJniOwnerShip::OWN:
      m_object = object;
      m_own    = true;
      break;

    case xiiJniOwnerShip::COPY:
      m_object = xiiJniAttachment::GetEnv()->NewLocalRef(object);
      m_own    = true;
      break;

    case xiiJniOwnerShip::BORROW:
      m_object = object;
      m_own    = false;
      break;
  }
}

xiiJniObject::xiiJniObject(const xiiJniObject& other) :
  m_class(nullptr)
{
  m_object = xiiJniAttachment::GetEnv()->NewLocalRef(other.m_object);
  m_own    = true;
}

xiiJniObject::xiiJniObject(xiiJniObject&& other)
{
  m_object = other.m_object;
  m_class  = other.m_class;
  m_own    = other.m_own;

  other.m_object = nullptr;
  other.m_class  = nullptr;
  other.m_own    = false;
}

xiiJniObject& xiiJniObject::operator=(const xiiJniObject& other)
{
  if (this == &other)
    return *this;

  Reset();
  m_object = xiiJniAttachment::GetEnv()->NewLocalRef(other.m_object);
  m_own    = true;
  return *this;
}

xiiJniObject& xiiJniObject::operator=(xiiJniObject&& other)
{
  if (this == &other)
    return *this;

  Reset();

  m_object = other.m_object;
  m_class  = other.m_class;
  m_own    = other.m_own;

  other.m_object = nullptr;
  other.m_class  = nullptr;
  other.m_own    = false;

  return *this;
}

xiiJniObject::~xiiJniObject()
{
  Reset();
}

void xiiJniObject::Reset()
{
  if (m_object && m_own)
  {
    xiiJniAttachment::GetEnv()->DeleteLocalRef(m_object);
    m_object = nullptr;
    m_own    = false;
  }
  if (m_class)
  {
    xiiJniAttachment::GetEnv()->DeleteLocalRef(m_class);
    m_class = nullptr;
  }
}

jobject xiiJniObject::GetJObject() const
{
  return m_object;
}

bool xiiJniObject::operator==(const xiiJniObject& other) const
{
  return xiiJniAttachment::GetEnv()->IsSameObject(m_object, other.m_object) == JNI_TRUE;
}

// Template specializations to dispatch to the correct JNI method for each C++ type.
template <typename T, bool unused = false>
struct xiiJniTraits
{
  static_assert(unused, "The passed C++ type is not supported by the JNI wrapper. Arguments and returns types must be one of bool, signed char/jbyte, unsigned short/jchar, short/jshort, int/jint, long long/jlong, float/jfloat, double/jdouble, xiiJniObject, xiiJniString or xiiJniClass.");

  // Places the argument inside a jvalue union.
  static jvalue ToValue(T);

  // Retrieves the Java class static type of the argument. For primitives, this is not the boxed type, but the primitive type.
  static xiiJniClass GetStaticType();

  // Retrieves the Java class dynamic type of the argument. For primitives, this is not the boxed type, but the primitive type.
  static xiiJniClass GetRuntimeType(T);

  // Creates an invalid/null object to return in case of errors.
  static T GetEmptyObject();

  // Call an instance method with the return type.
  template <typename... Args>
  static T CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  // Call a static method with the return type.
  template <typename... Args>
  static T CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  // Sets/gets a field of the type.
  static void SetField(jobject self, jfieldID field, T);
  static T    GetField(jobject self, jfieldID field);

  // Sets/gets a static field of the type.
  static void SetStaticField(jclass clazz, jfieldID field, T);
  static T    GetStaticField(jclass clazz, jfieldID field);

  // Appends the JNI type signature of this type to the string buf
  static bool        AppendSignature(const T& obj, xiiStringBuilder& str);
  static const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<bool>
{
  static inline jvalue ToValue(bool value);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(bool);

  static inline bool GetEmptyObject();

  template <typename... Args>
  static bool CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static bool CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, bool arg);
  static inline bool GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, bool arg);
  static inline bool GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(bool, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<jbyte>
{
  static inline jvalue ToValue(jbyte value);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(jbyte);

  static inline jbyte GetEmptyObject();

  template <typename... Args>
  static jbyte CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jbyte CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void  SetField(jobject self, jfieldID field, jbyte arg);
  static inline jbyte GetField(jobject self, jfieldID field);

  static inline void  SetStaticField(jclass clazz, jfieldID field, jbyte arg);
  static inline jbyte GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(jbyte, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<jchar>
{
  static inline jvalue ToValue(jchar value);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(jchar);

  static inline jchar GetEmptyObject();

  template <typename... Args>
  static jchar CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jchar CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void  SetField(jobject self, jfieldID field, jchar arg);
  static inline jchar GetField(jobject self, jfieldID field);

  static inline void  SetStaticField(jclass clazz, jfieldID field, jchar arg);
  static inline jchar GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(jchar, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<jshort>
{
  static inline jvalue ToValue(jshort value);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(jshort);

  static inline jshort GetEmptyObject();

  template <typename... Args>
  static jshort CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jshort CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void   SetField(jobject self, jfieldID field, jshort arg);
  static inline jshort GetField(jobject self, jfieldID field);

  static inline void   SetStaticField(jclass clazz, jfieldID field, jshort arg);
  static inline jshort GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(jshort, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<jint>
{
  static inline jvalue ToValue(jint value);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(jint);

  static inline jint GetEmptyObject();

  template <typename... Args>
  static jint CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jint CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jint arg);
  static inline jint GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jint arg);
  static inline jint GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(jint, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<jlong>
{
  static inline jvalue ToValue(jlong value);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(jlong);

  static inline jlong GetEmptyObject();

  template <typename... Args>
  static jlong CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jlong CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void  SetField(jobject self, jfieldID field, jlong arg);
  static inline jlong GetField(jobject self, jfieldID field);

  static inline void  SetStaticField(jclass clazz, jfieldID field, jlong arg);
  static inline jlong GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(jlong, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<jfloat>
{
  static inline jvalue ToValue(jfloat value);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(jfloat);

  static inline jfloat GetEmptyObject();

  template <typename... Args>
  static jfloat CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jfloat CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void   SetField(jobject self, jfieldID field, jfloat arg);
  static inline jfloat GetField(jobject self, jfieldID field);

  static inline void   SetStaticField(jclass clazz, jfieldID field, jfloat arg);
  static inline jfloat GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(jfloat, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<jdouble>
{
  static inline jvalue ToValue(jdouble value);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(jdouble);

  static inline jdouble GetEmptyObject();

  template <typename... Args>
  static jdouble CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jdouble CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void    SetField(jobject self, jfieldID field, jdouble arg);
  static inline jdouble GetField(jobject self, jfieldID field);

  static inline void    SetStaticField(jclass clazz, jfieldID field, jdouble arg);
  static inline jdouble GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(jdouble, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<xiiJniObject>
{
  static inline jvalue ToValue(const xiiJniObject& object);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(const xiiJniObject& object);

  static inline xiiJniObject GetEmptyObject();

  template <typename... Args>
  static xiiJniObject CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static xiiJniObject CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void         SetField(jobject self, jfieldID field, const xiiJniObject& arg);
  static inline xiiJniObject GetField(jobject self, jfieldID field);

  static inline void         SetStaticField(jclass clazz, jfieldID field, const xiiJniObject& arg);
  static inline xiiJniObject GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(const xiiJniObject& obj, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<xiiJniClass>
{
  static inline jvalue ToValue(const xiiJniClass& object);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(const xiiJniClass& object);

  static inline xiiJniClass GetEmptyObject();

  template <typename... Args>
  static xiiJniClass CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static xiiJniClass CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void        SetField(jobject self, jfieldID field, const xiiJniClass& arg);
  static inline xiiJniClass GetField(jobject self, jfieldID field);

  static inline void        SetStaticField(jclass clazz, jfieldID field, const xiiJniClass& arg);
  static inline xiiJniClass GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(const xiiJniClass& obj, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<xiiJniString>
{
  static inline jvalue ToValue(const xiiJniString& object);

  static inline xiiJniClass GetStaticType();

  static inline xiiJniClass GetRuntimeType(const xiiJniString& object);

  static inline xiiJniString GetEmptyObject();

  template <typename... Args>
  static xiiJniString CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static xiiJniString CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void         SetField(jobject self, jfieldID field, const xiiJniString& arg);
  static inline xiiJniString GetField(jobject self, jfieldID field);

  static inline void         SetStaticField(jclass clazz, jfieldID field, const xiiJniString& arg);
  static inline xiiJniString GetStaticField(jclass clazz, jfieldID field);

  static inline bool        AppendSignature(const xiiJniString& obj, xiiStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct xiiJniTraits<void>
{
  static inline xiiJniClass GetStaticType();

  static inline void GetEmptyObject();

  template <typename... Args>
  static void CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static void CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline const char* GetSignatureStatic();
};

// Helpers to unpack variadic templates.
struct xiiJniImpl
{
  static void CollectArgumentTypes(xiiJniClass* target)
  {
  }

  template <typename T, typename... Tail>
  static void CollectArgumentTypes(xiiJniClass* target, const T& arg, const Tail&... tail)
  {
    *target = xiiJniTraits<T>::GetRuntimeType(arg);
    return xiiJniImpl::CollectArgumentTypes(target + 1, tail...);
  }

  static void UnpackArgs(jvalue* target)
  {
  }

  template <typename T, typename... Tail>
  static void UnpackArgs(jvalue* target, const T& arg, const Tail&... tail)
  {
    *target = xiiJniTraits<T>::ToValue(arg);
    return UnpackArgs(target + 1, tail...);
  }

  template <typename Ret, typename... Args>
  static bool BuildMethodSignature(xiiStringBuilder& signature, const Args&... args)
  {
    signature.Append("(");
    if (!xiiJniImpl::AppendSignature(signature, args...))
    {
      return false;
    }
    signature.Append(")");
    signature.Append(xiiJniTraits<Ret>::GetSignatureStatic());
    return true;
  }

  static bool AppendSignature(xiiStringBuilder& signature)
  {
    return true;
  }

  template <typename T, typename... Tail>
  static bool AppendSignature(xiiStringBuilder& str, const T& arg, const Tail&... tail)
  {
    return xiiJniTraits<T>::AppendSignature(arg, str) && AppendSignature(str, tail...);
  }
};

jvalue xiiJniTraits<bool>::ToValue(bool value)
{
  jvalue result;
  result.z = value ? JNI_TRUE : JNI_FALSE;
  return result;
}

xiiJniClass xiiJniTraits<bool>::GetStaticType()
{
  return xiiJniClass("java/lang/Boolean").UnsafeGetStaticField<xiiJniClass>("TYPE", "Ljava/lang/Class;");
}

xiiJniClass xiiJniTraits<bool>::GetRuntimeType(bool)
{
  return GetStaticType();
}

bool xiiJniTraits<bool>::GetEmptyObject()
{
  return false;
}

template <typename... Args>
bool xiiJniTraits<bool>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallBooleanMethodA(self, method, array) == JNI_TRUE;
}

template <typename... Args>
bool xiiJniTraits<bool>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallStaticBooleanMethodA(clazz, method, array) == JNI_TRUE;
}

void xiiJniTraits<bool>::SetField(jobject self, jfieldID field, bool arg)
{
  return xiiJniAttachment::GetEnv()->SetBooleanField(self, field, arg ? JNI_TRUE : JNI_FALSE);
}

bool xiiJniTraits<bool>::GetField(jobject self, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetBooleanField(self, field) == JNI_TRUE;
}

void xiiJniTraits<bool>::SetStaticField(jclass clazz, jfieldID field, bool arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticBooleanField(clazz, field, arg ? JNI_TRUE : JNI_FALSE);
}

bool xiiJniTraits<bool>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetStaticBooleanField(clazz, field) == JNI_TRUE;
}

bool xiiJniTraits<bool>::AppendSignature(bool, xiiStringBuilder& str)
{
  str.Append("Z");
  return true;
}

const char* xiiJniTraits<bool>::GetSignatureStatic()
{
  return "Z";
}

jvalue xiiJniTraits<jbyte>::ToValue(jbyte value)
{
  jvalue result;
  result.b = value;
  return result;
}

xiiJniClass xiiJniTraits<jbyte>::GetStaticType()
{
  return xiiJniClass("java/lang/Byte").UnsafeGetStaticField<xiiJniClass>("TYPE", "Ljava/lang/Class;");
}

xiiJniClass xiiJniTraits<jbyte>::GetRuntimeType(jbyte)
{
  return GetStaticType();
}

jbyte xiiJniTraits<jbyte>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jbyte xiiJniTraits<jbyte>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallByteMethodA(self, method, array);
}

template <typename... Args>
jbyte xiiJniTraits<jbyte>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallStaticByteMethodA(clazz, method, array);
}

void xiiJniTraits<jbyte>::SetField(jobject self, jfieldID field, jbyte arg)
{
  return xiiJniAttachment::GetEnv()->SetByteField(self, field, arg);
}

jbyte xiiJniTraits<jbyte>::GetField(jobject self, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetByteField(self, field);
}

void xiiJniTraits<jbyte>::SetStaticField(jclass clazz, jfieldID field, jbyte arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticByteField(clazz, field, arg);
}

jbyte xiiJniTraits<jbyte>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetStaticByteField(clazz, field);
}

bool xiiJniTraits<jbyte>::AppendSignature(jbyte, xiiStringBuilder& str)
{
  str.Append("B");
  return true;
}

const char* xiiJniTraits<jbyte>::GetSignatureStatic()
{
  return "B";
}

jvalue xiiJniTraits<jchar>::ToValue(jchar value)
{
  jvalue result;
  result.c = value;
  return result;
}

xiiJniClass xiiJniTraits<jchar>::GetStaticType()
{
  return xiiJniClass("java/lang/Character").UnsafeGetStaticField<xiiJniClass>("TYPE", "Ljava/lang/Class;");
}

xiiJniClass xiiJniTraits<jchar>::GetRuntimeType(jchar)
{
  return GetStaticType();
}

jchar xiiJniTraits<jchar>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jchar xiiJniTraits<jchar>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallCharMethodA(self, method, array);
}

template <typename... Args>
jchar xiiJniTraits<jchar>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallStaticCharMethodA(clazz, method, array);
}

void xiiJniTraits<jchar>::SetField(jobject self, jfieldID field, jchar arg)
{
  return xiiJniAttachment::GetEnv()->SetCharField(self, field, arg);
}

jchar xiiJniTraits<jchar>::GetField(jobject self, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetCharField(self, field);
}

void xiiJniTraits<jchar>::SetStaticField(jclass clazz, jfieldID field, jchar arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticCharField(clazz, field, arg);
}

jchar xiiJniTraits<jchar>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetStaticCharField(clazz, field);
}

bool xiiJniTraits<jchar>::AppendSignature(jchar, xiiStringBuilder& str)
{
  str.Append("C");
  return true;
}

const char* xiiJniTraits<jchar>::GetSignatureStatic()
{
  return "C";
}

jvalue xiiJniTraits<jshort>::ToValue(jshort value)
{
  jvalue result;
  result.s = value;
  return result;
}

xiiJniClass xiiJniTraits<jshort>::GetStaticType()
{
  return xiiJniClass("java/lang/Short").UnsafeGetStaticField<xiiJniClass>("TYPE", "Ljava/lang/Class;");
}

xiiJniClass xiiJniTraits<jshort>::GetRuntimeType(jshort)
{
  return GetStaticType();
}

jshort xiiJniTraits<jshort>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jshort xiiJniTraits<jshort>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallShortMethodA(self, method, array);
}

template <typename... Args>
jshort xiiJniTraits<jshort>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallStaticShortMethodA(clazz, method, array);
}

void xiiJniTraits<jshort>::SetField(jobject self, jfieldID field, jshort arg)
{
  return xiiJniAttachment::GetEnv()->SetShortField(self, field, arg);
}

jshort xiiJniTraits<jshort>::GetField(jobject self, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetShortField(self, field);
}

void xiiJniTraits<jshort>::SetStaticField(jclass clazz, jfieldID field, jshort arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticShortField(clazz, field, arg);
}

jshort xiiJniTraits<jshort>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetStaticShortField(clazz, field);
}

bool xiiJniTraits<jshort>::AppendSignature(jshort, xiiStringBuilder& str)
{
  str.Append("S");
  return true;
}

const char* xiiJniTraits<jshort>::GetSignatureStatic()
{
  return "S";
}

jvalue xiiJniTraits<jint>::ToValue(jint value)
{
  jvalue result;
  result.i = value;
  return result;
}

xiiJniClass xiiJniTraits<jint>::GetStaticType()
{
  return xiiJniClass("java/lang/Integer").UnsafeGetStaticField<xiiJniClass>("TYPE", "Ljava/lang/Class;");
}

xiiJniClass xiiJniTraits<jint>::GetRuntimeType(jint)
{
  return GetStaticType();
}

jint xiiJniTraits<jint>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jint xiiJniTraits<jint>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallIntMethodA(self, method, array);
}

template <typename... Args>
jint xiiJniTraits<jint>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallStaticIntMethodA(clazz, method, array);
}

void xiiJniTraits<jint>::SetField(jobject self, jfieldID field, jint arg)
{
  return xiiJniAttachment::GetEnv()->SetIntField(self, field, arg);
}

jint xiiJniTraits<jint>::GetField(jobject self, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetIntField(self, field);
}

void xiiJniTraits<jint>::SetStaticField(jclass clazz, jfieldID field, jint arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticIntField(clazz, field, arg);
}

jint xiiJniTraits<jint>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetStaticIntField(clazz, field);
}

bool xiiJniTraits<jint>::AppendSignature(jint, xiiStringBuilder& str)
{
  str.Append("I");
  return true;
}

const char* xiiJniTraits<jint>::GetSignatureStatic()
{
  return "I";
}

jvalue xiiJniTraits<jlong>::ToValue(jlong value)
{
  jvalue result;
  result.j = value;
  return result;
}

xiiJniClass xiiJniTraits<jlong>::GetStaticType()
{
  return xiiJniClass("java/lang/Long").UnsafeGetStaticField<xiiJniClass>("TYPE", "Ljava/lang/Class;");
}

xiiJniClass xiiJniTraits<jlong>::GetRuntimeType(jlong)
{
  return GetStaticType();
}

jlong xiiJniTraits<jlong>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jlong xiiJniTraits<jlong>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallLongMethodA(self, method, array);
}

template <typename... Args>
jlong xiiJniTraits<jlong>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallStaticLongMethodA(clazz, method, array);
}

void xiiJniTraits<jlong>::SetField(jobject self, jfieldID field, jlong arg)
{
  return xiiJniAttachment::GetEnv()->SetLongField(self, field, arg);
}

jlong xiiJniTraits<jlong>::GetField(jobject self, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetLongField(self, field);
}

void xiiJniTraits<jlong>::SetStaticField(jclass clazz, jfieldID field, jlong arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticLongField(clazz, field, arg);
}

jlong xiiJniTraits<jlong>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetStaticLongField(clazz, field);
}

bool xiiJniTraits<jlong>::AppendSignature(jlong, xiiStringBuilder& str)
{
  str.Append("J");
  return true;
}

const char* xiiJniTraits<jlong>::GetSignatureStatic()
{
  return "J";
}

jvalue xiiJniTraits<jfloat>::ToValue(jfloat value)
{
  jvalue result;
  result.f = value;
  return result;
}

xiiJniClass xiiJniTraits<jfloat>::GetStaticType()
{
  return xiiJniClass("java/lang/Float").UnsafeGetStaticField<xiiJniClass>("TYPE", "Ljava/lang/Class;");
}

xiiJniClass xiiJniTraits<jfloat>::GetRuntimeType(jfloat)
{
  return GetStaticType();
}

jfloat xiiJniTraits<jfloat>::GetEmptyObject()
{
  return nanf("");
}

template <typename... Args>
jfloat xiiJniTraits<jfloat>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallFloatMethodA(self, method, array);
}

template <typename... Args>
jfloat xiiJniTraits<jfloat>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallStaticFloatMethodA(clazz, method, array);
}

void xiiJniTraits<jfloat>::SetField(jobject self, jfieldID field, jfloat arg)
{
  return xiiJniAttachment::GetEnv()->SetFloatField(self, field, arg);
}

jfloat xiiJniTraits<jfloat>::GetField(jobject self, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetFloatField(self, field);
}

void xiiJniTraits<jfloat>::SetStaticField(jclass clazz, jfieldID field, jfloat arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticFloatField(clazz, field, arg);
}

jfloat xiiJniTraits<jfloat>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetStaticFloatField(clazz, field);
}

bool xiiJniTraits<jfloat>::AppendSignature(jfloat, xiiStringBuilder& str)
{
  str.Append("F");
  return true;
}

const char* xiiJniTraits<jfloat>::GetSignatureStatic()
{
  return "F";
}

jvalue xiiJniTraits<jdouble>::ToValue(jdouble value)
{
  jvalue result;
  result.d = value;
  return result;
}

xiiJniClass xiiJniTraits<jdouble>::GetStaticType()
{
  return xiiJniClass("java/lang/Double").UnsafeGetStaticField<xiiJniClass>("TYPE", "Ljava/lang/Class;");
}

xiiJniClass xiiJniTraits<jdouble>::GetRuntimeType(jdouble)
{
  return GetStaticType();
}

jdouble xiiJniTraits<jdouble>::GetEmptyObject()
{
  return nan("");
}

template <typename... Args>
jdouble xiiJniTraits<jdouble>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallDoubleMethodA(self, method, array);
}

template <typename... Args>
jdouble xiiJniTraits<jdouble>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallStaticDoubleMethodA(clazz, method, array);
}

void xiiJniTraits<jdouble>::SetField(jobject self, jfieldID field, jdouble arg)
{
  return xiiJniAttachment::GetEnv()->SetDoubleField(self, field, arg);
}

jdouble xiiJniTraits<jdouble>::GetField(jobject self, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetDoubleField(self, field);
}

void xiiJniTraits<jdouble>::SetStaticField(jclass clazz, jfieldID field, jdouble arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticDoubleField(clazz, field, arg);
}

jdouble xiiJniTraits<jdouble>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniAttachment::GetEnv()->GetStaticDoubleField(clazz, field);
}

bool xiiJniTraits<jdouble>::AppendSignature(jdouble, xiiStringBuilder& str)
{
  str.Append("D");
  return true;
}

const char* xiiJniTraits<jdouble>::GetSignatureStatic()
{
  return "D";
}

jvalue xiiJniTraits<xiiJniObject>::ToValue(const xiiJniObject& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

xiiJniClass xiiJniTraits<xiiJniObject>::GetStaticType()
{
  return xiiJniClass("java/lang/Object");
}

xiiJniClass xiiJniTraits<xiiJniObject>::GetRuntimeType(const xiiJniObject& arg)
{
  return arg.GetClass();
}

xiiJniObject xiiJniTraits<xiiJniObject>::GetEmptyObject()
{
  return xiiJniObject();
}

template <typename... Args>
xiiJniObject xiiJniTraits<xiiJniObject>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniObject(xiiJniAttachment::GetEnv()->CallObjectMethodA(self, method, array), xiiJniOwnerShip::OWN);
}

template <typename... Args>
xiiJniObject xiiJniTraits<xiiJniObject>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniObject(xiiJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array), xiiJniOwnerShip::OWN);
}

void xiiJniTraits<xiiJniObject>::SetField(jobject self, jfieldID field, const xiiJniObject& arg)
{
  return xiiJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

xiiJniObject xiiJniTraits<xiiJniObject>::GetField(jobject self, jfieldID field)
{
  return xiiJniObject(xiiJniAttachment::GetEnv()->GetObjectField(self, field), xiiJniOwnerShip::OWN);
}

void xiiJniTraits<xiiJniObject>::SetStaticField(jclass clazz, jfieldID field, const xiiJniObject& arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

xiiJniObject xiiJniTraits<xiiJniObject>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniObject(xiiJniAttachment::GetEnv()->GetStaticObjectField(clazz, field), xiiJniOwnerShip::OWN);
}

bool xiiJniTraits<xiiJniObject>::AppendSignature(const xiiJniObject& obj, xiiStringBuilder& str)
{
  if (obj.IsNull())
  {
    // Ensure null objects never generate valid signatures in order to force using the reflection path
    return false;
  }
  else
  {
    str.Append("L");
    str.Append(obj.GetClass().UnsafeCall<xiiJniString>("getName", "()Ljava/lang/String;").GetData());
    str.ReplaceAll(".", "/");
    str.Append(";");
    return true;
  }
}

const char* xiiJniTraits<xiiJniObject>::GetSignatureStatic()
{
  return "Ljava/lang/Object;";
}

jvalue xiiJniTraits<xiiJniClass>::ToValue(const xiiJniClass& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

xiiJniClass xiiJniTraits<xiiJniClass>::GetStaticType()
{
  return xiiJniClass("java/lang/Class");
}

xiiJniClass xiiJniTraits<xiiJniClass>::GetRuntimeType(const xiiJniClass& arg)
{
  // Assume there are no types derived from Class
  return GetStaticType();
}

xiiJniClass xiiJniTraits<xiiJniClass>::GetEmptyObject()
{
  return xiiJniClass();
}

template <typename... Args>
xiiJniClass xiiJniTraits<xiiJniClass>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniClass(jclass(xiiJniAttachment::GetEnv()->CallObjectMethodA(self, method, array)), xiiJniOwnerShip::OWN);
}

template <typename... Args>
xiiJniClass xiiJniTraits<xiiJniClass>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniClass(jclass(xiiJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array)), xiiJniOwnerShip::OWN);
}

void xiiJniTraits<xiiJniClass>::SetField(jobject self, jfieldID field, const xiiJniClass& arg)
{
  return xiiJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

xiiJniClass xiiJniTraits<xiiJniClass>::GetField(jobject self, jfieldID field)
{
  return xiiJniClass(jclass(xiiJniAttachment::GetEnv()->GetObjectField(self, field)), xiiJniOwnerShip::OWN);
}

void xiiJniTraits<xiiJniClass>::SetStaticField(jclass clazz, jfieldID field, const xiiJniClass& arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

xiiJniClass xiiJniTraits<xiiJniClass>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniClass(jclass(xiiJniAttachment::GetEnv()->GetStaticObjectField(clazz, field)), xiiJniOwnerShip::OWN);
}

bool xiiJniTraits<xiiJniClass>::AppendSignature(const xiiJniClass& obj, xiiStringBuilder& str)
{
  str.Append("Ljava/lang/Class;");
  return true;
}

const char* xiiJniTraits<xiiJniClass>::GetSignatureStatic()
{
  return "Ljava/lang/Class;";
}

jvalue xiiJniTraits<xiiJniString>::ToValue(const xiiJniString& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

xiiJniClass xiiJniTraits<xiiJniString>::GetStaticType()
{
  return xiiJniClass("java/lang/String");
}

xiiJniClass xiiJniTraits<xiiJniString>::GetRuntimeType(const xiiJniString& arg)
{
  // Assume there are no types derived from String
  return GetStaticType();
}

xiiJniString xiiJniTraits<xiiJniString>::GetEmptyObject()
{
  return xiiJniString();
}

template <typename... Args>
xiiJniString xiiJniTraits<xiiJniString>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniString(jstring(xiiJniAttachment::GetEnv()->CallObjectMethodA(self, method, array)), xiiJniOwnerShip::OWN);
}

template <typename... Args>
xiiJniString xiiJniTraits<xiiJniString>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniString(jstring(xiiJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array)), xiiJniOwnerShip::OWN);
}

void xiiJniTraits<xiiJniString>::SetField(jobject self, jfieldID field, const xiiJniString& arg)
{
  return xiiJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

xiiJniString xiiJniTraits<xiiJniString>::GetField(jobject self, jfieldID field)
{
  return xiiJniString(jstring(xiiJniAttachment::GetEnv()->GetObjectField(self, field)), xiiJniOwnerShip::OWN);
}

void xiiJniTraits<xiiJniString>::SetStaticField(jclass clazz, jfieldID field, const xiiJniString& arg)
{
  return xiiJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

xiiJniString xiiJniTraits<xiiJniString>::GetStaticField(jclass clazz, jfieldID field)
{
  return xiiJniString(jstring(xiiJniAttachment::GetEnv()->GetStaticObjectField(clazz, field)), xiiJniOwnerShip::OWN);
}

bool xiiJniTraits<xiiJniString>::AppendSignature(const xiiJniString& obj, xiiStringBuilder& str)
{
  str.Append("Ljava/lang/String;");
  return true;
}

const char* xiiJniTraits<xiiJniString>::GetSignatureStatic()
{
  return "Ljava/lang/String;";
}

xiiJniClass xiiJniTraits<void>::GetStaticType()
{
  return xiiJniClass("java/lang/Void").UnsafeGetStaticField<xiiJniClass>("TYPE", "Ljava/lang/Class;");
}

void xiiJniTraits<void>::GetEmptyObject()
{
  return;
}

template <typename... Args>
void xiiJniTraits<void>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallVoidMethodA(self, method, array);
}

template <typename... Args>
void xiiJniTraits<void>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniAttachment::GetEnv()->CallStaticVoidMethodA(clazz, method, array);
}

const char* xiiJniTraits<void>::GetSignatureStatic()
{
  return "V";
}

template <typename... Args>
xiiJniObject xiiJniClass::CreateInstance(const Args&... args) const
{
  if (xiiJniAttachment::FailOnPendingErrorOrException())
  {
    return xiiJniObject();
  }

  const size_t N = sizeof...(args);

  xiiJniClass inputTypes[N];
  xiiJniImpl::CollectArgumentTypes(inputTypes, args...);

  xiiJniObject foundMethod = FindConstructor(*this, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return xiiJniObject();
  }

  jmethodID method = xiiJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.GetHandle());

  jvalue array[sizeof...(args)];
  xiiJniImpl::UnpackArgs(array, args...);
  return xiiJniObject(xiiJniAttachment::GetEnv()->NewObjectA(GetHandle(), method, array), xiiJniOwnerShip::OWN);
}

template <typename Ret, typename... Args>
Ret xiiJniClass::CallStatic(const char* name, const Args&... args) const
{
  if (xiiJniAttachment::FailOnPendingErrorOrException())
  {
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  if (!GetJObject())
  {
    xiiLog::Error("Attempting to call static method '{}' on null class.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  xiiStringBuilder signature;
  if (xiiJniImpl::BuildMethodSignature<Ret>(signature, args...))
  {
    jmethodID method = xiiJniAttachment::GetEnv()->GetStaticMethodID(GetHandle(), name, signature.GetData());

    if (method)
    {
      return xiiJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
    }
    else
    {
      xiiJniAttachment::GetEnv()->ExceptionClear();
    }
  }

  const size_t N = sizeof...(args);

  xiiJniClass returnType = xiiJniTraits<Ret>::GetStaticType();

  xiiJniClass inputTypes[N];
  xiiJniImpl::CollectArgumentTypes(inputTypes, args...);

  xiiJniObject foundMethod = FindMethod(true, name, *this, returnType, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = xiiJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.GetHandle());
  return xiiJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
}

template <typename Ret, typename... Args>
Ret xiiJniClass::UnsafeCallStatic(const char* name, const char* signature, const Args&... args) const
{
  if (!GetJObject())
  {
    xiiLog::Error("Attempting to call static method '{}' on null class.", name);
    xiiLog::Error("Attempting to call static method '{}' on null class.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = xiiJniAttachment::GetEnv()->GetStaticMethodID(GetHandle(), name, signature);
  if (!method)
  {
    xiiLog::Error("No such static method: '{}' with signature '{}' in class '{}'.", name, signature, ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_METHOD);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return xiiJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
  }
}

template <typename Ret>
Ret xiiJniClass::GetStaticField(const char* name) const
{
  if (xiiJniAttachment::FailOnPendingErrorOrException())
  {
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  if (!GetJObject())
  {
    xiiLog::Error("Attempting to get static field '{}' on null class.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID fieldID = xiiJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, xiiJniTraits<Ret>::GetSignatureStatic());
  if (fieldID)
  {
    return xiiJniTraits<Ret>::GetStaticField(GetHandle(), fieldID);
  }
  else
  {
    xiiJniAttachment::GetEnv()->ExceptionClear();
  }

  xiiJniObject field = UnsafeCall<xiiJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", xiiJniString(name));

  if (xiiJniAttachment::GetEnv()->ExceptionOccurred())
  {
    xiiJniAttachment::GetEnv()->ExceptionClear();

    xiiLog::Error("No field named '{}' found in class '{}'.", name, ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);

    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  if ((field.UnsafeCall<jint>("getModifiers", "()I") & xiiJniModifiers::STATIC) == 0)
  {
    xiiLog::Error("Field named '{}' in class '{}' isn't static.", name, ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  xiiJniClass fieldType = field.UnsafeCall<xiiJniClass>("getType", "()Ljava/lang/Class;");

  xiiJniClass returnType = xiiJniTraits<Ret>::GetStaticType();

  if (!returnType.IsAssignableFrom(fieldType))
  {
    xiiLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned to return type '{}'.", name, fieldType.ToString().GetData(), ToString().GetData(), returnType.ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  return xiiJniTraits<Ret>::GetStaticField(GetHandle(), xiiJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()));
}

template <typename Ret>
Ret xiiJniClass::UnsafeGetStaticField(const char* name, const char* signature) const
{
  if (!GetJObject())
  {
    xiiLog::Error("Attempting to get static field '{}' on null class.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID field = xiiJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, signature);
  if (!field)
  {
    xiiLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return xiiJniTraits<Ret>::GetStaticField(GetHandle(), field);
  }
}

template <typename T>
void xiiJniClass::SetStaticField(const char* name, const T& arg) const
{
  if (xiiJniAttachment::FailOnPendingErrorOrException())
  {
    return;
  }

  if (!GetJObject())
  {
    xiiLog::Error("Attempting to set static field '{}' on null class.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  xiiJniObject field = UnsafeCall<xiiJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", xiiJniString(name));

  if (xiiJniAttachment::GetEnv()->ExceptionOccurred())
  {
    xiiJniAttachment::GetEnv()->ExceptionClear();

    xiiLog::Error("No field named '{}' found in class '{}'.", name, ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);

    return;
  }

  xiiJniClass modifierClass("java/lang/reflect/Modifier");
  jint        modifiers = field.UnsafeCall<jint>("getModifiers", "()I");

  if ((modifiers & xiiJniModifiers::STATIC) == 0)
  {
    xiiLog::Error("Field named '{}' in class '{}' isn't static.", name, ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  if ((modifiers & xiiJniModifiers::FINAL) != 0)
  {
    xiiLog::Error("Field named '{}' in class '{}' is final.", name, ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  xiiJniClass fieldType = field.UnsafeCall<xiiJniClass>("getType", "()Ljava/lang/Class;");

  xiiJniClass argType = xiiJniTraits<T>::GetRuntimeType(arg);

  if (argType.IsNull())
  {
    if (fieldType.IsPrimitive())
    {
      xiiLog::Error("Field '{}' of type '{}' can't be assigned null because it is a primitive type.", name, fieldType.ToString().GetData());
      xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }
  else
  {
    if (!fieldType.IsAssignableFrom(argType))
    {
      xiiLog::Error("Field '{}' of type '{}' can't be assigned from type '{}'.", name, fieldType.ToString().GetData(), argType.ToString().GetData());
      xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }

  return xiiJniTraits<T>::SetStaticField(GetHandle(), xiiJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()), arg);
}

template <typename T>
void xiiJniClass::UnsafeSetStaticField(const char* name, const char* signature, const T& arg) const
{
  if (!GetJObject())
  {
    xiiLog::Error("Attempting to set static field '{}' on null class.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = xiiJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, signature);
  if (!field)
  {
    xiiLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return xiiJniTraits<T>::SetStaticField(GetHandle(), field, arg);
  }
}

template <typename Ret, typename... Args>
Ret xiiJniObject::Call(const char* name, const Args&... args) const
{
  if (xiiJniAttachment::FailOnPendingErrorOrException())
  {
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  if (!m_object)
  {
    xiiLog::Error("Attempting to call method '{}' on null object.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  // Fast path: Lookup method via signature built from parameters.
  // This only works for exact matches, but is roughly 50 times faster.
  xiiStringBuilder signature;
  if (xiiJniImpl::BuildMethodSignature<Ret>(signature, args...))
  {
    jmethodID method = xiiJniAttachment::GetEnv()->GetMethodID(reinterpret_cast<jclass>(GetClass().GetHandle()), name, signature.GetData());

    if (method)
    {
      return xiiJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
    }
    else
    {
      xiiJniAttachment::GetEnv()->ExceptionClear();
    }
  }

  // Fallback to slow path using reflection
  const size_t N = sizeof...(args);

  xiiJniClass returnType = xiiJniTraits<Ret>::GetStaticType();

  xiiJniClass inputTypes[N];
  xiiJniImpl::CollectArgumentTypes(inputTypes, args...);

  xiiJniObject foundMethod = FindMethod(false, name, GetClass(), returnType, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = xiiJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.m_object);
  return xiiJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
}

template <typename Ret, typename... Args>
Ret xiiJniObject::UnsafeCall(const char* name, const char* signature, const Args&... args) const
{
  if (!m_object)
  {
    xiiLog::Error("Attempting to call method '{}' on null object.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = xiiJniAttachment::GetEnv()->GetMethodID(jclass(GetClass().m_object), name, signature);
  if (!method)
  {
    xiiLog::Error("No such method: '{}' with signature '{}' in class '{}'.", name, signature, GetClass().ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_METHOD);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return xiiJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
  }
}

template <typename T>
void xiiJniObject::SetField(const char* name, const T& arg) const
{
  if (xiiJniAttachment::FailOnPendingErrorOrException())
  {
    return;
  }

  if (!m_object)
  {
    xiiLog::Error("Attempting to set field '{}' on null object.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  // No fast path here since we need to be able to report failures when attempting
  // to set final fields, which we can only do using reflection.

  xiiJniObject field = GetClass().UnsafeCall<xiiJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", xiiJniString(name));

  if (xiiJniAttachment::GetEnv()->ExceptionOccurred())
  {
    xiiJniAttachment::GetEnv()->ExceptionClear();

    xiiLog::Error("No field named '{}' found.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);

    return;
  }

  xiiJniClass modifierClass("java/lang/reflect/Modifier");
  jint        modifiers = field.UnsafeCall<jint>("getModifiers", "()I");

  if ((modifiers & xiiJniModifiers::STATIC) != 0)
  {
    xiiLog::Error("Field named '{}' in class '{}' is static.", name, GetClass().ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  if ((modifiers & xiiJniModifiers::FINAL) != 0)
  {
    xiiLog::Error("Field named '{}' in class '{}' is final.", name, GetClass().ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  xiiJniClass fieldType = field.UnsafeCall<xiiJniClass>("getType", "()Ljava/lang/Class;");

  xiiJniClass argType = xiiJniTraits<T>::GetRuntimeType(arg);

  if (argType.IsNull())
  {
    if (fieldType.IsPrimitive())
    {
      xiiLog::Error("Field '{}' of type '{}'  in class '{}' can't be assigned null because it is a primitive type.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData());
      xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }
  else
  {
    if (!fieldType.IsAssignableFrom(argType))
    {
      xiiLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned from type '{}'.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData(), argType.ToString().GetData());
      xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }

  return xiiJniTraits<T>::SetField(m_object, xiiJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()), arg);
}

template <typename T>
void xiiJniObject::UnsafeSetField(const char* name, const char* signature, const T& arg) const
{
  if (!m_object)
  {
    xiiLog::Error("Attempting to set field '{}' on null class.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = xiiJniAttachment::GetEnv()->GetFieldID(jclass(GetClass().GetHandle()), name, signature);
  if (!field)
  {
    xiiLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return xiiJniTraits<T>::SetField(m_object, field, arg);
  }
}

template <typename Ret>
Ret xiiJniObject::GetField(const char* name) const
{
  if (xiiJniAttachment::FailOnPendingErrorOrException())
  {
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  if (!m_object)
  {
    xiiLog::Error("Attempting to get field '{}' on null object.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID fieldID = xiiJniAttachment::GetEnv()->GetFieldID(GetClass().GetHandle(), name, xiiJniTraits<Ret>::GetSignatureStatic());
  if (fieldID)
  {
    return xiiJniTraits<Ret>::GetField(m_object, fieldID);
  }
  else
  {
    xiiJniAttachment::GetEnv()->ExceptionClear();
  }

  xiiJniObject field = GetClass().UnsafeCall<xiiJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", xiiJniString(name));

  if (xiiJniAttachment::GetEnv()->ExceptionOccurred())
  {
    xiiJniAttachment::GetEnv()->ExceptionClear();

    xiiLog::Error("No field named '{}' found in class '{}'.", name, GetClass().ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);

    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  if ((field.UnsafeCall<jint>("getModifiers", "()I") & xiiJniModifiers::STATIC) != 0)
  {
    xiiLog::Error("Field named '{}' in class '{}' is static.", name, GetClass().ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  xiiJniClass fieldType = field.UnsafeCall<xiiJniClass>("getType", "()Ljava/lang/Class;");

  xiiJniClass returnType = xiiJniTraits<Ret>::GetStaticType();

  if (!returnType.IsAssignableFrom(fieldType))
  {
    xiiLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned to return type '{}'.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData(), returnType.ToString().GetData());
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return xiiJniTraits<Ret>::GetEmptyObject();
  }

  return xiiJniTraits<Ret>::GetField(m_object, xiiJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()));
}

template <typename Ret>
Ret xiiJniObject::UnsafeGetField(const char* name, const char* signature) const
{
  if (!m_object)
  {
    xiiLog::Error("Attempting to get field '{}' on null class.", name);
    xiiJniAttachment::SetLastError(xiiJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = xiiJniAttachment::GetEnv()->GetFieldID(GetClass().GetHandle(), name, signature);
  if (!field)
  {
    xiiLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    xiiJniAttachment::SetLastError(xiiJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return xiiJniTraits<Ret>::GetField(m_object, field);
  }
}
