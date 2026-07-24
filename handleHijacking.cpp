#include <Windows.h>
#include <iostream>
#include <cstdint>

#include "hanhij.h"

int main()
{
    uint32_t pid;

    std::cout << "Digite o PID do processo alvo: ";
    std::cin >> pid;

    std::optional<HANDLE> resultado = Wraith::HijackHandle(pid);

    if (!resultado.has_value())
    {
        std::cout << "Nao foi possivel obter o handle.\n";
        std::cout << "NTSTATUS: 0x"
                  << std::hex
                  << static_cast<unsigned long>(Wraith::Error)
                  << std::dec
                  << '\n';

        return 1;
    }

    HANDLE processo = resultado.value();

    std::cout << "Handle obtido com sucesso.\n";
    std::cout << "Valor local do handle: 0x"
              << std::hex
              << reinterpret_cast<std::uintptr_t>(processo)
              << std::dec
              << '\n';

    DWORD pidConfirmado = GetProcessId(processo);

    std::cout << "PID associado ao handle: "
              << pidConfirmado
              << '\n';

    DWORD codigoSaida = 0;

    if (GetExitCodeProcess(processo, &codigoSaida))
    {
        if (codigoSaida == STILL_ACTIVE)
        {
            std::cout << "O processo alvo esta em execucao.\n";
        }
        else
        {
            std::cout << "O processo terminou com codigo: "
                      << codigoSaida
                      << '\n';
        }
    }
    else
    {
        std::cout << "Nao foi possivel consultar o estado do processo.\n";
        std::cout << "GetLastError: " << GetLastError() << '\n';
    }

    CloseHandle(processo);

    return 0;
}