#include <iostream>
#include <windows.h>
#include <thread>

using namespace std;

void escrever(volatile int& a){
    while(true){
        cout << "PID: " << GetCurrentProcessId()<< ", variavel: " << a <<
            ", memoria: 0x" << hex << reinterpret_cast<uintptr_t>(&a) << dec << endl;
        Sleep(5000);
    }
}

int main(){
    volatile int a = 100;
    int entrada;
    thread worker(escrever, ref(a));
    while(true){
        cout << "digite a: ";
        cin >> entrada;
        a = entrada;
    }
    return 0;
}