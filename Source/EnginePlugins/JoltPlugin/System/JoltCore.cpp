#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Factory.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Configuration/CVar.h>
#include <Jolt/Core/IssueReporting.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/RegisterTypes.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/Implementation/JoltCustomShapeInfo.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <stdarg.h>

#ifdef JPH_DEBUG_RENDERER
std::unique_ptr<xiiJoltDebugRenderer> xiiJoltCore::s_pDebugRenderer;
#endif

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiJoltSteppingMode, 1)
  XII_ENUM_CONSTANTS(xiiJoltSteppingMode::Variable, xiiJoltSteppingMode::Fixed, xiiJoltSteppingMode::SemiFixed)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiOnJoltContact, 1)
  //XII_BITFLAGS_CONSTANT(xiiOnJoltContact::SendReportMsg),
  XII_BITFLAGS_CONSTANT(xiiOnJoltContact::ImpactReactions),
  XII_BITFLAGS_CONSTANT(xiiOnJoltContact::SlideReactions),
  XII_BITFLAGS_CONSTANT(xiiOnJoltContact::RollXReactions),
  XII_BITFLAGS_CONSTANT(xiiOnJoltContact::RollYReactions),
  XII_BITFLAGS_CONSTANT(xiiOnJoltContact::RollZReactions),
XII_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

xiiJoltMaterial*                xiiJoltCore::s_pDefaultMaterial = nullptr;
std::unique_ptr<JPH::JobSystem> xiiJoltCore::s_pJobSystem;

xiiUniquePtr<xiiProxyAllocator> xiiJoltCore::s_pAllocator;

xiiJoltMaterial::xiiJoltMaterial()  = default;
xiiJoltMaterial::~xiiJoltMaterial() = default;

//#define XII_Jolt_DETAILED_MEMORY_STATS XII_ON
//#define XII_Jolt_DETAILED_MEMORY_STATS XII_OFF
//
// xiiJoltAllocatorCallback::xiiJoltAllocatorCallback()
//  : m_Allocator("Jolt", xiiFoundation::GetAlignedAllocator())
//{
//}
//
// void* xiiJoltAllocatorCallback::allocate(size_t size, const char* typeName, const char* filename, int line)
//{
//  void* pPtr = m_Allocator.Allocate(size, 16);
//
//#if XII_ENABLED(XII_Jolt_DETAILED_MEMORY_STATS)
//  xiiStringBuilder s;
//  s.Set(typeName, " - ", filename);
//  m_Allocations[pPtr] = s;
//#endif
//
//  return pPtr;
//}
//
// void xiiJoltAllocatorCallback::deallocate(void* ptr)
//{
//  if (ptr == nullptr)
//    return;
//
//#if XII_ENABLED(XII_Jolt_DETAILED_MEMORY_STATS)
//  m_Allocations.Remove(ptr);
//#endif
//
//  m_Allocator.Deallocate(ptr);
//}
//
// void xiiJoltAllocatorCallback::VerifyAllocations()
//{
//#if XII_ENABLED(XII_Jolt_DETAILED_MEMORY_STATS)
//  XII_ASSERT_DEV(m_Allocations.IsEmpty(), "There are {0} unfreed allocations", m_Allocations.GetCount());
//
//  for (auto it = m_Allocations.GetIterator(); it.IsValid(); ++it)
//  {
//    const char* s = it.Value().GetData();
//    xiiLog::Info(s);
//  }
//#endif
//}

static void JoltTraceFunc(const char* szText, ...)
{
  xiiStringBuilder tmp;

  va_list args;
  va_start(args, szText);
  tmp.PrintfArgs(szText, args);
  va_end(args);

  xiiLog::Dev("Jolt: {}", tmp);
}

#ifdef JPH_ENABLE_ASSERTS

static bool JoltAssertFailed(const char* inExpression, const char* inMessage, const char* inFile, uint32_t inLine)
{
  return xiiFailedCheck(inFile, inLine, "Jolt", inExpression, inMessage);
};

#endif // JPH_ENABLE_ASSERTS

void xiiJoltCore::DebugDraw(xiiWorld* pWorld)
{
#ifdef JPH_DEBUG_RENDERER
  if (s_pDebugRenderer == nullptr)
    return;

  xiiDebugRenderer::DrawSolidTriangles(pWorld, s_pDebugRenderer->m_Triangles, xiiColor::White);
  xiiDebugRenderer::DrawLines(pWorld, s_pDebugRenderer->m_Lines, xiiColor::White);

  s_pDebugRenderer->m_Triangles.Clear();
  s_pDebugRenderer->m_Lines.Clear();
#endif
}

void* xiiJoltCore::JoltMalloc(size_t inSize)
{
  return xiiJoltCore::s_pAllocator->Allocate(inSize, 16);
}

void xiiJoltCore::JoltFree(void* inBlock)
{
  xiiJoltCore::s_pAllocator->Deallocate(inBlock);
}

void* xiiJoltCore::JoltAlignedMalloc(size_t inSize, size_t inAlignment)
{
  return xiiJoltCore::s_pAllocator->Allocate(inSize, inAlignment);
}

void xiiJoltCore::JoltAlignedFree(void* inBlock)
{
  xiiJoltCore::s_pAllocator->Deallocate(inBlock);
}

void xiiJoltCore::Startup()
{
  s_pAllocator = XII_DEFAULT_NEW(xiiProxyAllocator, "Jolt-Core", xiiFoundation::GetAlignedAllocator());

  JPH::Trace = JoltTraceFunc;
  JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed);
  JPH::Allocate        = xiiJoltCore::JoltMalloc;
  JPH::Free            = xiiJoltCore::JoltFree;
  JPH::AlignedAllocate = xiiJoltCore::JoltAlignedMalloc;
  JPH::AlignedFree     = xiiJoltCore::JoltAlignedFree;

  JPH::Factory::sInstance = new JPH::Factory();

  JPH::RegisterTypes();

  xiiJoltCustomShapeInfo::sRegister();

  // TODO: custom job system
  s_pJobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

  s_pDefaultMaterial = new xiiJoltMaterial;
  s_pDefaultMaterial->AddRef();
  JPH::PhysicsMaterial::sDefault = s_pDefaultMaterial;

#ifdef JPH_DEBUG_RENDERER
  s_pDebugRenderer = std::make_unique<xiiJoltDebugRenderer>();
#endif

  xiiSurfaceResource::s_Events.AddEventHandler(&xiiJoltCore::SurfaceResourceEventHandler);
}

void xiiJoltCore::Shutdown()
{
#ifdef JPH_DEBUG_RENDERER
  s_pDebugRenderer = nullptr;
#endif

  JPH::PhysicsMaterial::sDefault = nullptr;

  s_pDefaultMaterial->Release();
  s_pDefaultMaterial = nullptr;

  s_pJobSystem = nullptr;

  delete JPH::Factory::sInstance;
  JPH::Factory::sInstance = nullptr;

  JPH::Trace = nullptr;

  s_pAllocator.Clear();

  xiiSurfaceResource::s_Events.RemoveEventHandler(&xiiJoltCore::SurfaceResourceEventHandler);
}

void xiiJoltCore::SurfaceResourceEventHandler(const xiiSurfaceResourceEvent& e)
{
  if (e.m_Type == xiiSurfaceResourceEvent::Type::Created)
  {
    const auto& desc = e.m_pSurface->GetDescriptor();

    auto pNewMaterial = new xiiJoltMaterial;
    pNewMaterial->AddRef();
    pNewMaterial->m_pSurface     = e.m_pSurface;
    pNewMaterial->m_fRestitution = desc.m_fPhysicsRestitution;
    pNewMaterial->m_fFriction    = xiiMath::Lerp(desc.m_fPhysicsFrictionStatic, desc.m_fPhysicsFrictionDynamic, 0.5f);

    e.m_pSurface->m_pPhysicsMaterialJolt = pNewMaterial;
  }
  else if (e.m_Type == xiiSurfaceResourceEvent::Type::Destroyed)
  {
    if (e.m_pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      xiiJoltMaterial* pMaterial = static_cast<xiiJoltMaterial*>(e.m_pSurface->m_pPhysicsMaterialJolt);
      pMaterial->Release();

      e.m_pSurface->m_pPhysicsMaterialJolt = nullptr;
    }
  }
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltCore);
