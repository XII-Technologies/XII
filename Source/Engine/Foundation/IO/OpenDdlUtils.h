#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlParser.h>

class xiiOpenDdlReader;
class xiiOpenDdlWriter;
class xiiOpenDdlReaderElement;

namespace xiiOpenDdlUtils
{
  /// \brief Converts the data that \a pElement points to to an xiiColor.
  ///
  /// \a pElement may be a primitives list of 3 or 4 floats or of 3 or 4 unsigned int8 values.
  /// It may also be a group that contains such a primitives list as the only child.
  /// floats will be interpreted as linear colors, unsigned int 8 will be interpreted as xiiColorGammaUB.
  /// If only 3 values are given, alpha will be filled with 1.0f.
  /// If less than 3 or more than 4 values are given, the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult ConvertToColor(const xiiOpenDdlReaderElement* pElement, xiiColor& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiColorGammaUB.
  ///
  /// \a pElement may be a primitives list of 3 or 4 floats or of 3 or 4 unsigned int8 values.
  /// It may also be a group that contains such a primitives list as the only child.
  /// floats will be interpreted as linear colors, unsigned int 8 will be interpreted as xiiColorGammaUB.
  /// If only 3 values are given, alpha will be filled with 1.0f.
  /// If less than 3 or more than 4 values are given, the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult ConvertToColorGamma(const xiiOpenDdlReaderElement* pElement, xiiColorGammaUB& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiTime.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float or double.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToTime(const xiiOpenDdlReaderElement* pElement, xiiTime& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec2.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec2(const xiiOpenDdlReaderElement* pElement, xiiVec2& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec2d.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 doubles.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec2d(const xiiOpenDdlReaderElement* pElement, xiiVec2d& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec3.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec3(const xiiOpenDdlReaderElement* pElement, xiiVec3& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec3d.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 doubles.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec3d(const xiiOpenDdlReaderElement* pElement, xiiVec3d& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec4.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec4(const xiiOpenDdlReaderElement* pElement, xiiVec4& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec4d.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 doubles.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec4d(const xiiOpenDdlReaderElement* pElement, xiiVec4d& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec2I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec2I(const xiiOpenDdlReaderElement* pElement, xiiVec2I32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec2I64.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 int64.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec2I64(const xiiOpenDdlReaderElement* pElement, xiiVec2I64& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec3I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec3I(const xiiOpenDdlReaderElement* pElement, xiiVec3I32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec3I64.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 int64.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec3I64(const xiiOpenDdlReaderElement* pElement, xiiVec3I64& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec4I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec4I(const xiiOpenDdlReaderElement* pElement, xiiVec4I32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec4I64.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 int64.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec4I64(const xiiOpenDdlReaderElement* pElement, xiiVec4I64& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec2U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec2U(const xiiOpenDdlReaderElement* pElement, xiiVec2U32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec2U64.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 uint64.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec2U64(const xiiOpenDdlReaderElement* pElement, xiiVec2U64& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec3U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec3U(const xiiOpenDdlReaderElement* pElement, xiiVec3U32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec3U64.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 uint64.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec3U64(const xiiOpenDdlReaderElement* pElement, xiiVec3U64& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec4U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec4U(const xiiOpenDdlReaderElement* pElement, xiiVec4U32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiVec4U64.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 uint64.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToVec4U64(const xiiOpenDdlReaderElement* pElement, xiiVec4U64& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiMat3.
  ///
  /// \a pElement maybe be a primitives list of exactly 9 floats.
  /// The elements are expected to be in column-major format. See xiiMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToMat3(const xiiOpenDdlReaderElement* pElement, xiiMat3& out_mResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiMat3d.
  ///
  /// \a pElement maybe be a primitives list of exactly 9 doubles.
  /// The elements are expected to be in column-major format. See xiiMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToMat3d(const xiiOpenDdlReaderElement* pElement, xiiMat3d& out_mResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiMat4.
  ///
  /// \a pElement maybe be a primitives list of exactly 16 floats.
  /// The elements are expected to be in column-major format. See xiiMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToMat4(const xiiOpenDdlReaderElement* pElement, xiiMat4& out_mResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiMat4d.
  ///
  /// \a pElement maybe be a primitives list of exactly 16 doubles.
  /// The elements are expected to be in column-major format. See xiiMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToMat4d(const xiiOpenDdlReaderElement* pElement, xiiMat4d& out_mResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiTransform.
  ///
  /// \a pElement maybe be a primitives list of exactly 12 floats.
  /// The first 9 elements are expected to be a mat3 in column-major format. See xiiMatrixLayout::ColumnMajor.
  /// The last 3 elements are the position vector.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToTransform(const xiiOpenDdlReaderElement* pElement, xiiTransform& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiTransformd.
  ///
  /// \a pElement maybe be a primitives list of exactly 12 doubles.
  /// The first 9 elements are expected to be a mat3 in column-major format. See xiiMatrixLayout::ColumnMajor.
  /// The last 3 elements are the position vector.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToTransformd(const xiiOpenDdlReaderElement* pElement, xiiTransformd& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiQuat.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToQuat(const xiiOpenDdlReaderElement* pElement, xiiQuat& out_qResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiQuatd.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 doubles.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToQuatd(const xiiOpenDdlReaderElement* pElement, xiiQuatd& out_qResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiUuid.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 unsigned_int64.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToUuid(const xiiOpenDdlReaderElement* pElement, xiiUuid& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiAngle.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float.
  /// The value is assumed to be in degree.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToAngle(const xiiOpenDdlReaderElement* pElement, xiiAngle& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an xiiAngled.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float.
  /// The value is assumed to be in degree.
  /// It may also be a group that contains such a primitives list as the only child.
  XII_FOUNDATION_DLL xiiResult ConvertToAngle(const xiiOpenDdlReaderElement* pElement, xiiAngled& out_result); // [tested]

  /// \brief Uses the elements custom type name to infer which type the object holds and reads it into the xiiVariant.
  ///
  /// Depending on the custom type name, one of the other ConvertToXY functions is called and the respective conditions to the data format apply.
  /// Supported type names are: "Color", "ColorGamma", "Time", "Vec2", "Vec3", "Vec4", "Mat3", "Mat4", "Transform", "Quat", "Uuid", "Angle"
  /// Type names are case sensitive.
  XII_FOUNDATION_DLL xiiResult ConvertToVariant(const xiiOpenDdlReaderElement* pElement, xiiVariant& out_result); // [tested]

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  /// \brief Writes an xiiColor to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreColor(
    xiiOpenDdlWriter& ref_writer,
    const xiiColor&   value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiColorGammaUB to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreColorGamma(
    xiiOpenDdlWriter&      ref_writer,
    const xiiColorGammaUB& value,
    const char*            szName      = nullptr,
    bool                   bGlobalName = false); // [tested]

  /// \brief Writes an xiiTime to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreTime(xiiOpenDdlWriter& ref_writer, const xiiTime& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec2 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec2(xiiOpenDdlWriter& ref_writer, const xiiVec2& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec2d to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec2d(xiiOpenDdlWriter& ref_writer, const xiiVec2d& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec3 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec3(xiiOpenDdlWriter& ref_writer, const xiiVec3& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec3d to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec3d(xiiOpenDdlWriter& ref_writer, const xiiVec3d& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec4 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec4(xiiOpenDdlWriter& ref_writer, const xiiVec4& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec4d to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec4d(xiiOpenDdlWriter& ref_writer, const xiiVec4d& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec2I32 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec2I(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec2I32& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec2I64 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec2I64(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec2I64& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec3I32 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec3I(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec3I32& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec3I64 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec3I64(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec3I64& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec4I32 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec4I(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec4I32& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec4I64 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec4I64(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec4I64& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec2U32 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec2U(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec2U32& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec2U64 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec2U64(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec2U64& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec3U32 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec3U(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec3U32& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec3U64 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec3U64(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec3U64& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec4U32 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec4U(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec4U32& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVec4U64 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVec4U64(
    xiiOpenDdlWriter& ref_writer,
    const xiiVec4U64& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiMat3 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreMat3(xiiOpenDdlWriter& ref_writer, const xiiMat3& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiMat3d to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreMat3d(xiiOpenDdlWriter& ref_writer, const xiiMat3d& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiMat4 to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreMat4(xiiOpenDdlWriter& ref_writer, const xiiMat4& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiMat4d to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreMat4d(xiiOpenDdlWriter& ref_writer, const xiiMat4d& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiTransform to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreTransform(
    xiiOpenDdlWriter&   ref_writer,
    const xiiTransform& value,
    const char*         szName      = nullptr,
    bool                bGlobalName = false); // [tested]

  /// \brief Writes an xiiTransformd to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreTransformd(
    xiiOpenDdlWriter&    ref_writer,
    const xiiTransformd& value,
    const char*          szName      = nullptr,
    bool                 bGlobalName = false); // [tested]

  /// \brief Writes an xiiQuat to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreQuat(xiiOpenDdlWriter& ref_writer, const xiiQuat& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiQuatd to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreQuatd(xiiOpenDdlWriter& ref_writer, const xiiQuatd& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiUuid to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreUuid(xiiOpenDdlWriter& ref_writer, const xiiUuid& value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes an xiiAngle to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreAngle(
    xiiOpenDdlWriter& ref_writer,
    const xiiAngle&   value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiAngled to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreAngle(
    xiiOpenDdlWriter& ref_writer,
    const xiiAngled&   value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes an xiiVariant to DDL such that the type can be reconstructed.
  XII_FOUNDATION_DLL void StoreVariant(
    xiiOpenDdlWriter& ref_writer,
    const xiiVariant& value,
    const char*       szName      = nullptr,
    bool              bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single string and an optional name.
  XII_FOUNDATION_DLL void StoreString(
    xiiOpenDdlWriter&    ref_writer,
    const xiiStringView& value,
    const char*          szName      = nullptr,
    bool                 bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreBool(xiiOpenDdlWriter& ref_writer, bool value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreFloat(xiiOpenDdlWriter& ref_writer, float value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreDouble(xiiOpenDdlWriter& ref_writer, double value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreInt8(xiiOpenDdlWriter& ref_writer, xiiInt8 value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreInt16(xiiOpenDdlWriter& ref_writer, xiiInt16 value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreInt32(xiiOpenDdlWriter& ref_writer, xiiInt32 value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreInt64(xiiOpenDdlWriter& ref_writer, xiiInt64 value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreUInt8(xiiOpenDdlWriter& ref_writer, xiiUInt8 value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreUInt16(xiiOpenDdlWriter& ref_writer, xiiUInt16 value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreUInt32(xiiOpenDdlWriter& ref_writer, xiiUInt32 value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreUInt64(xiiOpenDdlWriter& ref_writer, xiiUInt64 value, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  XII_FOUNDATION_DLL void StoreInvalid(xiiOpenDdlWriter& ref_writer, const char* szName = nullptr, bool bGlobalName = false);

} // namespace xiiOpenDdlUtils
