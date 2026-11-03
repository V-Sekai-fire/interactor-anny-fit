// Modified version of read_obj from libigl to include reading polyline elements
// as edges.
//
// Copyright (C) 2013 Alec Jacobson <alecjacobson@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <vector>

#include <Eigen/Core>
#include "OBJData.hpp"

namespace polyfem::io
{
	class OBJReader
	{
	public:
		OBJReader() = delete;


		/// @brief Read OBJ file with group and material information
		/// @param obj_file_name path to .obj file
		/// @param obj_data output structure containing vertices, faces, groups, and materials
		/// @returns true on success, false on errors
		static bool read_with_groups(
			const std::string &obj_file_name,
			polyfem::OBJData &obj_data);
	};
} // namespace polyfem::io
