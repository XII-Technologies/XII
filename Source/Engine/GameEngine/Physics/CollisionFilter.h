#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>

/// \brief A 32x32 matrix of named filters that can be configured to enable or disable collisions
class XII_GAMEENGINE_DLL xiiCollisionFilterConfig
{
public:
  xiiCollisionFilterConfig();
  ~xiiCollisionFilterConfig();

  void SetGroupName(xiiUInt32 uiGroup, xiiStringView sName);

  xiiStringView GetGroupName(xiiUInt32 uiGroup) const;

  void EnableCollision(xiiUInt32 uiGroup1, xiiUInt32 uiGroup2, bool bEnable = true);

  bool IsCollisionEnabled(xiiUInt32 uiGroup1, xiiUInt32 uiGroup2) const;

  inline xiiUInt32 GetFilterMask(xiiUInt32 uiGroup) const { return m_GroupMasks[uiGroup]; }

  /// \brief Returns how many groups have non-empty names
  xiiUInt32 GetNumNamedGroups() const;

  /// \brief Returns the index of the n-th group that has a non-empty name (ie. maps index '3' to index '5' if there are two unnamed groups in between).
  xiiUInt32 GetNamedGroupIndex(xiiUInt32 uiGroup) const;

  /// \brief Returns xiiInvalidIndex if no group with the given name exists.
  xiiUInt32 GetFilterGroupByName(xiiStringView sName) const;

  /// \brief Searches for a group without a name and returns the index or xiiInvalidIndex if none found.
  xiiUInt32 FindUnnamedGroup() const;

  void Save(xiiStreamWriter& inout_stream) const;
  void Load(xiiStreamReader& inout_stream);

  static constexpr const xiiStringView s_sConfigFile = ":project/RuntimeConfigs/CollisionLayers.cfg"_xiisv;

  xiiResult Save(xiiStringView sFile = s_sConfigFile) const;
  xiiResult Load(xiiStringView sFile = s_sConfigFile);

private:
  xiiUInt32 m_GroupMasks[32];
  xiiString m_GroupNames[32];
};
