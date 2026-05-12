#pragma once

#include <string>
#include <opencv2/videoio.hpp>
#include "Camera.h"

/// Camera backed by an HTTP/MJPEG stream, such as the one served by the
/// "IP Webcam" Android app or by ESP32-CAM modules.
///
/// Example URL: http://192.168.0.42:8080/video
class IPCamera : public Camera {
public:
    /// Opens the stream at the given URL. Throws std::runtime_error if the
    /// initial connection fails.
    explicit IPCamera(std::string url);

    cv::Mat  nextFrame() override;
    cv::Size resolution() const override;

private:
    /// Releases and re-opens the underlying capture. Called on empty frames.
    void reconnect();

    std::string       url_;
    cv::VideoCapture  capture_;
    cv::Size          resolution_;
};
