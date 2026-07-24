#pragma once //evita erros de redefnição e include duplicado
#include <Windows.h>
#include <cstdint>
#include <ntstatus.h>
#include <tlhelp32.h>
#include <optional>
#include "../Aether.h"
using namespace std;
namespace HijackHandle{
    // inline também é pra evitar conflito de nomes, parecido com o pragma once
    inline NTSTATUS error = STATUS_SUCCESS; // variável de erro
    inline optional<HANDLE> HijackHandle(const uint32_t PID){
        ULONG size=0x10000; //64KB para salvar os HANDLE's do sistema
        NTSTATUS status;
        PVOID buffer;
        BOOLEAN oPrivilege = FALSE;
        // NTSTATUS é só um formato de status do windows
        //(RtlAdjustPrivilege é a syscall pra ajustar o privilegio do meu processo, 0x14 é o privilégio que eu quero, enable é se quero habilitar, client indica qual token vai ser alterado - false para token do processo atual e true para a thread atual, wasEnabled)
        (void)Aether::Syscall<NTSTATUS>("RtlAdjustPrivilege", 0x14, TRUE, FALSE, &oPrivilege); // (void) - ignorando retorno de propósito

        do{
            //aloca uma região de memória dentro do processo atual, usarei de buffer pra receber os handles
            //(nullptr é pro windows escolher automaticamente o endereço alocado, size é tamanho alocado, MEM_RESERVE|MEM_COMMIT é o status que vai ficar a memória - vai ficar como utilizável, PAGE_READWRITE é a proteção da memória, essa região pode ser lida ou escrita)
            buffer = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if(!buffer){
                error = STATUS_INSUFFICIENT_RESOURCES;
                return nullopt;
            }
            status = 
        }while (status == STATUS_INFO_LENGTH_MISMATCH);
    }
}