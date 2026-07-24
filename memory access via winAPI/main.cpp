#include <iostream>
#include <windows.h>
using namespace std;
int main(){
    int a;
    while(true){
        cout << "digite a: ";
        cin >> a;
        cout << "PID: " << GetCurrentProcessId()<< ", variavel: " << a <<
        ", memoria: 0x" << hex << reinterpret_cast<uintptr_t>(&a) << dec << endl;
        Sleep(5000);
    }
    return 0;
}