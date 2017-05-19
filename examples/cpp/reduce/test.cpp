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
#include <array>
#include "listops.hpp"
#include "hof.hpp"
#include "reduce.hpp"
#include "zip.hpp"
#include "testops.hpp"
#ifdef BIT_ACCURATE
#include "ap_int.h"
#endif

#define MULTCONST 35
#define LOG_LIST_LENGTH 6
#define LIST_LENGTH (1<<LOG_LIST_LENGTH)

// -------------------- Min --------------------
class Min{
public:
	int operator()(std::array<int, 1> L, std::array<int, 1> R){
		return L[0] < R[0] ? L[0] : R[0];
	}
	int operator()(int L, int R){
		return L < R ? L : R;
	}
};

int hw_synth_for_min(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS PIPELINE
	int gold;
	for(int i = 0; i < LIST_LENGTH; ++i){
#pragma HLS UNROLL
		if(gold > IN[i]){
			gold = IN[i];
		}
	}
	return gold;
}

int hw_synth_reduce_min(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS PIPELINE
	return reduce<Min>(1001, IN);
}


int hw_synth_rreduce_min(std::array<int, LIST_LENGTH> IN){
#pragma HLS PIPELINE
	return rreduce<Min>(IN, 1001);
}

int test_min(){
	int output = 0, gold = 1001, init = 0;
	std::array<int, LIST_LENGTH> in = genarr<-1000, 1000, LIST_LENGTH>();
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(gold > in[i]){
			gold = in[i];
		}
	}

	output = hw_synth_reduce_min(in);
	if(output != gold){
		fprintf(stderr, "Error! Min (reduce) returned the incorrect value. Output: %d, Gold: %d\n", output, gold);
		return -1;
	}
	printf("Min (reduce) Test Passed!\n");
	
	output = hw_synth_rreduce_min(in);
	if(output != gold){
		fprintf(stderr, "Error! Min (rreduce) returned the incorrect value. Output: %d, Gold: %d\n", output, gold);
		return -1;
	}
	printf("Min (rreduce) Test Passed!\n");

	output = hw_synth_for_min(in);
	if(output != gold){
		fprintf(stderr, "Error! Min (for) returned the incorrect value. Output: %d, Gold: %d\n", output, gold);
		return -1;
	}

	printf("Min (for) Test Passed!\n");
	return 0;
}
// -------------------- End Min --------------------

// -------------------- Add --------------------
class Add{
public:
	template <typename T>
	T operator()(std::array<T, 1> L, std::array<T, 1> R){
		return L[0] + R[0];
	}
	template <typename T>
	T operator()(T L, T R){
		return L + R;
	}
};

float hw_synth_reduce_add(std::array<float, LIST_LENGTH> IN){
#pragma HLS PIPELINE
	return reduce<Add>(0.0f, IN);
}

float hw_synth_rreduce_add(std::array<float, LIST_LENGTH> IN){
#pragma HLS PIPELINE
	return rreduce<Add>(IN, 0.0f);
}

float hw_synth_for_add(std::array<float, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS PIPELINE
	float out = 0;
	for(int i = 0; i < LIST_LENGTH; ++i){
#pragma HLS UNROLL
		out += IN[i];
	}
	return out;
}

float test_sum(){
	float output = 0, gold = 0, init = 0;
	std::array<int, LIST_LENGTH> input = genarr<-1000, 1000, LIST_LENGTH>();
	std::array<float, LIST_LENGTH> in;
	for(int i = 0; i < LIST_LENGTH; ++i){
		gold += input[i];
		in[i] = input[i];
	}
	
	output = hw_synth_rreduce_add(in);
	if(output != gold){
		fprintf(stderr, "Error! Sum (rreduce) returned the incorrect value. Output: %f, Gold: %f\n", output, gold);
		return -1;
	}
	printf("Sum (rreduce) Test Passed!\n");

	output = hw_synth_reduce_add(in);
	if(output != gold){
		fprintf(stderr, "Error! Sum (reduce) returned the incorrect value. Output: %f, Gold: %f\n", output, gold);
		return -1;
	}
	printf("Sum (reduce) Test Passed!\n");

	output = hw_synth_for_add(in);
	if(output != gold){
		fprintf(stderr, "Error! Sum (for) returned the incorrect value. Output: %f, Gold: %f\n", output, gold);
		return -1;
	}
	printf("Sum (for) Test Passed!\n");
	
	return 0;
}
// -------------------- End Add --------------------

// -------------------- Map --------------------
template <class FTOR>
class RMap{
public:
	template<typename TA, typename TI, std::size_t LEN>
	auto operator()(TA CUR, std::array<TI, LEN> INIT) 
		-> std::array<decltype(FTOR()(CUR)), LEN+1> {
#pragma HLS ARRAY_PARTITION complete VARIABLE=INIT._M_instance
#pragma HLS INLINE
		return cons(FTOR()(CUR), INIT);
	}
};

template <class FTOR>
class Map{
public:
	template<typename TA, typename TI, std::size_t LEN>
	auto operator()(std::array<TI, LEN> INIT, TA CUR) 
		-> std::array<decltype(FTOR()(CUR)), LEN+1> {
#pragma HLS ARRAY_PARTITION complete VARIABLE=INIT._M_instance
#pragma HLS INLINE
		return rcons(INIT, FTOR()(CUR));
	}
};

template<int VAL>
class MultBy{
public:
	float operator()(float IN){
#pragma HLS PIPELINE
#pragma HLS INLINE
		return VAL*IN;
	}
};

std::array<float, LIST_LENGTH> hw_synth_rreduce_map(std::array<float, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS PIPELINE
	std::array<float, 0> init;
	return rreduce<RMap<MultBy<MULTCONST>>>(IN, init);
}

std::array<float, LIST_LENGTH> hw_synth_reduce_map(std::array<float, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS PIPELINE
	std::array<float, 0> init;
	return reduce<Map<MultBy<MULTCONST>>>(init, IN);
}


std::array<float, LIST_LENGTH> hw_synth_for_map(std::array<float, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS PIPELINE
	std::array<float, LIST_LENGTH> out;
#pragma HLS ARRAY_PARTITION complete VARIABLE=out._M_instance
	for(int i = 0; i < LIST_LENGTH; ++i){
		out[i] = MultBy<MULTCONST>()(IN[i]);
	}
	return out;
}

int test_map(){
	std::array<float, LIST_LENGTH> gold, in, out;
	std::array<int, LIST_LENGTH> input = genarr<-1000, 1000, LIST_LENGTH>();
	for(int i = 0; i < LIST_LENGTH; ++i){
		gold[i] = MULTCONST*input[i];
		in[i] = input[i];
	}

	out = hw_synth_rreduce_map(in);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(gold[i] != out[i]){
			fprintf(stderr, "Error! Map (rreduce) returned the incorrect value. Output: %f, Gold: %f\n", out[i], gold[i]);
			return -1;
		}
	}
	printf("Map (rreduce) Test Passed!\n");

	out = hw_synth_reduce_map(in);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(gold[i] != out[i]){
			fprintf(stderr, "Error! Map (reduce) returned the incorrect value. Output: %f, Gold: %f\n", out[i], gold[i]);
			return -1;
		}
	}
	printf("Map (reduce) Test Passed!\n");

	out = hw_synth_for_map(in);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(gold[i] != out[i]){
			fprintf(stderr, "Error! Map (for) returned the incorrect value. Output: %f, Gold: %f\n", out[i], gold[i]);
			return -1;
		}
	}

	printf("Map (for) Test Passed!\n");
	return 0;
}
// -------------------- End Map --------------------

// -------------------- Begin Reverse --------------------
std::array<int, LIST_LENGTH> hw_synth_reduce_reverse(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
#pragma HLS PIPELINE
	std::array<int, 0> init;
	return reduce<Flip<Cons>>(init, IN);
}

std::array<int, LIST_LENGTH> hw_synth_rreduce_reverse(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
#pragma HLS PIPELINE
	std::array<int, 0> init;
	return rreduce<Flip<Rcons>>(IN, init);
}

std::array<int, LIST_LENGTH> hw_synth_for_reverse(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
#pragma HLS PIPELINE
	std::array<int, LIST_LENGTH> out;
#pragma HLS ARRAY_PARTITION complete VARIABLE=out._M_instance
	for(int i = 0; i < LIST_LENGTH; ++i){
		out[i] = IN[LIST_LENGTH - i - 1];
	}
	return out;
}

int test_reverse(){
	std::array<int, LIST_LENGTH> out, in = genarr<-1000, 1000, LIST_LENGTH>();

	out = hw_synth_reduce_reverse(in);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(out[i] != in[LIST_LENGTH - i - 1]){
			fprintf(stderr, "Error! Reverse (reduce) returned the incorrect value at index %d: Output: %d, Input: %d\n", i, out[i], in[LIST_LENGTH-i-1]);
			return -1;
		}
	}
	printf("Reverse (reduce) Test Passed!\n");

	out = hw_synth_rreduce_reverse(in);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(out[i] != in[LIST_LENGTH - i - 1]){
			fprintf(stderr, "Error! Reverse (rreduce) returned the incorrect value at index %d: Output: %d, Input: %d\n", i, out[i], in[LIST_LENGTH-i-1]);
			return -1;
		}
	}
	printf("Reverse (rreduce) Test Passed!\n");

	out = hw_synth_for_reverse(in);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(out[i] != in[LIST_LENGTH - i - 1]){
			fprintf(stderr, "Error! Reverse (for) returned the incorrect value at index %d: Output: %d, Input: %d\n", i, out[i], in[LIST_LENGTH-i-1]);
			return -1;
		}
	}

	printf("Reverse (for) Test Passed!\n");
	return 0;
}
// -------------------- End Reverse --------------------

// -------------------- Begin Interleave --------------------
class Interleave{
public:
	template <typename T, std::size_t LEN>
	std::array<T, 2*LEN> operator()(std::array<T, LEN> L, std::array<T, LEN> R){
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=R._M_instance
		std::array<T, 0> init;
		return rreduce<Merge>(zipWith<Array>(L, R), init);
	}
};

template <typename T, std::size_t LEN>
std::array<T, 2*LEN> interleave(std::array<T, LEN> L, std::array<T, LEN> R){
	std::array<T, 2*LEN> out;
#pragma HLS ARRAY_PARTITION complete VARIABLE=out._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=R._M_instance
#pragma HLS INLINE
	for(int i = 0; i < LEN; ++i){
#pragma HLS UNROLL
		out[2*i    ] = L[i];
		out[2*i + 1] = R[i];
	}
	return out;
}


std::array<std::size_t, 2*LIST_LENGTH> hw_synth_rreduce_interleave(std::array<std::size_t, LIST_LENGTH> L, std::array<std::size_t, LIST_LENGTH> R){
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=R._M_instance
#pragma HLS INLINE
	std::array<std::size_t, 0> init; 
	return rreduce<Merge>(zipWith<Array>(L, R), init);
}

std::array<std::size_t, 2*LIST_LENGTH> hw_synth_reduce_interleave(std::array<std::size_t, LIST_LENGTH> L, std::array<std::size_t, LIST_LENGTH> R){
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=R._M_instance
#pragma HLS INLINE
	std::array<std::size_t, 0> init; 
	return reduce<Merge>(init, zipWith<Array>(L, R));
}

std::array<std::size_t, 2*LIST_LENGTH> hw_synth_for_interleave(std::array<std::size_t, LIST_LENGTH> L, std::array<std::size_t, LIST_LENGTH> R){
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=R._M_instance
#pragma HLS INLINE
	return interleave(L, R);
}

int test_interleave(){
	std::array<std::size_t, LIST_LENGTH> l = range<LIST_LENGTH>(), r = range<LIST_LENGTH>();;
	auto out = hw_synth_rreduce_interleave(l, r);
	for(int i =  0; i < 2*LIST_LENGTH; ++i){
		if(out[i] != l[i/2]){
			fprintf(stderr, "Error! Interleave (rreduce) did not produce correct value at index %d\n", i);
			return -1;
		}
	}
	printf("Interleave (rreduce) Test Passed!\n");

	out = hw_synth_reduce_interleave(l, r);
	for(int i =  0; i < 2*LIST_LENGTH; ++i){
		if(out[i] != l[i/2]){
			fprintf(stderr, "Error! Interleave (rreduce) did not produce correct value at index %d\n", i);
			return -1;
		}
	}
	printf("Interleave (reduce) Test Passed!\n");

	out = hw_synth_for_interleave(l, r);
	for(int i =  0; i < 2*LIST_LENGTH; ++i){
		if(out[i] != l[i/2]){
			fprintf(stderr, "Error! Interleave (for) did not produce correct value at index %d\n", i);
			return -1;
		}
	}
	printf("Interleave (for) Test Passed!\n");
	return 0;
}

// -------------------- End Interleave --------------------

int main(){
	int err;
	if((err = test_sum())){
		return err;
	}

	if((err = test_min())){
		return err;
	}

	if((err = test_map())){
		return err;
	}

	if((err = test_reverse())){
		return err;
	}

	if((err = test_interleave())){
		return err;
	}
	
	printf("Reduce/rreduce Tests passed\n");
	return 0;	
}

