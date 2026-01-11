#include "frame_extractor.h"
#include <iostream>
#include <csignal>
#include <getopt.h>

using namespace gst_frame_extractor;

// Global extractor pointer for signal handling
FrameExtractor* g_extractor = nullptr;

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;
    if (g_extractor) {
        g_extractor->stop();
    }
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "\nOptions:\n"
              << "  -i, --input <path>      Input video file or URI (required)\n"
              << "  -o, --output <dir>      Output directory (default: ./frames)\n"
              << "  -c, --config <file>     Configuration file (YAML)\n"
              << "  -m, --mode <mode>       Extraction mode: interval, keyframe, all, time_based\n"
              << "  -n, --interval <num>    Frame interval for 'interval' mode (default: 30)\n"
              << "  -t, --time <seconds>    Time interval for 'time_based' mode (default: 1.0)\n"
              << "  -f, --format <fmt>      Output format: png, jpeg, bmp (default: png)\n"
              << "  -q, --quality <num>     JPEG quality 1-100 (default: 95)\n"
              << "  -x, --max <num>         Maximum frames to extract (default: unlimited)\n"
              << "  -p, --prefix <name>     Output filename prefix (default: frame)\n"
              << "  -r, --resize <WxH>      Resize output to WxH (e.g., 640x480)\n"
              << "  --timestamp             Add timestamp overlay to frames\n"
              << "  -v, --verbose           Verbose output\n"
              << "  -h, --help              Show this help message\n"
              << "\nExamples:\n"
              << "  " << program_name << " -i video.mp4 -o ./frames -m interval -n 30\n"
              << "  " << program_name << " -i rtsp://camera/stream -m time_based -t 5.0\n"
              << "  " << program_name << " -c config.yaml\n"
              << std::endl;
}

ExtractorConfig parseCommandLine(int argc, char* argv[]) {
    ExtractorConfig config;

    // Set defaults
    config.output_dir = "./frames";
    config.output_prefix = "frame";
    config.mode = ExtractionMode::INTERVAL;
    config.format = OutputFormat::PNG;
    config.interval_frames = 30;
    config.interval_seconds = 1.0;
    config.jpeg_quality = 95;
    config.max_frames = 0;
    config.resize_output = false;
    config.output_width = 0;
    config.output_height = 0;
    config.add_timestamp_overlay = false;

    static struct option long_options[] = {
        {"input",     required_argument, nullptr, 'i'},
        {"output",    required_argument, nullptr, 'o'},
        {"config",    required_argument, nullptr, 'c'},
        {"mode",      required_argument, nullptr, 'm'},
        {"interval",  required_argument, nullptr, 'n'},
        {"time",      required_argument, nullptr, 't'},
        {"format",    required_argument, nullptr, 'f'},
        {"quality",   required_argument, nullptr, 'q'},
        {"max",       required_argument, nullptr, 'x'},
        {"prefix",    required_argument, nullptr, 'p'},
        {"resize",    required_argument, nullptr, 'r'},
        {"timestamp", no_argument,       nullptr, 'T'},
        {"verbose",   no_argument,       nullptr, 'v'},
        {"help",      no_argument,       nullptr, 'h'},
        {nullptr,     0,                 nullptr, 0}
    };

    std::string config_file;
    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "i:o:c:m:n:t:f:q:x:p:r:Tvh",
                               long_options, &option_index)) != -1) {
        switch (opt) {
            case 'i':
                config.input_uri = optarg;
                break;
            case 'o':
                config.output_dir = optarg;
                break;
            case 'c':
                config_file = optarg;
                break;
            case 'm':
                if (std::string(optarg) == "interval") {
                    config.mode = ExtractionMode::INTERVAL;
                } else if (std::string(optarg) == "keyframe") {
                    config.mode = ExtractionMode::KEYFRAME;
                } else if (std::string(optarg) == "all") {
                    config.mode = ExtractionMode::ALL_FRAMES;
                } else if (std::string(optarg) == "time_based") {
                    config.mode = ExtractionMode::TIME_BASED;
                }
                break;
            case 'n':
                config.interval_frames = std::stoi(optarg);
                break;
            case 't':
                config.interval_seconds = std::stod(optarg);
                break;
            case 'f':
                if (std::string(optarg) == "png") {
                    config.format = OutputFormat::PNG;
                } else if (std::string(optarg) == "jpeg" || std::string(optarg) == "jpg") {
                    config.format = OutputFormat::JPEG;
                } else if (std::string(optarg) == "bmp") {
                    config.format = OutputFormat::BMP;
                }
                break;
            case 'q':
                config.jpeg_quality = std::stoi(optarg);
                break;
            case 'x':
                config.max_frames = std::stoi(optarg);
                break;
            case 'p':
                config.output_prefix = optarg;
                break;
            case 'r': {
                std::string resize_str = optarg;
                size_t x_pos = resize_str.find('x');
                if (x_pos != std::string::npos) {
                    config.resize_output = true;
                    config.output_width = std::stoi(resize_str.substr(0, x_pos));
                    config.output_height = std::stoi(resize_str.substr(x_pos + 1));
                }
                break;
            }
            case 'T':
                config.add_timestamp_overlay = true;
                break;
            case 'v':
                // Verbose mode - could set GST_DEBUG level
                break;
            case 'h':
                printUsage(argv[0]);
                exit(0);
            default:
                printUsage(argv[0]);
                exit(1);
        }
    }

    // Load from config file if specified (overrides defaults but not CLI args)
    if (!config_file.empty()) {
        ExtractorConfig file_config = FrameExtractor::loadConfig(config_file);
        if (config.input_uri.empty()) {
            config.input_uri = file_config.input_uri;
        }
        // Merge other settings as needed
    }

    return config;
}

int main(int argc, char* argv[]) {
    // Parse command line
    ExtractorConfig config = parseCommandLine(argc, argv);

    // Validate input
    if (config.input_uri.empty()) {
        std::cerr << "Error: Input video file or URI is required." << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // Print configuration
    std::cout << "=== Video Frame Extractor ===" << std::endl;
    std::cout << "Input: " << config.input_uri << std::endl;
    std::cout << "Output: " << config.output_dir << std::endl;
    std::cout << "Mode: ";
    switch (config.mode) {
        case ExtractionMode::INTERVAL:
            std::cout << "interval (every " << config.interval_frames << " frames)" << std::endl;
            break;
        case ExtractionMode::KEYFRAME:
            std::cout << "keyframe" << std::endl;
            break;
        case ExtractionMode::ALL_FRAMES:
            std::cout << "all frames" << std::endl;
            break;
        case ExtractionMode::TIME_BASED:
            std::cout << "time-based (every " << config.interval_seconds << " seconds)" << std::endl;
            break;
    }
    std::cout << "Format: ";
    switch (config.format) {
        case OutputFormat::PNG:
            std::cout << "PNG" << std::endl;
            break;
        case OutputFormat::JPEG:
            std::cout << "JPEG (quality: " << config.jpeg_quality << ")" << std::endl;
            break;
        case OutputFormat::BMP:
            std::cout << "BMP" << std::endl;
            break;
    }
    if (config.max_frames > 0) {
        std::cout << "Max frames: " << config.max_frames << std::endl;
    }
    if (config.resize_output) {
        std::cout << "Resize: " << config.output_width << "x" << config.output_height << std::endl;
    }
    std::cout << "==============================" << std::endl;

    // Setup signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create and initialize extractor
    FrameExtractor extractor;
    g_extractor = &extractor;

    if (!extractor.initialize(config)) {
        std::cerr << "Failed to initialize extractor: " << extractor.getLastError() << std::endl;
        return 1;
    }

    // Optional: Set custom frame callback for additional processing
    extractor.setFrameCallback([](const cv::Mat& frame, int64_t pts, int frame_number) {
        // Custom processing can be done here
        // For example: face detection, object detection, etc.
    });

    // Start extraction
    if (!extractor.start()) {
        std::cerr << "Failed to start extraction: " << extractor.getLastError() << std::endl;
        return 1;
    }

    // Wait for completion
    extractor.waitForCompletion();

    // Print summary
    std::cout << "\n=== Extraction Complete ===" << std::endl;
    std::cout << "Total frames processed: " << extractor.getTotalFrameCount() << std::endl;
    std::cout << "Frames extracted: " << extractor.getExtractedFrameCount() << std::endl;
    std::cout << "Output directory: " << config.output_dir << std::endl;

    g_extractor = nullptr;
    return 0;
}
