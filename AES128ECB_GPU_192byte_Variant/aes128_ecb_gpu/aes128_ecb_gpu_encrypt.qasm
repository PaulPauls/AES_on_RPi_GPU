## Move uniforms that hold buffer addresses into registers
or ra31, ra32, 0;           nop     # ra31 = TMU address of 'sbox'
or ra30, ra32, 0;           nop     # ra30 = TMU address of 'key'
or ra29, ra32, 0;           nop     # ra29 = VPM address of assigned 'text'



## Var to keep track of cycle rounds
ldi ra28, 9

## Mask immediates for 32-bit split
ldi rb0, 0x000000ff
ldi rb1, 0x0000ff00
ldi rb2, 0x00ff0000
ldi rb3, 0xff000000

## Galois mask immediates
ldi rb4, 0x00000100
ldi rb5, 0x0000011b

## TMU Key Load immediates for TMU signal while fetching
ldi rb6, 16
ldi rb7, 0

## TMU SBOX Load immediate for TMU signal while fetching
ldi rb8, 4



## Acquire VPM mutex
or ra39, ra51, rb39;        nop

## VPM DMA Load Setup for 'assigned text' buffer as 32-bit values
## Loading 16 bytes of 'assigned text' into first row
## Then load the address of 'assigned text'
## Then wait for DMA to complete
# ID:     1
# MODEW:  0, width = 32-bit
# MPITCH: 1, pitch = 16 bytes
# ROWLEN: 4, 4*32-bit = 16 bytes
# NROWS:  1
# VPITCH: 0, set to zero to enable intuitive addressing
# VERT:   0, Horizontal
# ADDRXY: 0
ldi ra49, 0x81410000
or  ra50, ra29, 0;          nop
or  ra39, ra50, rb39;       nop

## VPM Block Read setup for 'assigned text' buffer as 32-bit values
## Then save 'assigned text' buffer values to ra20-ra23
# ID:     0
# NUM:    4, Numbers of Vectors to read
# STRIDE: 1
# HORIZ:  0, Vertical
# LANED:  0, Packed (ignored because 32 bit width)
# SIZE:   2, 32-bit
# ADDR:   0
ldi ra49, 0x00401200

or ra20, ra48, 0;           nop     # ra20 = text[0-3]
or ra21, ra48, 0;           nop     # ra21 = text[4-7]
or ra22, ra48, 0;           nop     # ra22 = text[8-11]
or ra23, ra48, 0;           nop     # ra23 = text[12-15]

## Release VPM mutex
or ra51, ra39, rb39;        nop



#############################################################################
################################ Text Split #################################
#############################################################################

## Split the 32-bit 'text' values in registers ra20-ra23 into 8-bit 'text'
## values and save them to registers ra0-ra15
and ra0,  ra20, rb0;        nop     # ra0 = text[0]
and ra32, ra20, rb1;        nop
shr ra1,  r0,   0x8;        nop     # ra1 = text[1]
and ra32, ra20, rb2;        nop
shr ra2,  r0,   0x10;       nop     # ra2 = text[2]
and ra32, ra20, rb3;        nop
shr ra3,  r0,   0x18;       nop     # ra3 = text[3]

and ra4,  ra21, rb0;        nop     # ra4 = text[4]
and ra32, ra21, rb1;        nop
shr ra5,  r0,   0x8;        nop     # ra5 = text[5]
and ra32, ra21, rb2;        nop
shr ra6,  r0,   0x10;       nop     # ra6 = text[6]
and ra32, ra21, rb3;        nop
shr ra7,  r0,   0x18;       nop     # ra7 = text[7]

and ra8,  ra22, rb0;        nop     # ra8 = text[8]
and ra32, ra22, rb1;        nop
shr ra9,  r0,   0x8;        nop     # ra9 = text[9]
and ra32, ra22, rb2;        nop
shr ra10, r0,   0x10;       nop     # ra10 = text[10]
and ra32, ra22, rb3;        nop
shr ra11, r0,   0x18;       nop     # ra11 = text[11]

and ra12, ra23, rb0;        nop     # ra12 = text[12]
and ra32, ra23, rb1;        nop
shr ra13, r0,   0x8;        nop     # ra13 = text[13]
and ra32, ra23, rb2;        nop
shr ra14, r0,   0x10;       nop     # ra14 = text[14]
and ra32, ra23, rb3;        nop
shr ra15, r0,   0x18;       nop     # ra15 = text[15]



#############################################################################
################################# AES Start #################################
#############################################################################

############################# VARIABLE OVERVIEW #############################
### Registers listed here hold the according values between the AES functions
### and should not be illegitimately overwritten
###
### ra0-ra15:  text[0]-text[15]
### ra16-ra19: key[0-3]-key[12-15], later expandedKey values
### rb0-rb3:   32bit to 8bit conversion immediates
### rb4-rb5:   Mask immediate for Galois mul
### rb6-rb7:   TMU Key Load immediates for TMU signal while fetching
### rb8:       TMU SBOX Load immediate for TMU signal while fetching
### ra31:      TMU address of 'sbox'
### ra30:      TMU address of 'key'
### ra29:      VPM address of 'assigned text'
### ra28:      Variable to keep track of cycle rounds
#############################################################################



#############################################################################
############################### Initial Round ###############################
#############################################################################

##################################
### ADD_ROUND_KEY, FETCH_NEXT_KEY
##################################

## Preload Keys to be fetched
add ra56, ra30, 0;          nop
add ra56, ra30, 4;          nop
add ra56, ra30, 8;          nop
add ra56, ra30, 12;         nop

## Advance TMU Key Address to next round key
add.tmu ra30, ra30, rb6;    nop

## Fetch Keys from TMU
or.tmu  ra16, r4,   rb7;    nop
or.tmu  ra17, r4,   rb7;    nop
or.tmu  ra18, r4,   rb7;    nop
or      ra19, r4,   rb7;    nop



###################
### ADD_ROUND_KEY
###################

and ra32, ra16, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[0]
xor ra0,  ra0,  r0;         nop     # text[0] = text[0] ^ key[0]

and ra32, ra16, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[1]
xor ra1,  ra1,  r0;         nop     # text[1] = text[1] ^ key[1]

and ra32, ra16, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[2]
xor ra2,  ra2,  r0;         nop     # text[2] = text[2] ^ key[2]

and ra32, ra16, rb0;        nop     # r0 = key[3]
xor ra3,  ra3,  r0;         nop     # text[3] = text[3] ^ key[3]


and ra32, ra17, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[4]
xor ra4,  ra4,  r0;         nop     # text[4] = text[4] ^ key[4]

and ra32, ra17, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[5]
xor ra5,  ra5,  r0;         nop     # text[5] = text[5] ^ key[5]

and ra32, ra17, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[6]
xor ra6,  ra6,  r0;         nop     # text[6] = text[6] ^ key[6]

and ra32, ra17, rb0;        nop     # r0 = key[7]
xor ra7,  ra7,  r0;         nop     # text[7] = text[7] ^ key[7]


and ra32, ra18, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[8]
xor ra8,  ra8,  r0;         nop     # text[8] = text[8] ^ key[8]

and ra32, ra18, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[9]
xor ra9,  ra9,  r0;         nop     # text[9] = text[9] ^ key[9]

and ra32, ra18, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[10]
xor ra10, ra10, r0;         nop     # text[10] = text[10] ^ key[10]

and ra32, ra18, rb0;        nop     # r0 = key[11]
xor ra11, ra11, r0;         nop     # text[11] = text[11] ^ key[11]


and ra32, ra19, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[12]
xor ra12, ra12, r0;         nop     # text[12] = text[12] ^ key[12]

and ra32, ra19, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[13]
xor ra13, ra13, r0;         nop     # text[13] = text[13] ^ key[13]

and ra32, ra19, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[14]
xor ra14, ra14, r0;         nop     # text[14] = text[14] ^ key[14]

and ra32, ra19, rb0;        nop     # r0 = key[15]
xor ra15, ra15, r0;         nop     # text[15] = text[15] ^ key[15]



#############################################################################
############################### Cycle Rounds ################################
#############################################################################

cycle:

###################
### sub_bytes
###################

nop     ra39, ra0,  rb8;    mul24 rb33, ra0, rb8;

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[0]*4)
nop.tmu ra39, ra1,  rb8;    mul24 rb33, ra1, rb8;
or      ra0,  r4,   0;      nop                 # text[0] = sbox[text[0]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[1]*4)
nop.tmu ra39, ra2,  rb8;    mul24 rb33, ra2, rb8;
or      ra1,  r4,   0;      nop                 # text[1] = sbox[text[1]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[2]*4)
nop.tmu ra39, ra3,  rb8;    mul24 rb33, ra3, rb8;
or      ra2,  r4,   0;      nop                 # text[2] = sbox[text[2]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[3]*4)
nop.tmu ra39, ra4,  rb8;    mul24 rb33, ra4, rb8;
or      ra3,  r4,   0;      nop                 # text[3] = sbox[text[3]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[4]*4)
nop.tmu ra39, ra5,  rb8;    mul24 rb33, ra5, rb8;
or      ra4,  r4,   0;      nop                 # text[4] = sbox[text[4]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[5]*4)
nop.tmu ra39, ra6,  rb8;    mul24 rb33, ra6, rb8;
or      ra5,  r4,   0;      nop                 # text[5] = sbox[text[5]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[6]*4)
nop.tmu ra39, ra7,  rb8;    mul24 rb33, ra7, rb8;
or      ra6,  r4,   0;      nop                 # text[6] = sbox[text[6]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[7]*4)
nop.tmu ra39, ra8,  rb8;    mul24 rb33, ra8, rb8;
or      ra7,  r4,   0;      nop                 # text[7] = sbox[text[7]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[8]*4)
nop.tmu ra39, ra9,  rb8;    mul24 rb33, ra9, rb8;
or      ra8,  r4,   0;      nop                 # text[8] = sbox[text[8]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[9]*4)
nop.tmu ra39, ra10, rb8;    mul24 rb33, ra10, rb8;
or      ra9,  r4,   0;      nop                 # text[9] = sbox[text[9]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[10]*4)
nop.tmu ra39, ra11, rb8;    mul24 rb33, ra11, rb8;
or      ra10, r4,   0;      nop                 # text[10] = sbox[text[10]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[11]*4)
nop.tmu ra39, ra12, rb8;    mul24 rb33, ra12, rb8;
or      ra11, r4,   0;      nop                 # text[11] = sbox[text[11]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[12]*4)
nop.tmu ra39, ra13, rb8;    mul24 rb33, ra13, rb8;
or      ra12, r4,   0;      nop                 # text[12] = sbox[text[12]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[13]*4)
nop.tmu ra39, ra14, rb8;    mul24 rb33, ra14, rb8;
or      ra13, r4,   0;      nop                 # text[13] = sbox[text[13]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[14]*4)
nop.tmu ra39, ra15, rb8;    mul24 rb33, ra15, rb8;
or      ra14, r4,   0;      nop                 # text[14] = sbox[text[14]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[15]*4)
nop.tmu ra39, ra39, rb39;   nop
or      ra15, r4,   0;      nop                 # text[15] = sbox[text[15]]



###################
### shift_row
###################

or ra32, ra1,  0;           nop     # tmp      = text[1]
or ra1,  ra5,  0;           nop     # text[1]  = text[5]
or ra5,  ra9,  0;           nop     # text[5]  = text[9]
or ra9,  ra13, 0;           nop     # text[9]  = text[13]
or ra13, r0,   0;           nop     # text[13] = tmp

or ra32, ra2,  0;           nop     # tmp      = text[2]
or ra2,  ra10, 0;           nop     # text[2]  = text[10]
or ra10, r0,   0;           nop     # text[10] = tmp
or ra32, ra6,  0;           nop     # tmp      = text[6]
or ra6,  ra14, 0;           nop     # text[6]  = text[14]
or ra14, r0,   0;           nop     # text[14] = tmp

or ra32, ra3,  0;           nop     # tmp      = text[3]
or ra3,  ra15, 0;           nop     # text[3]  = text[15]
or ra15, ra11, 0;           nop     # text[15] = text[11]
or ra11, ra7,  0;           nop     # text[11] = text[7]
or ra7,  r0,   0;           nop     # text[7]  = tmp



###################
### mix_columns
###################

shl ra32, ra0, 1;           nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra35, r0,  0x00000000;  nop     # r3 = gal_mul(text[0], 2)

xor ra34, ra1, 0x00000000;  nop
shl ra32, ra1, 1;           nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra35, r0, r3;           nop     # r3 = gal_mul(text[1], 3) ^ r3

xor ra35, ra2, r3;          nop     # r3 = gal_mul(text[2], 1) ^ r3

xor ra20, ra3, r3;          nop     # ra20 = tmp[0] = gal_mul(text[3], 1) ^ r3


or  ra35, ra0, 0;           nop     # r3 = gal_mul(text[0], 1)

shl ra32, ra1, 1;           nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra35, r0,  r3;          nop     # r3 = gal_mul(text[1], 2) ^ r3

xor ra34, ra2, 0x00000000;  nop
shl ra32, ra2, 1;           nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra35, r0, r3;           nop     # r3 = gal_mul(text[2], 3) ^ r3

xor ra21, ra3, r3;          nop     # ra21 = tmp[1] = gal_mul(text[3], 1) ^ r3


or  ra35, ra0, 0;           nop     # r3 = gal_mul(text[0], 1)

xor ra35, ra1, r3;          nop     # r3 = gal_mul(text[1], 1) ^ r3

shl ra32, ra2, 1;           nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra35, r0,  r3;          nop     # r3 = gal_mul(text[2], 2) ^ r3

xor ra34, ra3, 0x00000000;  nop
shl ra32, ra3, 1;           nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra22, r0, r3;           nop     # ra22 = tmp[2] = gal_mul(text[3], 3) ^ r3


xor ra34, ra0, 0x00000000;  nop
shl ra32, ra0, 1;           nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra35, r0, r2;           nop     # r3 = gal_mul(text[0], 3)

xor ra35, ra1, r3;          nop     # r3 = gal_mul(text[1], 1) ^ r3

xor ra35, ra2, r3;          nop     # r3 = gal_mul(text[2], 1) ^ r3

shl ra32, ra3, 1;           nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra23, r0,  r3;          nop     # ra23 = tmp[3] = gal_mul(text[3], 2) ^ r3


or  ra0, ra20, 0;           nop     # text[0] = tmp[0]
or  ra1, ra21, 0;           nop     # text[1] = tmp[1]
or  ra2, ra22, 0;           nop     # text[2] = tmp[2]
or  ra3, ra23, 0;           nop     # text[3] = tmp[3]


shl ra32, ra4, 1;           nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra35, r0,  0x00000000;  nop     # r3 = gal_mul(text[4], 2)

xor ra34, ra5, 0x00000000;  nop
shl ra32, ra5, 1;           nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra35, r0, r3;           nop     # r3 = gal_mul(text[5], 3) ^ r3

xor ra35, ra6, r3;          nop     # r3 = gal_mul(text[6], 1) ^ r3

xor ra20, ra7, r3;          nop     # ra20 = tmp[4] = gal_mul(text[7], 1) ^ r3


or  ra35, ra4, 0;           nop     # r3 = gal_mul(text[4], 1)

shl ra32, ra5, 1;           nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra35, r0,  r3;          nop     # r3 = gal_mul(text[5], 2) ^ r3

xor ra34, ra6, 0x00000000;  nop
shl ra32, ra6, 1;           nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra35, r0, r3;           nop     # r3 = gal_mul(text[6], 3) ^ r3

xor ra21, ra7, r3;          nop     # ra21 = tmp[5] = gal_mul(text[7], 1) ^ r3


or  ra35, ra4, 0;           nop     # r3 = gal_mul(text[4], 1)

xor ra35, ra5, r3;          nop     # r3 = gal_mul(text[5], 1) ^ r3

shl ra32, ra6, 1;           nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra35, r0,  r3;          nop     # r3 = gal_mul(text[6], 2) ^ r3

xor ra34, ra7, 0x00000000;  nop
shl ra32, ra7, 1;           nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra22, r0, r3;           nop     # ra22 = tmp[6] = gal_mul(text[7], 3) ^ r3


xor ra34, ra4, 0x00000000;  nop
shl ra32, ra4, 1;           nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra35, r0, r2;           nop     # r3 = gal_mul(text[4], 3)

xor ra35, ra5, r3;          nop     # r3 = gal_mul(text[5], 1) ^ r3

xor ra35, ra6, r3;          nop     # r3 = gal_mul(text[6], 1) ^ r3

shl ra32, ra7, 1;           nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra23, r0,  r3;          nop     # ra23 = tmp[7] = gal_mul(text[7], 2) ^ r3


or  ra4, ra20, 0;           nop     # text[4] = tmp[4]
or  ra5, ra21, 0;           nop     # text[5] = tmp[5]
or  ra6, ra22, 0;           nop     # text[6] = tmp[6]
or  ra7, ra23, 0;           nop     # text[7] = tmp[7]


shl ra32, ra8, 1;           nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra35, r0,  0x00000000;  nop     # r3 = gal_mul(text[8], 2)

xor ra34, ra9, 0x00000000;  nop
shl ra32, ra9, 1;           nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra35, r0, r3;           nop     # r3 = gal_mul(text[9], 3) ^ r3

xor ra35, ra10, r3;         nop     # r3 = gal_mul(text[10], 1) ^ r3

xor ra20, ra11, r3;         nop     # ra20 = tmp[8] = gal_mul(text[11], 1) ^ r3


or  ra35, ra8, 0;           nop     # r3 = gal_mul(text[8], 1)

shl ra32, ra9, 1;           nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra35, r0,  r3;          nop     # r3 = gal_mul(text[9], 2) ^ r3

xor ra34, ra10, 0x00000000; nop
shl ra32, ra10, 1;          nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra35, r0, r3;           nop     # r3 = gal_mul(text[10], 3) ^ r3

xor ra21, ra11, r3;         nop     # ra21 = tmp[9] = gal_mul(text[11], 1) ^ r3


or  ra35, ra8, 0;           nop     # r3 = gal_mul(text[8], 1)

xor ra35, ra9, r3;          nop     # r3 = gal_mul(text[9], 1) ^ r3

shl ra32, ra10, 1;          nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra35, r0,  r3;          nop     # r3 = gal_mul(text[10], 2) ^ r3

xor ra34, ra11, 0x00000000; nop
shl ra32, ra11, 1;          nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra22, r0, r3;           nop     # ra22 = tmp[10] = gal_mul(text[11], 3) ^ r3


xor ra34, ra8, 0x00000000;  nop
shl ra32, ra8, 1;           nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra35, r0, r2;           nop     # r3 = gal_mul(text[8], 3)

xor ra35, ra9, r3;          nop     # r3 = gal_mul(text[9], 1) ^ r3

xor ra35, ra10, r3;         nop     # r3 = gal_mul(text[10], 1) ^ r3

shl ra32, ra11, 1;          nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra23, r0,  r3;          nop     # ra23 = tmp[11] = gal_mul(text[11], 2) ^ r3


or  ra8,  ra20, 0;          nop     # text[8] = tmp[8]
or  ra9,  ra21, 0;          nop     # text[9] = tmp[9]
or  ra10, ra22, 0;          nop     # text[10] = tmp[10]
or  ra11, ra23, 0;          nop     # text[11] = tmp[11]


shl ra32, ra12, 1;          nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra35, r0,  0x00000000;  nop     # r3 = gal_mul(text[12], 2)

xor ra34, ra13, 0x00000000; nop
shl ra32, ra13, 1;          nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra35, r0, r3;           nop     # r3 = gal_mul(text[13], 3) ^ r3

xor ra35, ra14, r3;         nop     # r3 = gal_mul(text[14], 1) ^ r3

xor ra20, ra15, r3;         nop     # ra20 = tmp[12] = gal_mul(text[15], 1) ^ r3


or  ra35, ra12, 0;          nop     # r3 = gal_mul(text[12], 1)

shl ra32, ra13, 1;          nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra35, r0,  r3;          nop     # r3 = gal_mul(text[13], 2) ^ r3

xor ra34, ra14, 0x00000000; nop
shl ra32, ra14, 1;          nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra35, r0, r3;           nop     # r3 = gal_mul(text[14], 3) ^ r3

xor ra21, ra15, r3;         nop     # ra21 = tmp[13] = gal_mul(text[15], 1) ^ r3


or  ra35, ra12, 0;          nop     # r3 = gal_mul(text[12], 1)

xor ra35, ra13, r3;         nop     # r3 = gal_mul(text[13], 1) ^ r3

shl ra32, ra14, 1;          nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra35, r0,  r3;          nop     # r3 = gal_mul(text[14], 2) ^ r3

xor ra34, ra15, 0x00000000; nop
shl ra32, ra15, 1;          nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra32, r0, r2;           nop
xor ra22, r0, r3;           nop     # ra22 = tmp[14] = gal_mul(text[15], 3) ^ r3


xor ra34, ra12, 0x00000000; nop
shl ra32, ra12, 1;          nop
and ra33, r0, rb4;          nop
shr ra33, r1, 8;            nop
nop ra39, r1, rb5;          mul24 rb33, r1, rb5
xor ra32, r0, r1;           nop
xor ra35, r0, r2;           nop     # r3 = gal_mul(text[12], 3)

xor ra35, ra13, r3;         nop     # r3 = gal_mul(text[13], 1) ^ r3

xor ra35, ra14, r3;         nop     # r3 = gal_mul(text[14], 1) ^ r3

shl ra32, ra15, 1;          nop
and ra33, r0,  rb4;         nop
shr ra33, r1,  8;           nop
nop ra39, r1,  rb5;         mul24 rb33, r1, rb5
xor ra32, r0,  r1;          nop
xor ra32, r0,  0x00000000;  nop
xor ra23, r0,  r3;          nop     # ra23 = tmp[15] = gal_mul(text[15], 2) ^ r3


or  ra12, ra20, 0;          nop     # text[12] = tmp[12]
or  ra13, ra21, 0;          nop     # text[13] = tmp[13]
or  ra14, ra22, 0;          nop     # text[14] = tmp[14]
or  ra15, ra23, 0;          nop     # text[15] = tmp[15]



##################################
### ADD_ROUND_KEY, FETCH_NEXT_KEY
##################################

## Preload Keys to be fetched
add ra56, ra30, 0;          nop
add ra56, ra30, 4;          nop
add ra56, ra30, 8;          nop
add ra56, ra30, 12;         nop

## Advance TMU Key Address to next round key
add.tmu ra30, ra30, rb6;    nop

## Fetch Keys from TMU
or.tmu  ra16, r4,   rb7;    nop
or.tmu  ra17, r4,   rb7;    nop
or.tmu  ra18, r4,   rb7;    nop
or      ra19, r4,   rb7;    nop



###################
### ADD_ROUND_KEY
###################

and ra32, ra16, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[0]
xor ra0,  ra0,  r0;         nop     # text[0] = text[0] ^ key[0]

and ra32, ra16, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[1]
xor ra1,  ra1,  r0;         nop     # text[1] = text[1] ^ key[1]

and ra32, ra16, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[2]
xor ra2,  ra2,  r0;         nop     # text[2] = text[2] ^ key[2]

and ra32, ra16, rb0;        nop     # r0 = key[3]
xor ra3,  ra3,  r0;         nop     # text[3] = text[3] ^ key[3]


and ra32, ra17, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[4]
xor ra4,  ra4,  r0;         nop     # text[4] = text[4] ^ key[4]

and ra32, ra17, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[5]
xor ra5,  ra5,  r0;         nop     # text[5] = text[5] ^ key[5]

and ra32, ra17, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[6]
xor ra6,  ra6,  r0;         nop     # text[6] = text[6] ^ key[6]

and ra32, ra17, rb0;        nop     # r0 = key[7]
xor ra7,  ra7,  r0;         nop     # text[7] = text[7] ^ key[7]


and ra32, ra18, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[8]
xor ra8,  ra8,  r0;         nop     # text[8] = text[8] ^ key[8]

and ra32, ra18, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[9]
xor ra9,  ra9,  r0;         nop     # text[9] = text[9] ^ key[9]

and ra32, ra18, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[10]
xor ra10, ra10, r0;         nop     # text[10] = text[10] ^ key[10]

and ra32, ra18, rb0;        nop     # r0 = key[11]
xor ra11, ra11, r0;         nop     # text[11] = text[11] ^ key[11]


and ra32, ra19, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[12]
xor ra12, ra12, r0;         nop     # text[12] = text[12] ^ key[12]

and ra32, ra19, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[13]
xor ra13, ra13, r0;         nop     # text[13] = text[13] ^ key[13]

and ra32, ra19, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[14]
xor ra14, ra14, r0;         nop     # text[14] = text[14] ^ key[14]

and ra32, ra19, rb0;        nop     # r0 = key[15]
xor ra15, ra15, r0;         nop     # text[15] = text[15] ^ key[15]



##################################
### Branch to repeat cycle rounds
##################################

sub    ra28, ra28, 1;       nop
brr.ze ra39, rb39, cycle            # branch if ra27 != 0 to 'cycle'
nop    ra39, ra39, rb39;    nop
nop    ra39, ra39, rb39;    nop
nop    ra39, ra39, rb39;    nop



#############################################################################
################################ Final Round ################################
#############################################################################

###################
### sub_bytes
###################

nop     ra39, ra0,  rb8;    mul24 rb33, ra0, rb8;

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[0]*4)
nop.tmu ra39, ra1,  rb8;    mul24 rb33, ra1, rb8;
or      ra0,  r4,   0;      nop                 # text[0] = sbox[text[0]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[1]*4)
nop.tmu ra39, ra2,  rb8;    mul24 rb33, ra2, rb8;
or      ra1,  r4,   0;      nop                 # text[1] = sbox[text[1]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[2]*4)
nop.tmu ra39, ra3,  rb8;    mul24 rb33, ra3, rb8;
or      ra2,  r4,   0;      nop                 # text[2] = sbox[text[2]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[3]*4)
nop.tmu ra39, ra4,  rb8;    mul24 rb33, ra4, rb8;
or      ra3,  r4,   0;      nop                 # text[3] = sbox[text[3]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[4]*4)
nop.tmu ra39, ra5,  rb8;    mul24 rb33, ra5, rb8;
or      ra4,  r4,   0;      nop                 # text[4] = sbox[text[4]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[5]*4)
nop.tmu ra39, ra6,  rb8;    mul24 rb33, ra6, rb8;
or      ra5,  r4,   0;      nop                 # text[5] = sbox[text[5]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[6]*4)
nop.tmu ra39, ra7,  rb8;    mul24 rb33, ra7, rb8;
or      ra6,  r4,   0;      nop                 # text[6] = sbox[text[6]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[7]*4)
nop.tmu ra39, ra8,  rb8;    mul24 rb33, ra8, rb8;
or      ra7,  r4,   0;      nop                 # text[7] = sbox[text[7]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[8]*4)
nop.tmu ra39, ra9,  rb8;    mul24 rb33, ra9, rb8;
or      ra8,  r4,   0;      nop                 # text[8] = sbox[text[8]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[9]*4)
nop.tmu ra39, ra10, rb8;    mul24 rb33, ra10, rb8;
or      ra9,  r4,   0;      nop                 # text[9] = sbox[text[9]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[10]*4)
nop.tmu ra39, ra11, rb8;    mul24 rb33, ra11, rb8;
or      ra10, r4,   0;      nop                 # text[10] = sbox[text[10]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[11]*4)
nop.tmu ra39, ra12, rb8;    mul24 rb33, ra12, rb8;
or      ra11, r4,   0;      nop                 # text[11] = sbox[text[11]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[12]*4)
nop.tmu ra39, ra13, rb8;    mul24 rb33, ra13, rb8;
or      ra12, r4,   0;      nop                 # text[12] = sbox[text[12]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[13]*4)
nop.tmu ra39, ra14, rb8;    mul24 rb33, ra14, rb8;
or      ra13, r4,   0;      nop                 # text[13] = sbox[text[13]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[14]*4)
nop.tmu ra39, ra15, rb8;    mul24 rb33, ra15, rb8;
or      ra14, r4,   0;      nop                 # text[14] = sbox[text[14]]

add     ra56, ra31, r1;     nop                 # TMU_load(&sbox + text[15]*4)
nop.tmu ra39, ra39, rb39;   nop
or      ra15, r4,   0;      nop                 # text[15] = sbox[text[15]]



###################
### shift_row
###################

or ra32, ra1,  0;           nop     # tmp      = text[1]
or ra1,  ra5,  0;           nop     # text[1]  = text[5]
or ra5,  ra9,  0;           nop     # text[5]  = text[9]
or ra9,  ra13, 0;           nop     # text[9]  = text[13]
or ra13, r0,   0;           nop     # text[13] = tmp

or ra32, ra2,  0;           nop     # tmp      = text[2]
or ra2,  ra10, 0;           nop     # text[2]  = text[10]
or ra10, r0,   0;           nop     # text[10] = tmp
or ra32, ra6,  0;           nop     # tmp      = text[6]
or ra6,  ra14, 0;           nop     # text[6]  = text[14]
or ra14, r0,   0;           nop     # text[14] = tmp

or ra32, ra3,  0;           nop     # tmp      = text[3]
or ra3,  ra15, 0;           nop     # text[3]  = text[15]
or ra15, ra11, 0;           nop     # text[15] = text[11]
or ra11, ra7,  0;           nop     # text[11] = text[7]
or ra7,  r0,   0;           nop     # text[7]  = tmp



##################################
### ADD_ROUND_KEY, FETCH_NEXT_KEY
##################################

## Preload Keys to be fetched
add ra56, ra30, 0;          nop
add ra56, ra30, 4;          nop
add ra56, ra30, 8;          nop
add ra56, ra30, 12;         nop

## Advance TMU Key Address to next round key
add.tmu ra30, ra30, rb6;    nop

## Fetch Keys from TMU
or.tmu  ra16, r4,   rb7;    nop
or.tmu  ra17, r4,   rb7;    nop
or.tmu  ra18, r4,   rb7;    nop
or      ra19, r4,   rb7;    nop



###################
### ADD_ROUND_KEY
###################

and ra32, ra16, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[0]
xor ra0,  ra0,  r0;         nop     # text[0] = text[0] ^ key[0]

and ra32, ra16, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[1]
xor ra1,  ra1,  r0;         nop     # text[1] = text[1] ^ key[1]

and ra32, ra16, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[2]
xor ra2,  ra2,  r0;         nop     # text[2] = text[2] ^ key[2]

and ra32, ra16, rb0;        nop     # r0 = key[3]
xor ra3,  ra3,  r0;         nop     # text[3] = text[3] ^ key[3]


and ra32, ra17, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[4]
xor ra4,  ra4,  r0;         nop     # text[4] = text[4] ^ key[4]

and ra32, ra17, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[5]
xor ra5,  ra5,  r0;         nop     # text[5] = text[5] ^ key[5]

and ra32, ra17, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[6]
xor ra6,  ra6,  r0;         nop     # text[6] = text[6] ^ key[6]

and ra32, ra17, rb0;        nop     # r0 = key[7]
xor ra7,  ra7,  r0;         nop     # text[7] = text[7] ^ key[7]


and ra32, ra18, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[8]
xor ra8,  ra8,  r0;         nop     # text[8] = text[8] ^ key[8]

and ra32, ra18, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[9]
xor ra9,  ra9,  r0;         nop     # text[9] = text[9] ^ key[9]

and ra32, ra18, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[10]
xor ra10, ra10, r0;         nop     # text[10] = text[10] ^ key[10]

and ra32, ra18, rb0;        nop     # r0 = key[11]
xor ra11, ra11, r0;         nop     # text[11] = text[11] ^ key[11]


and ra32, ra19, rb3;        nop
shr ra32, r0,   0x18;       nop     # r0 = key[12]
xor ra12, ra12, r0;         nop     # text[12] = text[12] ^ key[12]

and ra32, ra19, rb2;        nop
shr ra32, r0,   0x10;       nop     # r0 = key[13]
xor ra13, ra13, r0;         nop     # text[13] = text[13] ^ key[13]

and ra32, ra19, rb1;        nop
shr ra32, r0,   0x8;        nop     # r0 = key[14]
xor ra14, ra14, r0;         nop     # text[14] = text[14] ^ key[14]

and ra32, ra19, rb0;        nop     # r0 = key[15]
xor ra15, ra15, r0;         nop     # text[15] = text[15] ^ key[15]



#############################################################################
################################## AES END ##################################
#############################################################################



#############################################################################
################################ Text Merge #################################
#############################################################################

## Combine the 8-bit 'text' values in registers ra0-ra15 int 32-bit 'text'
## values and save them to registers ra0-ra3
shl ra33, ra1, 0x8;         nop
or  ra32, ra0, r1;          nop
shl ra33, ra2, 0x10;        nop
or  ra32, r0,  r1;          nop
shl ra33, ra3, 0x18;        nop
or  ra0,  r0,  r1;          nop     # ra0 = text[0-3]

shl ra33, ra5, 0x8;         nop
or  ra32, ra4, r1;          nop
shl ra33, ra6, 0x10;        nop
or  ra32, r0,  r1;          nop
shl ra33, ra7, 0x18;        nop
or  ra1,  r0,  r1;          nop     # ra1 = text[4-7]

shl ra33, ra9,  0x8;        nop
or  ra32, ra8,  r1;         nop
shl ra33, ra10, 0x10;       nop
or  ra32, r0,   r1;         nop
shl ra33, ra11, 0x18;       nop
or  ra2,  r0,   r1;         nop     # ra2 = text[8-11]

shl ra33, ra13, 0x8;        nop
or  ra32, ra12, r1;         nop
shl ra33, ra14, 0x10;       nop
or  ra32, r0,   r1;         nop
shl ra33, ra15, 0x18;       nop
or  ra3,  r0,   r1;         nop     # ra3 = text[12-15]



#############################################################################
############################# Store and Finish ##############################
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

or ra48, ra0, 0;            nop     # text[0-3]
or ra48, ra1, 0;            nop     # text[4-7]
or ra48, ra2, 0;            nop     # text[8-11]
or ra48, ra3, 0;            nop     # text[12-15]

## VPM DMA Store setup for 'assigned text' buffer as 32-bit values
## Storing 16 bytes of 'text' into first row
## Then write the 'assigned text' address to store
## Then wait for the DMA to complete
# ID:      2
# UNITS:   1, Number of rows
# DEPTH:   4, Row length in units of width (4*32-bit = 16 bytes)
# LANED:   0
# HORIZ:   1, Horizontal
# VPMBASE: 0
# MODEW:   0, Width = 32-bit
ldi rb49, 0x80844000
or rb50, ra29, 0;           nop
or ra39, ra39, rb50;        nop

## Release VPM mutex
or ra51, ra39, rb39;        nop



## Trigger interrupt to finish the program and signal host
or  ra38, ra39, rb39;       nop
nop.tend  ra39, ra39, rb39; nop
nop ra39, ra39, rb39;       nop
nop ra39, ra39, rb39;       nop

