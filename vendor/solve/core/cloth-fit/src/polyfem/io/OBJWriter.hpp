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

#include <Eigen/Core>
#include "OBJData.hpp"

namespace polyfem::io
{
	class OBJWriter
	{
	public:
		OBJWriter() = delete;


		/// @brief Write OBJ file with group and material information
		/// @param path output file path
		/// @param obj_data structure containing vertices, faces, groups, and materials
		/// @returns true on success, false on errors
		static bool write_with_groups(
			const std::string &path,
			const polyfem::OBJData &obj_data);
	};
} // namespace polyfem::io
