/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_ALWAYS_INLINE xiiWorld* xiiWorldModule::GetWorld()
{
  return m_pWorld;
}

XII_ALWAYS_INLINE const xiiWorld* xiiWorldModule::GetWorld() const
{
  return m_pWorld;
}

//////////////////////////////////////////////////////////////////////////

template <typename ModuleType, typename RTTIType>
xiiWorldModuleTypeId xiiWorldModuleFactory::RegisterWorldModule()
{
  struct Helper
  {
    static xiiWorldModule* Create(xiiAllocator* pAllocator, xiiWorld* pWorld) { return XII_NEW(pAllocator, ModuleType, pWorld); }
  };

  const xiiRTTI* pRtti = xiiGetStaticRTTI<RTTIType>();
  return RegisterWorldModule(pRtti, &Helper::Create);
}
