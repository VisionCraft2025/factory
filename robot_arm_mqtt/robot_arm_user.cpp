#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sstream>

using namespace std;

class RobotArmController
{
private:
    string device = "/dev/robot_arm"; // 커널 모듈이 생성한ㄴ 디바이스 파일 경로
    ofstream arm_out;                 // 디바이스 파일에 명령어를 쓰기 위해
    ifstream arm_in;                  // 디바이스 파일에서 상태를 읽기 위해

public:
    // 디바이스 파일 열고 통신 준비 -> 초기화 작업
    bool initialize()
    {
        // 명령어를 디바이스 파일에 전송해야 하므로 쓰기 모드로 열기
        arm_out.open(device);
        if (!arm_out.is_open())
        {
            cout << "디바이스 파일을 열 수 없습니다: " << device << endl;
            return false;
        }

        // 디바이스 파일로부터 로봇팔의 상태값을 읽어야 함 -> 읽기 모드로 열기
        arm_in.open(device);
        if (!arm_in.is_open())
        {
            cout << "디바이스 파일을 열 수 없습니다:" << device << endl;
            return false;
        }

        return true;
    }

    // 커널 모듈에 명령어를 전송
    // -> 이 함수가 호출되면 커널 모듈의 robot_arm_write() 함수가 실행됨
    void sendCommand(const string &cmd)
    {
        // 명령어를 디바이스 파일에 쓰기
        arm_out << cmd << endl;
        arm_out.flush();
        cout << "[전송완료] " << cmd << endl;
    }

    // 로봇 팔의 현재 상태를 확인
    // -> 커널 모듈의 robot_arm_read() 함수에서 상태 정보를 읽어옴
    void showStatus()
    {
        string line;

        cout << endl;
        cout << "***** Robot Arm Status *****" << endl;

        // 디바이스 파일에서 상태 정보를 한 줄씩 읽어오고, 바로바로 출력하기
        while (getline(arm_in, line))
            cout << line << endl;

        // 이거 안 하면 다음에 못 읽음...
        // -> 디바이스 파일은 계속해서 읽어야 하므로 EOF 플래그를 클리어해야 됨
        arm_in.clear();
    }

    // 매뉴얼
    void showHelp()
    {
        cout << endl;
        cout << "***** Robot Arm Control Commands *****" << endl;
        cout << "auto_on     - 자동 모드 시작 (미리 정의된 동작 패턴 실행)" << endl;
        cout << "auto_off    - 자동 모드 중지" << endl;
        cout << "init        - 모든 서보를 중앙 위치로 이동하여 초기화" << endl;
        cout << "servo[0-3] [angle] - 개별 서보모터 제어 (예: servo0 45)" << endl;
        cout << "  servo0: 하단 모터 (0-180도) - 로봇 팔을 좌우로 회전" << endl;
        cout << "  servo1: 중단 모터 (0-180도) - 로봇 팔을 위아래로 움직임" << endl;
        cout << "  servo2: 상단 모터 (0-180도) - 상단 관절 위아래로 움직임" << endl;
        cout << "  servo3: 그리퍼 (0-180도) - 바닥을 좌우로 움직여 물건 싣거나 내리기" << endl;
        cout << "status      - 현재 상태 확인 (각 서보모터 각도, 자동모드 여부 등)" << endl;
        cout << "help        - 이 도움말 표시" << endl;
        cout << "exit        - 프로그램 종료" << endl;
        cout << "********************************" << endl;
    }

    // 프로그램을 초기화하고, 실행시키는 함수
    void run()
    {
        cout << "***** Robot Arm Controller *****" << endl;
        cout << "초기화 중..." << endl;

        // 프로그램 시작 시 로봇 팔을 초기 위치로 이동
        sendCommand("init");
        sleep(2);

        // 매뉴얼 보여주기
        showHelp();

        string input;
        while (true)
        {
            cout << "\n> ";
            getline(cin, input);

            // 그냥 엔터치면 무시하기
            if (input.empty())
                continue;

            if (input == "exit")
            {
                cout << "프로그램을 종료합니다..." << endl;
                sendCommand("auto_off"); // 종료하기 전에 자동모드 끄기
                break;
            }
            else if (input == "help")
            {
                showHelp();
            }
            else if (input == "status")
            {
                showStatus();
            }
            else if (input == "init")
            {
                cout << "로봇 팔 위치를 초기화합니다..." << endl;
                sendCommand(input);
            }
            else if (input == "auto_on")
            {
                cout << "자동 모드를 실행합니다..." << endl;
                sendCommand(input);
            }
            else if (input == "auto_off")
            {
                cout << "자동 모드를 중지합니다..." << endl;
                sendCommand(input);
            }
            else if (input.substr(0, 5) == "servo")
            {
                // "servo0 60"에서 공백 부분 찾기
                size_t space_pos = input.find(' ');
                if (space_pos < input.length())
                {
                    string servo_cmd = input.substr(0, space_pos);  // 공백 앞 부분 -> ex) "servo0"
                    string angle_str = input.substr(space_pos + 1); // 공백 뒷 부분 -> ex) "60"

                    try
                    {
                        int angle = stoi(angle_str);
                        if (angle >= 0 && angle <= 250)
                        {
                            sendCommand(input);
                            cout << "서보모터 수동 제어: " << input << endl;
                        }
                        else
                        {
                            cout << "[오류] 각도 범위 초과 - 허용 범위: 0~180" << endl;
                            cout << "입력된 각도: " << angle << endl;
                        }
                    }
                    catch (const std::exception &e)
                    {
                        cout << "[오류] 형식 오류 - 각도 값은 숫자여야 함" << endl;
                        cout << "입력된 값: " << angle_str << endl;
                    }
                }
                else
                {
                    cout << "[오류] 명령 형식이 올바르지 않음" << endl;
                    cout << "형식: servo<번호> <각도>  (예: servo0 90)" << endl;
                }
            }
            else
                cout << "[오류] 유효하지 않은 명령 - 'help'로 사용 가능한 명령 확인" << endl;
        }
    }

    // 나머지 리소스 정리하기
    ~RobotArmController()
    {
        if (arm_out.is_open())
            arm_out.close();
        if (arm_in.is_open())
            arm_in.close();

        cout << "프로그램 종료..." << endl;
    }
};

int main()
{
    RobotArmController controller;

    if (!controller.initialize())
    {
        cout << "[오류] 초기화 실패" << endl;
        return 1;
    }

    // 초기화 성공하면 프로그램 진짜 실행
    controller.run();

    return 0;
}