#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief A 32x32 matrix of named filters that can be configured to enable or disable collisions
class XII_GAMEENGINE_DLL xiiCollisionFilterConfig
{
public:
  xiiCollisionFilterConfig();

  void SetGroupName(xiiUInt32 uiGroup, const char* szName);

  const char* GetGroupName(xiiUInt32 uiGroup) const;

  void EnableCollision(xiiUInt32 uiGroup1, xiiUInt32 uiGroup2, bool bEnable = true);

  bool IsCollisionEnabled(xiiUInt32 uiGroup1, xiiUInt32 uiGroup2) const;

  inline xiiUInt32 GetFilterMask(xiiUInt32 uiGroup) const { return m_GroupMasks[uiGroup]; }

  /// \brief Returns how many groups have non-empty names
  xiiUInt32 GetNumNamedGroups() const;

  /// \brief Returns the index of the n-th group that has a non-empty name (ie. maps index '3' to index '5' if there are two unnamed groups in
  /// between)
  xiiUInt32 GetNamedGroupIndex(xiiUInt32 uiGroup) const;

  /// \brief Returns -1 if no group with the given name exists.
  xiiInt32 GetFilterGroupByName(const char* szName) const;

  /// \brief Searches for a group without a name and returns the index or -1 if none found.
  xiiInt32 FindUnnamedGroup() const;

  void Save(xiiStreamWriter& stream) const;
  void Load(xiiStreamReader& stream);

  xiiResult Save(const char* szFile) const;
  xiiResult Load(const char* szFile);


private:
  xiiUInt32 m_GroupMasks[32];
  char      m_GroupNames[32][32];
};
