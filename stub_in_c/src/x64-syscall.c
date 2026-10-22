unsigned long syscall0(long n)
{
	unsigned long ret;
	asm volatile(
		"syscall"
		: "=a"(ret) // rax
		: "a"(n) // rax
		: "rcx", "r11", "memory"
	);
	return ret;
}

unsigned long syscall1(long n, long a1)
{
	unsigned long ret;
	asm volatile(
		"syscall"
		: "=a"(ret) // rax
		: "a"(n) // rax
		, "D"(a1) // rdi
		: "rcx", "r11", "memory"
	);
	return ret;
}

unsigned long syscall2(long n, long a1, long a2)
{
	unsigned long ret;
	asm volatile(
		"syscall"
		: "=a"(ret) // rax
		: "a"(n) // rax
		, "D"(a1) // rdi
		, "S"(a2) // rsi
		: "rcx", "r11", "memory"
	);
	return ret;
}

unsigned long syscall3(long n, long a1, long a2, long a3)
{
	unsigned long ret;
	asm volatile(
		"syscall"
		: "=a"(ret) // rax
		: "a"(n) // rax
		, "D"(a1) // rdi
		, "S"(a2) // rsi
		, "d"(a3) // rdx
		: "rcx", "r11", "memory"
	);
	return ret;
}

unsigned long syscall4(long n, long a1, long a2, long a3, long a4)
{
	unsigned long ret;
	register long r10 __asm__("r10") = a4;
	asm volatile(
		"syscall"
		: "=a"(ret) // rax
		: "a"(n) // rax
		, "D"(a1) // rdi
		, "S"(a2) // rsi
		, "d"(a3) // rdx
		, "r"(r10)
		: "rcx", "r11", "memory"
	);
	return ret;
}

unsigned long syscall5(long n, long a1, long a2, long a3, long a4, long a5)
{
	unsigned long ret;
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	asm volatile(
		"syscall"
		: "=a"(ret) // rax
		: "a"(n) // rax
		, "D"(a1) // rdi
		, "S"(a2) // rsi
		, "d"(a3) // rdx
		, "r"(r10)
		, "r"(r8)
		: "rcx", "r11", "memory"
	);
	return ret;
}

unsigned long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6)
{
	unsigned long ret;
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	register long r9 __asm__("r9") = a6;
	asm volatile(
		"syscall"
		: "=a"(ret) // rax
		: "a"(n) // rax
		, "D"(a1) // rdi
		, "S"(a2) // rsi
		, "d"(a3) // rdx
		, "r"(r10)
		, "r"(r8)
		, "r"(r9)
		: "rcx", "r11", "memory"
	);
	return ret;
}

