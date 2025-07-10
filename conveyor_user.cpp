#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace std;

class conveyorController{
    private:
        string device = "/dev/conveyor_mqtt";
        ofstream conveyor_out;
        ifstream conveyor_in;

    public:
        bool initialize(){
            conveyor_out.open(device);
            conveyor_in.open(device);
            if(!conveyor_out.is_open() || !conveyor_in.is_open()){
                cout << "디바이스 파일 생성 안됨" << device << endl;
                return false;
            }
            return true;
        }

        void sendCommand(const string &cmd){
            conveyor_out << cmd << endl;
            conveyor_out.flush();
            cout << "완료 " << cmd << endl;
        }

        void readStatus(){
            ifstream status_reader(device);
            if(!status_reader.is_open()){
                cout << "상태 읽기 실패 " << endl;
                return;
            }

            string line;
            while(getline(status_reader, line)){
                cout << line << endl;
            }
            status_reader.close();
        }

        void run(){
    string input;
    while(true){
        cout << "\n";
        getline(cin, input);

        if(input.empty()){
            continue;
        }

        if(input == "exit"){
            sendCommand("off");
            break;
        }
        if(input == "off"){
            sendCommand("off");
        }
        if(input == "on"){
            sendCommand("on");
        }
        else if(input == "error_mode"){                
            sendCommand("error_mode");
            cout << "에러 모드 " << endl;
        }
        else if(input == "status"){
             readStatus();
        }
        else if(input == "help"){
            cout << "명령어: on, off, servo_center, error_mode, status" << endl;
        }
        else{
            cout << "다시 입력하세요 " << input << endl;
        }
    }
}

      
};

int main(){
    conveyorController *controller = new conveyorController();
    if(!controller->initialize()){
        cout << "초기화 실패" << endl;
        return 1;
    }
    controller->run();
    delete controller;
}