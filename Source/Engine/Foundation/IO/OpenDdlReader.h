#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/OpenDdlParser.h>
#include <Foundation/Logging/Log.h>

/// \brief Represents a single 'object' in a DDL document, e.g. either a custom type or a primitives list.
class XII_FOUNDATION_DLL xiiOpenDdlReaderElement
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Whether this is a custom object type that typically contains sub-elements.
  XII_ALWAYS_INLINE bool IsCustomType() const { return m_PrimitiveType == xiiOpenDdlPrimitiveType::Custom; } // [tested]

  /// \brief Whether this is a custom object type of the requested type.
  XII_ALWAYS_INLINE bool IsCustomType(const char* szTypeName) const
  {
    return m_PrimitiveType == xiiOpenDdlPrimitiveType::Custom && xiiStringUtils::IsEqual(m_szCustomType, szTypeName);
  }

  /// \brief Returns the string for the custom type name.
  XII_ALWAYS_INLINE const char* GetCustomType() const { return m_szCustomType; } // [tested]

  /// \brief Whether the name of the object is non-empty.
  XII_ALWAYS_INLINE bool HasName() const { return !xiiStringUtils::IsNullOrEmpty(m_szName); } // [tested]

  /// \brief Returns the name of the object.
  XII_ALWAYS_INLINE const char* GetName() const { return m_szName; } // [tested]

  /// \brief Returns whether the element name is a global or a local name.
  XII_ALWAYS_INLINE bool IsNameGlobal() const { return (m_uiNumChildElements & XII_BIT(31)) != 0; } // [tested]

  /// \brief How many sub-elements the object has.
  xiiUInt32 GetNumChildObjects() const; // [tested]

  /// \brief If this is a custom type element, the returned pointer is to the first child element.
  XII_ALWAYS_INLINE const xiiOpenDdlReaderElement* GetFirstChild() const
  {
    return reinterpret_cast<const xiiOpenDdlReaderElement*>(m_pFirstChild);
  } // [tested]

  /// \brief If the parent is a custom type element, the next child after this is returned.
  XII_ALWAYS_INLINE const xiiOpenDdlReaderElement* GetSibling() const { return m_pSiblingElement; } // [tested]

  /// \brief For non-custom types this returns how many primitives are stored at this element.
  xiiUInt32 GetNumPrimitives() const; // [tested]

  /// \brief For non-custom types this returns the type of primitive that is stored at this element.
  XII_ALWAYS_INLINE xiiOpenDdlPrimitiveType GetPrimitivesType() const { return m_PrimitiveType; } // [tested]

  /// \brief Returns true if the element stores the requested type of primitives AND has at least the desired amount of them, so that accessing the
  /// data array at certain indices is safe.
  bool HasPrimitives(xiiOpenDdlPrimitiveType type, xiiUInt32 uiMinNumberOfPrimitives = 1) const;

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const bool* GetPrimitivesBool() const { return reinterpret_cast<const bool*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const xiiInt8* GetPrimitivesInt8() const { return reinterpret_cast<const xiiInt8*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const xiiInt16* GetPrimitivesInt16() const { return reinterpret_cast<const xiiInt16*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const xiiInt32* GetPrimitivesInt32() const { return reinterpret_cast<const xiiInt32*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const xiiInt64* GetPrimitivesInt64() const { return reinterpret_cast<const xiiInt64*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const xiiUInt8* GetPrimitivesUInt8() const { return reinterpret_cast<const xiiUInt8*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const xiiUInt16* GetPrimitivesUInt16() const { return reinterpret_cast<const xiiUInt16*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const xiiUInt32* GetPrimitivesUInt32() const { return reinterpret_cast<const xiiUInt32*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const xiiUInt64* GetPrimitivesUInt64() const { return reinterpret_cast<const xiiUInt64*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const float* GetPrimitivesFloat() const { return reinterpret_cast<const float*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const double* GetPrimitivesDouble() const { return reinterpret_cast<const double*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  XII_ALWAYS_INLINE const xiiStringView* GetPrimitivesString() const { return reinterpret_cast<const xiiStringView*>(m_pFirstChild); } // [tested]

  /// \brief Searches for a child with the given name. It does not matter whether the object's name is 'local' or 'global'.
  /// \a szName is case-sensitive.
  const xiiOpenDdlReaderElement* FindChild(const char* szName) const; // [tested]

  /// \brief Searches for a child element that has the given type, name and if it is a primitives list, at least the desired number of primitives.
  const xiiOpenDdlReaderElement* FindChildOfType(xiiOpenDdlPrimitiveType type, const char* szName, xiiUInt32 uiMinNumberOfPrimitives = 1) const;

  /// \brief Searches for a child element with the given type and optionally also a certain name.
  const xiiOpenDdlReaderElement* FindChildOfType(const char* szType, const char* szName = nullptr) const;

private:
  friend class xiiOpenDdlReader;

  xiiOpenDdlPrimitiveType        m_PrimitiveType;
  xiiUInt32                      m_uiNumChildElements;
  const void*                    m_pFirstChild;
  const xiiOpenDdlReaderElement* m_pLastChild;
  const char*                    m_szCustomType;
  const char*                    m_szName;
  const xiiOpenDdlReaderElement* m_pSiblingElement;
};

/// \brief An OpenDDL reader parses an entire DDL document and creates an in-memory representation of the document structure.
class XII_FOUNDATION_DLL xiiOpenDdlReader : public xiiOpenDdlParser
{
public:
  xiiOpenDdlReader();
  ~xiiOpenDdlReader();

  /// \brief Parses the given document, returns XII_FAILURE if an unrecoverable parsing error was encountered.
  ///
  /// \param stream is the input data.
  /// \param uiFirstLineOffset allows to adjust the reported line numbers in error messages, in case the given stream represents a sub-section of a
  /// larger file. \param pLog is used for outputting details about parsing errors. If nullptr is given, no details are logged. \param uiCacheSizeInKB
  /// is the internal cache size that the parser uses. If the parsed documents contain primitives lists with several thousand elements in a single
  /// list, increasing the cache size can improve performance, but typically this doesn't need to be adjusted.
  xiiResult ParseDocument(xiiStreamReader& stream, xiiUInt32 uiFirstLineOffset = 0, xiiLogInterface* pLog = xiiLog::GetThreadLocalLogSystem(),
                          xiiUInt32 uiCacheSizeInKB = 4); // [tested]

  /// \brief Every document has exactly one root element.
  const xiiOpenDdlReaderElement* GetRootElement() const; // [tested]

  /// \brief Searches for an element with a global name. NULL if there is no such element.
  const xiiOpenDdlReaderElement* FindElement(const char* szGlobalName) const; // [tested]

protected:
  virtual void OnBeginObject(const char* szType, const char* szName, bool bGlobalName) override;
  virtual void OnEndObject() override;

  virtual void OnBeginPrimitiveList(xiiOpenDdlPrimitiveType type, const char* szName, bool bGlobalName) override;
  virtual void OnEndPrimitiveList() override;

  virtual void OnPrimitiveBool(xiiUInt32 count, const bool* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveInt8(xiiUInt32 count, const xiiInt8* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt16(xiiUInt32 count, const xiiInt16* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt32(xiiUInt32 count, const xiiInt32* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt64(xiiUInt32 count, const xiiInt64* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveUInt8(xiiUInt32 count, const xiiUInt8* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt16(xiiUInt32 count, const xiiUInt16* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt32(xiiUInt32 count, const xiiUInt32* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt64(xiiUInt32 count, const xiiUInt64* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveFloat(xiiUInt32 count, const float* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveDouble(xiiUInt32 count, const double* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveString(xiiUInt32 count, const xiiStringView* pData, bool bThisIsAll) override;

  virtual void OnParsingError(const char* szMessage, bool bFatal, xiiUInt32 uiLine, xiiUInt32 uiColumn) override;

protected:
  xiiOpenDdlReaderElement* CreateElement(xiiOpenDdlPrimitiveType type, const char* szType, const char* szName, bool bGlobalName);
  const char*              CopyString(const xiiStringView& string);
  void                     StorePrimitiveData(bool bThisIsAll, xiiUInt32 bytecount, const xiiUInt8* pData);

  void      ClearDataChunks();
  xiiUInt8* AllocateBytes(xiiUInt32 uiNumBytes);

  static const xiiUInt32 s_uiChunkSize = 1000 * 4; // 4 KiB

  xiiHybridArray<xiiUInt8*, 16> m_DataChunks;
  xiiUInt8*                     m_pCurrentChunk;
  xiiUInt32                     m_uiBytesInChunkLeft;

  xiiDynamicArray<xiiUInt8> m_TempCache;

  xiiDeque<xiiOpenDdlReaderElement>            m_Elements;
  xiiHybridArray<xiiOpenDdlReaderElement*, 16> m_ObjectStack;

  xiiDeque<xiiString> m_Strings;

  xiiMap<xiiString, xiiOpenDdlReaderElement*> m_GlobalNames;
};
