#pragma once

#include <string>

#include "OBJData.hpp"

namespace polyfem::io
{
	/// @brief Read an OpenUSD stage into OBJData, preserving all mesh data.
	///
	/// The counterpart to OBJReader::read_with_groups and the inverse of
	/// USDWriter::write_with_groups. Reads the default UsdGeomMesh: points -> V,
	/// faceVertexCounts/Indices -> F (arbitrary valence, no triangulation),
	/// primvars:st -> VT/FT, primvars:normals -> VN/FN, and each UsdGeomSubset
	/// (with the polyfem:* tags written by USDWriter) -> objects/groups. The
	/// face_to_group / face_to_object maps are recomputed from the recovered
	/// group structure.
	class USDReader
	{
	public:
		USDReader() = delete;

		/// @brief Read a USD file (.usd/.usda/.usdc) into OBJData.
		/// @param usd_file_name path to the USD file
		/// @param obj_data output structure
		/// @returns true on success, false on errors
		static bool read_with_groups(
			const std::string &usd_file_name,
			polyfem::OBJData &obj_data);
	};
} // namespace polyfem::io
