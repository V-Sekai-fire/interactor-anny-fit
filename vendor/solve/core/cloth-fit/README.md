<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->

- [[Siggraph 2025] Intersection-free Garment Retargeting](#siggraph-2025-intersection-free-garment-retargeting)
  - [Files](#files)
  - [Build](#build)
  - [Run](#run)
  - [Output Files](#output-files)
  - [Blender Import Settings](#blender-import-settings)
    - [OBJ Import Options:](#obj-import-options)
    - [Recommended Viewport Settings:](#recommended-viewport-settings)
    - [Notes:](#notes)
  - [Script Settings](#script-settings)
    - [Meshes](#meshes)
    - [Objectives and Weights](#objectives-and-weights)
    - [Optimizer](#optimizer)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# [Siggraph 2025] Intersection-free Garment Retargeting

[![Build](https://github.com/Huangzizhou/cloth-fit/actions/workflows/continuous.yml/badge.svg)](https://github.com/Huangzizhou/cloth-fit/actions/workflows/continuous.yml)

This is the opensource reference implementation of the SIGGRAPH 2025 paper [Intersection-free Garment Retargeting](https://huangzizhou.github.io/assets/img/research/cloth/paper.pdf). This code is modified based on [PolyFEM](https://github.com/polyfem/polyfem). It uses the nonlinear optimizers, linear solvers, and collision handling in [PolyFEM](https://github.com/polyfem/polyfem).

![Output sample](./garment-data/output.gif)

## Files

- `src/`: source code
- `cmake/` and `CMakeLists.txt`: CMake files
- `json-specs/`: input JSON configuration specifications
- `garment-data/`: input data and scripts needed to run the code
- `tests/`: unit-tests

## Build

The code can be compiled with

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

gcc 7 or later is recommended.

## Run

The folder `garment-data` contains four basic examples of retargeting garments to target avatars: `foxgirl_skirt`, `Goblin_Jacket`, `Goblin_Jumpsuit`, and `Trex_Jacket`. To run the examples, e.g. for `foxgirl_skirt`,

```
cd garment-data/foxgirl_skirt
../../build/PolyFEM_bin -j setup.json --max_threads 16 > log
```

## Output Files

- `step_avatar_<n>.obj`: optimization sequence of the target avatar geometry
- `step_garment_<n>.obj`: optimization sequence of the retargeted garment surface
- `sdf.obj`: avatar surface converted from the signed distance field
- `source_skeleton.obj`: skeleton of the source garment
- `target_skeleton.obj`: skeleton of the target avatar
- `target_avatar.obj`: target avatar geometry
- `projected_avatar.obj`: projected target avatar on its skeleton

## Blender Import Settings

When importing the generated OBJ files into Blender for visualization, use the following settings to ensure correct display:

### OBJ Import Options:

1. **File > Import > Wavefront (.obj)**
2. **Transform Settings:**

   - **Forward Axis**: `-Z Forward` (or adjust based on your scene orientation)
   - **Up Axis**: `Y Up`
   - **Scale**: `1.0` (adjust if meshes appear too large/small)

3. **Geometry Settings:**

   - ✅ **Smooth Groups**: Enable for proper shading
   - ✅ **Lines**: Enable to import skeleton edges
   - ✅ **Split by Object**: Enable to separate different mesh components
   - ✅ **Split by Group**: Enable for better organization

4. **Material Settings:**
   - ✅ **Image Search**: Enable if textures are present
   - **Material Import**: Enable for basic material support

### Recommended Viewport Settings:

- **Shading**: Use `Material Preview` or `Solid` mode for best visibility
- **Overlays**: Enable `Wireframe` if you need to see mesh topology
- **Viewport Shading**: Set `MatCap` to a neutral material for better geometry visualization

### Notes:

- Skeleton files (`.obj` with edge data) will import as wireframe meshes
- Multiple optimization steps can be imported as separate objects for animation
- Use Blender's Timeline to scrub through optimization sequences by toggling object visibility

## Script Settings

The specification of each JSON configuration is described in `json-specs/input-spec.json`.

### Meshes

- `avatar_mesh_path`: the target avatar mesh
- `garment_mesh_path`: the source garment mesh
- `source_skeleton_path`: the skeleton edge mesh of the source garment in `.obj` format
- `target_skeleton_path`: the skeleton edge mesh of the target avatar in `.obj` format
- `avatar_skin_weights_path`: the skinning weights of the target avatar, a matrix of size `#number_of_skeleton_nodes` times `#number_of_vertices` (optional)

For avatars and garments, only `.obj` triangular mesh is supported. The source and target skeletons should share the same mesh connectivity, and skeleton joints should be ordered in the same way. The skinning weights will be used to project the target avatar to its skeleton. If `avatar_skin_weights_path` is not provided, the vertices will be simply projected to the closest bone in distance. Ideally, the source and target skeletons are in the same pose, so that the difference in pose does not cause unecessary geometric distortion on the garment.

### Objectives and Weights

There are multiple objectives used in the method, each has its own weight:

- `similarity_penalty_weight`: $L_\text{surf}$ preserves the surface shape, always set to 1
- `curvature_penalty_weight`: $L_\text{curvature}$ preserves the curve curvature
- `twist_penalty_weight`: $L_\text{torsion}$ preserves the curve torsion
- `curve_center_target_weight`: $L_\text{pos}$ preserves the relative position of curve loops (e.g. hem, cuff)
- `solver/contact/barrier_stiffness`: $L_\text{contact}$ avoids contact by adding a barrier

The choice of `barrier_stiffness` depends on the mesh resolution, geometry scale, and the specific problem setup. The general rule is that when the min distance reported in the log is much smaller than `dhat` (say 1/100 of `dhat`), then it should probably be increased.

### Optimizer

The nonlinear optimizer lives in a separate Github repository called [PolySolve](https://github.com/Huangzizhou/polysolve/tree/garment). The optimizer parameters are specified under JSON entry `solver/nonlinear`, whose specification is in [nonlinear-solver-spec.json](https://github.com/Huangzizhou/polysolve/blob/garment/nonlinear-solver-spec.json).
