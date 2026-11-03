#include "USDWriter.hpp"

#include "USDPlugins.hpp"

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/subset.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdShade/tokens.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace polyfem::io
{
	namespace
	{
		// Custom attribute names used to round-trip the OBJData group structure
		// (in addition to the native UsdGeomSubset, which is what other DCCs read).
		const TfToken kMtlFilename("polyfem:mtlFilename");
		const TfToken kOrder("polyfem:order");
		const TfToken kObjectIndex("polyfem:objectIndex");
		const TfToken kObjectName("polyfem:objectName");
		const TfToken kGroupLocalIndex("polyfem:groupLocalIndex");
		const TfToken kGroupName("polyfem:groupName");
		const TfToken kMaterialName("polyfem:materialName");

		template <typename T>
		void set_custom(const UsdPrim &prim, const TfToken &name,
						const SdfValueTypeName &type, const T &value)
		{
			prim.CreateAttribute(name, type, /*custom=*/true).Set(value);
		}
	} // namespace

	bool USDWriter::write_with_groups(const std::string &path, const polyfem::OBJData &d)
	{
		usd_detail::ensure_plugins_registered();

		UsdStageRefPtr stage = UsdStage::CreateNew(path);
		if (!stage)
			return false;

		// cloth-fit solves and writes in the canonical Godot frame: +Y up,
		// right-handed. Declare it so USD/glTF consumers place the mesh correctly
		// instead of falling back to an assumed up-axis.
		UsdGeomSetStageUpAxis(stage, UsdGeomTokens->y);

		UsdGeomMesh mesh = UsdGeomMesh::Define(stage, SdfPath("/Mesh"));
		if (!mesh)
			return false;
		stage->SetDefaultPrim(mesh.GetPrim());

		// Right-handed / counter-clockwise front-face winding (canonical frame).
		mesh.CreateOrientationAttr(VtValue(UsdGeomTokens->rightHanded));

		// Polygonal mesh, not a subdivision surface: keep ngons as authored.
		mesh.CreateSubdivisionSchemeAttr(VtValue(UsdGeomTokens->none));

		// Points (vertex positions, order preserved).
		VtVec3fArray points(d.V.size());
		for (size_t i = 0; i < d.V.size(); ++i)
		{
			const auto &v = d.V[i];
			points[i] = GfVec3f(
				v.size() > 0 ? float(v[0]) : 0.0f,
				v.size() > 1 ? float(v[1]) : 0.0f,
				v.size() > 2 ? float(v[2]) : 0.0f);
		}
		mesh.CreatePointsAttr(VtValue(points));

		// Face topology: per-face valence + flattened vertex indices (no triangulation).
		VtIntArray face_counts(d.F.size());
		VtIntArray face_indices;
		for (size_t f = 0; f < d.F.size(); ++f)
		{
			face_counts[f] = int(d.F[f].size());
			for (int idx : d.F[f])
				face_indices.push_back(idx);
		}
		mesh.CreateFaceVertexCountsAttr(VtValue(face_counts));
		mesh.CreateFaceVertexIndicesAttr(VtValue(face_indices));

		UsdGeomPrimvarsAPI primvars(mesh);

		// UVs -> indexed primvars:st (faceVarying), values from VT, indices from FT.
		if (!d.VT.empty())
		{
			VtVec2fArray uv(d.VT.size());
			for (size_t i = 0; i < d.VT.size(); ++i)
			{
				const auto &t = d.VT[i];
				uv[i] = GfVec2f(t.size() > 0 ? float(t[0]) : 0.0f,
								t.size() > 1 ? float(t[1]) : 0.0f);
			}
			VtIntArray st_indices;
			for (const auto &face : d.FT)
				for (int idx : face)
					st_indices.push_back(idx);

			UsdGeomPrimvar st = primvars.CreatePrimvar(
				TfToken("st"), SdfValueTypeNames->TexCoord2fArray, UsdGeomTokens->faceVarying);
			st.Set(uv);
			if (!st_indices.empty())
				st.SetIndices(st_indices);
		}

		// Normals -> indexed primvars:normals (faceVarying), values from VN, indices from FN.
		if (!d.VN.empty())
		{
			VtVec3fArray nrm(d.VN.size());
			for (size_t i = 0; i < d.VN.size(); ++i)
			{
				const auto &n = d.VN[i];
				nrm[i] = GfVec3f(n.size() > 0 ? float(n[0]) : 0.0f,
								 n.size() > 1 ? float(n[1]) : 0.0f,
								 n.size() > 2 ? float(n[2]) : 0.0f);
			}
			VtIntArray n_indices;
			for (const auto &face : d.FN)
				for (int idx : face)
					n_indices.push_back(idx);

			UsdGeomPrimvar np = primvars.CreatePrimvar(
				TfToken("normals"), SdfValueTypeNames->Normal3fArray, UsdGeomTokens->faceVarying);
			np.Set(nrm);
			if (!n_indices.empty())
				np.SetIndices(n_indices);
		}

		// Referenced MTL library.
		if (!d.mtl_filename.empty())
			set_custom(mesh.GetPrim(), kMtlFilename, SdfValueTypeNames->String, d.mtl_filename);

		// Groups -> UsdGeomSubset per (object, group), tagged for lossless recovery.
		int order = 0;
		for (size_t oi = 0; oi < d.objects.size(); ++oi)
		{
			const OBJObject &obj = d.objects[oi];
			for (size_t gi = 0; gi < obj.groups.size(); ++gi)
			{
				const OBJGroup &g = obj.groups[gi];

				VtIntArray indices(g.face_indices.begin(), g.face_indices.end());
				const std::string subset_name =
					"subset_" + std::to_string(oi) + "_" + std::to_string(gi);

				UsdGeomSubset subset = UsdGeomSubset::CreateGeomSubset(
					mesh, TfToken(subset_name), UsdGeomTokens->face, indices,
					UsdShadeTokens->materialBind);

				const UsdPrim prim = subset.GetPrim();
				set_custom(prim, kOrder, SdfValueTypeNames->Int, order++);
				set_custom(prim, kObjectIndex, SdfValueTypeNames->Int, int(oi));
				set_custom(prim, kObjectName, SdfValueTypeNames->String, obj.name);
				set_custom(prim, kGroupLocalIndex, SdfValueTypeNames->Int, int(gi));
				set_custom(prim, kGroupName, SdfValueTypeNames->String, g.name);
				set_custom(prim, kMaterialName, SdfValueTypeNames->String, g.material_name);
			}
		}

		return stage->GetRootLayer()->Save();
	}
} // namespace polyfem::io
