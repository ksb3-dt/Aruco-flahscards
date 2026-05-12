// AR Flashcards — smoke test entry point.
//
// At this stage `main` only exercises the Camera hierarchy: it opens a
// camera via Camera::create() and displays frames in a window. This is
// enough to verify that:
//   - the build system finds OpenCV (including the videoio backend that
//     handles MJPEG over HTTP for the IP Webcam app),
//   - the runtime can reach the smartphone stream,
//   - the polymorphic Camera interface works at the call site.
//
// Once MarkerDetector / PoseEstimator / Renderer / AssetManager are
// implemented, this file will be replaced by a thin wrapper that
// constructs ARApplication and calls run().

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/aruco.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "Camera.h"
#include "MarkerDetector.h"
#include "PoseEstimator.h"

namespace {

constexpr int kEscKey = 27;
constexpr double kMarkerSizeMeters = 0.05;
constexpr float kAxisLengthMeters = 0.05F;

void printUsage(const char* programName) {
    std::cerr
        << "Usage:\n"
        << "  " << programName << " <camera-uri>\n\n"
        << "Examples:\n"
        << "  " << programName
        << " http://192.168.0.42:8080/video   # IP Webcam (Android)\n"
        << "  " << programName
        << " webcam:0                          # local USB / integrated webcam\n"
        << "  " << programName
        << " file:assets/test_clip.mp4         # recorded video clip\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    const std::string uri = argv[1];

    try {
        std::unique_ptr<Camera> camera = Camera::create(uri);
        MarkerDetector          detector;
        PoseEstimator           estimator(camera->resolution(),
                                          kMarkerSizeMeters);

        std::cout << "Camera opened: " << uri << "\n"
                  << "Resolution:    " << camera->resolution() << "\n"
                  << "Press ESC in the preview window to quit.\n";

        const std::string window = "AR Flashcards - smoke test";
        cv::namedWindow(window, cv::WINDOW_AUTOSIZE);

        while (true) {
            cv::Mat frame = camera->nextFrame();
            if (frame.empty()) {
                std::cerr << "Empty frame, retrying...\n";
                continue;
            }

            // Detect ArUco markers and overlay outlines, IDs, and 3D axes.
            const std::vector<DetectedMarker> markers = detector.detect(frame);
            if (!markers.empty()) {
                std::vector<std::vector<cv::Point2f>> corners;
                std::vector<int>                      ids;
                corners.reserve(markers.size());
                ids.reserve(markers.size());
                for (const DetectedMarker& m : markers) {
                    corners.push_back(m.corners);
                    ids.push_back(m.id);
                }
                cv::aruco::drawDetectedMarkers(frame, corners, ids);

                for (const DetectedMarker& marker : markers) {
                    const Pose pose = estimator.estimate(marker);
                    cv::drawFrameAxes(frame,
                                      estimator.cameraMatrix(),
                                      estimator.distCoeffs(),
                                      pose.rvec,
                                      pose.tvec,
                                      kAxisLengthMeters);
                }
            }

            cv::imshow(window, frame);
            if (cv::waitKey(1) == kEscKey) {
                break;
            }
        }

        cv::destroyAllWindows();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}
