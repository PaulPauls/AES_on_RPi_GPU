## Move uniforms that hold buffer addresses into registers
or ra31, ra32, 0;           nop



#############################################################################

## Acquire VPM mutex
or ra39, ra51, rb39;        nop

## VPM DMA Load setup for 'assigned text' buffer as 32-bit values
## Loading 32 bytes of 'text' into first row
## Then load the address of 'assigned text'
## Then wait for DMA to complete
# ID:     1
# MODEW:  0, width = 32-bit
# MPITCH: 2, pitch = 32 bytes
# ROWLEN: 8, 4*32-bit = 32 bytes
# NROWS:  1
# VPITCH: 0, set to zero to enable intuitive addressing
# VERT:   0, Horizontal
# ADDRXY: 0
ldi ra49, 0x82810000
or  ra50, ra31, 0;          nop
or  ra39, ra50, rb39;       nop


## SOURCE OF BUG:
## Release and then Reacquire VPM mutex
or ra51, ra39, rb39;        nop
or ra39, ra51, rb39;        nop


## VPM Block Read setup for 'assigned text' buffer as 32-bit values
## Then save 'assigned text' buffer values to ra20-ra23
# ID:     0
# NUM:    8, Numbers of Vectors to read
# STRIDE: 1
# HORIZ:  0, Vertical
# LANED:  0, Packed (ignored because 32 bit width)
# SIZE:   2, 32-bit
# ADDR:   x, read the vectors starting at byte x*4
ldi ra49, 0x00801200

or ra20, ra48, 0;           nop
or ra21, ra48, 0;           nop
or ra22, ra48, 0;           nop
or ra23, ra48, 0;           nop

or ra24, ra48, 0;           nop
or ra25, ra48, 0;           nop
or ra26, ra48, 0;           nop
or ra27, ra48, 0;           nop

## Release VPM mutex
or ra51, ra39, rb39;        nop



#############################################################################

ldi ra32, 0x11111111
nop ra39, ra39, rb39; nop

add ra0, ra20, r0; nop
add ra1, ra21, r0; nop
add ra2, ra22, r0; nop
add ra3, ra23, r0; nop

add ra4, ra24, r0; nop
add ra5, ra25, r0; nop
add ra6, ra26, r0; nop
add ra7, ra27, r0; nop



#############################################################################

## Acquire VPM mutex
or ra39, ra51, rb39;        nop

## VPM Block Write setup for 'assigned text' buffer as 32-bit values
# ID:     0
# STRIDE: 1
# HORIZ:  0, Vertical
# LANED:  0, Packed
# SIZE:   2, 32-bit
# ADDR:   0
ldi rb49, 0x00001200

or ra48, ra0, 0;            nop     # text[0x - 3x]
or ra48, ra1, 0;            nop     # text[4x - 7x]
or ra48, ra2, 0;            nop     # text[8x - 11x]
or ra48, ra3, 0;            nop     # text[12x - 15x]

or ra48, ra4, 0;            nop
or ra48, ra5, 0;            nop
or ra48, ra6, 0;            nop
or ra48, ra7, 0;            nop

## VPM DMA Store setup for 'assigned text' buffer as 32-bit values
## Storing 32 bytes of 'text' into first row
## Then write the 'assigned text' address to store
## Then wait for the DMA to complete
# ID:      2
# UNITS:   1, Number of rows
# DEPTH:   8, Row length assumed in units of width, 8*32-bit = 32 bytes
# LANED:   0
# HORIZ:   1, Horizontal
# VPMBASE: 0
# MODEW:   0, Width = 32-bit
ldi rb49, 0x80884000
or rb50, ra31, 0;           nop
or ra39, ra39, rb50;        nop

## Release VPM mutex
or ra51, ra39, rb39;        nop



##################################
### Finish program and signal host
##################################

or  ra38, ra39, rb39;       nop
nop.tend  ra39, ra39, rb39; nop
nop ra39, ra39, rb39;       nop
nop ra39, ra39, rb39;       nop

