// Modified version of read_obj from libigl to include reading polyline elements
// as edges.
//
// Copyright (C) 2013 Alec Jacobson <alecjacobson@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

#include "OBJWriter.hpp"

#include <polyfem/utils/Logger.hpp>

#include <fstream>

namespace polyfem::io
{

	bool OBJWriter::write_with_groups(
		const std::string &path,
		const polyfem::OBJData &obj_data)
	{
		std::ofstream obj(path, std::ios::out);
		if (!obj.is_open())
		{
			logger().error("OBJWriter::write_with_groups: could not open file {:s} for writing", path);
			return false;
		}

		// Write header comment
		obj << fmt::format(
			"# Vertices: {:d}\n# Faces: {:d}\n# Objects: {:d}\n",
			obj_data.V.size(), obj_data.F.size(), obj_data.objects.size());

		// Write MTL library reference if present
		if (!obj_data.mtl_filename.empty())
		{
			obj << fmt::format("mtllib {}\n", obj_data.mtl_filename);
		}

		obj << "\n";

		// Write all vertices first
		for (const auto &vertex : obj_data.V)
		{
			obj << "v";
			for (double coord : vertex)
			{
				obj << fmt::format(" {}", coord);
			}
			obj << "\n";
		}

		obj << "\n";

		// Write texture coordinates if present
		if (!obj_data.VT.empty())
		{
			for (const auto &tex_coord : obj_data.VT)
			{
				obj << "vt";
				for (double coord : tex_coord)
				{
					obj << fmt::format(" {}", coord);
				}
				obj << "\n";
			}
			obj << "\n";
		}

		// Write vertex normals if present
		if (!obj_data.VN.empty())
		{
			for (const auto &normal : obj_data.VN)
			{
				obj << "vn";
				for (double coord : normal)
				{
					obj << fmt::format(" {}", coord);
				}
				obj << "\n";
			}
			obj << "\n";
		}

		// Write objects and groups with their faces
		for (size_t obj_idx = 0; obj_idx < obj_data.objects.size(); ++obj_idx)
		{
			const auto &object = obj_data.objects[obj_idx];

			// Write object declaration
			obj << fmt::format("o {}\n", object.name);

			for (const auto &group : object.groups)
			{
				// Write group declaration if not default
				if (group.name != "default")
				{
					obj << fmt::format("g {}\n", group.name);
				}

				// Write material usage if present
				if (!group.material_name.empty())
				{
					obj << fmt::format("usemtl {}\n", group.material_name);
				}

				// Write faces belonging to this group
				for (int face_idx : group.face_indices)
				{
					const auto &face = obj_data.F[face_idx];
					const auto &face_tex = (face_idx < obj_data.FT.size()) ? obj_data.FT[face_idx] : std::vector<int>();
					const auto &face_norm = (face_idx < obj_data.FN.size()) ? obj_data.FN[face_idx] : std::vector<int>();

					obj << "f";
					for (size_t i = 0; i < face.size(); ++i)
					{
						obj << fmt::format(" {}", face[i] + 1); // OBJ uses 1-based indexing

						// Add texture coordinate index if available
						if (i < face_tex.size() && !obj_data.VT.empty())
						{
							obj << fmt::format("/{}", face_tex[i] + 1);
						}
						else if (!face_norm.empty() || (!obj_data.VN.empty() && i < face_norm.size()))
						{
							obj << "/"; // Empty texture coordinate slot
						}

						// Add normal index if available
						if (i < face_norm.size() && !obj_data.VN.empty())
						{
							obj << fmt::format("/{}", face_norm[i] + 1);
						}
					}
					obj << "\n";
				}

				obj << "\n";
			}
		}

		return true;
	}
} // namespace polyfem::io
