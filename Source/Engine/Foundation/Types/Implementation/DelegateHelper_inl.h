/// Copyright (c) Theophilus Eriata. All Rights Reserved.

/// [Internal] Storage for lambdas with captures in xiiDelegate.
struct XII_FOUNDATION_DLL xiiLambdaDelegateStorageBase
{
  xiiLambdaDelegateStorageBase()                                              = default;
  virtual ~xiiLambdaDelegateStorageBase()                                     = default;
  virtual xiiLambdaDelegateStorageBase* Clone(xiiAllocator* pAllocator) const = 0;
  virtual void                          InplaceCopy(xiiUInt8* pBuffer) const  = 0;
  virtual void                          InplaceMove(xiiUInt8* pBuffer)        = 0;

private:
  xiiLambdaDelegateStorageBase(const xiiLambdaDelegateStorageBase&)            = delete;
  xiiLambdaDelegateStorageBase& operator=(const xiiLambdaDelegateStorageBase&) = delete;
  xiiLambdaDelegateStorageBase(xiiLambdaDelegateStorageBase&&)                 = delete;
  xiiLambdaDelegateStorageBase& operator=(xiiLambdaDelegateStorageBase&&)      = delete;
};

template <typename Function>
struct xiiLambdaDelegateStorage : public xiiLambdaDelegateStorageBase
{
  xiiLambdaDelegateStorage(Function&& func) :
    m_func(std::move(func))
  {
  }

private:
  template <typename = typename std::enable_if<std::is_copy_constructible<Function>::value>>
  xiiLambdaDelegateStorage(const Function& func) :
    m_func(func)
  {
  }

public:
  virtual xiiLambdaDelegateStorageBase* Clone(xiiAllocator* pAllocator) const override
  {
    if constexpr (std::is_copy_constructible<Function>::value)
    {
      return XII_NEW(pAllocator, xiiLambdaDelegateStorage<Function>, m_func);
    }
    else
    {
      XII_REPORT_FAILURE("The xiiDelegate stores a lambda that is not copyable. Copying this xiiDelegate is not supported.");
      return nullptr;
    }
  }

  virtual void InplaceCopy(xiiUInt8* pBuffer) const override
  {
    if constexpr (std::is_copy_constructible<Function>::value)
    {
      new (pBuffer) xiiLambdaDelegateStorage<Function>(m_func);
    }
    else
    {
      XII_REPORT_FAILURE("The xiiDelegate stores a lambda that is not copyable. Copying this xiiDelegate is not supported.");
    }
  }

  virtual void InplaceMove(xiiUInt8* pBuffer) override
  {
    if constexpr (std::is_move_constructible<Function>::value)
    {
      new (pBuffer) xiiLambdaDelegateStorage<Function>(std::move(m_func));
    }
    else
    {
      XII_REPORT_FAILURE("The xiiDelegate stores a lambda that is not movable. Moving this xiiDelegate is not supported.");
    }
  }

  Function m_func;
};


template <typename R, class... Args, xiiUInt32 DataSize>
struct xiiDelegate<R(Args...), DataSize> : public xiiDelegateBase
{
private:
  using SelfType = xiiDelegate<R(Args...), DataSize>;
  constexpr const void* HeapLambda() const { return reinterpret_cast<const void*>((size_t)-1); }
  constexpr const void* InplaceLambda() const { return reinterpret_cast<const void*>((size_t)-2); }

public:
  XII_ALWAYS_INLINE xiiDelegate() :
    m_DispatchFunction(nullptr)
  {
  }

  XII_ALWAYS_INLINE xiiDelegate(const SelfType& other) { *this = other; }

  XII_ALWAYS_INLINE xiiDelegate(SelfType&& other) { *this = std::move(other); }

  /// Constructs the delegate from a member function type and takes the class instance on which to call the function later.
  template <typename Method, typename Class>
  XII_FORCE_INLINE xiiDelegate(Method method, Class* pInstance)
  {
    CopyMemberFunctionToInplaceStorage(method);

    m_Instance.m_Ptr   = pInstance;
    m_DispatchFunction = &DispatchToMethod<Method, Class>;
  }

  /// Constructs the delegate from a member function type and takes the (const) class instance on which to call the function later.
  template <typename Method, typename Class>
  XII_FORCE_INLINE xiiDelegate(Method method, const Class* pInstance)
  {
    CopyMemberFunctionToInplaceStorage(method);

    m_Instance.m_ConstPtr = pInstance;
    m_DispatchFunction    = &DispatchToConstMethod<Method, Class>;
  }

  /// Constructs the delegate from a regular C function type.
  template <typename Function>
  XII_FORCE_INLINE xiiDelegate(Function function, xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator())
  {
    static_assert(DataSize >= 16, "DataSize must be at least 16 bytes");

    // Pure function pointers or lambdas that can be cast into pure functions (no captures) can be
    // copied directly into the inplace storage of the delegate.
    // Lambdas with captures need to be wrapped into a xiiLambdaDelegateStorage object as they can
    // capture non-pod or non-memmoveable data. This wrapper can also be stored inplace if it is small enough,
    // otherwise it will be heap allocated with the specified allocator.
    constexpr size_t functionSize = sizeof(Function);
    using signature               = R(Args...);
    if constexpr (functionSize <= DataSize && std::is_assignable<signature*&, Function>::value)
    {
      // Lambdas with no capture have a size of 1.
      // Lambdas with no capture actually have no data. Do not copy the 1 uninitialized byte.
      // Propper function pointers have a size of > 4 or 8 (depending on pointer size)
      if constexpr (functionSize > 1)
      {
        CopyFunctionToInplaceStorage(function);
      }
      else
      {
        memset(m_Data, 0, DataSize);
      }

      m_Instance.m_ConstPtr = nullptr;
      m_DispatchFunction    = &DispatchToFunction<Function>;
    }
    else
    {
      constexpr size_t storageSize = sizeof(xiiLambdaDelegateStorage<Function>);
      if constexpr (storageSize <= DataSize)
      {
        m_Instance.m_ConstPtr = InplaceLambda();
        new (m_Data) xiiLambdaDelegateStorage<Function>(std::move(function));
        memset(m_Data + storageSize, 0, DataSize - storageSize);
        m_DispatchFunction = &DispatchToInplaceLambda<Function>;
      }
      else
      {
        m_Instance.m_ConstPtr = HeapLambda();
        m_pLambdaStorage      = XII_NEW(pAllocator, xiiLambdaDelegateStorage<Function>, std::move(function));
        m_pAllocator          = pAllocator;
        memset(m_Data + 2 * sizeof(void*), 0, DataSize - 2 * sizeof(void*));
        m_DispatchFunction = &DispatchToHeapLambda<Function>;
      }
    }
  }

  XII_ALWAYS_INLINE ~xiiDelegate() { Invalidate(); }

  /// Copies the data from another delegate.
  XII_FORCE_INLINE void operator=(const SelfType& other)
  {
    Invalidate();

    if (other.IsHeapLambda())
    {
      m_pAllocator     = other.m_pAllocator;
      m_pLambdaStorage = other.m_pLambdaStorage->Clone(m_pAllocator);
    }
    else if (other.IsInplaceLambda())
    {
      auto pOtherLambdaStorage = reinterpret_cast<xiiLambdaDelegateStorageBase*>(&other.m_Data);
      pOtherLambdaStorage->InplaceCopy(m_Data);
    }
    else
    {
      memcpy(m_Data, other.m_Data, DataSize);
    }

    m_Instance         = other.m_Instance;
    m_DispatchFunction = other.m_DispatchFunction;
  }

  /// Moves the data from another delegate.
  XII_FORCE_INLINE void operator=(SelfType&& other)
  {
    Invalidate();
    m_Instance         = other.m_Instance;
    m_DispatchFunction = other.m_DispatchFunction;

    if (other.IsInplaceLambda())
    {
      auto pOtherLambdaStorage = reinterpret_cast<xiiLambdaDelegateStorageBase*>(&other.m_Data);
      pOtherLambdaStorage->InplaceMove(m_Data);
    }
    else
    {
      memcpy(m_Data, other.m_Data, DataSize);
    }

    other.m_Instance.m_Ptr   = nullptr;
    other.m_DispatchFunction = nullptr;
    memset(other.m_Data, 0, DataSize);
  }

  /// Resets a delegate to an invalid state.
  XII_FORCE_INLINE void operator=(std::nullptr_t) { Invalidate(); }

  /// Function call operator. This will call the function that is bound to the delegate, or assert if nothing was bound.
  XII_FORCE_INLINE R operator()(Args... params) const
  {
    XII_ASSERT_DEBUG(m_DispatchFunction != nullptr, "Delegate is not bound.");
    return (*m_DispatchFunction)(*this, params...);
  }

  /// This function only exists to make code compile, but it will assert when used. Use IsEqualIfNotHeapAllocated() instead.
  XII_ALWAYS_INLINE bool operator==(const SelfType& other) const
  {
    XII_REPORT_FAILURE("operator== for xiiDelegate must not be used. Use IsEqualIfNotHeapAllocated() and read its documentation!");
    return false;
  }

  /// Checks whether two delegates are bound to the exact same function, including the class instance.
  /// \note If \a this or \a other or both return false for IsComparable(), the function returns always false!
  /// Therefore, do not use this to search for delegates that are not comparable. xiiEvent uses this function, but goes to great lengths to
  /// assert that it is used correctly. It is best to not use this function at all.
  XII_ALWAYS_INLINE bool IsEqualIfComparable(const SelfType& other) const
  {
    return m_Instance.m_Ptr == other.m_Instance.m_Ptr && m_DispatchFunction == other.m_DispatchFunction && memcmp(m_Data, other.m_Data, DataSize) == 0;
  }

  /// Returns true when the delegate is bound to a valid non-nullptr function.
  XII_ALWAYS_INLINE bool IsValid() const { return m_DispatchFunction != nullptr; }

  /// Resets a delegate to an invalid state.
  XII_FORCE_INLINE void Invalidate()
  {
    m_DispatchFunction = nullptr;
    if (IsHeapLambda())
    {
      XII_DELETE(m_pAllocator, m_pLambdaStorage);
    }
    else if (IsInplaceLambda())
    {
      auto pLambdaStorage = reinterpret_cast<xiiLambdaDelegateStorageBase*>(&m_Data);
      pLambdaStorage->~xiiLambdaDelegateStorageBase();
    }

    m_Instance.m_Ptr = nullptr;
    memset(m_Data, 0, DataSize);
  }

  /// Returns the class instance that is used to call a member function pointer on.
  XII_ALWAYS_INLINE void* GetClassInstance() const { return IsComparable() ? m_Instance.m_Ptr : nullptr; }

  /// Returns whether the delegate is comparable with other delegates of the same type. This is not the case for i.e. lambdas with captures.
  XII_ALWAYS_INLINE bool IsComparable() const { return m_Instance.m_ConstPtr < InplaceLambda(); } // [tested]

private:
  template <typename Function>
  XII_FORCE_INLINE void CopyFunctionToInplaceStorage(Function function)
  {
    XII_ASSERT_DEBUG(xiiMemoryUtils::IsAligned(&m_Data, alignof(Function)), "Wrong alignment. Expected {0} bytes alignment", alignof(Function));

    memcpy(m_Data, &function, sizeof(Function));
    memset(m_Data + sizeof(Function), 0, DataSize - sizeof(Function));
  }

  template <typename Method>
  XII_FORCE_INLINE void CopyMemberFunctionToInplaceStorage(Method method)
  {
    static_assert(DataSize >= 16, "DataSize must be at least 16 bytes");
    static_assert(sizeof(Method) <= DataSize, "Member function pointer must not be bigger than 16 bytes");

    CopyFunctionToInplaceStorage(method);

    // Member Function Pointers in MSVC are 12 bytes in size and have 4 byte padding
    // MSVC builds a member function pointer on the stack writing only 12 bytes and then copies it
    // to the final location by copying 16 bytes. Thus the 4 byte padding get a random value (whatever is on the stack at that time).
    // To make the delegate comparable by memcmp we zero out those 4 byte padding.
    // Apparently clang does the same on windows but not on linux etc.
#if XII_ENABLED(XII_COMPILER_MSVC) || (XII_ENABLED(XII_PLATFORM_WINDOWS) && XII_ENABLED(XII_COMPILER_CLANG))
    *reinterpret_cast<xiiUInt32*>(m_Data + 12) = 0;
#endif
  }

  XII_ALWAYS_INLINE bool IsInplaceLambda() const { return m_Instance.m_ConstPtr == InplaceLambda(); }
  XII_ALWAYS_INLINE bool IsHeapLambda() const { return m_Instance.m_ConstPtr == HeapLambda(); }

  template <typename Method, typename Class>
  static XII_FORCE_INLINE R DispatchToMethod(const SelfType& self, Args... params)
  {
    XII_ASSERT_DEBUG(self.m_Instance.m_Ptr != nullptr, "Instance must not be null.");
    Method method = *reinterpret_cast<Method*>(&self.m_Data);
    return (static_cast<Class*>(self.m_Instance.m_Ptr)->*method)(params...);
  }

  template <typename Method, typename Class>
  static XII_FORCE_INLINE R DispatchToConstMethod(const SelfType& self, Args... params)
  {
    XII_ASSERT_DEBUG(self.m_Instance.m_ConstPtr != nullptr, "Instance must not be null.");
    Method method = *reinterpret_cast<Method*>(&self.m_Data);
    return (static_cast<const Class*>(self.m_Instance.m_ConstPtr)->*method)(params...);
  }

  template <typename Function>
  static XII_ALWAYS_INLINE R DispatchToFunction(const SelfType& self, Args... params)
  {
    return (*reinterpret_cast<Function*>(&self.m_Data))(params...);
  }

  template <typename Function>
  static XII_ALWAYS_INLINE R DispatchToHeapLambda(const SelfType& self, Args... params)
  {
    return static_cast<xiiLambdaDelegateStorage<Function>*>(self.m_pLambdaStorage)->m_func(params...);
  }

  template <typename Function>
  static XII_ALWAYS_INLINE R DispatchToInplaceLambda(const SelfType& self, Args... params)
  {
    return reinterpret_cast<xiiLambdaDelegateStorage<Function>*>(&self.m_Data)->m_func(params...);
  }

  using DispatchFunction = R (*)(const SelfType&, Args...);
  DispatchFunction m_DispatchFunction;

  union
  {
    mutable xiiUInt8 m_Data[DataSize];
    struct
    {
      xiiLambdaDelegateStorageBase* m_pLambdaStorage;
      xiiAllocator*                 m_pAllocator;
    };
  };
};

template <typename T>
struct xiiMakeDelegateHelper
{
};

template <typename Class, typename R, typename... Args>
struct xiiMakeDelegateHelper<R (Class::*)(Args...)>
{
  using DelegateType = xiiDelegate<R(Args...)>;
};

template <typename Class, typename R, typename... Args>
struct xiiMakeDelegateHelper<R (Class::*)(Args...) const>
{
  using DelegateType = xiiDelegate<R(Args...)>;
};
