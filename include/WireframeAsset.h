#pragma once

#include <string>
#include <utility>
#include <vector>
#include <opencv2/core.hpp>
#include "Asset.h"

/// A 3D wireframe model. Vertices are expressed in marker-local coordinates
/// (marker plane = XY, marker center = origin, Z points out of the card).
/// Edges are pairs of indices into the vertex array.
class WireframeAsset : public Asset {
public:
    /// Constructs from explicit vertex and edge lists.
    WireframeAsset(std::string name,
                   std::vector<cv::Point3f> vertices,
                   std::vector<std::pair<int, int>> edges);

    /// Loads a subset of the Wavefront OBJ format: `v x y z` lines for
    /// vertices and `l i j` lines for edges. Faces are not interpreted.
    explicit WireframeAsset(const std::string& objPath);

    std::string name() const override { return name_; }

    std::unique_ptr<Renderer> createRenderer() const override;

    const std::vector<cv::Point3f>&         vertices() const { return vertices_; }
    const std::vector<std::pair<int, int>>& edges()    const { return edges_;    }

    /// Convenience factory: a unit cube sitting on the marker, side length
    /// equal to the marker side. Useful as a calibration sanity check.
    static std::unique_ptr<WireframeAsset> makeUnitCube(double markerSize);

private:
    std::string                      name_;
    std::vector<cv::Point3f>         vertices_;
    std::vector<std::pair<int, int>> edges_;
};
