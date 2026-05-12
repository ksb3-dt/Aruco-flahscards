#include "PoseEstimator.h"

#include <stdexcept>
#include <vector>

#include <opencv2/calib3d.hpp>
#include <opencv2/core/persistence.hpp>

namespace {

void validateMarkerSize(double markerSizeMeters) {
    if (markerSizeMeters <= 0.0) {
        throw std::invalid_argument(
            "PoseEstimator: marker size must be positive");
    }
}

std::vector<cv::Point3f> markerObjectPoints(double markerSizeMeters) {
    const float halfSize = static_cast<float>(markerSizeMeters / 2.0);
    return {
        {-halfSize,  halfSize, 0.0F},
        { halfSize,  halfSize, 0.0F},
        { halfSize, -halfSize, 0.0F},
        {-halfSize, -halfSize, 0.0F},
    };
}

}  // namespace

PoseEstimator::PoseEstimator(const std::string& calibrationFile,
                             double markerSizeMeters)
    : markerSize_(markerSizeMeters) {
    validateMarkerSize(markerSize_);

    cv::FileStorage storage(calibrationFile, cv::FileStorage::READ);
    if (!storage.isOpened()) {
        throw std::runtime_error(
            "PoseEstimator: failed to open calibration file " +
            calibrationFile);
    }

    storage["camera_matrix"] >> cameraMatrix_;
    storage["dist_coeffs"] >> distCoeffs_;

    if (cameraMatrix_.empty() || distCoeffs_.empty()) {
        throw std::runtime_error(
            "PoseEstimator: calibration file must contain camera_matrix "
            "and dist_coeffs");
    }

    cameraMatrix_.convertTo(cameraMatrix_, CV_64F);
    distCoeffs_.convertTo(distCoeffs_, CV_64F);
}

PoseEstimator::PoseEstimator(cv::Size frameSize, double markerSizeMeters)
    : markerSize_(markerSizeMeters) {
    validateMarkerSize(markerSize_);
    if (frameSize.width <= 0 || frameSize.height <= 0) {
        throw std::invalid_argument(
            "PoseEstimator: frame size must be positive");
    }

    const double width = static_cast<double>(frameSize.width);
    const double height = static_cast<double>(frameSize.height);

    cameraMatrix_ = (cv::Mat_<double>(3, 3) << width, 0.0, width / 2.0,
                     0.0, width, height / 2.0,
                     0.0, 0.0, 1.0);
    distCoeffs_ = cv::Mat::zeros(1, 5, CV_64F);
}

Pose PoseEstimator::estimate(const DetectedMarker& marker) const {
    if (marker.corners.size() != 4) {
        throw std::invalid_argument(
            "PoseEstimator: a marker pose requires exactly four corners");
    }

    Pose pose;
    const bool ok = cv::solvePnP(markerObjectPoints(markerSize_),
                                 marker.corners,
                                 cameraMatrix_,
                                 distCoeffs_,
                                 pose.rvec,
                                 pose.tvec,
                                 false,
                                 cv::SOLVEPNP_IPPE_SQUARE);
    if (!ok) {
        throw std::runtime_error("PoseEstimator: cv::solvePnP failed");
    }

    return pose;
}
