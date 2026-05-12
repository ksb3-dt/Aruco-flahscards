#include "Camera.h"

#include <stdexcept>

#include "IPCamera.h"
#include "VideoFileCamera.h"
#include "WebcamCamera.h"

namespace {

/// True iff `s` begins with `prefix`.
bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() &&
           s.compare(0, prefix.size(), prefix) == 0;
}

}  // namespace

std::unique_ptr<Camera> Camera::create(const std::string& uri) {
    if (startsWith(uri, "http://") || startsWith(uri, "https://")) {
        return std::make_unique<IPCamera>(uri);
    }
    if (startsWith(uri, "file:")) {
        return std::make_unique<VideoFileCamera>(uri.substr(5));
    }
    if (startsWith(uri, "webcam:")) {
        const int index = std::stoi(uri.substr(7));
        return std::make_unique<WebcamCamera>(index);
    }
    throw std::invalid_argument(
        "Camera::create: unrecognized URI scheme: '" + uri + "'");
}
