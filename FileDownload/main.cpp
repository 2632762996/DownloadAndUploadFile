#include <iostream>
#include<string.h>
#include<fstream>
#include<sys/stat.h>
#include<winsock2.h>
#include "IOSocket.h"
#include "task.h"
using namespace std;




void InitSocket() {
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        cout << "WSAStartup failed" << endl;
    }
}

void UnInitSocket()
{
    WSACleanup();
}


void menu() {
    cout << "----------MENU-----------" << endl;
    cout << "1-Download File" << endl;
    cout << "2-Upload File" << endl;
    cout << "3-Quit" << endl;
    cout << "Please input command " << endl;
}


void run() {
    filetask task;
    while (true) {
        menu();
        string command ;
        cin >> command;
        if (command == "1") {
            cout << "Please input file MD5" << endl;
            string filemd5;
            cin >> filemd5;
            if (filemd5.length() == 32) {
                task.Setfilename(filemd5.c_str());
                task.DownloadFile();
            }
            else {
                cout << "Input file MD5 error" << endl;
            }

        }
        else if (command == "2") {
            cout << "Please input upload file path" << endl;
            string filepath;
            cin >> filepath;
            task.Setfilename(filepath.c_str());
            task.UploadFile();
        }
        else if (command == "3") {
            cout << "Thank you for you using, Bye Bye." << endl;
            break;
        }
        else
        {
            cout << "Input command error, please input again" << endl;
        }

    }
}

int main()
{
    InitSocket();
    run(); 
    UnInitSocket(); 
    return 0;
}

