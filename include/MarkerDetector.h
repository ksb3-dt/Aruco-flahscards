#pragma once

#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/aruco.hpp>

/// A single ArUco marker found in a frame.
struct DetectedMarker {
    int                       id;       ///< Numeric marker ID from the dictionary.
    std::vector<cv::Point2f>  corners;  ///< 4 corners in pixel coords, clockwise from top-left.
};

/// Detects ArUco markers in camera frames. Stateless aside from configuration.
class MarkerDetector {
public:
    /// Default dictionary: 6x6 markers, 250 unique IDs — a good balance of
    /// robustness and ID space for this project.
    explicit MarkerDetector(
        cv::aruco::PredefinedDictionaryType dict = cv::aruco::DICT_6X6_250);

    /// Detects every marker visible in the frame.
    std::vector<DetectedMarker> detect(const cv::Mat& frame) const;

private:
    cv::aruco::Dictionary         dictionary_;
    cv::aruco::DetectorParameters parameters_;
};
