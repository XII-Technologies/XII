/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

/// A simple registry that stores name/value pairs of types that are common to store game state
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

  void StoreBool(const xiiTempHashedString& sName, bool value);
  void StoreInteger(const xiiTempHashedString& sName, xiiInt64 value);
  void StoreDouble(const xiiTempHashedString& sName, double value);
  void StoreVec3(const xiiTempHashedString& sName, const xiiVec3& value);
  void StoreVec3d(const xiiTempHashedString& sName, const xiiVec3d& value);
  void StoreColor(const xiiTempHashedString& sName, const xiiColor& value);
  void StoreString(const xiiTempHashedString& sName, const xiiString& value);

  void RetrieveBool(const xiiTempHashedString& sName, bool& out_bValue, bool bDefaultValue = false);
  void RetrieveInteger(const xiiTempHashedString& sName, xiiInt64& out_iValue, xiiInt64 iDefaultValue = 0);
  void RetrieveDouble(const xiiTempHashedString& sName, double& out_fValue, double fDefaultValue = 0);
  void RetrieveVec3(const xiiTempHashedString& sName, xiiVec3& out_vValue, xiiVec3 vDefaultValue = xiiVec3(0));
  void RetrieveVec3d(const xiiTempHashedString& sName, xiiVec3d& out_vValue, xiiVec3d vDefaultValue = xiiVec3d(0));
  void RetrieveColor(const xiiTempHashedString& sName, xiiColor& out_value, xiiColor defaultValue = xiiColor::White);
  void RetrieveString(const xiiTempHashedString& sName, xiiString& out_sValue, xiiStringView sDefaultValue = {});

private:
  xiiHashTable<xiiTempHashedString, bool>      m_Bools;
  xiiHashTable<xiiTempHashedString, xiiInt64>  m_Integers;
  xiiHashTable<xiiTempHashedString, double>    m_Doubles;
  xiiHashTable<xiiTempHashedString, xiiVec3>   m_Vec3s;
  xiiHashTable<xiiTempHashedString, xiiVec3d>  m_Vec3ds;
  xiiHashTable<xiiTempHashedString, xiiColor>  m_Colors;
  xiiHashTable<xiiTempHashedString, xiiString> m_Strings;
};
