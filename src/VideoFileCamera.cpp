#include "VideoFileCamera.h"

#include <stdexcept>
#include <utility>

VideoFileCamera::VideoFileCamera(std::string path, bool loop)
    : path_(std::move(path)), loop_(loop) {
    if (!capture_.open(path_)) {
        throw std::runtime_error("VideoFileCamera: failed to open " + path_);
    }

    // Probe resolution, then rewind so the first nextFrame() returns frame 0.
    cv::Mat probe;
    capture_ >> probe;
    if (!probe.empty()) {
        resolution_ = probe.size();
    }
    capture_.set(cv::CAP_PROP_POS_FRAMES, 0);
}

cv::Mat VideoFileCamera::nextFrame() {
    cv::Mat frame;
    capture_ >> frame;

    if (frame.empty() && loop_) {
        capture_.set(cv::CAP_PROP_POS_FRAMES, 0);
        capture_ >> frame;
    }
    return frame;
}

cv::Size VideoFileCamera::resolution() const {
    return resolution_;
}
