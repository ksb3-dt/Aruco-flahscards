#include "MarkerDetector.h"

#include <opencv2/aruco.hpp>
#include <opencv2/imgproc.hpp>

MarkerDetector::MarkerDetector(cv::aruco::PREDEFINED_DICTIONARY_NAME dict)
    : dictionary_(cv::aruco::getPredefinedDictionary(dict)),
      parameters_(cv::aruco::DetectorParameters::create()) {}

std::vector<DetectedMarker> MarkerDetector::detect(const cv::Mat& frame) const {
    std::vector<DetectedMarker> result;
    if (frame.empty()) {
        return result;
    }

    std::vector<int>                      ids;
    std::vector<std::vector<cv::Point2f>> corners;

    cv::aruco::detectMarkers(frame, dictionary_, corners, ids, parameters_);

    result.reserve(ids.size());
    for (std::size_t i = 0; i < ids.size(); ++i) {
        result.push_back(DetectedMarker{ids[i], std::move(corners[i])});
    }
    return result;
}
