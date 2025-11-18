;;
;; Copyright (c) 2025 Ian Marco Moffett and the Osmora Team.
;; All rights reserved.
;;
;; Redistribution and use in source and binary forms, with or without
;; modification, are permitted provided that the following conditions are met:
;;
;; 1. Redistributions of source code must retain the above copyright notice,
;;    this list of conditions and the following disclaimer.
;; 2. Redistributions in binary form must reproduce the above copyright
;;    notice, this list of conditions and the following disclaimer in the
;;    documentation and/or other materials provided with the distribution.
;; 3. Neither the name of Hyra nor the names of its
;;    contributors may be used to endorse or promote products derived from
;;    this software without specific prior written permission.
;;
;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;; ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
;; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
;; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
;; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
;; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
;; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
;; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
;; POSSIBILITY OF SUCH DAMAGE.
;;

[bits 16]
[org 0x8000]

%define AP_BUDA     0x9000
%define IA32_EFER   0xC0000080

[bits 16]
_start:
    cli
    mov al, 0xFF                    ;; Mask i8259 inputs
    out 0x21, al                    ;; Disable master PIC
    out 0xA1, al                    ;; Disable slave PIC

    mov eax, cr4                    ;; CR4 -> EAX
    or eax, 0xA0                    ;; Enable physical address extension + PGE
    mov cr4, eax                    ;; Write it back

    mov eax, dword [AP_BUDA]        ;; BUDA.CR3 -> EAX
    mov cr3, eax                    ;; EAX -> CR3

    mov ecx, IA32_EFER              ;; Read IA32_EFER
    rdmsr                           ;; -> EAX
    or eax, 0xD00                   ;; Set EFER.LME + defaults
    wrmsr                           ;; Write it back

    mov eax, cr0                    ;; CR0 -> EAX
    or eax, 0x80000001              ;; CR0 |= PG/PM
    mov cr0, eax                    ;; Write it back

    lgdt [GDTR]
    jmp 0x08:thunk64

GDT:
.NULL:
    dq 0x0000000000000000       ;; NULL DESCRIPTOR
.CODE:                          ;; ---------------
    dq 0x00209A0000000000       ;; CODE DESCRIPTOR
.DATA:                          ;; ---------------
    dq 0x0000920000000000       ;; DATA DESCRIPTOR
GDTR:                           ;; ---------------
    dw $ - GDT - 1              ;; LIMIT
    dq GDT                      ;; OFFSET

[bits 64]
thunk64:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    xor ax, ax
    mov gs, ax
    mov rsp, qword [AP_BUDA + 0x08]
    mov rbx, qword [AP_BUDA + 0x10]
    mov rax, 1
    xchg qword [AP_BUDA + 0x18], rax
    cld
    jmp rbx

times 4096 - ($ - $$) db 0
