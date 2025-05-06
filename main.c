#include <stdint.h>

#define MTIME_FREQ 10000000UL          /* QEMU virt: mtime = 10 MHz */
#define UART_BASE  0x10000000UL        /* 8250‑compatible UART */

static inline uint64_t rdcycle(void)
{
    uint64_t v;
    asm volatile ("rdcycle %0" : "=r"(v));
    return v;
}

static inline uint64_t rdtime(void)
{
    uint64_t v;
    asm volatile ("rdtime %0" : "=r"(v));
    return v;
}

/* ------- UART -------- */
static inline void uart_putc(char c)
{
    *(volatile uint8_t *)UART_BASE = (uint8_t)c;
}
static void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}
static void uart_put_dec(uint64_t v)
{
    char buf[24];
    int i = 0;
    if (!v) { uart_putc('0'); return; }
    while (v) { buf[i++] = '0' + v % 10; v /= 10; }
    while (i--) uart_putc(buf[i]);
}

__attribute__((always_inline))
static inline void mhz_body(void)
{
    register uint64_t a __asm__("a0") = 1;
    register uint64_t b __asm__("a1") = 0x9e3779b97f4a7c15ULL;
    asm volatile(
        "add %[a], %[a], %[b]\n\t"
        "slli %[a], %[a], 13\n\t"
        "add %[a], %[a], %[b]\n\t"
        "srli %[a], %[a], 7\n\t"
        "add %[a], %[a], %[b]\n\t"
        "slli %[a], %[a], 17\n\t"
        "add %[a], %[a], %[b]\n\t"
        "srli %[a], %[a], 11\n\t"
        "add %[a], %[a], %[b]\n\t"
        "slli %[a], %[a], 5\n\t"
        "add %[a], %[a], %[b]\n\t"
        "srli %[a], %[a], 3\n\t"
        "add %[a], %[a], %[b]\n\t"
        "slli %[a], %[a], 6\n\t"
        "add %[a], %[a], %[b]\n\t"
        "srli %[a], %[a], 2\n\t"
        : [a] "+r"(a)
        : [b] "r"(b));
}

static void run_loops(uint32_t n)
{
    while (n--) mhz_body();
}

int main(void)
{
    uint32_t loops = 1;

    for (;;)
    {
        uint64_t t0 = rdtime();
        uint64_t c0 = rdcycle();

        run_loops(loops);

        uint64_t c1 = rdcycle();
        uint64_t t1 = rdtime();

        uint64_t dt = t1 - t0;          /* mtime ticks  */
        uint64_t dc = c1 - c0;          /* cycle ticks  */

        if (dt < 100) { loops <<= 1; continue; }

        uint64_t freq_hz = (dc * MTIME_FREQ) / dt;     /* f = dc / dt * Fmtime */
        uint64_t mhz_int  = freq_hz / 1000000ULL;
        uint64_t mhz_frac = (freq_hz % 1000000ULL) / 10000ULL;   /* 两位小数 */

        uart_puts("CPU frequency: ");
        uart_put_dec(mhz_int);
        uart_putc('.');
        if (mhz_frac < 10) uart_putc('0');
        uart_put_dec(mhz_frac);
        uart_puts(" MHz\n");

        uint64_t period_ps = 1000000000000ULL / freq_hz;
        uart_puts("Clock period : ");
        uart_put_dec(period_ps);
        uart_puts(" ps\n");

        for (;;) asm volatile("wfi");
    }
}

