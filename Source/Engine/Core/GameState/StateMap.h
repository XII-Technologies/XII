#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

/// \brief A simple registry that stores name/value pairs of types that are common to store game state
///
class XII_CORE_DLL xiiStateMap
{
public:
  xiiStateMap();
  ~xiiStateMap();

  /// void Load(xiiStreamReader& stream);
  /// void Save(xiiStreamWriter& stream) const;
  /// Lock / Unlock

  void Clear();

  void StoreBool(const xiiTempHashedString& name, bool value);
  void StoreInteger(const xiiTempHashedString& name, xiiInt64 value);
  void StoreDouble(const xiiTempHashedString& name, double value);
  void StoreVec3(const xiiTempHashedString& name, const xiiVec3& value);
  void StoreVec3d(const xiiTempHashedString& name, const xiiVec3d& value);
  void StoreColor(const xiiTempHashedString& name, const xiiColor& value);
  void StoreString(const xiiTempHashedString& name, const xiiString& value);

  void RetrieveBool(const xiiTempHashedString& name, bool& out_Value, bool defaultValue = false);
  void RetrieveInteger(const xiiTempHashedString& name, xiiInt64& out_Value, xiiInt64 defaultValue = 0);
  void RetrieveDouble(const xiiTempHashedString& name, double& out_Value, double defaultValue = 0);
  void RetrieveVec3(const xiiTempHashedString& name, xiiVec3& out_Value, xiiVec3 defaultValue = xiiVec3(0));
  void RetrieveVec3d(const xiiTempHashedString& name, xiiVec3d& out_Value, xiiVec3d defaultValue = xiiVec3d(0));
  void RetrieveColor(const xiiTempHashedString& name, xiiColor& out_Value, xiiColor defaultValue = xiiColor::White);
  void RetrieveString(const xiiTempHashedString& name, xiiString& out_Value, const char* defaultValue = nullptr);

private:
  xiiHashTable<xiiTempHashedString, bool>      m_Bools;
  xiiHashTable<xiiTempHashedString, xiiInt64>  m_Integers;
  xiiHashTable<xiiTempHashedString, double>    m_Doubles;
  xiiHashTable<xiiTempHashedString, xiiVec3>   m_Vec3s;
  xiiHashTable<xiiTempHashedString, xiiVec3d>  m_Vec3ds;
  xiiHashTable<xiiTempHashedString, xiiColor>  m_Colors;
  xiiHashTable<xiiTempHashedString, xiiString> m_Strings;
};
