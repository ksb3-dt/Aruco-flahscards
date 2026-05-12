#pragma once

#include <opencv2/videoio.hpp>
#include "Camera.h"

/// Camera backed by a local capture device (USB webcam, integrated camera,
/// or a v4l2loopback-based virtual device such as the one DroidCam exposes).
class WebcamCamera : public Camera {
public:
    /// Opens the device at the given index (0 = default). Throws
    /// std::runtime_error if the device cannot be opened.
    explicit WebcamCamera(int deviceIndex);

    cv::Mat  nextFrame() override;
    cv::Size resolution() const override;

private:
    int               deviceIndex_;
    cv::VideoCapture  capture_;
    cv::Size          resolution_;
};
