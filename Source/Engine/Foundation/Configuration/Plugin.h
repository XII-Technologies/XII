/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

/// The data that is broadcast whenever a plugin is (un-) loaded.
struct xiiPluginEvent
{
  enum Type
  {
    BeforeLoading,          ///< Sent shortly before a new plugin is loaded.
    AfterLoadingBeforeInit, ///< Sent immediately after a new plugin has been loaded, even before it is initialized (which might trigger loading of other plugins).
    AfterLoading,           ///< Sent after a new plugin has been loaded and initialized.
    BeforeUnloading,        ///< Sent before a plugin is going to be unloaded.
    StartupShutdown,        ///< Used by the startup system for automatic shutdown.
    AfterStartupShutdown,   ///< Used by the ResourceManager to unload now unreferenced resources after the startup system shutdown is through.
    AfterUnloading,         ///< Sent after a plugin has been unloaded.
    BeforePluginChanges,    ///< Sent (once) before any (group) plugin changes (load/unload) are done.
    AfterPluginChanges,     ///< Sent (once) after all (group) plugin changes (unload/load) are finished.
  };

  Type          m_EventType;               ///< Which type of event this is.
  xiiStringView m_sPluginBinary = nullptr; ///< The file name of the affected plugin.
};

/// Flags for loading a plugin.
struct xiiPluginLoadFlags
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    LoadCopy         = XII_BIT(0), ///< Don't load a DLL directly, but create a copy of the file and load that instead. This allows to continue working on (and compiling) the DLL in parallel.
    PluginIsOptional = XII_BIT(1), ///< When an optional plugin can't be loaded (missing file usually), no error is logged. LoadPlugin() will still return XII_FAILURE though.
    CustomDependency = XII_BIT(2), ///< The plugin is an injected dependency (usually for the editor), and thus might get treated differently (this is just a tag)

    Default = 0,
  };

  struct Bits
  {
    StorageType LoadCopy : 1;
    StorageType PluginIsOptional : 1;
  };
};

using xiiPluginInitCallback = void (*)();

/// xiiPlugin manages all dynamically loadable plugins.
///
/// To load a plugin, call xiiPlugin::LoadPlugin() with the filename of the plugin (without a path).
/// The plugin DLL has to be located next to the application binary.
///
/// It is not possible to unload individual plugins, but you can unload all plugins. Make sure to only do this when no code and data
/// from any of the plugins is still referenced somewhere.
///
/// When a plugin has a dependency on another plugin, it should contain a call to XII_PLUGIN_DEPENDENCY() in one of its cpp files.
/// This instructs the system to load that plugin as well, and makes sure to delay initialization until all (transitive) dependencies are loaded.
///
/// A plugin may contain one or multiple XII_PLUGIN_ON_LOADED() and XII_PLUGIN_ON_UNLOADED() functions.
/// These are called automatically once a plugin and all its dependencies are loaded. This can be used to make sure basic things get set up.
/// For anything more complicated, use xiiStartup instead. Once a plugin is loaded, the startup system will initialize new startup code properly.
class XII_FOUNDATION_DLL xiiPlugin
{
public:
  /// Code that needs to be execute whenever a plugin is loaded or unloaded can register itself here to be notified of such events.
  static const xiiCopyOnBroadcastEvent<const xiiPluginEvent&>& Events(); // [tested]

  /// Calls the XII_PLUGIN_ON_LOADED() functions for all code that is already linked into the executable at startup.
  ///
  /// If code that was meant to be loaded dynamically ends up being statically linked (e.g. on platforms where only static linking is used),
  /// the XII_PLUGIN_ON_LOADED() functions should still be called. The application can decide when the best time is.
  /// Usually a good point in time is right before the app would load the first dynamic plugin.
  /// If this function is never called manually, but xiiPlugin::LoadPlugin() is called, this function will be called automatically before loading the first actual plugin.
  static void InitializeStaticallyLinkedPlugins(); // [tested]

  /// Call this before loading / unloading several plugins in a row, to prevent unnecessary re-initializations.
  static void BeginPluginChanges();

  /// Must be called to finish what BeginPluginChanges started.
  static void EndPluginChanges();

  /// Checks whether a plugin with the given name exists. Does not guarantee that the plugin could be loaded successfully.
  static bool ExistsPluginFile(xiiStringView sPluginFile);

  /// Tries to load a DLL dynamically into the program.
  ///
  /// XII_SUCCESS is returned when the DLL is either successfully loaded or has already been loaded before.
  /// XII_FAILURE is returned if the DLL cannot be located or it could not be loaded properly.
  ///
  /// See xiiPluginLoadFlags for additional options.
  static xiiResult LoadPlugin(xiiStringView sPluginFile, xiiBitflags<xiiPluginLoadFlags> flags = xiiPluginLoadFlags::Default); // [tested]

  /// Unloads all previously loaded plugins in the reverse order in which they were loaded.
  ///
  /// Also calls XII_PLUGIN_ON_UNLOADED() of all statically linked code.
  static void UnloadAllPlugins(); // [tested]

  /// Sets how many tries the system will do to find a free plugin file name.
  ///
  /// During plugin loading the system may create copies of the plugin DLLs. This only works if the system can find a
  /// file to write to. If too many instances of the engine are running, no such free file name might be found and plugin loading fails.
  /// This value specifies how often the system tries to find a free file. The default is 32.
  static void SetMaxParallelInstances(xiiUInt32 uiMaxParallelInstances);

  /// \internal struct used by xiiPlugin macros
  struct XII_FOUNDATION_DLL Init
  {
    Init(xiiPluginInitCallback onLoadOrUnloadCB, bool bOnLoad);
    Init(xiiStringView sAddPluginDependency);
  };

  /// Contains basic information about a loaded plugin.
  struct XII_FOUNDATION_DLL PluginInfo
  {
    xiiString                       m_sName;
    xiiHybridArray<xiiString, 2>    m_sDependencies;
    xiiBitflags<xiiPluginLoadFlags> m_LoadFlags;
  };

  /// Returns information about all currently loaded plugins.
  static void GetAllPluginInfos(xiiDynamicArray<PluginInfo>& ref_infos);

  /// \internal Determines the plugin paths.
  static void GetPluginPaths(xiiStringView sPluginName, xiiStringBuilder& ref_sOriginalFile, xiiStringBuilder& ref_sCopiedFile, xiiUInt8 uiFileCopyNumber);

  /// \internal Determines if a plugin copy is required for hot reloading for plugin code.
  static bool PlatformNeedsPluginCopy();

private:
  xiiPlugin() = delete;
};

/// Adds a dependency on another plugin to the plugin in which this call is located.
///
/// If Plugin2 requires Plugin1 to be loaded when Plugin2 is used, insert this into a CPP file of Plugin2:\n
/// XII_PLUGIN_DEPENDENCY(Plugin1);
///
/// That instructs the xiiPlugin system to make sure that Plugin1 gets loaded and initialized before Plugin2 is initialized.
#define XII_PLUGIN_DEPENDENCY(PluginName) \
  xiiPlugin::Init XII_PP_CONCAT(XII_PP_CONCAT(plugin_dep_, PluginName), XII_SOURCE_LINE)(XII_PP_STRINGIFY(PluginName))

/// Creates a function that is executed when the plugin gets loaded.
///
/// Just insert XII_PLUGIN_ON_LOADED() { /* function body */ } into a CPP file of a plugin to add a function that is called
/// right after the plugin got loaded.
/// If the plugin has depenencies (set via XII_PLUGIN_DEPENDENCY()), it is guaranteed that all ON_LOADED functions of the
/// dependencies are called first.
/// If there are multiple such functions defined within the same DLL, there is no guarantee in which order they are called.
#define XII_PLUGIN_ON_LOADED()                                \
  static void     plugin_OnLoaded();                          \
  xiiPlugin::Init plugin_OnLoadedInit(plugin_OnLoaded, true); \
  static void     plugin_OnLoaded()

/// Creates a function that is executed when the plugin gets unloaded.
///
/// This is typically the case when the application shuts down.
///
/// Just insert XII_PLUGIN_ON_UNLOADED() { /* function body */ } into a CPP file of a plugin to add a function that is called
/// right before the plugin gets unloaded.
/// ON_UNLOADED function calls across DLLs are done in reverse order to the ON_LOADED function calls.
/// If there are multiple such functions defined within the same DLL, there is no guarantee in which order they are called.
#define XII_PLUGIN_ON_UNLOADED()                                   \
  static void     plugin_OnUnloaded();                             \
  xiiPlugin::Init plugin_OnUnloadedInit(plugin_OnUnloaded, false); \
  static void     plugin_OnUnloaded()
