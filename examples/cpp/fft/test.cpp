// ----------------------------------------------------------------------
// Copyright (c) 2016, The Regents of the University of Calfornia All
// rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of The Regents of the University of California
//       nor the names of its contributors may be used to endorse or
//       promote products derived from this software without specific
//       prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL REGENTS OF THE
// UNIVERSITY OF CALIFORNIA BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
// TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
// ----------------------------------------------------------------------
#include "fft.hpp"
#include "testops.hpp"
#include <iostream>
#include <stdio.h>
#include <complex>
#include <cmath>
#ifdef BIT_ACCURATE
#include "hls_math.h"
#include "ap_fixed.h"
#endif
#define LOG_LIST_LENGTH 6
#define LIST_LENGTH (1<<LOG_LIST_LENGTH)
#ifdef BIT_ACCURATE
#define DTYPE float
#else 
#define DTYPE float
#endif

std::array<std::complex<DTYPE>, LIST_LENGTH> hw_synth_fft(std::array<std::complex<DTYPE>, LIST_LENGTH> input){
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=input._M_instance COMPLETE
	return fft(input);
}

std::array<std::complex<DTYPE>, LIST_LENGTH> hw_synth_for_fft(std::array<std::complex<DTYPE>, LIST_LENGTH> input){
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=input._M_instance COMPLETE
	return imperative::fft(input);
}

int test_fft(){
	std::array<std::complex<DTYPE>, LIST_LENGTH> gold, in, out;
	for(int i = 0; i < LIST_LENGTH; i ++){
		gold[i] = in[i] = {(DTYPE)(i + 1), (DTYPE)(i + 1)};
	}

	gold = software::fft(gold);

	out = hw_synth_fft(in);
	for(int i = 0; i < LIST_LENGTH; i ++){
		if(std::abs(float(gold[i].real() - out[i].real())) > 1){
			fprintf(stderr, "Error! Recursive Real FFT Values at index %d did not match\n", i);
			std::cout << "(" << out[i].real() << "," << out[i].imag() << ")\n";
			std::cout << "(" << gold[i].real() << "," << gold[i].imag() << ")\n";
			return -1;
		}
		if(std::abs(float(gold[i].imag() - out[i].imag())) > 1){
			fprintf(stderr, "Error! Recursive Imag FFT Values at index %d did not match\n", i);
			std::cout << "(" << out[i].real() << "," << out[i].imag() << ")\n";
			std::cout << "(" << gold[i].real() << "," << gold[i].imag() << ")\n";
			return -1;
		}
	}
	printf("FFT (hof) test passed!\n");

	out = hw_synth_for_fft(in);
	for(int i = 0; i < LIST_LENGTH; i ++){
		if(std::abs(float(gold[i].real() - out[i].real())) > 1){
			fprintf(stderr, "Error! Non-Recursive Real FFT Values at index %d did not match\n", i);
			return -1;
		}
		if(std::abs(float(gold[i].imag() - out[i].imag())) > 1){
			fprintf(stderr, "Error! Non-Recursive Imag FFT Values at index %d did not match\n", i);
			return -1;
		}
	}
	printf("FFT (for) test passed!\n");

	printf("FFT Tests Passed!\n");
	return 0;
}


std::array<FFT_t<DTYPE>, 2*LIST_LENGTH> 
hw_synth_for_nptfft(std::array<FFT_t<DTYPE>, LIST_LENGTH> L, 
		std::array<FFT_t<DTYPE>, LIST_LENGTH> R){
	std::array<std::complex<DTYPE>, 2*LIST_LENGTH> in, OUT;
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=L._M_instance COMPLETE
#pragma HLS ARRAY_PARTITION variable=R._M_instance COMPLETE
#pragma HLS ARRAY_PARTITION variable=in._M_instance COMPLETE
#pragma HLS ARRAY_PARTITION variable=OUT._M_instance COMPLETE
	in = L + R;

	std::size_t l = LOG_LIST_LENGTH;
	std::size_t dftp = (1 << (l + 1));
	std::size_t stride = 1 << l;
	std::size_t mask = stride - 1;

bfly_pair:
	for(std::size_t idx = 0; idx < (1 << (LOG_LIST_LENGTH)); ++idx){
#pragma HLS UNROLL
		std::size_t grp = idx >> l;
		std::size_t top = idx & mask;
		std::size_t base = top + (grp<<(l + 1));

		data_t<DTYPE> i = {in[base], in[base + stride]};
		DTYPE c = cos((M_PI*-2*top)/(1<<(l + 1)));
		DTYPE s = sin((M_PI*-2*top)/(1<<(l + 1)));

		data_t<DTYPE> o = FFTOP()({c,s}, i);
		OUT[base]= o.first;
		OUT[base + stride] = o.second;
	}
	return OUT;
}

std::array<std::complex<DTYPE>, 2*LIST_LENGTH> hw_synth_nptfft(std::array<std::complex<DTYPE>, LIST_LENGTH> L, std::array<std::complex<DTYPE>, LIST_LENGTH> R){
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=L._M_instance COMPLETE
#pragma HLS ARRAY_PARTITION variable=R._M_instance COMPLETE
	return NPtFFT<FFTOP>()(L, R);
}

int test_nptfft(){
	std::array<std::complex<DTYPE>, LIST_LENGTH> l, r;
	std::array<std::complex<DTYPE>, 2*LIST_LENGTH> fout, iout;
	for(int i = 0; i < LIST_LENGTH; ++i){
		l[i] = r[i] = {(DTYPE)(i + 1), (DTYPE)(i + 1)};
	}

	fout = hw_synth_nptfft(l, r);
	iout = hw_synth_for_nptfft(l, r);
	for(int i = 0; i < 2*LIST_LENGTH; i ++){
		if(std::abs(float(fout[i].real() - iout[i].real())) > 1){
			fprintf(stderr, "Error! Real NPtFFT Values at index %d did not match\n", i);
			return -1;
		}
		if(std::abs(float(fout[i].imag() - iout[i].imag())) > 1){
			fprintf(stderr, "Error! Real NPtFFT Values at index %d did not match\n", i);
			return -1;
		}
	}
	printf("NPtFFT test passed!\n");
	return 0;
}

int main(){
	int err;
	if((err = test_nptfft())){
		return err;
	}
	if((err = test_fft())){
		return err;
	}
	return 0;	
}

