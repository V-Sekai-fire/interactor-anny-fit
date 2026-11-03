#pragma once

// Coordinate-system convention handling.
//
// cloth-fit's canonical frame is Godot's: +Y up, +Z model-front, right-handed,
// metric. Input meshes are authored in an arbitrary (up, forward, handedness)
// convention (default: Blender — +Z up, -Y front, right-handed); this maps them
// into the canonical frame on load so the solve and all USD/OBJ output are
// Godot/glTF-ready regardless of how the assets were authored.

#include <Eigen/Dense>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace polyfem::garment
{
    // A world axis with a sign: axis 0=X, 1=Y, 2=Z; sign +1 or -1.
    struct SignedAxis
    {
        int axis = 1;
        int sign = 1;

        Eigen::Vector3d vec() const
        {
            Eigen::Vector3d v = Eigen::Vector3d::Zero();
            v[axis] = double(sign);
            return v;
        }
    };

    // Which authored axis is "up", which is the model's front-facing ("forward")
    // direction, and the chirality.
    struct Convention
    {
        SignedAxis up;
        SignedAxis forward;
        bool right_handed = true;
    };

    // cloth-fit canonical == Godot: +Y up, +Z model-front (Vector3.MODEL_FRONT),
    // right-handed.
    inline Convention godot_canonical()
    {
        return Convention{SignedAxis{1, +1}, SignedAxis{2, +1}, true};
    }

    // Default input authoring convention == Blender: +Z up, -Y front, right-handed.
    inline Convention blender_default()
    {
        return Convention{SignedAxis{2, +1}, SignedAxis{1, -1}, true};
    }

    namespace detail
    {
        inline int axis_from_name(const std::string &s, int fallback)
        {
            for (char c : s)
            {
                if (c == 'x' || c == 'X') return 0;
                if (c == 'y' || c == 'Y') return 1;
                if (c == 'z' || c == 'Z') return 2;
            }
            return fallback;
        }
        inline int sign_from_name(const std::string &s)
        {
            return (s.find('-') != std::string::npos) ? -1 : +1;
        }
    } // namespace detail

    // Parse an optional convention block, e.g.
    //   { "up": "+Z", "forward": "-Y", "handedness": "right" }
    // Missing fields fall back to `fallback`.
    inline Convention parse_convention(const nlohmann::json &j, const Convention &fallback)
    {
        Convention c = fallback;
        if (!j.is_object())
            return c;
        if (j.contains("up") && j["up"].is_string())
        {
            const std::string s = j["up"];
            c.up = SignedAxis{detail::axis_from_name(s, fallback.up.axis), detail::sign_from_name(s)};
        }
        if (j.contains("forward") && j["forward"].is_string())
        {
            const std::string s = j["forward"];
            c.forward = SignedAxis{detail::axis_from_name(s, fallback.forward.axis), detail::sign_from_name(s)};
        }
        if (j.contains("handedness") && j["handedness"].is_string())
        {
            const std::string s = j["handedness"];
            c.right_handed = !(s.find("left") != std::string::npos || s == "L" || s == "l");
        }
        return c;
    }

    // 3x3 matrix mapping a point expressed in `from` into the canonical Godot
    // frame. Built as M = S^T where S = [right | up | forward] (columns) in the
    // source's own coordinates, with right = up x forward (right-handed) or
    // forward x up (left-handed). det(M) < 0 iff `from` is left-handed relative
    // to the canonical frame — the caller should flip face winding in that case.
    inline Eigen::Matrix3d to_canonical(const Convention &from)
    {
        const Eigen::Vector3d up = from.up.vec();
        const Eigen::Vector3d fwd = from.forward.vec();
        const Eigen::Vector3d right = from.right_handed ? up.cross(fwd) : fwd.cross(up);
        Eigen::Matrix3d S;
        S.col(0) = right;
        S.col(1) = up;
        S.col(2) = fwd;
        return S.transpose();
    }

    // Apply M to each row-vector of an Nx3 vertex/normal matrix: V <- V * M^T.
    inline void apply_to_rows(const Eigen::Matrix3d &M, Eigen::MatrixXd &V)
    {
        if (V.rows() == 0 || V.cols() < 3)
            return;
        const Eigen::MatrixXd R = V.leftCols<3>() * M.transpose();
        V.leftCols<3>() = R;
    }

    // Apply M to normals stored as a vector of {x,y,z}. M is orthonormal, so the
    // same matrix transforms directions (no inverse-transpose needed).
    inline void apply_to_normals(const Eigen::Matrix3d &M, std::vector<std::vector<double>> &N)
    {
        for (auto &n : N)
        {
            if (n.size() < 3)
                continue;
            const Eigen::Vector3d r = M * Eigen::Vector3d(n[0], n[1], n[2]);
            n[0] = r[0];
            n[1] = r[1];
            n[2] = r[2];
        }
    }

    // Reverse triangle winding (swap 2nd and 3rd index) so outward normals
    // survive a handedness-flipping (det < 0) transform.
    inline void flip_winding(Eigen::MatrixXi &F)
    {
        if (F.cols() >= 3)
            for (Eigen::Index i = 0; i < F.rows(); ++i)
                std::swap(F(i, 1), F(i, 2));
    }

    // Index (0/1/2) of a convention's up axis — used by the mesh validators.
    inline int up_axis_index(const Convention &c) { return c.up.axis; }
} // namespace polyfem::garment
