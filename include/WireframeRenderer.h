#pragma once

#include <opencv2/core.hpp>
#include "Renderer.h"

/// Renders a WireframeAsset by projecting each vertex with cv::projectPoints
/// and drawing each edge with cv::line.
///
/// Expects `asset` to be a WireframeAsset; downcasts at runtime.
class WireframeRenderer : public Renderer {
public:
    /// `color` is BGR; `thickness` in pixels.
    explicit WireframeRenderer(cv::Scalar color = cv::Scalar(0, 255, 0),
                               int        thickness = 2);

    void draw(cv::Mat&             frame,
              const Pose&          pose,
              const Asset&         asset,
              const PoseEstimator& estimator) override;

private:
    cv::Scalar  color_;
    int         thickness_;
};
