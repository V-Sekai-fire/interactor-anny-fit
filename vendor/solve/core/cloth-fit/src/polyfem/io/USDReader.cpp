#include "USDReader.hpp"

#include "USDPlugins.hpp"

#include <algorithm>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/subset.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace polyfem::io
{
	namespace
	{
		const TfToken kMtlFilename("polyfem:mtlFilename");
		const TfToken kOrder("polyfem:order");
		const TfToken kObjectIndex("polyfem:objectIndex");
		const TfToken kObjectName("polyfem:objectName");
		const TfToken kGroupLocalIndex("polyfem:groupLocalIndex");
		const TfToken kGroupName("polyfem:groupName");
		const TfToken kMaterialName("polyfem:materialName");

		int get_int(const UsdPrim &prim, const TfToken &name, int fallback)
		{
			int v = fallback;
			if (UsdAttribute a = prim.GetAttribute(name))
				a.Get(&v);
			return v;
		}

		std::string get_string(const UsdPrim &prim, const TfToken &name)
		{
			std::string v;
			if (UsdAttribute a = prim.GetAttribute(name))
				a.Get(&v);
			return v;
		}

		UsdGeomMesh find_mesh(const UsdStageRefPtr &stage)
		{
			if (UsdPrim def = stage->GetDefaultPrim())
			{
				if (UsdGeomMesh m = UsdGeomMesh(def))
					return m;
			}
			for (const UsdPrim &prim : stage->Traverse())
			{
				if (UsdGeomMesh m = UsdGeomMesh(prim))
					return m;
			}
			return UsdGeomMesh();
		}

		// Split a flat, face-varying index list back into per-face vectors using
		// the per-face vertex counts.
		std::vector<std::vector<int>> unflatten(const VtIntArray &flat, const VtIntArray &counts)
		{
			std::vector<std::vector<int>> out(counts.size());
			size_t k = 0;
			for (size_t f = 0; f < counts.size(); ++f)
			{
				for (int c = 0; c < counts[f] && k < flat.size(); ++c)
					out[f].push_back(flat[k++]);
			}
			return out;
		}
	} // namespace

	bool USDReader::read_with_groups(const std::string &usd_file_name, polyfem::OBJData &d)
	{
		usd_detail::ensure_plugins_registered();

		d = polyfem::OBJData();

		UsdStageRefPtr stage = UsdStage::Open(usd_file_name);
		if (!stage)
			return false;

		UsdGeomMesh mesh = find_mesh(stage);
		if (!mesh)
			return false;

		// Points -> V.
		VtVec3fArray points;
		mesh.GetPointsAttr().Get(&points);
		d.V.resize(points.size());
		for (size_t i = 0; i < points.size(); ++i)
			d.V[i] = {points[i][0], points[i][1], points[i][2]};

		// Topology -> F (arbitrary valence preserved).
		VtIntArray counts, indices;
		mesh.GetFaceVertexCountsAttr().Get(&counts);
		mesh.GetFaceVertexIndicesAttr().Get(&indices);
		d.F = unflatten(indices, counts);

		UsdGeomPrimvarsAPI primvars(mesh);

		// primvars:st -> VT / FT.
		if (UsdGeomPrimvar st = primvars.GetPrimvar(TfToken("st")))
		{
			VtVec2fArray uv;
			st.Get(&uv);
			d.VT.resize(uv.size());
			for (size_t i = 0; i < uv.size(); ++i)
				d.VT[i] = {uv[i][0], uv[i][1]};

			VtIntArray st_indices;
			if (st.GetIndices(&st_indices) && !st_indices.empty())
				d.FT = unflatten(st_indices, counts);
		}

		// primvars:normals -> VN / FN.
		if (UsdGeomPrimvar np = primvars.GetPrimvar(TfToken("normals")))
		{
			VtVec3fArray nrm;
			np.Get(&nrm);
			d.VN.resize(nrm.size());
			for (size_t i = 0; i < nrm.size(); ++i)
				d.VN[i] = {nrm[i][0], nrm[i][1], nrm[i][2]};

			VtIntArray n_indices;
			if (np.GetIndices(&n_indices) && !n_indices.empty())
				d.FN = unflatten(n_indices, counts);
		}

		d.mtl_filename = get_string(mesh.GetPrim(), kMtlFilename);

		// Subsets -> objects/groups (recover exact structure via the polyfem:* tags).
		struct Rec
		{
			int order, object_index, group_local_index;
			std::string object_name, group_name, material_name;
			std::vector<int> face_indices;
		};
		std::vector<Rec> recs;
		for (const UsdGeomSubset &subset : UsdGeomSubset::GetAllGeomSubsets(mesh))
		{
			const UsdPrim prim = subset.GetPrim();
			Rec r;
			r.order = get_int(prim, kOrder, int(recs.size()));
			r.object_index = get_int(prim, kObjectIndex, 0);
			r.group_local_index = get_int(prim, kGroupLocalIndex, 0);
			r.object_name = get_string(prim, kObjectName);
			r.group_name = get_string(prim, kGroupName);
			r.material_name = get_string(prim, kMaterialName);
			VtIntArray idx;
			subset.GetIndicesAttr().Get(&idx);
			r.face_indices.assign(idx.begin(), idx.end());
			recs.push_back(std::move(r));
		}

		if (!recs.empty())
		{
			std::sort(recs.begin(), recs.end(),
					  [](const Rec &a, const Rec &b) { return a.order < b.order; });

			int num_objects = 0;
			for (const Rec &r : recs)
				num_objects = std::max(num_objects, r.object_index + 1);
			d.objects.resize(num_objects);

			for (const Rec &r : recs)
			{
				OBJObject &obj = d.objects[r.object_index];
				obj.name = r.object_name;
				if (int(obj.groups.size()) <= r.group_local_index)
					obj.groups.resize(r.group_local_index + 1);
				OBJGroup &g = obj.groups[r.group_local_index];
				g.name = r.group_name;
				g.material_name = r.material_name;
				g.face_indices = r.face_indices;
			}

			// Recompute face -> (object, per-object group) maps from the structure.
			d.face_to_object.assign(d.F.size(), -1);
			d.face_to_group.assign(d.F.size(), -1);
			for (size_t oi = 0; oi < d.objects.size(); ++oi)
			{
				for (size_t gi = 0; gi < d.objects[oi].groups.size(); ++gi)
				{
					for (int f : d.objects[oi].groups[gi].face_indices)
					{
						if (f >= 0 && f < int(d.F.size()))
						{
							d.face_to_object[f] = int(oi);
							d.face_to_group[f] = int(gi);
						}
					}
				}
			}
		}

		return true;
	}
} // namespace polyfem::io
