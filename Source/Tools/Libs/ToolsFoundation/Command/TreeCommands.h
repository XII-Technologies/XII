#pragma once

#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiAddObjectCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAddObjectCommand, xiiCommand);

public:
  xiiAddObjectCommand();

public: // Properties
  void        SetType(const char* szType);
  const char* GetType() const;

  const xiiRTTI* m_pType = nullptr;
  xiiUuid        m_Parent;
  xiiString      m_sParentProperty;
  xiiVariant     m_Index;
  xiiUuid        m_NewObjectGuid; ///< This is optional. If not filled out, a new guid is assigned automatically.

private:
  virtual bool      HasReturnValues() const override { return true; }
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override;

private:
  xiiDocumentObject* m_pObject = nullptr;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class XII_TOOLSFOUNDATION_DLL xiiPasteObjectsCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPasteObjectsCommand, xiiCommand);

public:
  xiiPasteObjectsCommand();

public: // Properties
  xiiUuid   m_Parent;
  xiiString m_sGraphTextFormat;
  xiiString m_sMimeType;
  bool      m_bAllowPickedPosition = true;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override;

private:
  struct PastedObject
  {
    xiiDocumentObject* m_pObject;
    xiiDocumentObject* m_pParent;
    xiiString          m_sParentProperty;
    xiiVariant         m_Index;
  };

  xiiHybridArray<PastedObject, 4> m_PastedObjects;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiInstantiatePrefabCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInstantiatePrefabCommand, xiiCommand);

public:
  xiiInstantiatePrefabCommand();

public: // Properties
  xiiUuid   m_Parent;
  xiiInt32  m_Index = -1;
  xiiUuid   m_CreateFromPrefab;
  xiiUuid   m_RemapGuid;
  xiiString m_sBasePrefabGraph;
  xiiString m_sObjectGraph;
  xiiUuid   m_CreatedRootObject;
  bool      m_bAllowPickedPosition;

private:
  virtual bool      HasReturnValues() const override { return true; }
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override;

private:
  struct PastedObject
  {
    xiiDocumentObject* m_pObject;
    xiiDocumentObject* m_pParent;
    xiiString          m_sParentProperty;
    xiiVariant         m_Index;
  };

  // at the moment this array always only holds a single item, the group node for the prefab
  xiiHybridArray<PastedObject, 4> m_PastedObjects;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiUnlinkPrefabCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiUnlinkPrefabCommand, xiiCommand);

public:
  xiiUnlinkPrefabCommand() = default;

  xiiUuid m_Object;

private:
  virtual bool      HasReturnValues() const override { return false; }
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override {}

private:
  xiiUuid   m_OldCreateFromPrefab;
  xiiUuid   m_OldRemapGuid;
  xiiString m_sOldGraphTextFormat;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiRemoveObjectCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRemoveObjectCommand, xiiCommand);

public:
  xiiRemoveObjectCommand();

public: // Properties
  xiiUuid m_Object;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override;

private:
  xiiDocumentObject* m_pParent = nullptr;
  xiiString          m_sParentProperty;
  xiiVariant         m_Index;
  xiiDocumentObject* m_pObject = nullptr;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiMoveObjectCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMoveObjectCommand, xiiCommand);

public:
  xiiMoveObjectCommand();

public: // Properties
  xiiUuid    m_Object;
  xiiUuid    m_NewParent;
  xiiString  m_sParentProperty;
  xiiVariant m_Index;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override {}

private:
  xiiDocumentObject* m_pObject;
  xiiDocumentObject* m_pOldParent;
  xiiDocumentObject* m_pNewParent;
  xiiString          m_sOldParentProperty;
  xiiVariant         m_OldIndex;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiSetObjectPropertyCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSetObjectPropertyCommand, xiiCommand);

public:
  xiiSetObjectPropertyCommand();

public: // Properties
  xiiUuid    m_Object;
  xiiVariant m_NewValue;
  xiiVariant m_Index;
  xiiString  m_sProperty;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override {}

private:
  xiiDocumentObject* m_pObject;
  xiiVariant         m_OldValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiResizeAndSetObjectPropertyCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiResizeAndSetObjectPropertyCommand, xiiCommand);

public:
  xiiResizeAndSetObjectPropertyCommand();

public: // Properties
  xiiUuid    m_Object;
  xiiVariant m_NewValue;
  xiiVariant m_Index;
  xiiString  m_sProperty;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override { return xiiStatus(XII_SUCCESS); }
  virtual void      CleanupInternal(CommandState state) override {}

  xiiDocumentObject* m_pObject;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiInsertObjectPropertyCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInsertObjectPropertyCommand, xiiCommand);

public:
  xiiInsertObjectPropertyCommand();

public: // Properties
  xiiUuid    m_Object;
  xiiVariant m_NewValue;
  xiiVariant m_Index;
  xiiString  m_sProperty;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override {}

private:
  xiiDocumentObject* m_pObject;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiRemoveObjectPropertyCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRemoveObjectPropertyCommand, xiiCommand);

public:
  xiiRemoveObjectPropertyCommand();

public: // Properties
  xiiUuid    m_Object;
  xiiVariant m_Index;
  xiiString  m_sProperty;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override {}

private:
  xiiDocumentObject* m_pObject;
  xiiVariant         m_OldValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_TOOLSFOUNDATION_DLL xiiMoveObjectPropertyCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMoveObjectPropertyCommand, xiiCommand);

public:
  xiiMoveObjectPropertyCommand();

public: // Properties
  xiiUuid    m_Object;
  xiiVariant m_OldIndex;
  xiiVariant m_NewIndex;
  xiiString  m_sProperty;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override {}

private:
  xiiDocumentObject* m_pObject;
};
