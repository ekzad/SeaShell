section .text
global get_cpu_vendor

; void get_cpu_vendor(char *out);
; Windows x64 calling convention: first argument (the output buffer) arrives in rcx

get_cpu_vendor:
    push rbx            ; rbx must be preserved (callee-saved) and cpuid clobbers it

    mov r8, rcx          ; stash the output pointer in r8 -- cpuid never touches r8

    mov eax, 0           ; cpuid leaf 0 requests the vendor string
    cpuid                ; after this: ebx/edx/ecx together hold the 12-character vendor string

    mov [r8+0], ebx       ; first 4 characters
    mov [r8+4], edx       ; next 4 characters
    mov [r8+8], ecx       ; last 4 characters
    mov byte [r8+12], 0    ; null terminator

    pop rbx
    ret