    .section .text.init
    .globl _start
_start:
    la   sp, _stack_end
    call main
1:  wfi
    j    1b

