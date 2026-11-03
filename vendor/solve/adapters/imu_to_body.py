"""imu_to_body -- calibrate per-tracker orientation offsets and solve 24-joint body local pose.

The forward model for a tracker n rigidly worn on the body joint j = NODE2JOINT[n]:

    sensor[n] = S . bone_global[j] . M[n]

S is one world rotation shared by every tracker (body world -> sensor world); M[n] is the
constant sensor-to-bone mount for tracker n, in the bone frame. Both are constant.

A single reference pose cannot separate S from the M[n]: rotating the whole world and every
mount together by the same yaw leaves every sensor reading unchanged, so one held pose leaves
the heading undetermined (which is why inertial rigs do a heading reset or use a second pose).
`calibrate` therefore takes two or more reference poses whose local rotations are known and
solves the shared S and per-tracker M[n] together by an alternating hand-eye fit. `solve` then
inverts the model, bone_global[j] = S^-1 . sensor[n] . M[n]^-1, and converts the chain to local.

Quaternions are (w, x, y, z), scalar-first, unit. No torch: this is the orientation algebra
only; the skinning/LBS lives in core/.

    python imu_to_body.py --self-test
"""
import sys

import numpy as np

# 24-joint body skeleton kinematic tree (parent index; -1 is the root).
PARENT = [-1, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 12, 13, 14, 16, 17, 18, 19, 20, 21]
JOINT_NAMES = ["Pelvis", "L_Hip", "R_Hip", "Spine1", "L_Knee", "R_Knee", "Spine2", "L_Ankle",
               "R_Ankle", "Spine3", "L_Foot", "R_Foot", "Neck", "L_Collar", "R_Collar", "Head",
               "L_Shoulder", "R_Shoulder", "L_Elbow", "R_Elbow", "L_Wrist", "R_Wrist", "L_Hand", "R_Hand"]
# rebocap's 15 trackers to their body joint. The root tracker (waist) is index 0.
NODE2JOINT = {0: 0, 1: 1, 2: 2, 3: 4, 4: 5, 5: 7, 6: 8, 7: 9, 8: 15,
              9: 16, 10: 17, 11: 18, 12: 19, 13: 20, 14: 21}
ROOT_NODE = 0


def qmul(a, b):
    w1, x1, y1, z1 = a
    w2, x2, y2, z2 = b
    return np.array([w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2,
                     w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
                     w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
                     w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2])


def qconj(q):
    return q * np.array([1.0, -1, -1, -1])


def qnorm(q):
    n = np.linalg.norm(q)
    return q / n if n else np.array([1.0, 0, 0, 0])


def geodesic_deg(a, b):
    return np.degrees(2 * np.arccos(min(1.0, abs(float(np.dot(qnorm(a), qnorm(b)))))))


def forward_kinematics(local):
    """local[24] quats -> global[24] quats down the body tree."""
    g = [None] * len(local)
    for j in range(len(local)):
        g[j] = local[j] if PARENT[j] < 0 else qmul(g[PARENT[j]], local[j])
    return g


def to_local(global_q):
    """global[24] quats -> local[24] quats (inverse of forward_kinematics)."""
    return [global_q[j] if PARENT[j] < 0 else qmul(qconj(global_q[PARENT[j]]), global_q[j])
            for j in range(len(global_q))]


def qavg(quats):
    """Markley average of unit quaternions (largest-eigenvector of the accumulated outer product)."""
    Q = np.array([q if q[0] >= 0 else -q for q in quats])
    w, v = np.linalg.eigh(Q.T @ Q)
    return qnorm(v[:, np.argmax(w)])


def rotvec(q):
    """Unit quat -> axis*angle rotation vector."""
    q = qnorm(q)
    q = q if q[0] >= 0 else -q
    s = np.linalg.norm(q[1:])
    return q[1:] / s * (2 * np.arccos(min(1.0, q[0]))) if s > 1e-12 else np.zeros(3)


def mat_to_quat(R):
    t = np.trace(R)
    if t > 0:
        w = np.sqrt(1 + t) / 2
        return qnorm(np.array([w, (R[2, 1] - R[1, 2]) / (4 * w),
                               (R[0, 2] - R[2, 0]) / (4 * w), (R[1, 0] - R[0, 1]) / (4 * w)]))
    i = int(np.argmax(np.diag(R)))
    j, k = (i + 1) % 3, (i + 2) % 3
    r = np.sqrt(1 + R[i, i] - R[j, j] - R[k, k])
    v = np.zeros(3)
    v[i] = r / 2
    v[j] = (R[j, i] + R[i, j]) / (2 * r)
    v[k] = (R[k, i] + R[i, k]) / (2 * r)
    w = (R[k, j] - R[j, k]) / (2 * r)
    return qnorm(np.array([w, *v]))


def calibrate(ref_frames, node2joint=NODE2JOINT):
    """ref_frames: list of (sensor_dict, known_local_pose). Returns (S, M) with M a {node: quat}
    solving sensor[n] = S . bone_global[j_n] . M[n] over all reference frames.

    The relative rotation between two reference poses cancels M (Δsensor = S . Δbone . S^-1), so
    S is a Kabsch alignment of the delta axes; each M[n] then follows exactly. Needs >= 2 distinct
    reference poses (the deltas), and their axes must span 3D to fix the heading."""
    bones = [forward_kinematics(local) for _, local in ref_frames]
    nodes = list(ref_frames[0][0])
    A, B, wts = [], [], []
    for a in range(len(ref_frames)):
        for b in range(a + 1, len(ref_frames)):
            for n in nodes:
                j = node2joint[n]
                db = rotvec(qmul(bones[b][j], qconj(bones[a][j])))
                ds = rotvec(qmul(ref_frames[b][0][n], qconj(ref_frames[a][0][n])))
                m = min(np.linalg.norm(db), np.linalg.norm(ds))
                if m > np.radians(2):
                    A.append(ds / np.linalg.norm(ds)); B.append(db / np.linalg.norm(db)); wts.append(m)
    if len(A) < 3 or np.linalg.matrix_rank(np.array(B)) < 3:
        S = np.array([1.0, 0, 0, 0])       # heading undetermined: too few / coplanar reference deltas
    else:
        A, B, wts = np.array(A), np.array(B), np.array(wts)
        H = (B * wts[:, None]).T @ A       # solve R minimizing sum w * ||A - R B||
        U, _, Vt = np.linalg.svd(H)
        d = np.sign(np.linalg.det(Vt.T @ U.T))
        S = mat_to_quat(Vt.T @ np.diag([1, 1, d]) @ U.T)
    M = {n: qavg([qmul(qmul(qconj(bones[k][node2joint[n]]), qconj(S)), sensor[n])
                  for k, (sensor, _) in enumerate(ref_frames)]) for n in nodes}
    return S, M


def solve(sensor, S, M, node2joint=NODE2JOINT):
    """Trackers this frame -> 24-joint body local pose[24] via bone_global = S^-1 . sensor . M^-1.
    Bones with no tracker are set to their parent orientation (identity local); `mapped`
    reports which joints are observed."""
    glob = [None] * 24
    for n, q in sensor.items():
        glob[node2joint[n]] = qnorm(qmul(qmul(qconj(S), q), qconj(M[n])))
    mapped = [j for j in range(24) if glob[j] is not None]
    for j in range(24):  # fill unobserved bones with their parent's global (local = identity)
        if glob[j] is None:
            glob[j] = np.array([1.0, 0, 0, 0]) if PARENT[j] < 0 else glob[PARENT[j]]
    return to_local(glob), mapped, glob


def _rand_quat(rng):
    return qnorm(rng.standard_normal(4))


def _self_test():
    rng = np.random.default_rng(20260925)
    ok = True
    S_true = _rand_quat(rng)
    M_true = {n: _rand_quat(rng) for n in NODE2JOINT}
    # A T-pose plus two other held poses as calibration references; a separate pose to test on.
    ref_locals = [[_rand_quat(rng) for _ in range(24)] for _ in range(3)]
    test_local = [_rand_quat(rng) for _ in range(24)]
    truth = forward_kinematics(test_local)

    def synth(local, node2joint=NODE2JOINT):
        g = forward_kinematics(local)
        return {n: qnorm(qmul(qmul(qmul(S_true, g[j]), M_true[n]), np.array([1.0, 0, 0, 0])))
                for n, j in node2joint.items()}

    ref_frames = [(synth(local), local) for local in ref_locals]

    # 1. Multi-pose calibration recovers the shared S and mounts exactly, so a fresh pose's
    #    observed bones reconstruct to ~0 error.
    S, M = calibrate(ref_frames)
    _, mapped, glob = solve(synth(test_local), S, M)
    gerr = max(geodesic_deg(glob[j], truth[j]) for j in mapped)
    print("multi-pose calibration, fresh-pose bone error: max %.2e deg over %d bones -> %s"
          % (gerr, len(mapped), "PASS" if gerr < 1e-3 else "FAIL"))
    ok &= gerr < 1e-3

    # 2. One reference pose is underdetermined (the heading is free): it must NOT recover.
    S1, M1 = calibrate(ref_frames[:1])
    _, m1, g1 = solve(synth(test_local), S1, M1)
    e1 = np.median([geodesic_deg(g1[j], truth[j]) for j in m1])
    print("single-pose calibration (underdetermined): median bone error %.1f deg -> %s"
          % (e1, "PASS" if e1 > 5 else "FAIL"))
    ok &= e1 > 5

    # 3. Negative control: a shuffled tracker->joint map must NOT reconstruct the pose.
    wrong = dict(zip(NODE2JOINT.keys(), list(NODE2JOINT.values())[1:] + [NODE2JOINT[0]]))
    Sw, Mw = calibrate(ref_frames, node2joint=wrong)
    _, mw, gw = solve(synth(test_local), Sw, Mw, node2joint=wrong)
    werr = np.median([geodesic_deg(gw[j], truth[j]) for j in mw])
    print("negative control (wrong map): median bone error %.1f deg -> %s"
          % (werr, "PASS" if werr > 10 else "FAIL"))
    ok &= werr > 10

    # 4. Noise robustness: 1% sensor noise degrades gracefully, not catastrophically.
    noisy = {n: qnorm(q + 0.01 * rng.standard_normal(4)) for n, q in synth(test_local).items()}
    _, mn, gn = solve(noisy, S, M)
    nerr = np.median([geodesic_deg(gn[j], truth[j]) for j in mn])
    print("1%% sensor noise: median bone error %.2f deg -> %s" % (nerr, "PASS" if nerr < 5 else "FAIL"))
    ok &= nerr < 5

    print("RESULT:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(_self_test() if "--self-test" in sys.argv else 2)
