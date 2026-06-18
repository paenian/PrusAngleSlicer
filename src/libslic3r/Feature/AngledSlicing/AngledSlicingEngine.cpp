///|/ Copyright (c) 2026 Paul Chase
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "AngledSlicingEngine.hpp"
#include "libslic3r/ClipperUtils.hpp"

#include <boost/log/trivial.hpp>
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace Slic3r::AngledSlicing {

std::vector<TiltedSliceResult> slice_mesh_tilted(
    const indexed_triangle_set      &mesh,
    const std::vector<SlicingPlane> &planes,
    const Eigen::Transform<double, 3, Eigen::Affine> &trafo)
{
    std::vector<TiltedSliceResult> results;
    results.reserve(planes.size());

    // Transform mesh vertices to print-position coordinates
    std::vector<Eigen::Vector3d> verts(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i)
        verts[i] = trafo * mesh.vertices[i].cast<double>();

    const size_t n_vertices = verts.size();
    
    for (const SlicingPlane &plane : planes) {
        // For each triangle, compute intersection with the tilted plane.
        // Compute signed distance of each vertex to this plane
        std::vector<double> vert_dist(n_vertices);
        for (size_t i = 0; i < n_vertices; ++i) {
            const auto &v = verts[i];
            vert_dist[i] = plane.normal.x() * v.x() + plane.normal.y() * v.y() + plane.normal.z() * v.z() - plane.distance;
        }

        // Collect intersection line segments using edge-based intersection.
        // Key insight: for a watertight mesh, each interior edge is shared by exactly 2 triangles.
        // If a point is computed for a shared edge, it must be identical for both triangles.
        // We use edge indices (sorted vertex pair) as keys to ensure consistency.
        
        // Map from edge (sorted vertex pair) to intersection point
        std::unordered_map<int64_t, Point> edge_intersection;
        auto edge_key = [](int a, int b) -> int64_t {
            return a < b ? (int64_t(a) << 32) | int64_t(b) : (int64_t(b) << 32) | int64_t(a);
        };

        auto get_or_compute_edge_point = [&](int vi0, int vi1) -> Point {
            int64_t key = edge_key(vi0, vi1);
            auto it = edge_intersection.find(key);
            if (it != edge_intersection.end())
                return it->second;
            double d0 = vert_dist[vi0], d1 = vert_dist[vi1];
            double t = d0 / (d0 - d1);
            const auto &v0 = verts[vi0];
            const auto &v1 = verts[vi1];
            double ix = v0.x() + t * (v1.x() - v0.x());
            double iy = v0.y() + t * (v1.y() - v0.y());
            Point p(coord_t(scale_(ix)), coord_t(scale_(iy)));
            edge_intersection[key] = p;
            return p;
        };

        auto vertex_point = [&](int vi) -> Point {
            return Point(coord_t(scale_(verts[vi].x())), coord_t(scale_(verts[vi].y())));
        };

        // For each triangle, find 0 or 2 intersection points forming a segment.
        // Uses the standard rule: a vertex exactly ON the plane is counted as "above" (positive side).
        // This ensures each edge-on-plane is only contributed by ONE of its two neighboring triangles.
        std::vector<std::pair<Point, Point>> segments;
        for (const auto &tri : mesh.indices) {
            int vi[3] = { tri(0), tri(1), tri(2) };
            double d[3] = { vert_dist[vi[0]], vert_dist[vi[1]], vert_dist[vi[2]] };

            // Classify: count vertices strictly above, strictly below, on plane
            int above = 0, below = 0, on = 0;
            for (int i = 0; i < 3; ++i) {
                if (d[i] > 0) above++;
                else if (d[i] < 0) below++;
                else on++;
            }

            // No intersection if all on same side or all on plane
            if (above == 0 || below == 0) continue;

            // General case: find exactly 2 edge crossings
            Point pts[2];
            int n_pts = 0;
            for (int i = 0; i < 3 && n_pts < 2; ++i) {
                int j = (i + 1) % 3;
                if ((d[i] > 0) != (d[j] > 0)) {
                    // Edge crosses the plane (both endpoints strictly on different sides)
                    pts[n_pts++] = get_or_compute_edge_point(vi[i], vi[j]);
                } else if (d[i] == 0.0 && d[j] != 0.0) {
                    // Vertex i is exactly on the plane, j is off
                    // Only add if the OTHER vertex (not j) is on the opposite side of j
                    // This prevents double-counting of on-plane vertices
                    int k = (i + 2) % 3;
                    if ((d[j] > 0) != (d[k] > 0) || d[k] == 0.0) {
                        pts[n_pts++] = vertex_point(vi[i]);
                    }
                }
            }

            if (n_pts == 2 && pts[0] != pts[1]) {
                segments.emplace_back(pts[0], pts[1]);
            }
        }

        // Chain segments into closed polygons.
        // Build adjacency map: point → list of segment indices that START or END at that point.
        std::unordered_multimap<int64_t, size_t> point_to_seg;
        auto point_hash = [](const Point &p) -> int64_t {
            return (int64_t(p.x()) * 2654435761LL) ^ int64_t(p.y());
        };
        for (size_t i = 0; i < segments.size(); ++i) {
            point_to_seg.emplace(point_hash(segments[i].first), i);
            point_to_seg.emplace(point_hash(segments[i].second), i);
        }

        Polygons polys;
        std::vector<bool> used(segments.size(), false);

        // Debug: log segment count for first few planes
        static int plane_count = 0;
        if (plane_count < 3) {
            BOOST_LOG_TRIVIAL(info) << "AngledSlicing plane " << plane_count << ": " << segments.size() << " segments, d=" << plane.distance << " pz=" << plane.print_z;
            for (size_t i = 0; i < segments.size(); ++i)
                BOOST_LOG_TRIVIAL(info) << "  seg " << i << ": (" << unscaled(segments[i].first.x()) << "," << unscaled(segments[i].first.y()) << ") -> (" << unscaled(segments[i].second.x()) << "," << unscaled(segments[i].second.y()) << ")";
        }
        ++plane_count;
        for (size_t start = 0; start < segments.size(); ++start) {
            if (used[start]) continue;
            
            Polygon poly;
            poly.points.push_back(segments[start].first);
            poly.points.push_back(segments[start].second);
            used[start] = true;

            // Extend forward from the last point
            bool extended = true;
            while (extended) {
                extended = false;
                const Point &last = poly.points.back();
                int64_t h = point_hash(last);
                auto range = point_to_seg.equal_range(h);
                for (auto it = range.first; it != range.second; ++it) {
                    size_t idx = it->second;
                    if (used[idx]) continue;
                    if (segments[idx].first == last) {
                        poly.points.push_back(segments[idx].second);
                        used[idx] = true;
                        extended = true;
                        break;
                    } else if (segments[idx].second == last) {
                        poly.points.push_back(segments[idx].first);
                        used[idx] = true;
                        extended = true;
                        break;
                    }
                }
            }

            // Check closure
            if (poly.points.size() >= 3 && poly.points.back() == poly.points.front())
                poly.points.pop_back();
            
            if (poly.points.size() >= 3)
                polys.push_back(std::move(poly));
        }

        ExPolygons expolys = union_ex(polys);
        results.push_back({std::move(expolys), plane.print_z, 0.0});
    }

    // Log stats
    size_t non_empty = 0;
    for (const auto &r : results)
        if (!r.contours.empty()) ++non_empty;
    BOOST_LOG_TRIVIAL(info) << "AngledSlicing: " << planes.size() << " planes, " 
                            << non_empty << " non-empty, " << (planes.size() - non_empty) << " empty";

    // Set heights: vertical distance between consecutive layers
    for (size_t i = 0; i < results.size(); ++i) {
        if (i == 0)
            results[i].height = std::max(0.1, results[i].print_z);
        else
            results[i].height = std::max(0.05, results[i].print_z - results[i - 1].print_z);
    }

    return results;
}

std::vector<SlicingPlane> generate_tilted_planes(
    const Params &params,
    double layer_height,
    double first_layer_height,
    const Eigen::AlignedBox<double, 3> &object_bbox)
{
    std::vector<SlicingPlane> planes;

    if (!params.enabled()) {
        // Standard horizontal planes — shouldn't be called in this case
        return planes;
    }

    double angle_rad = params.tilt_angle_rad();
    double dir_rad   = params.tilt_direction_rad();

    // The tilted plane normal: starts as (0,0,1) (horizontal) then rotated by angle around
    // the axis perpendicular to the tilt direction.
    // Tilt direction = compass bearing of where the "high side" points.
    // The plane tilts so that moving in direction (cos(dir), sin(dir)) goes UP.
    // Normal after tilt:
    //   nx = -sin(angle) * cos(dir)
    //   ny = -sin(angle) * sin(dir)  
    //   nz = cos(angle)
    Eigen::Vector3d normal(
        -std::sin(angle_rad) * std::cos(dir_rad),
        -std::sin(angle_rad) * std::sin(dir_rad),
        std::cos(angle_rad)
    );
    normal.normalize();

    // The planes pass through the object. The first plane touches the bottom of the object.
    // The "distance" in the plane equation (normal · point = distance) determines plane position.
    //
    // Object center: midpoint of bounding box
    Eigen::Vector3d obj_center = object_bbox.center();
    
    // The range of distances: project the bbox corners onto the normal to find min/max
    double d_min = std::numeric_limits<double>::max();
    double d_max = std::numeric_limits<double>::lowest();
    for (int i = 0; i < 8; ++i) {
        Eigen::Vector3d corner(
            (i & 1) ? object_bbox.max().x() : object_bbox.min().x(),
            (i & 2) ? object_bbox.max().y() : object_bbox.min().y(),
            (i & 4) ? object_bbox.max().z() : object_bbox.min().z()
        );
        double d = normal.dot(corner);
        d_min = std::min(d_min, d);
        d_max = std::max(d_max, d);
    }

    // Generate planes from d_min to d_max, spaced by layer_height
    double current_d = d_min + first_layer_height * 0.5;
    bool first = true;
    int layer_idx = 0;
    
    // Z offset to convert from mesh-local Z (centered) to bed Z (bottom at 0)
    double z_offset = -object_bbox.min().z();  // shift so min Z maps to 0
    double object_height = object_bbox.max().z() + z_offset;  // max Z in bed coords
    
    while (current_d < d_max) {
        // print_z: Z at the object center (x=0, y=0 in mesh-local coords).
        // The G-code generator adds dx*tan(angle) offset per point.
        double local_z = current_d / normal.z();
        double print_z = local_z + z_offset;
        
        // Include this plane if any part of it intersects the object Z range [0, object_height]
        // A plane intersects if its min_z < object_height AND max_z > 0
        double min_z_on_plane = std::numeric_limits<double>::max();
        double max_z_on_plane = std::numeric_limits<double>::lowest();
        for (int i = 0; i < 4; ++i) {
            double x = (i & 1) ? object_bbox.max().x() : object_bbox.min().x();
            double y = (i & 2) ? object_bbox.max().y() : object_bbox.min().y();
            double z = (current_d - normal.x() * x - normal.y() * y) / normal.z();
            double z_bed = z + z_offset;
            min_z_on_plane = std::min(min_z_on_plane, z_bed);
            max_z_on_plane = std::max(max_z_on_plane, z_bed);
        }
        
        // Include if the plane's Z range overlaps with [0, object_height]
        if (max_z_on_plane > 0.0 && min_z_on_plane < object_height) {
            // Use the TRUE center-of-plane Z as print_z. This can be negative for
            // early planes. The G-code generator uses this directly in the Z offset
            // formula, and clamps the final result to max(0, z).
            // For the Layer object, we need print_z to be >= some minimum for flow calc.
            // We'll handle that in the integration code.
            planes.push_back({normal, current_d, print_z});
        }
        
        current_d += layer_height;
        first = false;
        ++layer_idx;
    }

    BOOST_LOG_TRIVIAL(info) << "AngledSlicing generate_tilted_planes: d_min=" << d_min << " d_max=" << d_max 
                            << " layer_h=" << layer_height << " planes_generated=" << planes.size()
                            << " first_pz=" << (planes.empty() ? 0.0 : planes.front().print_z)
                            << " last_pz=" << (planes.empty() ? 0.0 : planes.back().print_z);
    return planes;
}

} // namespace Slic3r::AngledSlicing
