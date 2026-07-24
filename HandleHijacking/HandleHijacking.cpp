#include <windows.h>
#include <iostream>
#include <cstdint>
#include "HandleHijacking.h"
using namespace std;
int main(){
    //obtém o HANDLE
    uint32_t pid;
    cout << "Digite o PID do processo alvo: ";
    cin >> pid;
    optional<HANDLE> resultado = HijackHandle::HijackHandle(pid);
    if(!resultado.has_value()){
        cout << "Não foi possível obter o handle, NTSTATUS: 0x" << hex << static_cast<unsigned long>(HijackHandle::error) << dec << endl;
        return 1;
    }
    HANDLE processo = resultado.value();
    cout << "HANDLE obtido com sucesso, valor local do handle: 0x" << hex << reinterpret_cast<std::uintptr_t>(processo) << dec << endl;
    //verifica se o processo está rodando
    DWORD codigoSaida = 0;
    if(GetExitCodeProcess(processo, &codigoSaida)){
        if(codigoSaida == STILL_ACTIVE) cout << "O processo alvo esta em execucao" << endl;
        else cout << "O processo terminou com o código " << codigoSaida << endl;
    }
    else cout << "não foi possível obter o status do processo, erro: " << GetLastError() << endl;

    //fim do processo
    CloseHandle(processo);
}