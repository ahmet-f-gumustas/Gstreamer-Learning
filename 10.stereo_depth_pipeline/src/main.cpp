#include "stereo_pipeline.h"
#include "depth_estimator.h"
#include "obstacle_detector.h"

#include <gst/gst.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <csignal>
#include <atomic>
#include <memory>
#include <string>
#include <chrono>

// ─── Global durdurma bayrağı ─────────────────────────────────────────────────
static std::atomic<bool> g_stop{false};

static void signalHandler(int) { g_stop = true; }

// ─── Yardım mesajı ───────────────────────────────────────────────────────────
static void printUsage(const char* name)
{
    std::cout
        << "\nKullanim: " << name << " [SECENEKLER]\n\n"
        << "Kaynak modlari:\n"
        << "  --sim                Simulasyon (varsayilan, kamera gerekmez)\n"
        << "  --webcam             Cift USB kamera\n"
        << "  --file   <yol>       Video dosyasi\n\n"
        << "Kamera secenekleri:\n"
        << "  --left   <cihaz>     Sol kamera (varsayilan: /dev/video0)\n"
        << "  --right  <cihaz>     Sag kamera (varsayilan: /dev/video2)\n"
        << "  --width  <px>        Genislik (varsayilan: 640)\n"
        << "  --height <px>        Yukseklik (varsayilan: 480)\n\n"
        << "Derinlik secenekleri:\n"
        << "  --sgbm               StereoSGBM kullan (daha hassas, yavas)\n"
        << "  --focal  <px>        Odak uzunlugu piksel cinsinden (varsayilan: 554)\n"
        << "  --base   <m>         Baseline metre cinsinden (varsayilan: 0.06)\n"
        << "  --calib  <yaml>      Kalibrasyon dosyasi\n\n"
        << "Engel secenekleri:\n"
        << "  --danger  <m>        Tehlike esigi (varsayilan: 1.0 m)\n"
        << "  --caution <m>        Dikkat esigi  (varsayilan: 3.0 m)\n\n"
        << "Diger:\n"
        << "  --save   <yol>       Ciktilari video olarak kaydet\n"
        << "  -h, --help           Bu mesaji goster\n\n"
        << "Ornekler:\n"
        << "  " << name << " --sim\n"
        << "  " << name << " --webcam --left /dev/video0 --right /dev/video2\n"
        << "  " << name << " --file video.mp4 --sgbm\n";
}

// ─── Pencere düzeni ──────────────────────────────────────────────────────────
static cv::Mat buildDisplay(const cv::Mat& left,
                            const cv::Mat& right,
                            const cv::Mat& colorDepth,
                            const cv::Mat& overlay)
{
    // Her görüntüyü aynı boyuta getir
    cv::Size sz = left.size();
    cv::Mat rR, rCD, rOV;
    cv::resize(right,      rR,  sz);
    cv::resize(colorDepth, rCD, sz);
    cv::resize(overlay,    rOV, sz);

    // Etiket ekle
    auto label = [](cv::Mat& img, const std::string& txt) {
        cv::rectangle(img, cv::Rect(0, 0, img.cols, 22), cv::Scalar(20,20,20), cv::FILLED);
        cv::putText(img, txt, cv::Point(4, 16),
                    cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(220,220,220), 1);
    };

    cv::Mat L = left.clone(), R = rR.clone(), D = rCD.clone(), O = rOV.clone();
    label(L, "Sol Kamera");
    label(R, "Sag Kamera");
    label(D, "Derinlik Haritasi  (kirmizi=yakin, mavi=uzak)");
    label(O, "Engel Analizi");

    cv::Mat top, bot, full;
    cv::hconcat(L, R,   top);
    cv::hconcat(D, O,   bot);
    cv::vconcat(top, bot, full);
    return full;
}

// ─── FPS hesaplayıcı ─────────────────────────────────────────────────────────
class FpsCounter {
    std::chrono::steady_clock::time_point t0_ = std::chrono::steady_clock::now();
    int frames_ = 0;
    float fps_  = 0.f;
public:
    float tick() {
        ++frames_;
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - t0_).count();
        if (dt >= 1.0f) {
            fps_    = frames_ / dt;
            frames_ = 0;
            t0_     = now;
        }
        return fps_;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

    gst_init(&argc, &argv);

    // ── Argüman ayrıştırma ────────────────────────────────────────────────────
    StereoPipeline::SourceMode mode = StereoPipeline::SourceMode::SIMULATION;
    std::string leftSrc  = "/dev/video0";
    std::string rightSrc = "/dev/video2";
    std::string filePath;
    std::string calibPath;
    std::string savePath;
    int   width   = 640;
    int   height  = 480;
    float focal   = 554.f;
    float base    = 0.06f;
    float danger  = 1.0f;
    float caution = 3.0f;
    bool  useSGBM = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            return (i + 1 < argc) ? argv[++i] : "";
        };

        if      (a == "--sim")                mode     = StereoPipeline::SourceMode::SIMULATION;
        else if (a == "--webcam")             mode     = StereoPipeline::SourceMode::DUAL_WEBCAM;
        else if (a == "--file")             { mode     = StereoPipeline::SourceMode::VIDEO_FILE;
                                              filePath = next(); }
        else if (a == "--left")               leftSrc  = next();
        else if (a == "--right")              rightSrc = next();
        else if (a == "--width")              width    = std::stoi(next());
        else if (a == "--height")             height   = std::stoi(next());
        else if (a == "--focal")              focal    = std::stof(next());
        else if (a == "--base")               base     = std::stof(next());
        else if (a == "--danger")             danger   = std::stof(next());
        else if (a == "--caution")            caution  = std::stof(next());
        else if (a == "--calib")              calibPath = next();
        else if (a == "--save")               savePath  = next();
        else if (a == "--sgbm")               useSGBM  = true;
        else if (a == "-h" || a == "--help") { printUsage(argv[0]); return 0; }
        else std::cerr << "[main] Bilinmeyen arguman: " << a << "\n";
    }

    // ── Bileşen başlatma ─────────────────────────────────────────────────────
    StereoPipeline pipeline;
    std::string src = (mode == StereoPipeline::SourceMode::VIDEO_FILE) ? filePath : leftSrc;
    if (!pipeline.initialize(mode, src, rightSrc, width, height)) {
        std::cerr << "[main] Pipeline baslatilamadi!\n";
        gst_deinit();
        return 1;
    }

    DepthEstimator depth;
    depth.setAlgorithm(useSGBM ? DepthEstimator::Algorithm::SEMI_GLOBAL_BM
                               : DepthEstimator::Algorithm::BLOCK_MATCHING);
    depth.setFocalLength(focal);
    depth.setBaseline(base);

    // Simülasyon modunda yapay sağ görüntü üret
    if (mode == StereoPipeline::SourceMode::SIMULATION)
        depth.setSim(true, /*shift_px=*/30);

    if (!calibPath.empty())
        depth.loadCalibration(calibPath);

    ObstacleDetector detector(danger, caution, /*rows=*/3, /*cols=*/4);

    // Video kayıt
    cv::VideoWriter writer;
    bool doSave = !savePath.empty();

    // ── Pipeline başlat ───────────────────────────────────────────────────────
    pipeline.start();

    cv::namedWindow("Stereo Depth Pipeline", cv::WINDOW_NORMAL);
    cv::resizeWindow("Stereo Depth Pipeline", width * 2, height * 2);

    FpsCounter fps;
    StereoFrame frame;
    int frameIdx = 0;

    std::cout << "\n[main] Calistirildi. Cikis icin 'q' veya Ctrl+C\n";
    std::cout << "[main] Mod: "
              << (mode == StereoPipeline::SourceMode::SIMULATION ? "Simulasyon" :
                  mode == StereoPipeline::SourceMode::DUAL_WEBCAM ? "Cift Webcam" : "Video Dosyasi")
              << "  Algo: " << (useSGBM ? "SGBM" : "BM")
              << "  f=" << focal << "px  B=" << base * 100.f << "cm\n\n";

    // ── Ana döngü ─────────────────────────────────────────────────────────────
    while (!g_stop && pipeline.isRunning()) {

        if (!pipeline.getFrame(frame)) {
            // Timeout – bus mesajlarını kontrol et
            continue;
        }

        // Derinlik hesabı
        DepthResult depth_r = depth.compute(frame.left, frame.right);
        if (!depth_r.valid) continue;

        // Engel analizi
        ObstacleResult obs = detector.analyze(depth_r.depthMap, frame.left);

        // Ekran görüntüsü
        cv::Mat display = buildDisplay(frame.left, frame.right,
                                       depth_r.colorDepth, obs.overlay);

        // FPS göster
        float f = fps.tick();
        if (f > 0.f) {
            std::ostringstream fpsStr;
            fpsStr << std::fixed << std::setprecision(1) << f << " fps";
            cv::putText(display, fpsStr.str(),
                        cv::Point(display.cols - 100, display.rows - 8),
                        cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(200,200,200), 1);
        }

        cv::imshow("Stereo Depth Pipeline", display);

        // Video kayıt
        if (doSave) {
            if (!writer.isOpened())
                writer.open(savePath, cv::VideoWriter::fourcc('M','J','P','G'),
                            30, display.size());
            writer.write(display);
        }

        // Terminal çıktısı (her 30 frame'de bir)
        if (++frameIdx % 30 == 0) {
            std::cout << "[Frame " << frameIdx << "] "
                      << "Min: " << std::fixed << std::setprecision(2) << depth_r.minDepthM << "m  "
                      << "Max: " << depth_r.maxDepthM << "m  "
                      << "En yakin engel: " << obs.closestM << "m  "
                      << "Durum: ";
            switch (obs.overallDanger) {
                case DangerLevel::DANGER:  std::cout << "TEHLIKE\n"; break;
                case DangerLevel::CAUTION: std::cout << "DIKKAT\n";  break;
                default:                   std::cout << "GUVENLI\n"; break;
            }
        }

        int key = cv::waitKey(1);
        if (key == 'q' || key == 'Q' || key == 27) g_stop = true;
    }

    // ── Temizlik ─────────────────────────────────────────────────────────────
    pipeline.stop();
    if (writer.isOpened()) writer.release();
    cv::destroyAllWindows();
    gst_deinit();

    std::cout << "[main] Program sonlandi. Toplam frame: " << frameIdx << "\n";
    return 0;
}
