#pragma once

#include <string>
#include <opencv2/videoio.hpp>
#include "Camera.h"

/// Camera backed by a recorded video file. Useful during development to
/// iterate on detection / rendering without holding the phone steady.
///
/// By default the clip loops indefinitely.
class VideoFileCamera : public Camera {
public:
    /// Opens the given file. Throws std::runtime_error on failure.
    explicit VideoFileCamera(std::string path, bool loop = true);

    cv::Mat  nextFrame() override;
    cv::Size resolution() const override;

private:
    std::string       path_;
    bool              loop_;
    cv::VideoCapture  capture_;
    cv::Size          resolution_;
};
