#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>
#include <cstring>

using namespace std;

struct RegiaoMemoria
{
    uintptr_t inicio;
    SIZE_T tamanho;
};

vector<RegiaoMemoria> lerPaginacoes(HANDLE processo){
    vector<RegiaoMemoria> regioes;
    SYSTEM_INFO info{}; //criando um SYSTEM_INFO vazio
    GetSystemInfo(&info); //pega as informações do sistema e coloca na variável info
    uintptr_t enderecoAtual = reinterpret_cast<uintptr_t>(info.lpMinimumApplicationAddress); // uintptr_t é um tipo que é grande o bastante pra guardar um endereço de memória
    uintptr_t enderecoFinal = reinterpret_cast<uintptr_t>(info.lpMaximumApplicationAddress);
    while(enderecoAtual < enderecoFinal){
        MEMORY_BASIC_INFORMATION mbi{}; // tipo que guarda informações de memória virtual como BaseAddress, RegionSize, State, Protect e Type, está sendo inicializado como vazio
        SIZE_T resultado = VirtualQueryEx( //SIZE_T é pra guardar tamanho do que foi escrtito
            processo, //HANDLE do processo
            reinterpret_cast<LPCVOID>(enderecoAtual), //LPCVOID é o const void*, um ponteiro genérico constante
            &mbi, // endereço da memória que vai ser escrita
            sizeof(mbi) //tamanho da memória
        );
        if(resultado == 0) break; //caso não leia nada

        uintptr_t inicio = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && !(mbi.Protect & PAGE_NOACCESS) &&!(mbi.Protect & PAGE_GUARD)) regioes.push_back({inicio, mbi.RegionSize});
        enderecoAtual = inicio + mbi.RegionSize;
    }
    return regioes;
}

vector<uintptr_t> pegarEnderecos(vector<RegiaoMemoria> regioes, HANDLE processo){
    vector<uintptr_t> enderecosEncontrados;
    for(RegiaoMemoria& regiao : regioes) {
        vector<unsigned char> buffer(regiao.tamanho);
        SIZE_T bytesLidos = 0;
        BOOL sucesso = ReadProcessMemory( //retorna positivo caso leia com sucesso
            processo, //HANDLE do processo
            reinterpret_cast<LPCVOID>(regiao.inicio), //ponteiro pro início da região
            buffer.data(), //ponteiro pra primeira posição do vetor, onde os dados vão ser copiados
            regiao.tamanho, //tamanho da região
            &bytesLidos //quantidade de bytes lidos
        );
        if(!sucesso) continue;
        for(SIZE_T offset = 0; offset+sizeof(int) <= bytesLidos; offset++){
            uintptr_t endereco = regiao.inicio + offset;
            enderecosEncontrados.push_back(endereco);
        }
    }
    return enderecosEncontrados;
}

int main(){
    //abre o processo
    DWORD pid;
    cout << "digite o PID: ";
    cin >> pid;
    HANDLE processo = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
    if(processo == nullptr){cout << "Não foi possível abrir o processo" << endl; return 1;}
    //pega os endereços da memória virtual
    
    //lendo paginações
    vector<RegiaoMemoria> regioes = lerPaginacoes(processo);
    cout << "Regioes encontradas: " << regioes.size() << endl;

    //pegando os endereços
    vector<uintptr_t> enderecosEncontrados = pegarEnderecos(regioes, processo);

    //procurando endereço desejado
    int opcao = 1;
    while(opcao!=0){
        cout << "digite o que quer fazer: " << endl 
        << "(0) sair" << endl
        << "(1) escrever em um endereço" << endl 
        << "(2) listar posições de variável" << endl 
        << "(3) resetar a busca" << endl
        << "(4) monitorar os registradores" << endl;
        cin >> opcao;
        if(opcao == 2){
            int numeroProcurado;
            cout << "Digite o numero inteiro: ";
            cin >> numeroProcurado;
            vector<uintptr_t> novosEnderecos;
            for(uintptr_t& endereco : enderecosEncontrados) {
                int buffer = 0;
                SIZE_T bytesLidos = 0;
                BOOL sucesso = ReadProcessMemory( //retorna positivo caso leia com sucesso
                    processo, //HANDLE do processo
                    reinterpret_cast<LPCVOID>(endereco), //ponteiro pro início da região
                    &buffer, //ponteiro pra primeira posição do vetor, onde os dados vão ser copiados
                    sizeof(buffer), //tamanho da região
                    &bytesLidos //quantidade de bytes lidos
                );
                if(!sucesso) continue;
                if(buffer == numeroProcurado) novosEnderecos.push_back(endereco);
            }
            enderecosEncontrados = novosEnderecos;
            cout << "Enderecos encontrados: " << enderecosEncontrados.size()<< endl;
            for (uintptr_t endereco : enderecosEncontrados) cout << "0x"<< hex << endereco << dec << endl;
            if (enderecosEncontrados.empty())cout << "Nenhum endereco restante." << endl;
        }
        else if(opcao == 1){
            string entradaEndereco;
            cout << "Digite o endereco no formato hexadecimal 0xXXXXXXXX: ";
            cin >> entradaEndereco;
            uintptr_t endereco;
            try {endereco = static_cast<uintptr_t>(stoull(entradaEndereco, nullptr, 0));}
            catch (...){
                cout << "Endereco invalido." << endl;
                continue;
            }
            int novoValor;
            cout << "Digite o novo valor: ";
            cin >> novoValor;
            SIZE_T bytesEscritos = 0;
            BOOL sucesso = WriteProcessMemory(
                processo,
                reinterpret_cast<LPVOID>(endereco),
                &novoValor,
                sizeof(novoValor),
                &bytesEscritos
            );
            if(sucesso) cout << "deu certo, bytes escritos: " << bytesEscritos << endl;
            else cout << "não deu certo";
        }
        else if(opcao == 3){
            regioes = lerPaginacoes(processo);
            cout << "Regioes encontradas: " << regioes.size() << endl;
            //pegando os endereços
            vector<uintptr_t> enderecosEncontrados = pegarEnderecos(regioes, processo);
        }
        else if(opcao == 4){

        }
    }
    CloseHandle(processo);
    return 0;
}