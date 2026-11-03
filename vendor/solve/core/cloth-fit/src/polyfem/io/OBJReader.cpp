// Modified version of read_obj from libigl to include reading polyline elements
// as edges.
//
// Copyright (C) 2013 Alec Jacobson <alecjacobson@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

#include "OBJReader.hpp"

#include <cstdio>
#include <fstream>
#include <iterator>

#include <igl/edges.h>
#include <igl/list_to_matrix.h>

#include <polyfem/utils/Logger.hpp>

namespace polyfem::io
{
	namespace
	{
		std::string remove_newline(std::string s)
		{
			s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
			return s;
		}
	} // namespace


	bool OBJReader::read_with_groups(
		const std::string &obj_file_name,
		polyfem::OBJData &obj_data)
	{
		// Open file, and check for error
		FILE *obj_file = fopen(obj_file_name.c_str(), "r");
		if (obj_file == NULL)
		{
			logger().error("OBJReader::read_with_groups: {:s} could not be opened!", obj_file_name);
			return false;
		}

		// Clear output data
		obj_data.V.clear();
		obj_data.VT.clear();
		obj_data.VN.clear();
		obj_data.F.clear();
		obj_data.FT.clear();
		obj_data.FN.clear();
		obj_data.objects.clear();
		obj_data.face_to_group.clear();
		obj_data.face_to_object.clear();
		obj_data.mtl_filename.clear();

		// Parsing state
		std::string current_object_name = "default";
		std::string current_group_name = "default";
		std::string current_material_name = "";
		int current_object_index = -1;
		int current_group_index = -1;

		const int LINE_MAX_LEN = 2048;
		char line[LINE_MAX_LEN];
		int line_no = 1;

		while (fgets(line, LINE_MAX_LEN, obj_file) != NULL)
		{
			char type[LINE_MAX_LEN];
			// Read first word containing type
			if (sscanf(line, "%s", type) == 1)
			{
				// Get pointer to rest of line right after type
				char *rest_of_line = &line[strlen(type)];

				if (strcmp(type, "v") == 0)
				{
					std::istringstream ls(&line[1]);
					std::vector<double> vertex{std::istream_iterator<double>(ls),
											   std::istream_iterator<double>()};
					if (vertex.size() < 3)
					{
						logger().error("OBJReader::read_with_groups: vertex on line {:d} should have at least 3 coordinates", line_no);
						fclose(obj_file);
						return false;
					}
					obj_data.V.push_back(vertex);
				}
				else if (strcmp(type, "vt") == 0)
				{
					double x[3];
					int count = sscanf(rest_of_line, "%lf %lf %lf\n", &x[0], &x[1], &x[2]);
					if (count != 2 && count != 3)
					{
						logger().error("OBJReader::read_with_groups: texture coords on line {:d} should have 2 or 3 coordinates (has {:d})", line_no, count);
						fclose(obj_file);
						return false;
					}
					std::vector<double> tex(count);
					for (int i = 0; i < count; i++)
					{
						tex[i] = x[i];
					}
					obj_data.VT.push_back(tex);
				}
				else if (strcmp(type, "vn") == 0)
				{
					double x[3];
					int count = sscanf(rest_of_line, "%lf %lf %lf\n", &x[0], &x[1], &x[2]);
					if (count != 3)
					{
						logger().error("OBJReader::read_with_groups: normal on line {:d} should have 3 coordinates", line_no);
						fclose(obj_file);
						return false;
					}
					std::vector<double> normal(count);
					for (int i = 0; i < count; i++)
					{
						normal[i] = x[i];
					}
					obj_data.VN.push_back(normal);
				}
				else if (strcmp(type, "f") == 0)
				{
					const auto &shift = [&obj_data](const int i) -> int {
						return i < 0 ? i + obj_data.V.size() : i - 1;
					};
					const auto &shift_t = [&obj_data](const int i) -> int {
						return i < 0 ? i + obj_data.VT.size() : i - 1;
					};
					const auto &shift_n = [&obj_data](const int i) -> int {
						return i < 0 ? i + obj_data.VN.size() : i - 1;
					};

					std::vector<int> face;
					std::vector<int> face_tex;
					std::vector<int> face_norm;

					// Read each "word" after type
					char word[LINE_MAX_LEN];
					int offset;
					while (sscanf(rest_of_line, "%s%n", word, &offset) == 1)
					{
						// adjust offset
						rest_of_line += offset;
						// Process word - handle vertex, texture, and normal indices
						long int i, it, in;
						if (sscanf(word, "%ld/%ld/%ld", &i, &it, &in) == 3)
						{
							face.push_back(shift(i));
							face_tex.push_back(shift_t(it));
							face_norm.push_back(shift_n(in));
						}
						else if (sscanf(word, "%ld/%ld", &i, &it) == 2)
						{
							face.push_back(shift(i));
							face_tex.push_back(shift_t(it));
						}
						else if (sscanf(word, "%ld//%ld", &i, &in) == 2)
						{
							face.push_back(shift(i));
							face_norm.push_back(shift_n(in));
						}
						else if (sscanf(word, "%ld", &i) == 1)
						{
							face.push_back(shift(i));
						}
						else
						{
							logger().error("OBJReader::read_with_groups: face on line {:d} has invalid element format", line_no);
							fclose(obj_file);
							return false;
						}
					}

					if (face.size() < 3)
					{
						logger().error("OBJReader::read_with_groups: face on line {:d} has fewer than 3 vertices", line_no);
						fclose(obj_file);
						return false;
					}

					// Ensure we have a current object and group
					if (current_object_index == -1)
					{
						polyfem::OBJObject new_object;
						new_object.name = current_object_name;
						obj_data.objects.push_back(new_object);
						current_object_index = obj_data.objects.size() - 1;
						current_group_index = -1; // Reset group index for new object
					}

					if (current_group_index == -1)
					{
						polyfem::OBJGroup new_group;
						new_group.name = current_group_name;
						new_group.material_name = current_material_name;
						obj_data.objects[current_object_index].groups.push_back(new_group);
						current_group_index = obj_data.objects[current_object_index].groups.size() - 1;
					}

					// Add face and track group/object mapping
					obj_data.F.push_back(face);
					obj_data.FT.push_back(face_tex);
					obj_data.FN.push_back(face_norm);

					int face_index = obj_data.F.size() - 1;

					obj_data.face_to_object.push_back(current_object_index);
					obj_data.face_to_group.push_back(current_group_index);
					obj_data.objects[current_object_index].groups[current_group_index].face_indices.push_back(face_index);
				}
				else if (strcmp(type, "o") == 0)
				{
					// Object definition
					char object_name[LINE_MAX_LEN];
					if (sscanf(rest_of_line, "%s", object_name) == 1)
					{
						current_object_name = object_name;
						current_object_index = -1; // Will be created when first face is encountered
						current_group_index = -1;
						current_group_name = "default"; // Reset group name for new object
					}
				}
				else if (strcmp(type, "g") == 0)
				{
					// Group definition
					char group_name[LINE_MAX_LEN];
					if (sscanf(rest_of_line, "%s", group_name) == 1)
					{
						current_group_name = group_name;
						current_group_index = -1; // Will be created when first face is encountered
					}
					else
					{
						// Empty group line resets to default
						current_group_name = "default";
						current_group_index = -1;
					}
				}
				else if (strcmp(type, "usemtl") == 0)
				{
					// Material usage
					char material_name[LINE_MAX_LEN];
					if (sscanf(rest_of_line, "%s", material_name) == 1)
					{
						current_material_name = material_name;
						current_group_index = -1; // Force creation of new group with this material
					}
				}
				else if (strcmp(type, "mtllib") == 0)
				{
					// Material library reference
					char mtl_filename[LINE_MAX_LEN];
					if (sscanf(rest_of_line, "%s", mtl_filename) == 1)
					{
						obj_data.mtl_filename = mtl_filename;
					}
				}
				// Ignore other directives (comments, normals, texture coords, etc.)
			}
			line_no++;
		}

		fclose(obj_file);

		// Validate that we have consistent data
		if (obj_data.F.size() != obj_data.face_to_group.size() ||
			obj_data.F.size() != obj_data.face_to_object.size())
		{
			logger().error("OBJReader::read_with_groups: inconsistent face-to-group/object mapping");
			return false;
		}

		logger().debug("OBJReader::read_with_groups: loaded {} vertices, {} faces, {} objects",
					  obj_data.V.size(), obj_data.F.size(), obj_data.objects.size());

		return true;
	}
} // namespace polyfem::io
