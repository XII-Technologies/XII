#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Physics/CollisionFilter.h>


xiiCollisionFilterConfig::xiiCollisionFilterConfig()
{
  for (int i = 0; i < 32; ++i)
  {
    xiiMemoryUtils::ZeroFill<char>(m_GroupNames[i], 32);

    m_GroupMasks[i] = 0xFFFFFFFF; // collide with everything
  }
}

void xiiCollisionFilterConfig::SetGroupName(xiiUInt32 uiGroup, xiiStringView sName)
{
  xiiStringBuilder tmp;
  xiiStringUtils::Copy(m_GroupNames[uiGroup], 32, sName.GetData(tmp));
}

xiiStringView xiiCollisionFilterConfig::GetGroupName(xiiUInt32 uiGroup) const
{
  return m_GroupNames[uiGroup];
}

void xiiCollisionFilterConfig::EnableCollision(xiiUInt32 uiGroup1, xiiUInt32 uiGroup2, bool bEnable)
{
  if (bEnable)
  {
    m_GroupMasks[uiGroup1] |= XII_BIT(uiGroup2);
    m_GroupMasks[uiGroup2] |= XII_BIT(uiGroup1);
  }
  else
  {
    m_GroupMasks[uiGroup1] &= ~XII_BIT(uiGroup2);
    m_GroupMasks[uiGroup2] &= ~XII_BIT(uiGroup1);
  }
}

bool xiiCollisionFilterConfig::IsCollisionEnabled(xiiUInt32 uiGroup1, xiiUInt32 uiGroup2) const
{
  return (m_GroupMasks[uiGroup1] & XII_BIT(uiGroup2)) != 0;
}

xiiUInt32 xiiCollisionFilterConfig::GetNumNamedGroups() const
{
  xiiUInt32 count = 0;

  for (xiiUInt32 i = 0; i < 32; ++i)
  {
    if (!xiiStringUtils::IsNullOrEmpty(m_GroupNames[i]))
      ++count;
  }

  return count;
}

xiiUInt32 xiiCollisionFilterConfig::GetNamedGroupIndex(xiiUInt32 uiGroup) const
{
  for (xiiUInt32 i = 0; i < 32; ++i)
  {
    if (!xiiStringUtils::IsNullOrEmpty(m_GroupNames[i]))
    {
      if (uiGroup == 0)
        return i;

      --uiGroup;
    }
  }

  XII_REPORT_FAILURE("Invalid index, there are not so many named collision filter groups");
  return xiiInvalidIndex;
}

xiiUInt32 xiiCollisionFilterConfig::GetFilterGroupByName(xiiStringView sName) const
{
  for (xiiUInt32 i = 0; i < 32; ++i)
  {
    if (sName.IsEqual_NoCase(m_GroupNames[i]))
      return i;
  }

  return xiiInvalidIndex;
}

xiiUInt32 xiiCollisionFilterConfig::FindUnnamedGroup() const
{
  for (xiiUInt32 i = 0; i < 32; ++i)
  {
    if (xiiStringUtils::IsNullOrEmpty(m_GroupNames[i]))
      return i;
  }

  return xiiInvalidIndex;
}

xiiResult xiiCollisionFilterConfig::Save(xiiStringView sFile) const
{
  xiiFileWriter file;
  if (file.Open(sFile).Failed())
    return XII_FAILURE;

  Save(file);

  return XII_SUCCESS;
}

xiiResult xiiCollisionFilterConfig::Load(xiiStringView sFile)
{
  xiiFileReader file;
  if (file.Open(sFile).Failed())
    return XII_FAILURE;

  Load(file);
  return XII_SUCCESS;
}

void xiiCollisionFilterConfig::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  inout_stream.WriteBytes(m_GroupMasks, sizeof(xiiUInt32) * 32).IgnoreResult();
  inout_stream.WriteBytes(m_GroupNames, sizeof(char) * 32 * 32).IgnoreResult();
}


void xiiCollisionFilterConfig::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion == 1, "Invalid version {0} for xiiCollisionFilterConfig file", uiVersion);

  inout_stream.ReadBytes(m_GroupMasks, sizeof(xiiUInt32) * 32);
  inout_stream.ReadBytes(m_GroupNames, sizeof(char) * 32 * 32);
}



XII_STATICLINK_FILE(GameEngine, GameEngine_Physics_Implementation_CollisionFilter);
