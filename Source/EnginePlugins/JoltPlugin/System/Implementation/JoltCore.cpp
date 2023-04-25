#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Factory.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Semaphore.h>
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

#if 0
class xiiJoltJobSystem final : public JPH::JobSystem
{
public:
  class xiiJoltBarrier : public JPH::JobSystem::Barrier
  {
  public:
    JPH_OVERRIDE_NEW_DELETE;

    xiiJoltBarrier()
    {
      m_Semaphore.Create().AssertSuccess();
#  if 1
      for (xiiAtomicInteger64& uiJob : m_Jobs)
      {
        uiJob = (intptr_t) nullptr;
      }
#  endif
    }

    virtual ~xiiJoltBarrier() override
    {
      XII_ASSERT_DEV(IsEmpty(), "");
    }

    virtual void AddJob(const JobHandle& inJob) override
    {
      XII_PROFILE_SCOPE(XII_SOURCE_FUNCTION);

      bool bReleaseSemaphore = false;

      // Set the barrier on the job, this returns true if the barrier was successfully set (otherwise the job is already done and we don't need to add it to our list)
      Job* pJob = inJob.GetPtr();
      if (pJob->SetBarrier(this))
      {
        // If the job can be executed we want to release the semaphore an extra time to allow the waiting thread to start executing it
        m_NumSemaphoresToAcquire.Increment();

        if (pJob->CanBeExecuted())
        {
          bReleaseSemaphore = true;
          m_NumSemaphoresToAcquire.Increment();
        }

        // Add the job to the job list
        pJob->AddRef();
        xiiInt32 uiWriteIndex = m_iJobWriteIndex.Increment();
        while ((m_iJobWriteIndex - m_iJobReadIndex) >= m_uiMaxJobs)
        {
          XII_ASSERT_DEV(false, "Barrier full, stalling execution!");
          xiiThreadUtils::Sleep(xiiTime::Microseconds(100));
        }

        m_Jobs[uiWriteIndex & (m_uiMaxJobs - 1)] = reinterpret_cast<intptr_t>(pJob);
      }

      // Notify waiting thread that a new executable job is available.
      if (bReleaseSemaphore)
        m_Semaphore.ReturnToken();
    }

    virtual void AddJobs(const JobHandle* inHandles, xiiUInt32 inNumHandles)
    {
      XII_PROFILE_SCOPE(XII_SOURCE_FUNCTION);

      bool bReleaseSemaphore = false;

      for (const JobHandle *pHandle = inHandles, *pHandlesEnd = inHandles + inNumHandles; pHandle < pHandlesEnd; ++pHandle)
      {
        // Set the barrier on the job, this returns true if the barrier was successfully set (otherwise the job is already done and we don't need to add it to our list).
        Job* pJob = pHandle->GetPtr();
        if (pJob->SetBarrier(this))
        {
          // If the job can be executed we want to release the semaphore an extra time to allow the waiting thread to start executing it.
          m_NumSemaphoresToAcquire.Increment();
          if (!bReleaseSemaphore && pJob->CanBeExecuted())
          {
            bReleaseSemaphore = true;
            m_NumSemaphoresToAcquire.Increment();
          }

          // Add the job to the job list.
          pJob->AddRef();
          xiiUInt32 uiWriteIndex = m_iJobWriteIndex.Increment();
          while ((uiWriteIndex - m_iJobReadIndex) >= m_uiMaxJobs)
          {
            XII_ASSERT_DEV(false, "Barrier full, stalling execution!");
            xiiThreadUtils::Sleep(xiiTime::Microseconds(100));
          }

          m_Jobs[uiWriteIndex & (m_uiMaxJobs - 1)] = reinterpret_cast<intptr_t>(pJob);
        }
      }

      // Notify waiting thread that a new executable job is available
      if (bReleaseSemaphore)
        m_Semaphore.ReturnToken();
    }

    XII_ALWAYS_INLINE bool IsEmpty() const { return m_iJobReadIndex == m_iJobWriteIndex; }

    void Wait()
    {
      while (m_NumSemaphoresToAcquire > 0)
      {
        {
          XII_PROFILE_SCOPE("JoltBarrier: Execute Jobs");

          // Go through all jobs
          bool bHasExecuted = false;
          do
          {
            bHasExecuted = false;

            // Loop through the jobs and erase jobs from the beginning of the list that are done.
            while (m_iJobReadIndex < m_iJobWriteIndex)
            {
              xiiUInt32           uiJobReadIndex  = m_iJobReadIndex;
              xiiUInt32           uiJobWriteIndex = m_iJobWriteIndex;
              xiiAtomicInteger64& uiJobPtr        = m_Jobs[uiJobReadIndex & (m_uiMaxJobs - 1)];

              if (uiJobPtr == 0)
                break;

              Job* pJob = reinterpret_cast<Job*>((intptr_t)uiJobPtr);
              if (!pJob->IsDone())
                break;

              // Job is finished, release it.
              pJob->Release();
              pJob     = nullptr;
              uiJobPtr = reinterpret_cast<intptr_t>(pJob);

              m_iJobReadIndex.Increment();
            }
          } while (bHasExecuted);

          // Loop through the jobs and execute the first executable job.
          for (xiiUInt32 uiIndex = m_iJobReadIndex; uiIndex < m_iJobWriteIndex; ++uiIndex)
          {
            const xiiAtomicInteger64& uiJobPtr = m_Jobs[uiIndex & (m_uiMaxJobs - 1)];

            if (uiJobPtr == 0)
              continue;

            Job* pJob = reinterpret_cast<Job*>((intptr_t)uiJobPtr);
            if (pJob->CanBeExecuted())
            {
              // This will only execute the job if it has not already executed.
              pJob->Execute();
              bHasExecuted = true;
              break;
            }
          }
        }
      }
    }

    xiiAtomicBool m_bInUse = false;

  protected:
    virtual void OnJobFinished(Job* inJob) override
    {
      XII_PROFILE_SCOPE(XII_SOURCE_FUNCTION);

      m_Semaphore.ReturnToken();
    }

    static constexpr xiiUInt32 m_uiMaxJobs = 2048;
    XII_CHECK_AT_COMPILETIME(xiiMath::IsPowerOf2(m_uiMaxJobs));

    xiiAtomicInteger64 m_Jobs[m_uiMaxJobs];

    xiiAtomicInteger32 m_iJobReadIndex;
    xiiAtomicInteger32 m_iJobWriteIndex;

    xiiSemaphore       m_Semaphore;
    xiiAtomicInteger32 m_NumSemaphoresToAcquire;
  };

  class xiiJoltTask final : public xiiTask
  {
  public:
    virtual void Execute() override
    {
      if (!m_pTask->CanBeExecuted())
        return;

      m_pTask->Execute();
      m_pTask->Release();
      m_pTask = nullptr;
    }

    JPH::JobSystem::Job* m_pTask = nullptr;
  };


  xiiInt32 GetMaxConcurrency() const override
  {
    return xiiTaskSystem::GetWorkerThreadCount(xiiWorkerThreadType::ShortTasks);
  }

  virtual JPH::JobHandle CreateJob(const char* inName, JPH::ColorArg inColor, const JobFunction& inJobFunction, xiiUInt32 inNumDependencies = 0) override
  {
    return JPH::JobHandle();
  }

private:
  virtual void QueueJob(JPH::JobSystem::Job* inJob) override
  {
    // Add reference to job because we are adding the job to the queue.
    inJob->AddRef();

    xiiSharedPtr<xiiTask> pTask;

    {
      XII_LOCK(m_Mutex);

      if (m_FreeTasks.IsEmpty())
      {
        m_TaskStorage.PushBack(XII_DEFAULT_NEW(xiiTask));
        pTask = m_TaskStorage.PeekBack();
      }
      else
      {
        pTask = m_FreeTasks.PeekBack();
        m_FreeTasks.PopBack();
      }
    }

    // pTask->ConfigureTask(in);
  }

  virtual void QueueJobs(JPH::JobSystem::Job** inJobs, xiiUInt32 inNumJobs) override
  {
  }

  virtual void FreeJob(JPH::JobSystem::Job* inJob) override
  {
  }

  virtual Barrier* CreateBarrier() override
  {
    return nullptr;
  }

  virtual void DestroyBarrier(Barrier* inBarrier) override
  {
  }

  virtual void WaitForJobs(Barrier* inBarrier)
  {
  }

private:
  xiiMutex                                                          m_Mutex;
  xiiDeque<xiiSharedPtr<xiiTask>, xiiStaticAllocatorWrapper>        m_TaskStorage;
  xiiDynamicArray<xiiSharedPtr<xiiTask>, xiiStaticAllocatorWrapper> m_FreeTasks;

  /// Array of barriers (we keep them constructed all the time since constructing a semaphore/mutex is not cheap)
  xiiUInt32       m_uiMaxBarriers = 0;
  xiiJoltBarrier* m_pBarriers     = nullptr;
};
#endif

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

static bool JoltAssertFailed(const char* szInExpression, const char* szInMessage, const char* szInFile, uint32_t inLine)
{
  return xiiFailedCheck(szInFile, inLine, "Jolt", szInExpression, szInMessage);
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

  ///\todo JoltPlugin: Add XII Custom Job System
  s_pJobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 1 /* std::thread::hardware_concurrency() - 1 */);

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
