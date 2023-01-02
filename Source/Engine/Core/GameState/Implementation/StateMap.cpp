#include <Core/CorePCH.h>

#include <Core/GameState/StateMap.h>

xiiStateMap::xiiStateMap() {}
xiiStateMap::~xiiStateMap() {}


void xiiStateMap::Clear()
{
  m_Bools.Clear();
  m_Integers.Clear();
  m_Doubles.Clear();
  m_Vec3s.Clear();
  m_Colors.Clear();
  m_Strings.Clear();
}

void xiiStateMap::StoreBool(const xiiTempHashedString& name, bool value)
{
  m_Bools[name] = value;
}

void xiiStateMap::StoreInteger(const xiiTempHashedString& name, xiiInt64 value)
{
  m_Integers[name] = value;
}

void xiiStateMap::StoreDouble(const xiiTempHashedString& name, double value)
{
  m_Doubles[name] = value;
}

void xiiStateMap::StoreVec3(const xiiTempHashedString& name, const xiiVec3& value)
{
  m_Vec3s[name] = value;
}

void xiiStateMap::StoreVec3d(const xiiTempHashedString& name, const xiiVec3d& value)
{
  m_Vec3ds[name] = value;
}

void xiiStateMap::StoreColor(const xiiTempHashedString& name, const xiiColor& value)
{
  m_Colors[name] = value;
}

void xiiStateMap::StoreString(const xiiTempHashedString& name, const xiiString& value)
{
  m_Strings[name] = value;
}

void xiiStateMap::RetrieveBool(const xiiTempHashedString& name, bool& out_Value, bool defaultValue /*= false*/)
{
  if (!m_Bools.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void xiiStateMap::RetrieveInteger(const xiiTempHashedString& name, xiiInt64& out_Value, xiiInt64 defaultValue /*= 0*/)
{
  if (!m_Integers.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void xiiStateMap::RetrieveDouble(const xiiTempHashedString& name, double& out_Value, double defaultValue /*= 0*/)
{
  if (!m_Doubles.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void xiiStateMap::RetrieveVec3(const xiiTempHashedString& name, xiiVec3& out_Value, xiiVec3 defaultValue /*= xiiVec3(0)*/)
{
  if (!m_Vec3s.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void xiiStateMap::RetrieveVec3d(const xiiTempHashedString& name, xiiVec3d& out_Value, xiiVec3d defaultValue /*= xiiVec3d(0)*/)
{
  if (!m_Vec3ds.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void xiiStateMap::RetrieveColor(const xiiTempHashedString& name, xiiColor& out_Value, xiiColor defaultValue /*= xiiColor::White*/)
{
  if (!m_Colors.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void xiiStateMap::RetrieveString(const xiiTempHashedString& name, xiiString& out_Value, const char* defaultValue /*= nullptr*/)
{
  if (!m_Strings.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}



XII_STATICLINK_FILE(Core, Core_GameState_Implementation_StateMap);
