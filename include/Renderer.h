#pragma once

#include <opencv2/core.hpp>

class Asset;          // forward decl
class PoseEstimator;  // forward decl
struct Pose;          // forward decl

/// Abstract drawing strategy. Concrete subclasses know how to render
/// a specific kind of Asset onto a frame at a given pose.
///
/// `draw` modifies the frame in place — this matches OpenCV's drawing
/// conventions and avoids copies.
class Renderer {
public:
    virtual ~Renderer() = default;

    /// Draws `asset` onto `frame` at `pose`. The renderer may need
    /// camera intrinsics (held by `estimator`) to project 3D points.
    virtual void draw(cv::Mat&             frame,
                      const Pose&          pose,
                      const Asset&         asset,
                      const PoseEstimator& estimator) = 0;

protected:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
};
