#pragma once

#include <Core/ActorSystem/ActorPlugin.h>
#include <Foundation/Types/UniquePtr.h>

struct xiiActorImpl;

class XII_CORE_DLL xiiActor : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiActor, xiiReflectedClass);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiActor);

public:
  xiiActor(xiiStringView sActorName, const void* pCreatedBy);
  ~xiiActor();

  /// \brief Returns the name of this actor
  xiiStringView GetName() const;

  /// \brief Returns the 'created by' pointer of the actor
  const void* GetCreatedBy() const;

  /// \brief Transfers ownership of the xiiActorPlugin to the xiiActor
  void AddPlugin(xiiUniquePtr<xiiActorPlugin>&& pPlugin);

  /// \brief Queries the xiiActor for an xiiActorPlugin of the given type. Returns null if no such plugin was added to the actor.
  xiiActorPlugin* GetPlugin(const xiiRTTI* pType) const;

  /// \brief Templated overload of GetPlugin() that automatically casts to the desired class type.
  template <typename Type>
  Type* GetPlugin() const
  {
    return static_cast<Type*>(GetPlugin(xiiGetStaticRTTI<Type>()));
  }

  /// \brief Deletes the given plugin from the actor
  void DestroyPlugin(xiiActorPlugin* pPlugin);

  /// \brief Fills the list with all plugins that have been added to the actor.
  void GetAllPlugins(xiiHybridArray<xiiActorPlugin*, 8>& out_allPlugins);

  /// \brief Checks whether the actor is queued for destruction at the end of the frame
  bool IsActorQueuedForDestruction() const
  {
    return m_State == State::QueuedForDestruction;
  }

protected:
  void UpdateAllPlugins();


protected: // directly touched by xiiActorManager
  friend class xiiActorManager;

  /// \brief Called shortly before the first call to Update()
  virtual void Activate();

  /// \brief Called once per frame to update the actor state.
  ///
  /// By default this calls UpdateAllPlugins() internally.
  virtual void Update();

private: // directly touched by xiiActorManager
  enum class State
  {
    New,
    Active,
    QueuedForDestruction
  };

  State m_State = State::New;

private:
  xiiUniquePtr<xiiActorImpl> m_pImpl;
};
