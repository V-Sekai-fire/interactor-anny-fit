"""Learn the SOMA-X / ANNY API: joint count & names, pose/identity tensor
shapes, forward signature, and mesh faces. Run once after `pixi install`.
Drives nothing — just prints what the calibration code needs to know."""
import inspect
import torch

import soma
print("soma:", getattr(soma, "__version__", "?"), soma.__file__)
from soma import SOMALayer

layer = SOMALayer(identity_model_type="anny", device="cuda", mode="warp",
                  enable_procedural_transforms=True)
print("forward signature:", inspect.signature(layer.forward))
for attr in ("num_joints", "scale_param_names", "scale_param_segments",
             "joint_names", "num_verts", "faces"):
    v = getattr(layer, attr, None)
    if hasattr(v, "shape"):
        print(f"{attr}: tensor{tuple(v.shape)}")
    else:
        print(f"{attr}: {v}")

pub = list(layer.public_joint_names or [])
print(f"public_joint_names ({len(pub)}):", pub)
pp = getattr(layer, "public_joint_parent_ids", None)
print("public_joint_parent_ids:", pp.tolist() if hasattr(pp, "tolist") else pp)

J = len(pub)
pose = torch.zeros(1, J - 1, 3, device="cuda")        # axis-angle per joint (pose2rot=True)
ident = torch.zeros(1, 11, device="cuda")          # ANNY phenotype = 11 coeffs (neutral)
# For ANNY, SOMA repurposes scale_params as local_changes_kwargs; must be {} not None.
out = layer(pose, ident, scale_params={})
print("\nOUTPUT type:", type(out).__name__)
fields = [a for a in dir(out) if not a.startswith("_")]
print("output fields:", fields)
for a in ("vertices", "joints", "faces"):
    v = getattr(out, a, None)
    if hasattr(v, "shape"):
        print(f"  {a}: {tuple(v.shape)}")
