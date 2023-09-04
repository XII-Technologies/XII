#pragma once

class xiiDocument;
class xiiDocumentManager;
class xiiDocumentObjectManager;
class xiiAbstractObjectGraph;

struct xiiDocumentFlags
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    None                 = 0,
    RequestWindow        = XII_BIT(0), ///< Open the document visibly (not only internally)
    AddToRecentFilesList = XII_BIT(1), ///< Add the document path to the recently used list for users.
    AsyncSave            = XII_BIT(2), ///<
    EmptyDocument        = XII_BIT(3), ///< Do not populate the new document with default state (Eg. templates, etcetera).

    Default = None,
  };

  struct Bits
  {
    StorageType RequestWindow : 1;
    StorageType AddToRecentFilesList : 1;
    StorageType AsyncSave : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiDocumentFlags);


struct XII_TOOLSFOUNDATION_DLL xiiDocumentTypeDescriptor
{
  xiiString           m_sFileExtension;
  xiiString           m_sDocumentTypeName;
  bool                m_bCanCreate = true;
  xiiString           m_sIcon;
  const xiiRTTI*      m_pDocumentType = nullptr;
  xiiDocumentManager* m_pManager      = nullptr;

  /// This list is used to decide which asset types can be picked from the asset browser for a property.
  /// The strings are arbitrary and don't need to be registered anywhere else.
  /// An asset may be compatible for multiple scenarios, e.g. a skinned mesh may also be used as a static mesh, but not the other way round.
  /// In such a case the skinned mesh is set to be compatible to both "CompatibleAsset_Mesh_Static" and "CompatibleAsset_Mesh_Skinned", but the non-skinned mesh only to "CompatibleAsset_Mesh_Static".
  /// A component then only needs to specify that it takes an "CompatibleAsset_Mesh_Static" as input, and all asset types that are compatible to that will be browseable.
  xiiHybridArray<xiiString, 1> m_CompatibleTypes;
};


struct xiiDocumentEvent
{
  enum class Type
  {
    ModifiedChanged,
    ReadOnlyChanged,
    EnsureVisible,
    DocumentSaved,
    DocumentRenamed,
    DocumentStatusMsg,
  };

  Type               m_Type;
  const xiiDocument* m_pDocument;

  xiiStringView m_sStatusMsg;
};

class XII_TOOLSFOUNDATION_DLL xiiDocumentInfo : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentInfo, xiiReflectedClass);

public:
  xiiDocumentInfo();

  xiiUuid m_DocumentID;
};
