#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/Plugin_Win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Posix/Plugin_Posix.h>
#else
#  error "Plugins not implemented on this Platform."
#endif

xiiResult UnloadPluginModule(xiiPluginModule& ref_pModule, xiiStringView sPluginFile);
xiiResult LoadPluginModule(xiiStringView sFileToLoad, xiiPluginModule& ref_pModule, xiiStringView sPluginFile);

xiiDynamicArray<xiiString>& GetStaticPlugins()
{
  static xiiDynamicArray<xiiString> s_StaticPlugins;
  return s_StaticPlugins;
}

struct ModuleData
{
  xiiPluginModule                          m_hModule       = 0;
  xiiUInt8                                 m_uiFileNumber  = 0;
  bool                                     m_bCalledOnLoad = false;
  xiiHybridArray<xiiPluginInitCallback, 2> m_OnLoadCB;
  xiiHybridArray<xiiPluginInitCallback, 2> m_OnUnloadCB;
  xiiHybridArray<xiiString, 2>             m_sPluginDependencies;
  xiiBitflags<xiiPluginLoadFlags>          m_LoadFlags;

  void Initialize();
  void Uninitialize();
};

static ModuleData                    g_StaticModule;
static ModuleData*                   g_pCurrentlyLoadingModule = nullptr;
static xiiMap<xiiString, ModuleData> g_LoadedModules;
static xiiDynamicArray<xiiString>    s_PluginLoadOrder;
static xiiUInt32                     s_uiMaxParallelInstances        = 32;
static xiiInt32                      s_iPluginChangeRecursionCounter = 0;

xiiCopyOnBroadcastEvent<const xiiPluginEvent&> s_PluginEvents;

void xiiPlugin::SetMaxParallelInstances(xiiUInt32 uiMaxParallelInstances)
{
  s_uiMaxParallelInstances = xiiMath::Max(1u, uiMaxParallelInstances);
}

void xiiPlugin::InitializeStaticallyLinkedPlugins()
{
  if (!g_StaticModule.m_bCalledOnLoad)
  {
    // We need to trigger the xiiPlugin events to make sure the sub-systems are initialized at least once.
    xiiPlugin::BeginPluginChanges();
    XII_SCOPE_EXIT(xiiPlugin::EndPluginChanges());
    g_StaticModule.Initialize();

#if XII_DISABLED(XII_COMPILE_ENGINE_AS_DLL)
    XII_LOG_BLOCK("Initialize Statically Linked Plugins");
    // Merely add dummy entries so plugins can be enumerated etc.
    for (xiiStringView sPlugin : GetStaticPlugins())
    {
      g_LoadedModules.FindOrAdd(sPlugin);
      xiiLog::Debug("Plugin '{0}' statically linked.", sPlugin);
    }
#endif
  }
}

void xiiPlugin::GetAllPluginInfos(xiiDynamicArray<PluginInfo>& ref_infos)
{
  ref_infos.Clear();

  ref_infos.Reserve(g_LoadedModules.GetCount());

  for (auto mod : g_LoadedModules)
  {
    auto& pi           = ref_infos.ExpandAndGetRef();
    pi.m_sName         = mod.Key();
    pi.m_sDependencies = mod.Value().m_sPluginDependencies;
    pi.m_LoadFlags     = mod.Value().m_LoadFlags;
  }
}

void ModuleData::Initialize()
{
  if (m_bCalledOnLoad)
    return;

  m_bCalledOnLoad = true;

  for (const auto& dep : m_sPluginDependencies)
  {
    // TODO: ignore ??
    xiiPlugin::LoadPlugin(dep).IgnoreResult();
  }

  for (auto cb : m_OnLoadCB)
  {
    cb();
  }
}

void ModuleData::Uninitialize()
{
  if (!m_bCalledOnLoad)
    return;

  for (xiiUInt32 i = m_OnUnloadCB.GetCount(); i > 0; --i)
  {
    m_OnUnloadCB[i - 1]();
  }

  m_bCalledOnLoad = false;
}

void xiiPlugin::BeginPluginChanges()
{
  if (s_iPluginChangeRecursionCounter == 0)
  {
    xiiPluginEvent e;
    e.m_EventType = xiiPluginEvent::BeforePluginChanges;
    s_PluginEvents.Broadcast(e);
  }

  ++s_iPluginChangeRecursionCounter;
}

void xiiPlugin::EndPluginChanges()
{
  --s_iPluginChangeRecursionCounter;

  if (s_iPluginChangeRecursionCounter == 0)
  {
    xiiPluginEvent e;
    e.m_EventType = xiiPluginEvent::AfterPluginChanges;
    s_PluginEvents.Broadcast(e);
  }
}

static xiiResult UnloadPluginInternal(xiiStringView sPluginFile)
{
  auto thisMod = g_LoadedModules.Find(sPluginFile);

  if (!thisMod.IsValid())
    return XII_SUCCESS;

  xiiLog::Debug("Plugin to unload: \"{0}\"", sPluginFile);

  xiiPlugin::BeginPluginChanges();
  XII_SCOPE_EXIT(xiiPlugin::EndPluginChanges());

  // Broadcast event: Before unloading plugin
  {
    xiiPluginEvent e;
    e.m_EventType     = xiiPluginEvent::BeforeUnloading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: Startup Shutdown
  {
    xiiPluginEvent e;
    e.m_EventType     = xiiPluginEvent::StartupShutdown;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: After Startup Shutdown
  {
    xiiPluginEvent e;
    e.m_EventType     = xiiPluginEvent::AfterStartupShutdown;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  thisMod.Value().Uninitialize();

  // unload the plugin module
  if (UnloadPluginModule(thisMod.Value().m_hModule, sPluginFile) == XII_FAILURE)
  {
    xiiLog::Error("Unloading plugin module '{}' failed.", sPluginFile);
    return XII_FAILURE;
  }

  // delete the plugin copy that we had loaded
  if (xiiPlugin::PlatformNeedsPluginCopy())
  {
    xiiStringBuilder sOriginalFile, sCopiedFile;
    xiiPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, g_LoadedModules[sPluginFile].m_uiFileNumber);

    xiiOSFile::DeleteFile(sCopiedFile).IgnoreResult();
  }

  // Broadcast event: After unloading plugin
  {
    xiiPluginEvent e;
    e.m_EventType     = xiiPluginEvent::AfterUnloading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  xiiLog::Success("Plugin '{0}' is unloaded.", sPluginFile);
  g_LoadedModules.Remove(thisMod);

  return XII_SUCCESS;
}

#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)

static xiiResult LoadPluginInternal(xiiStringView sPluginFile, xiiBitflags<xiiPluginLoadFlags> flags)
{
  xiiUInt8 uiFileNumber = 0;

  xiiStringBuilder sOriginalFile, sCopiedFile;
  xiiPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, uiFileNumber);

  if (!xiiOSFile::ExistsFile(sOriginalFile))
  {
    xiiLog::Error("The plugin '{0}' does not exist.", sPluginFile);
    return XII_FAILURE;
  }

  if (xiiPlugin::PlatformNeedsPluginCopy() && flags.IsSet(xiiPluginLoadFlags::LoadCopy))
  {
    // create a copy of the original plugin file
    const xiiUInt8 uiMaxParallelInstances = static_cast<xiiUInt8>(s_uiMaxParallelInstances);
    for (uiFileNumber = 0; uiFileNumber < uiMaxParallelInstances; ++uiFileNumber)
    {
      xiiPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, uiFileNumber);
      if (xiiOSFile::CopyFile(sOriginalFile, sCopiedFile) == XII_SUCCESS)
        goto Success;
    }

    xiiLog::Error("Could not copy the plugin file '{0}' to '{1}' (and all previous file numbers). Plugin MaxParallelInstances is set to {2}.", sOriginalFile, sCopiedFile, s_uiMaxParallelInstances);

    g_LoadedModules.Remove(sCopiedFile);
    return XII_FAILURE;
  }
  else
  {
    sCopiedFile = sOriginalFile;
  }

Success:

  auto& thisMod          = g_LoadedModules[sPluginFile];
  thisMod.m_uiFileNumber = uiFileNumber;
  thisMod.m_LoadFlags    = flags;

  xiiPlugin::BeginPluginChanges();
  XII_SCOPE_EXIT(xiiPlugin::EndPluginChanges());

  // Broadcast Event: Before loading plugin
  {
    xiiPluginEvent e;
    e.m_EventType     = xiiPluginEvent::BeforeLoading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  g_pCurrentlyLoadingModule = &thisMod;

  if (LoadPluginModule(sCopiedFile, g_pCurrentlyLoadingModule->m_hModule, sPluginFile) == XII_FAILURE)
  {
    // loaded, but failed
    g_pCurrentlyLoadingModule = nullptr;
    thisMod.m_hModule         = 0;

    return XII_FAILURE;
  }

  g_pCurrentlyLoadingModule = nullptr;

  {
    // Broadcast Event: After loading plugin, before init
    {
      xiiPluginEvent e;
      e.m_EventType     = xiiPluginEvent::AfterLoadingBeforeInit;
      e.m_sPluginBinary = sPluginFile;
      s_PluginEvents.Broadcast(e);
    }

    thisMod.Initialize();

    // Broadcast Event: After loading plugin
    {
      xiiPluginEvent e;
      e.m_EventType     = xiiPluginEvent::AfterLoading;
      e.m_sPluginBinary = sPluginFile;
      s_PluginEvents.Broadcast(e);
    }
  }

  xiiLog::Success("Plugin '{0}' is loaded.", sPluginFile);
  return XII_SUCCESS;
}

#endif

bool xiiPlugin::ExistsPluginFile(xiiStringView sPluginFile)
{
  xiiStringBuilder sOriginalFile, sCopiedFile;
  GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, 0);

  return xiiOSFile::ExistsFile(sOriginalFile);
}

xiiResult xiiPlugin::LoadPlugin(xiiStringView sPluginFile, xiiBitflags<xiiPluginLoadFlags> flags /*= xiiPluginLoadFlags::Default*/)
{
  XII_LOG_BLOCK("Loading Plugin", sPluginFile);

  // make sure this is done first
  InitializeStaticallyLinkedPlugins();

  if (g_LoadedModules.Find(sPluginFile).IsValid())
  {
    xiiLog::Debug("Plugin '{0}' already loaded.", sPluginFile);
    return XII_SUCCESS;
  }

#if XII_DISABLED(XII_COMPILE_ENGINE_AS_DLL)
  // #TODO XII_COMPILE_ENGINE_AS_DLL and being able to load plugins are not necessarily the same thing.
  XII_IGNORE_UNUSED(flags);
  return XII_FAILURE;
#else

  if (flags.IsSet(xiiPluginLoadFlags::PluginIsOptional))
  {
    // early out without logging an error
    if (!ExistsPluginFile(sPluginFile))
      return XII_FAILURE;
  }

  xiiLog::Debug("Plugin to load: \"{0}\"", sPluginFile);

  // make sure to use a static string pointer from now on, that stays where it is
  sPluginFile = g_LoadedModules.FindOrAdd(sPluginFile).Key();

  xiiResult res = LoadPluginInternal(sPluginFile, flags);

  if (res.Succeeded())
  {
    s_PluginLoadOrder.PushBack(sPluginFile);
  }
  else
  {
    // If we failed to load the plugin, it shouldn't be in the loaded modules list
    g_LoadedModules.Remove(sPluginFile);
  }

  return res;
#endif
}

void xiiPlugin::UnloadAllPlugins()
{
  BeginPluginChanges();
  XII_SCOPE_EXIT(EndPluginChanges());

  for (xiiUInt32 i = s_PluginLoadOrder.GetCount(); i > 0; --i)
  {
    if (UnloadPluginInternal(s_PluginLoadOrder[i - 1]).Failed())
    {
      // not sure what to do
    }
  }

  XII_ASSERT_DEBUG(g_LoadedModules.IsEmpty(), "Not all plugins were unloaded somehow.");

  for (auto mod : g_LoadedModules)
  {
    mod.Value().Uninitialize();
  }

  // also shut down all plugin objects that are statically linked
  g_StaticModule.Uninitialize();

  s_PluginLoadOrder.Clear();
  g_LoadedModules.Clear();
}

const xiiCopyOnBroadcastEvent<const xiiPluginEvent&>& xiiPlugin::Events()
{
  return s_PluginEvents;
}

xiiPlugin::Init::Init(xiiPluginInitCallback onLoadOrUnloadCB, bool bOnLoad)
{
  ModuleData* pMD = g_pCurrentlyLoadingModule ? g_pCurrentlyLoadingModule : &g_StaticModule;

  if (bOnLoad)
  {
    pMD->m_OnLoadCB.PushBack(onLoadOrUnloadCB);
  }
  else
  {
    pMD->m_OnUnloadCB.PushBack(onLoadOrUnloadCB);
  }
}

xiiPlugin::Init::Init(xiiStringView sAddPluginDependency)
{
  ModuleData* pMD = g_pCurrentlyLoadingModule ? g_pCurrentlyLoadingModule : &g_StaticModule;

  pMD->m_sPluginDependencies.PushBack(sAddPluginDependency);
}

#if XII_DISABLED(XII_COMPILE_ENGINE_AS_DLL)
xiiPluginRegister::xiiPluginRegister(xiiStringView sAddPlugin)
{
  if (g_pCurrentlyLoadingModule == nullptr)
  {
    GetStaticPlugins().PushBack(sAddPlugin);
  }
}
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Plugin);
