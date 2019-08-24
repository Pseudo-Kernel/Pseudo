
EXTERN PiPreInitialize : PROC

PUBLIC PiPreStackSwitch

_TEXT SEGMENT

; ...
; [Return Address] <- after call
; ==================================
; [Pushed Params / Shadow Area] <- RSP
; [Align Padding]
; [Local Variables]
; [...]
; [Return Address]
; ==================================
; [Pushed Params / Shadow Area]
; ...

; 
; func(P1, P2, P3, P4, P5, P6, P7);
; 
; sub rsp, 38h + padding
; mov rcx, P1
; mov rdx, P2
; mov r8, P3
; mov r9, P4
; mov qword ptr [rsp+20h], P5
; mov qword ptr [rsp+28h], P6
; mov qword ptr [rsp+30h], P7
; ==> at this point, stack must be aligned with 10h-byte boundary!!!
; call func
; add rsp, 38h + padding
; 

; VOID
; KERNELAPI
; PiPreStackSwitch(
;     IN OS_LOADER_BLOCK *LoaderBlock,
;     IN U32 SizeOfLoaderBlock, 
;     IN PVOID SafeStackTop);

PiPreStackSwitch PROC
	and r8, not 7h					; align to 16-byte boundary

	sub r8, 20h						; shadow area
	mov qword ptr [r8], rcx			; copy of 1st param
	mov qword ptr [r8+8h], rdx		; copy of 2nd param

	xchg r8, rsp					; switch the stack
	call PiPreInitialize

	; MUST NOT REACH HERE!

_InitFailed:
	cli
	hlt
	jmp _InitFailed

	ret
PiPreStackSwitch ENDP

_TEXT ENDS

END
