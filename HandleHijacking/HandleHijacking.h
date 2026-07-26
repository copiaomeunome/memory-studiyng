#pragma once //evita erros de redefnição e include duplicado
#include <Windows.h>
#include <cstdint>
#include <ntstatus.h>
#include <tlhelp32.h>
#include <optional>
#include "Aether.h"
using namespace std;
//defino aqui e limpo no fim do arquivo pra não sobrescrever nada fora daqui
#define NT_FAILURE(ntStatus) ntStatus !=0x00000000
#define VALID_HANDLE(handle) handle!=nullptr && (handle)!=INVALID_HANDLE_VALUE
#define INVALID_HANDLE(Handle) Handle == nullptr || (Handle) == INVALID_HANDLE_VALUE

namespace HijackHandle{
    typedef struct _CLIENT_ID{
		HANDLE uniqueProcess; //guarda um PID pro processo
		HANDLE uniqueThread; //guarda um identificador da thread
	} CLIENT_ID, *PCLIENT_ID;

	typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX{ //usado para guardar cada um dos handles
		PVOID object; //endereço do handle no kernel
		HANDLE uniqueProcessId; //id do processo que tem esswe handle
		HANDLE handleValue; //valor do handle dentro do processo que contém ele
		ACCESS_MASK grantedAccess; //permissões do handle
		USHORT creatorBackTraceIndex; //rastreamento de criação do handle, não uso nesse codigo
		USHORT objectTypeIndex; //diz pra que esse handle está apontando (processo, thread, ...)
		ULONG handleAttributes; //atributos não usados do handle
		ULONG reserved; //serve pra nada por enquanto, carai viu
	} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX; // define o nome do struct e o nome do ponteiro dele

    typedef struct _SYSTEM_HANDLE_INFORMATION_EX { //contém todos os handles do buffer
        ULONG_PTR numberOfHandles; //quantidade de handles
        ULONG_PTR reserved; //campo reservado do windows, serve pra lixo nenhum
		_Field_size_(numberOfHandles) SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handles[1]; //cria um vetor de handles e diz que ele tem exatamente o tamanho de numberOfHandles
    } SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;

    typedef struct _OBJECT_ATTRIBUTES{
		ULONG  Length;
		HANDLE RootDirectory;
		PVOID  ObjectName;
		ULONG  Attributes;
		PVOID  SecurityDescriptor;
		PVOID  SecurityQualityOfService;
	} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

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
            //pede ao Windows uma lista estendida dos handles existentes no sistema e grava o resultado em buffer
            //(64 é o tipo de informação que queremos - SystemExtendedHandleInformation, buffer é onde gravaremos os handles, size é o tamanho que qeuremos, &size é o tamanho gravado)
            status = Aether::Syscall<NTSTATUS>( "NtQuerySystemInformation", 64, buffer, size, &size );
            if(status == STATUS_INFO_LENGTH_MISMATCH){
                //se for pequeno demais ele aumenta o tamanho do buffer
                VirtualFree( buffer, 0, MEM_RELEASE );
				size *= 2;
            }
        }while (status == STATUS_INFO_LENGTH_MISMATCH);
        if(NT_FAILURE(status)){
            error = status;
            VirtualFree( buffer, 0, MEM_RELEASE );
            return nullopt;
        }
        //a partir daqui já tenho a lista de handles no buffer
        const auto handleInfo = static_cast<_SYSTEM_HANDLE_INFORMATION_EX*>(buffer);
        HANDLE handleDuplicado = nullptr;
        HANDLE handleAtual = nullptr;
        CLIENT_ID clID = {}; //declarando vazio
        OBJECT_ATTRIBUTES attributes = {sizeof(OBJECT_ATTRIBUTES),nullptr,nullptr,0,nullptr,nullptr};

        for(ULONG_PTR i=0; i<handleInfo->numberOfHandles;i++){
            SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX& info = handleInfo->handles[i];
            const HANDLE handle = info.handleValue;
            if(INVALID_HANDLE(handle)) continue;
            if(reinterpret_cast<ULONG_PTR>(info.uniqueProcessId) == PID) continue; //caso seja o processo alvo
            if(VALID_HANDLE(handleAtual)){
                CloseHandle(handleAtual);
                handleAtual = nullptr;
            }
            clID.uniqueProcess = info.uniqueProcessId;
            clID.uniqueThread  = nullptr;
            //duplica o handle
            status = Aether::Syscall<NTSTATUS>( "NtOpenProcess", &handleAtual, PROCESS_DUP_HANDLE, &attributes, &clID);
            if(NT_FAILURE(status)) continue;
            //( "NtDuplicateObject", SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options )
            status = Aether::Syscall<NTSTATUS>("NtDuplicateObject", handleAtual, info.handleValue, reinterpret_cast<HANDLE>( -1 ), &handleDuplicado, PROCESS_ALL_ACCESS, 0, 0 );
            if(INVALID_HANDLE(handleDuplicado) || NT_FAILURE(status)) continue;
            if(GetProcessId(handleDuplicado) != PID){
                CloseHandle(handleDuplicado);
                continue;
            }
            return handleDuplicado;
        }
        error = STATUS_INVALID_HANDLE;
        return nullopt;
    }
}
#undef NT_FAILURE
#undef VALID_HANDLE
#undef INVALID_HANDLE