## Implementation of AES-ECB on the Raspberry-Pi GPU

This repository contains the accompanying code to my [Bachelor-Thesis 'Parallelizing AES on Raspberry-Pi GPU in Custom Assembly'](www.paulpauls.de/documents/bsc_thesis.pdf). It consists of three parts:



### AES128ECB_GPU_192byte_Variant / AES256ECB_GPU_192byte_Variant

Implementation of AES in the Variants 128-bit and 256-bit in ECB mode in custom written Assembly as dictated by the custom GPU Assembler. Each repository contains furthermore comparison implementations of AES, such as tiny-aes or the original rijndael proposal, to which the GPU implementation is compared to in a timing benchmark.


### GPU_Assembler

Custom GPU Assembler, that creates binary code from customly defined Syntax, which is extensively documented in my BSc-Thesis. The GPU is a Broadcom VideoCore IV and the Assembler is implemented according to its [Architecture Reference Guide](https://docs.broadcom.com/docs/12358545).
The Custom GPU Assembler was first started by [Eric Lorimer](https://rpiplayground.wordpress.com/) and extended upon by [Pete Warden](https://petewarden.com/2014/08/07/how-to-optimize-raspberry-pi-code-using-its-gpu/). This implementation again extends Pete Warden's implementation and adds multiple convenience and functionality features as well as plenty of in-code documentation.



### VPM_Problem

Example Code to showcase the problems encountered while using more than 192byte of the GPUs Virtual Pipelined Memory (VPM) when using the custom GPU Assembler

