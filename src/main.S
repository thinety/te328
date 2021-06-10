.global main

foo:
    inc     r17
    cpi     r17, 0x04
    brlo    foo
    ret

main:
    ldi     r16, 0x08   ; setup
    out     0x3e, r16
    ldi     r16, 0xff
    out     0x3d, r16
    sei                 ; end setup

    ldi     r16, 0
loop:
    push    r16
    inc     r16
    cpi     r16, 0x0a
    brlo    loop
    ldi     r17, 0
    rcall   foo

    cli                 ; finish
end:
    rjmp    end         ; end finish
