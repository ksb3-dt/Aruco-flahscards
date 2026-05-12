#pragma once

#include <memory>
#include <string>
#include <opencv2/core.hpp>

/// Abstract base for all camera sources. Concrete subclasses adapt different
/// frame providers (USB webcam, IP/HTTP stream from a smartphone, recorded
/// video file) behind a single interface.
///
/// Construction may throw std::runtime_error if the source cannot be opened.
/// nextFrame() returns an empty cv::Mat on transient failure; callers must
/// check frame.empty() before use.
class Camera {
public:
    virtual ~Camera() = default;

    /// Returns the next frame from the source.
    /// On transient failure, returns an empty cv::Mat (caller should retry).
    virtual cv::Mat nextFrame() = 0;

    /// Native resolution of the source. May be (0, 0) until the first frame
    /// has been read.
    virtual cv::Size resolution() const = 0;

    /// Factory method — selects the concrete subclass from the URI scheme:
    ///
    ///   http://<host>:<port>/...   -> IPCamera
    ///   https://<host>:<port>/...  -> IPCamera
    ///   webcam:<index>             -> WebcamCamera   (e.g. "webcam:0")
    ///   file:<path>                -> VideoFileCamera (e.g. "file:clip.mp4")
    ///
    /// Throws std::invalid_argument for unrecognized schemes.
    static std::unique_ptr<Camera> create(const std::string& uri);

protected:
    Camera() = default;
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
};
