/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/Reflection/NetworkAttributes.h>
#include <Foundation/Time/Timestamp.h>

struct NetworkExampleNested
{
  xiiInt32  m_iNumber = 0;
  xiiString m_sLocal  = "constructor";
};
struct NetworkExampleState
{
  xiiInt32                                m_iSequence = 0;
  bool                                    m_bActive   = true;
  xiiString                               m_sText;
  float                                   m_fQuantized = 0;
  xiiVec3                                 m_vPosition  = xiiVec3::MakeZero();
  xiiTimestamp                            m_Time       = xiiTimestamp::MakeFromInt(123456789, xiiSIUnitOfTime::Microsecond);
  xiiVariant                              m_Value;
  xiiVariantArray                         m_Array;
  xiiVariantDictionary                    m_Dictionary;
  xiiDataBuffer                           m_Data;
  NetworkExampleNested                    m_Nested;
  xiiHybridArray<NetworkExampleNested, 2> m_Children;
  xiiMap<xiiString, NetworkExampleNested> m_Map;
  xiiStaticArray<xiiInt32, 4>             m_Fixed;
  xiiString                               m_sLocal = "constructor";
  double                                  GetAccessor() const { return m_fAccessor; }
  void                                    SetAccessor(double fValue) { m_fAccessor = fValue; }
  double                                  m_fAccessor = 0;
};
struct NetworkWireFixture
{
  xiiInt32 m_iNumber = 42;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkExampleNested);
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkExampleState);
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkWireFixture);
