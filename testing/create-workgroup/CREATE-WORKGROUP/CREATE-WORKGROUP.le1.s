## Operations - 123
-- FUNC_main
sub.0 r0.1, r0.1, 0x8
;
mov.0 r0.3, 0x40
;
stw.0 r0.1[0x4], l0.0
;
mov.0 r0.4, 0x50
;
mov.0 r0.5, 0x60
;
call.0 l0.0, FUNC_vector_mult
;
mov.0 r0.2, r0.0
;
ldw.0 l0.0, r0.1[0x4]
;
mov.0 r0.3, r0.0
;
.call exit, caller, arg($r0.3:s32), ret($r0.3:s32)
call.0 l0.0, FUNC_exit
;
return.0 r0.1, r0.1, 0x0, l0.0
;
-- FUNC_vector_mult
sub.0 r0.1, r0.1, 0x10
;
stw.0 r0.1[0x8], r0.57
;
mov.0 r0.57, r0.5
;
stw.0 r0.1[0x4], r0.58
;
mov.0 r0.58, r0.3
;
stw.0 r0.1[0x0], r0.59
;
mov.0 r0.59, r0.4
;
stw.0 r0.1[0xc], l0.0
;
call.0 l0.0, pocl.barrier
;
ldw.0 r0.3, r0.0[(local_size+0)]
;
cpuid.0 r0.2
;
mpylu.0 r0.4, r0.3, r0.2
;
mpyhs.0 r0.3, r0.3, r0.2
;
add.0 r0.3, r0.4, r0.3
;
add.0 r0.5, r0.3, r0.2
;
sh4add.0 r0.4, r0.5, r0.58
;
sh4add.0 r0.3, r0.5, r0.59
;
ldw.0 r0.7, r0.4[0xc]
;
sh4add.0 r0.5, r0.5, r0.57
;
ldw.0 r0.8, r0.3[0xc]
;
ldw.0 r0.9, r0.4[0x8]
;
mpyhs.0 r0.6, r0.7, r0.8
;
ldw.0 r0.10, r0.3[0x8]
;
mpylu.0 r0.7, r0.7, r0.8
;
ldw.0 r0.12, r0.3[0x4]
;
mpyhs.0 r0.8, r0.9, r0.10
;
ldw.0 r0.3, r0.3[0x0]
;
mpylu.0 r0.9, r0.9, r0.10
;
ldw.0 r0.10, r0.4[0x4]
;
add.0 r0.6, r0.7, r0.6
;
ldw.0 r0.4, r0.4[0x0]
;
mpyhs.0 r0.11, r0.10, r0.12
;
stw.0 r0.5[0xc], r0.6
;
mpylu.0 r0.10, r0.10, r0.12
;
add.0 r0.8, r0.9, r0.8
;
mpylu.0 r0.6, r0.4, r0.3
;
add.0 r0.10, r0.10, r0.11
;
mpyhs.0 r0.3, r0.4, r0.3
;
stw.0 r0.5[0x8], r0.8
;
add.0 r0.3, r0.6, r0.3
;
stw.0 r0.5[0x4], r0.10
;
stw.0 r0.5[0x0], r0.3
;
ldw.0 r0.3, r0.0[(local_size+0)]
;
mpylu.0 r0.4, r0.3, r0.2
;
mpyhs.0 r0.3, r0.3, r0.2
;
add.0 r0.3, r0.4, r0.3
;
add.0 r0.5, r0.3, r0.2
;
sh4add.0 r0.4, r0.5, r0.58
;
sh4add.0 r0.3, r0.5, r0.59
;
ldw.0 r0.7, r0.4[0xc]
;
sh4add.0 r0.5, r0.5, r0.57
;
ldw.0 r0.8, r0.3[0xc]
;
ldw.0 r0.9, r0.4[0x8]
;
mpyhs.0 r0.6, r0.7, r0.8
;
ldw.0 r0.10, r0.3[0x8]
;
mpylu.0 r0.7, r0.7, r0.8
;
ldw.0 r0.12, r0.3[0x4]
;
mpyhs.0 r0.8, r0.9, r0.10
;
ldw.0 r0.3, r0.3[0x0]
;
mpylu.0 r0.9, r0.9, r0.10
;
ldw.0 r0.10, r0.4[0x4]
;
add.0 r0.6, r0.7, r0.6
;
ldw.0 r0.4, r0.4[0x0]
;
mpyhs.0 r0.11, r0.10, r0.12
;
stw.0 r0.5[0xc], r0.6
;
mpylu.0 r0.10, r0.10, r0.12
;
add.0 r0.8, r0.9, r0.8
;
mpylu.0 r0.6, r0.4, r0.3
;
add.0 r0.10, r0.10, r0.11
;
mpyhs.0 r0.3, r0.4, r0.3
;
stw.0 r0.5[0x8], r0.8
;
add.0 r0.3, r0.6, r0.3
;
stw.0 r0.5[0x4], r0.10
;
stw.0 r0.5[0x0], r0.3
;
ldw.0 r0.3, r0.0[(local_size+0)]
;
mpylu.0 r0.4, r0.3, r0.2
;
mpyhs.0 r0.3, r0.3, r0.2
;
add.0 r0.3, r0.4, r0.3
;
add.0 r0.4, r0.3, r0.2
;
sh4add.0 r0.3, r0.4, r0.58
;
sh4add.0 r0.2, r0.4, r0.59
;
ldw.0 r0.6, r0.3[0xc]
;
sh4add.0 r0.4, r0.4, r0.57
;
ldw.0 r0.7, r0.2[0xc]
;
ldw.0 r0.8, r0.3[0x8]
;
mpyhs.0 r0.5, r0.6, r0.7
;
ldw.0 r0.9, r0.2[0x8]
;
mpylu.0 r0.6, r0.6, r0.7
;
ldw.0 r0.11, r0.2[0x4]
;
mpyhs.0 r0.7, r0.8, r0.9
;
ldw.0 r0.2, r0.2[0x0]
;
mpylu.0 r0.8, r0.8, r0.9
;
ldw.0 r0.9, r0.3[0x4]
;
add.0 r0.5, r0.6, r0.5
;
ldw.0 r0.3, r0.3[0x0]
;
mpyhs.0 r0.10, r0.9, r0.11
;
stw.0 r0.4[0xc], r0.5
;
mpylu.0 r0.9, r0.9, r0.11
;
add.0 r0.7, r0.8, r0.7
;
mpylu.0 r0.5, r0.3, r0.2
;
add.0 r0.9, r0.9, r0.10
;
mpyhs.0 r0.2, r0.3, r0.2
;
stw.0 r0.4[0x8], r0.7
;
add.0 r0.2, r0.5, r0.2
;
stw.0 r0.4[0x4], r0.9
;
stw.0 r0.4[0x0], r0.2
;
call.0 l0.0, pocl.barrier
;
ldw.0 l0.0, r0.1[0xc]
;
ldw.0 r0.59, r0.1[0x0]
;
ldw.0 r0.58, r0.1[0x4]
;
ldw.0 r0.57, r0.1[0x8]
;
return.0 r0.1, r0.1, 0x10, l0.0
;

##Data Labels
0010 - local_size
001c - num_groups
0028 - global_offset
0000 - work_dim
0004 - global_size

##Data Section - 48 - Data_align=32
0000 - 00000002 - 00000000000000000000000000000010
0004 - 00000400 - 00000000000000000000010000000000
0008 - 00000400 - 00000000000000000000010000000000
000c - 00000000 - 00000000000000000000000000000000
0010 - 00000100 - 00000000000000000000000100000000
0014 - 00000100 - 00000000000000000000000100000000
0018 - 00000000 - 00000000000000000000000000000000
001c - 00000004 - 00000000000000000000000000000100
0020 - 00000004 - 00000000000000000000000000000100
0024 - 00000000 - 00000000000000000000000000000000
0028 - 00000000 - 00000000000000000000000000000000
002c - 00000000 - 00000000000000000000000000000000

