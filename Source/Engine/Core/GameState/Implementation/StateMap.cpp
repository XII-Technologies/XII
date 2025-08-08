#include <Core/CorePCH.h>

#include <Core/GameState/StateMap.h>

xiiStateMap::xiiStateMap()  = default;
xiiStateMap::~xiiStateMap() = default;

void xiiStateMap::Clear()
{
  m_Bools.Clear();
  m_Integers.Clear();
  m_Doubles.Clear();
  m_Vec3s.Clear();
  m_Colors.Clear();
  m_Strings.Clear();
}

void xiiStateMap::StoreBool(const xiiTempHashedString& sName, bool value)
{
  m_Bools[sName] = value;
}

void xiiStateMap::StoreInteger(const xiiTempHashedString& sName, xiiInt64 value)
{
  m_Integers[sName] = value;
}

void xiiStateMap::StoreDouble(const xiiTempHashedString& sName, double value)
{
  m_Doubles[sName] = value;
}

void xiiStateMap::StoreVec3(const xiiTempHashedString& sName, const xiiVec3& value)
{
  m_Vec3s[sName] = value;
}

void xiiStateMap::StoreVec3d(const xiiTempHashedString& sName, const xiiVec3d& value)
{
  m_Vec3ds[sName] = value;
}

void xiiStateMap::StoreColor(const xiiTempHashedString& sName, const xiiColor& value)
{
  m_Colors[sName] = value;
}

void xiiStateMap::StoreString(const xiiTempHashedString& sName, const xiiString& value)
{
  m_Strings[sName] = value;
}

void xiiStateMap::RetrieveBool(const xiiTempHashedString& sName, bool& out_bValue, bool bDefaultValue /*= false*/)
{
  if (!m_Bools.TryGetValue(sName, out_bValue))
  {
    out_bValue = bDefaultValue;
  }
}

void xiiStateMap::RetrieveInteger(const xiiTempHashedString& sName, xiiInt64& out_iValue, xiiInt64 iDefaultValue /*= 0*/)
{
  if (!m_Integers.TryGetValue(sName, out_iValue))
  {
    out_iValue = iDefaultValue;
  }
}

void xiiStateMap::RetrieveDouble(const xiiTempHashedString& sName, double& out_fValue, double fDefaultValue /*= 0*/)
{
  if (!m_Doubles.TryGetValue(sName, out_fValue))
  {
    out_fValue = fDefaultValue;
  }
}

void xiiStateMap::RetrieveVec3(const xiiTempHashedString& sName, xiiVec3& out_vValue, xiiVec3 vDefaultValue /*= xiiVec3(0)*/)
{
  if (!m_Vec3s.TryGetValue(sName, out_vValue))
  {
    out_vValue = vDefaultValue;
  }
}

void xiiStateMap::RetrieveVec3d(const xiiTempHashedString& sName, xiiVec3d& out_vValue, xiiVec3d vDefaultValue /*= xiiVec3d(0)*/)
{
  if (!m_Vec3ds.TryGetValue(sName, out_vValue))
  {
    out_vValue = vDefaultValue;
  }
}

void xiiStateMap::RetrieveColor(const xiiTempHashedString& sName, xiiColor& out_value, xiiColor defaultValue /*= xiiColor::White*/)
{
  if (!m_Colors.TryGetValue(sName, out_value))
  {
    out_value = defaultValue;
  }
}

void xiiStateMap::RetrieveString(const xiiTempHashedString& sName, xiiString& out_sValue, xiiStringView sDefaultValue /*= {}*/)
{
  if (!m_Strings.TryGetValue(sName, out_sValue))
  {
    out_sValue = sDefaultValue;
  }
}

XII_STATICLINK_FILE(Core, Core_GameState_Implementation_StateMap);
