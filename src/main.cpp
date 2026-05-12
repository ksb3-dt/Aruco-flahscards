#include <exception>
#include <iostream>
#include <string>
#include <utility>

#include "ARApplication.h"

namespace {

void printUsage(const char* programName) {
    std::cerr
        << "Usage:\n"
        << "  " << programName
        << " <camera-uri> [assets-json] [calibration-yaml]\n\n"
        << "Examples:\n"
        << "  " << programName
        << " http://10.22.142.16:8080/video config/assets.json\n"
        << "  " << programName
        << " webcam:0 config/assets.json\n"
        << "  " << programName
        << " file:assets/test_clip.mp4 config/assets.json\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        printUsage(argv[0]);
        return 1;
    }

    try {
        ARApplication::Config config;
        config.cameraUri = argv[1];
        config.assetsConfig = argc >= 3 ? argv[2] : "config/assets.json";
        config.calibrationFile = argc >= 4 ? argv[3] : "";

        ARApplication app(std::move(config));
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}
