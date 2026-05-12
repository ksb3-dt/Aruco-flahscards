#include "WebcamCamera.h"

#include <stdexcept>
#include <string>

WebcamCamera::WebcamCamera(int deviceIndex) : deviceIndex_(deviceIndex) {
    if (!capture_.open(deviceIndex_)) {
        throw std::runtime_error(
            "WebcamCamera: failed to open device " +
            std::to_string(deviceIndex_));
    }

    cv::Mat probe;
    capture_ >> probe;
    if (!probe.empty()) {
        resolution_ = probe.size();
    }
}

cv::Mat WebcamCamera::nextFrame() {
    cv::Mat frame;
    capture_ >> frame;
    return frame;
}

cv::Size WebcamCamera::resolution() const {
    return resolution_;
}
