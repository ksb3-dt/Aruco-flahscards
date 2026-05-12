#pragma once

#include <string>
#include <opencv2/core.hpp>
#include "Asset.h"

/// A 2D image to be warped onto the marker plane. The primary asset type
/// for the flashcard demo (skeleton diagram, molecule, globe, ...).
class TextureAsset : public Asset {
public:
    /// Loads the image from disk. Throws std::runtime_error on failure.
    explicit TextureAsset(const std::string& imagePath);

    std::string name() const override { return name_; }

    std::unique_ptr<Renderer> createRenderer() const override;

    /// The texture pixels (BGR or BGRA). Returned by const reference to
    /// avoid copying a potentially large cv::Mat.
    const cv::Mat& image() const { return image_; }

private:
    std::string  name_;
    cv::Mat      image_;
};
