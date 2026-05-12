#include "IPCamera.h"

#include <iostream>
#include <stdexcept>
#include <utility>

IPCamera::IPCamera(std::string url) : url_(std::move(url)) {
    if (!capture_.open(url_)) {
        throw std::runtime_error("IPCamera: failed to connect to " + url_);
    }

    // Probe resolution from the first frame. Some MJPEG streams don't
    // populate CAP_PROP_FRAME_WIDTH/HEIGHT correctly, so we just read once.
    cv::Mat probe;
    capture_ >> probe;
    if (!probe.empty()) {
        resolution_ = probe.size();
    }
}

cv::Mat IPCamera::nextFrame() {
    cv::Mat frame;
    capture_ >> frame;

    if (frame.empty()) {
        std::cerr << "IPCamera: empty frame from " << url_
                  << ", attempting reconnect...\n";
        reconnect();
        capture_ >> frame;  // one retry; caller handles further failures
    }
    return frame;
}

cv::Size IPCamera::resolution() const {
    return resolution_;
}

void IPCamera::reconnect() {
    capture_.release();
    capture_.open(url_);
}
