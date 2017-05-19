// ----------------------------------------------------------------------
// Copyright (c) 2016, The Regents of the University of California All
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
#include <array>
#include "listops.hpp"
#include "hof.hpp"
#include "map.hpp"
#include "zip.hpp"
#include "testops.hpp"
#ifdef BIT_ACCURATE
#include "ap_int.h"
#endif
#define LOG_LIST_LENGTH 6
#define LIST_LENGTH (1<<LOG_LIST_LENGTH)

class Truncate{
public:
	char operator()(int const& i){
#pragma HLS INLINE
		return (char)i;
	}
};

unsigned int breverse(unsigned int const& i){
#pragma HLS INLINE
	unsigned int o = 0;
	for(int j = 0; j < LOG_LIST_LENGTH; ++j){
#pragma HLS UNROLL
		o = (o << 1) | ((i >> j)&1);
	}
	return o;
}
template <std::size_t LEN>
struct Bitreverse{
	static constexpr std::size_t breverse(std::size_t v){
		return (v & 1) << (LEN-1) | Bitreverse<LEN-1>::breverse(v>>1);
	}
};
template <>
struct Bitreverse<0>{
	static constexpr std::size_t breverse(std::size_t v){
		return 0;
	}
};
template <std::size_t LEN>
constexpr std::size_t bitreverse(std::size_t v){
	return Bitreverse<LEN>::breverse(v);
}

class Breverse{
public:
	unsigned int operator()(unsigned int const& v){
#pragma HLS INLINE
		return bitreverse<LOG_LIST_LENGTH>(v);
	}
};

std::array<char, LIST_LENGTH> hw_synth_trunc(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
	return map<Truncate>(IN);
}

std::array<unsigned int, LIST_LENGTH> hw_synth_breverse(std::array<unsigned int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
	return map<Breverse>(IN);
}

template <int VAL>
class MultBy{
public:
	float operator()(float const& IN){
#pragma HLS INLINE
		return VAL*IN;
	}
};

std::array<float, LIST_LENGTH> hw_synth_multby(std::array<float, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS PIPELINE
	return map<MultBy<35>>(IN);
}

struct Mult{
	float operator()(float L, float R){
#pragma HLS INLINE
		return L * R;
	}
};

int test_truncate(){
	std::array<int, LIST_LENGTH> in = genarr<-1000, 1000, LIST_LENGTH>();
	auto output = hw_synth_trunc(in);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if((char)(in[i] & 0xff) != output[i]){
			fprintf(stderr, "Error (functional)! Value at index %d was not truncated correctly\n", i);
			return -1;
		}
	}
	
	printf("Truncate Test Passed!\n");
	return 0;
}

int test_multby(){
	std::array<float, LIST_LENGTH> in;
	for(int i = 0; i < LIST_LENGTH; ++i){
		in[i] = 1.0 * i;
	}

	auto output = hw_synth_multby(in);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(in[i]*35 != output[i]){
			fprintf(stderr, "Error! Value at index %d was not multiplied correctly\n", i);
			return -1;
		}
	}
	
	printf("Multby Test Passed!\n");
	return 0;
}

std::array<float, LIST_LENGTH> hw_synth_pair_mult(std::array<float, LIST_LENGTH> L, std::array<float, LIST_LENGTH> R){
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=R._M_instance
#pragma HLS PIPELINE
	return map<Unpair<Mult>>(zip(L, R));
}

int test_pair_mult(){
	std::array<float, LIST_LENGTH> in;
	for(int i = 0; i < LIST_LENGTH; ++i){
		in[i] = 1.0 * i;
	}

	auto output = hw_synth_pair_mult(in, in);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(in[i]*in[i] != output[i]){
			fprintf(stderr, "Error! Pair-mult value at index %d was not multiplied correctly\n", i);
			return -1;
		}
	}
	
	printf("Pair-wise Multiplication Test Passed!\n");
	return 0;
}

int test_breverse(){
	std::array<unsigned int, LIST_LENGTH> in;
	std::array<unsigned int, LIST_LENGTH> output;

	
	for(int i= 0; i < LIST_LENGTH; i ++){
		in[i] = i*((1<<30)/LIST_LENGTH);
	}
	
	output = hw_synth_breverse(in);
	for(int i= 0; i < LIST_LENGTH; ++i){
		if(output[i] != breverse(in[i])){
			fprintf(stderr, "Error! Value at index %d was not bit-reversed correctly\n", i);
			return -1;
		}
	}
	
	printf("Bit-reverse test passed!\n");
	return 0;
}

int main(){
	int err;
	if((err = test_truncate())){
		return err;
	}
	if((err = test_breverse())){
		return err;
	}
	if((err = test_multby())){
		return err;
	}
	if((err = test_pair_mult())){
		return err;
	}
	printf("Map Tests passed\n");
	return 0;	
}

