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
    /// Default dictionary: 4x4 markers, 50 unique IDs. Matches the markers
    /// produced by chev.me/arucogen with the default settings, and the small
    /// dictionary size gives the largest inter-marker Hamming distance, which
    /// minimizes false positives. 50 IDs is plenty for a flashcard set.
    explicit MarkerDetector(
        cv::aruco::PREDEFINED_DICTIONARY_NAME dict = cv::aruco::DICT_4X4_50);

    /// Detects every marker visible in the frame.
    std::vector<DetectedMarker> detect(const cv::Mat& frame) const;

private:
    // OpenCV 4.6 ArUco API uses Ptr<>-managed dictionary and parameters.
    cv::Ptr<cv::aruco::Dictionary>         dictionary_;
    cv::Ptr<cv::aruco::DetectorParameters> parameters_;
};
