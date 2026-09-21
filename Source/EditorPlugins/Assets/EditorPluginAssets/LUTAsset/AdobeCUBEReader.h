/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Types/Status.h>

class xiiLogInterface;
class xiiStreamReader;

/// Simple implementation to read Adobe CUBE LUT files
///
/// Currently only reads 3D LUTs as this is the data we need for our lookup textures in the tone mapping step.
class xiiAdobeCUBEReader
{
public:
  xiiAdobeCUBEReader();
  ~xiiAdobeCUBEReader();

  xiiStatus ParseFile(xiiStreamReader& inout_stream, xiiLogInterface* pLog = nullptr);

  xiiVec3 GetDomainMin() const;
  xiiVec3 GetDomainMax() const;

  xiiUInt32        GetLUTSize() const;
  const xiiString& GetTitle() const;

  xiiVec3 GetLUTEntry(xiiUInt32 r, xiiUInt32 g, xiiUInt32 b) const;

protected:
  xiiUInt32 m_uiLUTSize = 0;
  xiiString m_sTitle    = "<UNTITLED>";

  xiiVec3 m_vDomainMin = xiiVec3::MakeZero();
  xiiVec3 m_vDomainMax = xiiVec3(1.0f);

  xiiDynamicArray<xiiVec3> m_LUTValues;

  xiiUInt32 GetLUTIndex(xiiUInt32 r, xiiUInt32 g, xiiUInt32 b) const;
};
