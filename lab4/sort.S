        //  sort.S
        //
        // sort 12 numbers for E155 lab4
        //
        // Eric Mueller -- emueller@hmc.edu
        // 5 October 2015
        //
        // Based off of the sample code written by Prof. Harris in the lab write up.
        // Implements insertion sort as follows:
        //
        // extern char a[12];
        // for (int i = 0; i < 12; i++)
        //     for (int j = i + 1; j < 12; j++)
        //         if (a[j] < a[i]) {
        //             char tmp = a[j];
        //             a[j] = a[i];
        //             a[i] = tmp;
        //         }
        

.text
.global main
main:
        ldr R3, =array          // Load base addr of array into R3
        mov R4, #0              // i = 0

loop1:  cmp R4, #12             // i < 12 ?
        bge end2                // end loop if i >= 12
        add R5, R4, #1          // j = i + 1

loop2:  cmp R5, #12             // j < 12 ?
        bge end1                // end loop if j >= 12
        ldrsb R6, [R3, R5]      // load a[j]
        ldrsb R7, [R3, R4]      // load a[i]
        cmp R6, R7              // a[j] < a[i] ?
        bge else                // else branch -- end loop
        strb R7, [R3, R5]       // a[j] = a[i]
        strb R6, [R3, R4]       // a[i] = tmp
else:   add R5, R5, #1          // j++
        b loop2                 // back to top of inner loop

end1:   add R4, R4, #1          // i++
        b loop1                 // back to top of outer loop

end2:   mov R0, #0              // return value of 0
        bx LR                   // return from main 

.data
array:
        .byte 90
        .byte -112
        .byte 24
        .byte -107
        .byte 16
        .byte -118
        .byte -25
        .byte 79
        .byte -34
        .byte 97
        .byte 109
        .byte -32
        .end
