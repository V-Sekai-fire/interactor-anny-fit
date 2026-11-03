"""Extract the ANNY rig + LBS data from PyTorch SOMALayer into soma_rig.npz, so make_rig.py can bake
the native soma_rig.h with no ONNX hop and no onnxruntime.  The native runtime skins via Slang/Vulkan
(solve/core/soma_pheno_slang + vk_lbs), so the ONNX serialization was never a runtime dependency — it
was only a way to carry the bind / weights / inverse-bind out to make_rig.  Those come straight off the
torch model instead.  Verifies in PyTorch that the per-frame LBS reproduces SOMALayer.forward at rest.

    pixi run -e export python solve/adapters/bake/extract_rig.py    # -> soma_rig.npz (rig + LBS weights + faces + rest)

The per-frame forward (identity baked to constants) is pure LBS:
    verts = Σ_j w[v,j] · (R_j · bind_v + t_j),   bone = world_transforms @ invbind.
"""
import numpy as np
import torch
from soma import SOMALayer


def dense(t):
    return t.to_dense() if t.layout != torch.strided else t


class LBS(torch.nn.Module):  # the per-frame skinning, identity folded to constants
    def __init__(self, bind, inverse_bind, weights):
        super().__init__()
        self.register_buffer("bind", bind)             # (V,3) identity-shaped rest
        self.register_buffer("invbind", inverse_bind)  # (J,4,4)
        self.register_buffer("w", weights)             # (V,J)

    def forward(self, world_transforms):  # (B,J,4,4)
        bone = world_transforms @ self.invbind
        R, t = bone[..., :3, :3], bone[..., :3, 3]
        tv = torch.einsum("bjmk,vk->bjvm", R, self.bind) + t[..., None, :]
        return torch.einsum("vj,bjvm->bvm", self.w, tv)


def main():
    L = SOMALayer(identity_model_type="anny", device="cpu", mode="torch",
                  enable_procedural_transforms=False).eval()
    npose = len(L.public_joint_names) - 1
    ident = torch.tensor([[0.5, 0.7, 0.5, 0.5, 0.6, 0.0, 0, 0, 0, 0, 0]], dtype=torch.float32)
    pose = torch.eye(3).repeat(1, npose, 1, 1)
    with torch.no_grad():
        out = L(pose, ident, scale_params={}, pose2rot=False, apply_correctives=False)
    v0 = out["vertices"][0].float()  # (V,3) bind = identity-shaped rest (rest LBS == bind)

    bs = L.batched_skinning
    invbind = dense(bs.inverse_bind_transform).float()
    if invbind.ndim == 4:
        invbind = invbind[0]                       # -> (J,4,4)
    weights = dense(bs.skinning_weights).float()   # (V,J)
    bind = torch.linalg.inv(invbind)               # (J,4,4) identity-shaped rest skeleton
    J = invbind.shape[0]
    print(f"V={v0.shape[0]} J={J} weights={tuple(weights.shape)}")

    # PyTorch verify: the baked per-frame LBS reproduces the rest mesh (replaces the onnxruntime check).
    lbs = LBS(v0, invbind, weights).eval()
    wt_rest = torch.linalg.inv(invbind).unsqueeze(0)  # rest world transforms == bind
    with torch.no_grad():
        v_rest = lbs(wt_rest)[0]
    d = float((v_rest - v0).abs().max())
    print(f"rest LBS vs v0 max diff: {d:.3e} m  {'OK' if d < 1e-3 else 'MISMATCH'}  (pytorch verify)")

    parents = L.public_joint_parent_ids.detach().cpu().numpy().astype(np.int64)
    np.savez("soma_rig.npz",
             rest_R=bind[:, :3, :3].numpy().astype(np.float32),
             rest_pos=bind[:, :3, 3].numpy().astype(np.float32),
             parents=parents,
             joint_names=np.array(list(L.public_joint_names)),
             faces=L.faces.detach().cpu().numpy().astype(np.int32),
             rest_verts=v0.numpy().astype(np.float32),         # (V,3) bind
             skin_weights=weights.numpy().astype(np.float32),  # (V,J) — make_rig bakes these
             invbind=invbind.numpy().astype(np.float32))       # (J,4,4)
    print("saved soma_rig.npz (rig + LBS weights + invbind + faces + rest; no ONNX, no onnxruntime)")


if __name__ == "__main__":
    main()
