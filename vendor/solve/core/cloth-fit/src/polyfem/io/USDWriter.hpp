#pragma once

#include <string>

#include "OBJData.hpp"

namespace polyfem::io
{
	/// @brief Write an OpenUSD stage from OBJData, preserving all mesh data.
	///
	/// The counterpart to OBJWriter::write_with_groups. Faces of arbitrary
	/// valence are written verbatim (no triangulation) as a polygonal
	/// UsdGeomMesh (subdivisionScheme = none): points from V, per-face valence
	/// from F, indexed UVs from VT/FT (primvars:st, faceVarying), indexed
	/// normals from VN/FN (primvars:normals, faceVarying), and each OBJ group as
	/// a UsdGeomSubset (family materialBind) carrying object/group/material names
	/// so the full OBJData structure round-trips losslessly.
	class USDWriter
	{
	public:
		USDWriter() = delete;

		/// @brief Write a USD file (.usd/.usda/.usdc) from OBJData.
		/// @param path output file path; the extension selects the USD format
		/// @param obj_data vertices, faces, UVs, normals, groups, materials
		/// @returns true on success, false on errors
		static bool write_with_groups(
			const std::string &path,
			const polyfem::OBJData &obj_data);
	};
} // namespace polyfem::io
