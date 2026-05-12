#pragma once

#include <string>
#include <opencv2/core.hpp>
#include "MarkerDetector.h"

/// 6-DoF pose of a marker relative to the camera. Stored in Rodrigues form
/// to match the convention of cv::solvePnP / cv::aruco::estimatePoseSingleMarkers.
struct Pose {
    cv::Vec3d rvec;  ///< Rotation vector (Rodrigues).
    cv::Vec3d tvec;  ///< Translation vector.
};

/// Estimates 6-DoF marker pose given camera intrinsics and a known marker
/// side length.
class PoseEstimator {
public:
    /// Loads intrinsics from a YAML file produced by OpenCV's
    /// calibrateCamera tutorial. Expected keys: camera_matrix, dist_coeffs.
    ///
    /// markerSizeMeters is the side length of the printed marker (e.g. 0.05
    /// for a 5 cm marker).
    PoseEstimator(const std::string& calibrationFile, double markerSizeMeters);

    /// Convenience constructor for development without calibration. Builds
    /// approximate intrinsics from frame dimensions (fx = fy = width,
    /// principal point at the image center, no distortion).
    PoseEstimator(cv::Size frameSize, double markerSizeMeters);

    /// Estimates pose for a single detected marker.
    Pose estimate(const DetectedMarker& marker) const;

    /// Underlying intrinsics — exposed by reference so renderers can call
    /// cv::projectPoints without copying.
    const cv::Mat& cameraMatrix() const { return cameraMatrix_; }
    const cv::Mat& distCoeffs()   const { return distCoeffs_;   }

    double markerSize() const { return markerSize_; }

private:
    cv::Mat  cameraMatrix_;
    cv::Mat  distCoeffs_;
    double   markerSize_;
};
