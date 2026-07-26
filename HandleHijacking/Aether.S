.intel_syntax noprefix

.text

.globl SyscallStub
.def SyscallStub
    .scl 2
    .type 32
.endef

SyscallStub:
    mov r10, rcx
    pop rcx
    pop rax
    mov qword ptr [rsp], rcx
    mov eax, dword ptr [rsp + 24]
    syscall
    sub rsp, 8
    jmp qword ptr [rsp + 8]