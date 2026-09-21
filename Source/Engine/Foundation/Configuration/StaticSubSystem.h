/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

/// *** Example Subsystem declarations ***
///
/// XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ExampleSubSystem)
///
///  BEGIN_SUBSYSTEM_DEPENDENCIES
///    "SomeOtherSubSystem",
///    "SomeOtherSubSystem2"
///  END_SUBSYSTEM_DEPENDENCIES
///
///  ON_CORESYSTEMS_STARTUP
///  {
///    xiiExampleSubSystem::BasicStartup();
///  }
///
///  ON_CORESYSTEMS_SHUTDOWN
///  {
///    xiiExampleSubSystem::BasicShutdown();
///  }
///
///  ON_HIGHLEVELSYSTEMS_STARTUP
///  {
///    xiiExampleSubSystem::EngineStartup();
///  }
///
///  ON_HIGHLEVELSYSTEMS_SHUTDOWN
///  {
///    xiiExampleSubSystem::EngineShutdown();
///  }
///
/// XII_END_SUBSYSTEM_DECLARATION;

/// Put this in some cpp file of a subsystem to start its startup / shutdown sequence declaration.
///
/// The first parameter is the name of the group, in which the subsystem resides, the second is the name of the subsystem itself.
#define XII_BEGIN_SUBSYSTEM_DECLARATION(GroupName, SubsystemName) \
  class GroupName##SubsystemName##SubSystem;                      \
  class GroupName##SubsystemName##SubSystem : public xiiSubSystem \
  {                                                               \
  public:                                                         \
    virtual xiiStringView GetGroupName() const override           \
    {                                                             \
      return #GroupName;                                          \
    }                                                             \
                                                                  \
  public:                                                         \
    virtual xiiStringView GetSubSystemName() const override       \
    {                                                             \
      return #SubsystemName;                                      \
    }

/// Finishes a subsystem's startup / shutdown sequence declaration.
#define XII_END_SUBSYSTEM_DECLARATION \
  }                                   \
  static XII_PP_CONCAT(s_SubSystem, XII_SOURCE_LINE)

/// Defines what code is to be executed upon base startup.
///
/// Put this inside the subsystem declaration block.
#define ON_BASESYSTEMS_STARTUP \
private:                       \
  virtual void OnBaseSystemsStartup() override

/// Defines what code is to be executed upon core startup.
///
/// Put this inside the subsystem declaration block.
#define ON_CORESYSTEMS_STARTUP \
private:                       \
  virtual void OnCoreSystemsStartup() override

/// Defines what code is to be executed upon core shutdown.
///
/// Put this inside the subsystem declaration block.
#define ON_CORESYSTEMS_SHUTDOWN \
private:                        \
  virtual void OnCoreSystemsShutdown() override

/// Defines what code is to be executed upon engine startup.
///
/// Put this inside the subsystem declaration block.
#define ON_HIGHLEVELSYSTEMS_STARTUP \
private:                            \
  virtual void OnHighLevelSystemsStartup() override

/// Defines what code is to be executed upon engine shutdown.
///
/// Put this inside the subsystem declaration block.
#define ON_HIGHLEVELSYSTEMS_SHUTDOWN \
private:                             \
  virtual void OnHighLevelSystemsShutdown() override

/// Begins the list of subsystems, on which the currently declared system depends on.
///
/// Must be followed by a series of strings with the names of the dependencies.
#define BEGIN_SUBSYSTEM_DEPENDENCIES                          \
public:                                                       \
  virtual xiiStringView GetDependency(xiiInt32 iDep) override \
  {                                                           \
    xiiStringView szDeps[] = {

/// Ends the list of subsystems, on which the currently declared system depends on.
#define END_SUBSYSTEM_DEPENDENCIES \
  , nullptr                        \
  }                                \
  ;                                \
  return szDeps[iDep];             \
  }

/// This inserts a friend declaration into a class, such that the given group/subsystem can access private functions which it might need.
#define XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GroupName, SubsystemName) friend class GroupName##SubsystemName##SubSystem
