#pragma once

#include "Renderer.h"

/// Renders a TextureAsset by computing the homography from the asset's
/// rectangle to the projected marker quad and warping the texture onto the
/// frame via cv::warpPerspective.
///
/// Expects `asset` to be a TextureAsset; downcasts at runtime.
class TextureRenderer : public Renderer {
public:
    void draw(OpenGLRenderContext& context,
              const Pose&          pose,
              const Asset&         asset,
              const PoseEstimator& estimator) override;
};
