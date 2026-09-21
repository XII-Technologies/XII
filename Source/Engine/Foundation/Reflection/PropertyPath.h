/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>

class xiiAbstractProperty;

///Reflected property step that can be used to init a xiiPropertyPath
struct XII_FOUNDATION_DLL xiiPropertyPathStep
{
  xiiString  m_sProperty;
  xiiVariant m_Index;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiPropertyPathStep);

///Stores a path from an object of a given type to a property inside of it.
/// Once initialized to a specific path, the target property/object of the path can be read or written on
/// multiple root objects.
/// An empty path is allowed in which case WriteToLeafObject/ReadFromLeafObject will return pRootObject directly.
///
/// TODO: read/write methods and ResolvePath should return a failure state.
class XII_FOUNDATION_DLL xiiPropertyPath
{
public:
  xiiPropertyPath();
  ~xiiPropertyPath();

  /// Returns true if InitializeFromPath() has been successfully called and it is therefore possible to use the other functions.
  bool IsValid() const;

  ///Resolves a path in the syntax 'propertyName[index]/propertyName[index]/...' into steps.
  /// The '[index]' part is only added for properties that require indices (arrays and maps).
  xiiResult InitializeFromPath(const xiiRTTI& rootObjectRtti, xiiStringView sPath);
  ///Resolves a path provided as an array of xiiPropertyPathStep.
  xiiResult InitializeFromPath(const xiiRTTI* pRootObjectRtti, const xiiArrayPtr<const xiiPropertyPathStep> path);

  ///Applies the entire path and allows writing to the target object.
  xiiResult WriteToLeafObject(void* pRootObject, const xiiRTTI* pType, xiiDelegate<void(void* pLeaf, const xiiRTTI& pType)> func) const;
  ///Applies the entire path and allows reading from the target object.
  xiiResult ReadFromLeafObject(void* pRootObject, const xiiRTTI* pType, xiiDelegate<void(void* pLeaf, const xiiRTTI& pType)> func) const;

  ///Applies the path up to the last step and allows a functor to write to the final property.
  xiiResult WriteProperty(void* pRootObject, const xiiRTTI& type, xiiDelegate<void(void* pLeafObject, const xiiRTTI& pLeafType, const xiiAbstractProperty* pProp, const xiiVariant& index)> func) const;
  ///Applies the path up to the last step and allows a functor to read from the final property.
  xiiResult ReadProperty(void* pRootObject, const xiiRTTI& type, xiiDelegate<void(void* pLeafObject, const xiiRTTI& pLeafType, const xiiAbstractProperty* pProp, const xiiVariant& index)> func) const;

  ///Convenience function that writes 'value' to the 'pRootObject' at the current path.
  void SetValue(void* pRootObject, const xiiRTTI& type, const xiiVariant& value) const;
  ///Convenience function that writes 'value' to the 'pRootObject' at the current path.
  template <typename T>
  XII_ALWAYS_INLINE void SetValue(T* pRootObject, const xiiVariant& value) const
  {
    SetValue(pRootObject, *xiiGetStaticRTTI<T>(), value);
  }

  ///Convenience function that reads the value from 'pRootObject' at the current path and stores it in 'out_value'.
  void GetValue(void* pRootObject, const xiiRTTI& type, xiiVariant& out_value) const;
  ///Convenience function that reads the value from 'pRootObject' at the current path and stores it in 'out_value'.
  template <typename T>
  XII_ALWAYS_INLINE void GetValue(T* pRootObject, xiiVariant& out_value) const
  {
    GetValue(pRootObject, *xiiGetStaticRTTI<T>(), out_value);
  }

private:
  struct ResolvedStep
  {
    const xiiAbstractProperty* m_pProperty = nullptr;
    xiiVariant                 m_Index;
  };

  static xiiResult ResolvePath(void* pCurrentObject, const xiiRTTI* pType, const xiiArrayPtr<const ResolvedStep> path, bool bWriteToObject, const xiiDelegate<void(void* pLeaf, const xiiRTTI& pType)>& func);

  bool                            m_bIsValid = false;
  xiiHybridArray<ResolvedStep, 2> m_PathSteps;
};
