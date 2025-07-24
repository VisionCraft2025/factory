#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <chrono>
#include <deque>
#include <signal.h>

// --- 전역 변수 선언 ---
std::vector<cv::Point> g_points;
cv::Mat g_currentFrame;
bool g_drawing = false;
cv::Mat g_roiMask;
bool g_roiSelected = false;

// 성능 최적화를 위한 변수
const int CAPTURE_WIDTH = 320;   
const int CAPTURE_HEIGHT = 240;
const int CAPTURE_FPS = 30;      // 안정성을 위해 30FPS로 낮춤
const int BLUR_SIZE = 5;         // 블러 크기도 약간 줄임

// SAD 감지 관련
cv::Mat g_prevFrame;
cv::Mat g_baselineFrame;
std::atomic<double> g_sadThreshold(50000.0);
const int DEBOUNCE_FRAMES = 15;
int g_framesSinceDetection = 0;
bool g_isBottlePresent = false;

// 통계 및 성능 측정
double g_currentSAD = 0.0;
std::deque<double> g_sadHistory;  // queue 대신 deque 사용
const int HISTORY_SIZE = 30;
double g_fps = 0.0;
auto g_lastTime = std::chrono::high_resolution_clock::now();
int g_frameCounter = 0;

// 멀티스레딩
std::atomic<bool> g_shouldExit(false);
std::mutex g_thresholdMutex;
cv::VideoCapture* g_cap = nullptr;  // 전역 포인터로 관리

// 시그널 핸들러
void signalHandler(int signum) {
    std::cout << "\n인터럽트 신호 받음. 프로그램을 종료합니다..." << std::endl;
    g_shouldExit = true;
}

// --- 함수 선언 ---
void onMouse(int event, int x, int y, int flags, void* userdata);
void pushBottle();
void inputHandler();
void captureBaseline();
double calculateFastSAD(const cv::Mat& current, const cv::Mat& previous, const cv::Mat& mask);
void updateFPS();

// --- 마우스 콜백 함수 ---
void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (g_roiSelected) return;

    // 디스플레이 좌표를 캡처 좌표로 변환
    x = x * CAPTURE_WIDTH / 640;
    y = y * CAPTURE_HEIGHT / 480;

    if (event == cv::EVENT_LBUTTONDOWN) {
        g_points.push_back(cv::Point(x, y));
        g_drawing = true;
        std::cout << "점 추가: (" << x << ", " << y << ")" << std::endl;
    } else if (event == cv::EVENT_RBUTTONDOWN) {
        if (g_drawing && g_points.size() > 2) {
            g_drawing = false;
            g_roiSelected = true;

            g_roiMask = cv::Mat::zeros(cv::Size(CAPTURE_WIDTH, CAPTURE_HEIGHT), CV_8UC1);
            const cv::Point* pts[1] = { g_points.data() };
            int npts[] = { (int)g_points.size() };
            cv::fillPoly(g_roiMask, pts, npts, 1, cv::Scalar(255));

            std::cout << "\n=== ROI 선택 완료 ===" << std::endl;
            std::cout << "기준 프레임 캡처: 'b'" << std::endl;
            std::cout << "임계값 조절: 숫자 입력 또는 [/] (10%씩 감소/증가)" << std::endl;
            std::cout << "자동 임계값 설정: 'a' (현재 SAD의 150%)" << std::endl;
            std::cout << "통계: 's', 종료: 'q'" << std::endl;
            std::cout << "========================\n" << std::endl;
        }
    }
}

// --- 병 감지 액션 ---
void pushBottle() {
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    std::cout << "\n[" << timestamp << "] *** 병 감지! ***" << std::endl;
    std::cout << "SAD: " << std::fixed << std::setprecision(2) << g_currentSAD 
              << " (임계값: " << g_sadThreshold << ")" << std::endl;
}

// --- 터미널 입력 처리 ---
void inputHandler() {
    std::string input;
    while (!g_shouldExit) {
        if (!std::getline(std::cin, input)) break;  // EOF 체크
        
        if (input.empty()) continue;
        
        try {
            // 숫자 입력 시 임계값 직접 설정
            double newThreshold = std::stod(input);
            if (newThreshold > 0) {
                std::lock_guard<std::mutex> lock(g_thresholdMutex);
                g_sadThreshold = newThreshold;
                std::cout << "임계값 설정: " << newThreshold << std::endl;
            }
        } catch (std::invalid_argument&) {
            // 명령어 처리
            if (input == "]") {  // 증가
                g_sadThreshold = g_sadThreshold * 1.1;
                std::cout << "임계값 증가: " << g_sadThreshold << std::endl;
            } else if (input == "[") {  // 감소
                g_sadThreshold = g_sadThreshold * 0.9;
                std::cout << "임계값 감소: " << g_sadThreshold << std::endl;
            } else if (input == "s") {
                // 통계 정보
                double avg = 0.0;
                if (!g_sadHistory.empty()) {
                    for (auto val : g_sadHistory) {
                        avg += val;
                    }
                    avg /= g_sadHistory.size();
                }
                
                std::cout << "\n=== 성능 및 통계 ===" << std::endl;
                std::cout << "FPS: " << std::fixed << std::setprecision(1) << g_fps << std::endl;
                std::cout << "현재 SAD: " << g_currentSAD << std::endl;
                std::cout << "평균 SAD (최근 " << g_sadHistory.size() << "프레임): " << avg << std::endl;
                std::cout << "임계값: " << g_sadThreshold << std::endl;
                std::cout << "병 감지 상태: " << (g_isBottlePresent ? "YES" : "NO") << std::endl;
                std::cout << "===================\n" << std::endl;
            } else if (input == "a" && g_roiSelected) {
                // 자동 임계값 설정
                if (g_currentSAD > 0) {
                    g_sadThreshold = g_currentSAD * 1.5;
                    std::cout << "자동 임계값 설정: " << g_sadThreshold << " (현재 SAD의 150%)" << std::endl;
                }
            } else if (input == "q") {
                g_shouldExit = true;
                break;
            }
        }
    }
}

// --- 기준 프레임 캡처 ---
void captureBaseline() {
    if (!g_currentFrame.empty() && !g_roiMask.empty()) {
        g_baselineFrame = g_currentFrame.clone();
        std::cout << "기준 프레임 캡처 완료 (현재 상태를 기준으로 설정)" << std::endl;
    }
}

// --- 최적화된 SAD 계산 ---
double calculateFastSAD(const cv::Mat& current, const cv::Mat& previous, const cv::Mat& mask) {
    if (current.empty() || previous.empty() || mask.empty()) {
        return 0.0;
    }
    
    if (current.size() != previous.size() || current.size() != mask.size()) {
        return 0.0;
    }
    
    cv::Mat diff;
    cv::absdiff(current, previous, diff);
    
    // 마스크 적용
    cv::Mat maskedDiff;
    cv::bitwise_and(diff, mask, maskedDiff);
    
    // 빠른 합산
    return cv::sum(maskedDiff)[0];
}

// --- FPS 업데이트 ---
void updateFPS() {
    g_frameCounter++;
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastTime).count();
    
    if (duration >= 1000) {  // 1초마다 업데이트
        g_fps = (g_frameCounter * 1000.0) / duration;
        g_frameCounter = 0;
        g_lastTime = now;
    }
}

// --- 메인 함수 ---
int main() {
    // 시그널 핸들러 설정
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "=== 고속 플라스틱병 감지 시스템 v3.1 ===" << std::endl;
    std::cout << "해상도: " << CAPTURE_WIDTH << "x" << CAPTURE_HEIGHT 
              << " @ " << CAPTURE_FPS << "FPS" << std::endl;
    
    // 더 단순한 파이프라인 사용
    std::string pipeline = 
        "libcamerasrc ! "
        "video/x-raw,width=" + std::to_string(CAPTURE_WIDTH) + 
        ",height=" + std::to_string(CAPTURE_HEIGHT) + 
        ",framerate=" + std::to_string(CAPTURE_FPS) + "/1 ! "
        "videoconvert ! "
        "videoscale ! "
        "appsink drop=true max-buffers=1";  // 버퍼 설정 추가
    
    g_cap = new cv::VideoCapture(pipeline, cv::CAP_GSTREAMER);
    
    if (!g_cap->isOpened()) {
        std::cerr << "오류: 카메라를 열 수 없습니다." << std::endl;
        delete g_cap;
        return -1;
    }
    
    // 버퍼 크기 설정
    g_cap->set(cv::CAP_PROP_BUFFERSIZE, 1);
    
    std::cout << "카메라 준비 완료!\n" << std::endl;
    std::cout << "=== 사용 방법 ===" << std::endl;
    std::cout << "1. 마우스 왼쪽 클릭: ROI 점 추가" << std::endl;
    std::cout << "2. 마우스 오른쪽 클릭: ROI 완성" << std::endl;
    std::cout << "3. 'b': 기준 프레임 캡처" << std::endl;
    std::cout << "4. '[' / ']': 임계값 10% 감소/증가" << std::endl;
    std::cout << "5. 숫자 입력: 임계값 직접 설정" << std::endl;
    std::cout << "6. 'a': 자동 임계값 (현재 SAD x 1.5)" << std::endl;
    std::cout << "7. 's': 통계 보기" << std::endl;
    std::cout << "8. 'q': 종료" << std::endl;
    std::cout << "==================\n" << std::endl;
    
    cv::namedWindow("Camera Feed", cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback("Camera Feed", onMouse, NULL);
    
    // 입력 스레드 시작
    std::thread inputThread(inputHandler);
    
    // 프로세싱용 변수
    cv::Mat frame, grayFrame, blurredFrame, displayFrame;
    int errorCount = 0;
    const int MAX_ERRORS = 10;
    
    while (!g_shouldExit) {
        if (!g_cap->read(frame)) {
            errorCount++;
            if (errorCount > MAX_ERRORS) {
                std::cerr << "프레임 읽기 실패가 계속됩니다. 종료합니다." << std::endl;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        errorCount = 0;  // 성공 시 에러 카운트 리셋
        
        if (frame.empty()) continue;
        
        // 그레이스케일 변환
        if (frame.channels() > 1) {
            cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
        } else {
            grayFrame = frame;
        }
        
        // 가우시안 블러 적용
        cv::GaussianBlur(grayFrame, blurredFrame, cv::Size(BLUR_SIZE, BLUR_SIZE), 0);
        
        // 현재 프레임 저장
        g_currentFrame = blurredFrame.clone();
        
        // 디스플레이용 프레임 준비 (확대)
        cv::resize(blurredFrame, displayFrame, cv::Size(640, 480), 0, 0, cv::INTER_LINEAR);
        
        if (!g_roiSelected) {
            // ROI 선택 모드
            if (g_drawing && g_points.size() > 0) {
                // 디스플레이 좌표로 변환하여 그리기
                std::vector<cv::Point> displayPoints;
                for (const auto& pt : g_points) {
                    displayPoints.push_back(cv::Point(pt.x * 640 / CAPTURE_WIDTH, 
                                                    pt.y * 480 / CAPTURE_HEIGHT));
                }
                
                for (size_t i = 0; i < displayPoints.size() - 1; ++i) {
                    cv::line(displayFrame, displayPoints[i], displayPoints[i+1], cv::Scalar(255), 2);
                }
            }
            cv::putText(displayFrame, "Click points, right-click to close ROI", 
                       cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255), 2);
            
        } else {
            // 감지 모드
            // ROI 영역 표시
            if (g_points.size() > 1) {
                std::vector<cv::Point> displayPoints;
                for (const auto& pt : g_points) {
                    displayPoints.push_back(cv::Point(pt.x * 640 / CAPTURE_WIDTH, 
                                                    pt.y * 480 / CAPTURE_HEIGHT));
                }
                const cv::Point* pts[1] = { displayPoints.data() };
                int npts[] = { (int)displayPoints.size() };
                cv::polylines(displayFrame, pts, npts, 1, true, cv::Scalar(255), 2);
            }
            
            // 초기 프레임 설정
            if (g_prevFrame.empty()) {
                g_prevFrame = blurredFrame.clone();
                g_baselineFrame = blurredFrame.clone();
                std::cout << "초기화 완료 - 감지 시작" << std::endl;
            } else {
                // SAD 계산
                if (!g_baselineFrame.empty()) {
                    g_currentSAD = calculateFastSAD(blurredFrame, g_baselineFrame, g_roiMask);
                } else {
                    g_currentSAD = calculateFastSAD(blurredFrame, g_prevFrame, g_roiMask);
                }
                
                // 히스토리 업데이트
                g_sadHistory.push_back(g_currentSAD);
                if (g_sadHistory.size() > HISTORY_SIZE) {
                    g_sadHistory.pop_front();
                }
                
                // 병 감지 로직
                double threshold = g_sadThreshold.load();
                if (!g_isBottlePresent && g_currentSAD > threshold) {
                    g_isBottlePresent = true;
                    g_framesSinceDetection = 0;
                    pushBottle();
                } else if (g_isBottlePresent) {
                    g_framesSinceDetection++;
                    if (g_framesSinceDetection > DEBOUNCE_FRAMES && g_currentSAD < threshold * 0.6) {
                        g_isBottlePresent = false;
                        std::cout << "병 통과 완료" << std::endl;
                    }
                }
                
                // 정보 표시
                std::stringstream ss;
                ss << "FPS: " << std::fixed << std::setprecision(1) << g_fps 
                   << " | SAD: " << std::setprecision(0) << g_currentSAD 
                   << " / " << threshold;
                cv::putText(displayFrame, ss.str(), cv::Point(10, 30), 
                           cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255), 2);
                
                // '[' ']' 키 안내
                cv::putText(displayFrame, "[ ] : adjust threshold", cv::Point(10, 460), 
                           cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200), 1);
                
                // SAD 레벨 바
                int barWidth = static_cast<int>((g_currentSAD / threshold) * 200);
                barWidth = std::min(barWidth, 200);
                cv::rectangle(displayFrame, cv::Point(10, 45), 
                            cv::Point(10 + barWidth, 55), cv::Scalar(200), cv::FILLED);
                cv::rectangle(displayFrame, cv::Point(10, 45), 
                            cv::Point(210, 55), cv::Scalar(255), 1);
                
                if (g_isBottlePresent) {
                    cv::putText(displayFrame, "DETECTED!", cv::Point(10, 85), 
                               cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(255), 3);
                    cv::rectangle(displayFrame, cv::Point(5, 5), 
                                cv::Point(635, 475), cv::Scalar(255), 5);
                }
                
                g_prevFrame = blurredFrame.clone();
            }
        }
        
        // FPS 업데이트
        updateFPS();
        
        // 그레이스케일을 3채널로 변환하여 컬러 표시
        cv::Mat displayColor;
        cv::cvtColor(displayFrame, displayColor, cv::COLOR_GRAY2BGR);
        cv::imshow("Camera Feed", displayColor);
        
        char key = cv::waitKey(1) & 0xFF;
        if (key == 'q' || key == 27) {
            g_shouldExit = true;
            break;
        } else if (key == 'b' && g_roiSelected) {
            captureBaseline();
        } else if (key == '[') {
            g_sadThreshold = g_sadThreshold * 0.9;
            std::cout << "임계값 감소: " << g_sadThreshold << std::endl;
        } else if (key == ']') {
            g_sadThreshold = g_sadThreshold * 1.1;
            std::cout << "임계값 증가: " << g_sadThreshold << std::endl;
        }
    }
    
    // 정리
    g_shouldExit = true;
    
    // 카메라 해제
    if (g_cap) {
        g_cap->release();
        delete g_cap;
    }
    
    // 스레드 종료 대기
    if (inputThread.joinable()) {
        std::cout << "\n입력 스레드 종료 중..." << std::endl;
        inputThread.join();
    }
    
    cv::destroyAllWindows();
    
    std::cout << "\n프로그램이 안전하게 종료되었습니다." << std::endl;
    return 0;
}