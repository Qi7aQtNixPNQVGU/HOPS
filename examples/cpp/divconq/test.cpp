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
#include "listops.hpp"
#include "hof.hpp"
#include "divconq.hpp"
#include "reduce.hpp"
#include "map.hpp"
#include "zip.hpp"
#include "testops.hpp"
#ifdef BIT_ACCURATE
#include "ap_int.h"
#endif

#define MULTCONST 35
#define LOG_LIST_LENGTH 6
#define LIST_LENGTH (1<<LOG_LIST_LENGTH)

template<class FTOR, typename TA, std::size_t LEN>
TA fdivconq(const std::array<TA, LEN>& IN){
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
	static const std::size_t LEV = clog2(LEN);
	std::array<std::array<TA, (1<<LEV)>, LEV+1> temp;
#pragma HLS ARRAY_PARTITION complete VARIABLE=temp[0]._M_instance
	for(std::size_t idx = 0; idx < (1<<LEV); ++idx){
#pragma HLS UNROLL
		temp[0][idx] = IN[idx];
	}
	for(std::size_t lev = 1; lev <= LEV; ++lev){
#pragma HLS UNROLL
		for(std::size_t idx = 0; idx < 1<<(LEV-lev); ++idx){
#pragma HLS UNROLL
			temp[lev][idx] = FTOR()(temp[lev-1][2*idx], temp[lev-1][2*idx+1]);
		}
	}
	return temp[LEV][0];
}

class Fdivconq{
public:
	template<class FTOR, typename TA, std::size_t LEN>
	TA operator()(const std::array<TA, LEN>& IN){
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
		fdivconq<FTOR>(IN);
	}
};


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

int hw_synth_divconq_min(std::array<int, LIST_LENGTH>& IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS PIPELINE
	return divconq<Min>(IN);
}

int hw_synth_for_min(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS PIPELINE
	return fdivconq<Min>(IN);
}

int test_min(){
	int output = 0, gold = 1001, init = 0;
	std::array<int, LIST_LENGTH> in = genarr<-1000, 1000, LIST_LENGTH>();
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(gold > in[i]){
			gold = in[i];
		}
	}
	
	output = hw_synth_divconq_min(in);
	if(output != gold){
		fprintf(stderr, "Error! Min (divconq) returned the incorrect value. Output: %d, Gold: %d\n", output, gold);
		return -1;
	}
	printf("Min (divconq) test passed!\n");

	output = hw_synth_for_min(in);
	if(output != gold){
		fprintf(stderr, "Error! Min (for) returned the incorrect value. Output: %d, Gold: %d\n", output, gold);
		return -1;
	}
	printf("Min (for) test passed!\n");

	printf("Min Tests Passed!\n");
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

float hw_synth_divconq_add(std::array<float, LIST_LENGTH>& in){
#pragma HLS ARRAY_PARTITION complete VARIABLE=in._M_instance
#pragma HLS PIPELINE
	return divconq<Add>(in);
}

float hw_synth_for_add(std::array<float, LIST_LENGTH>& in){
#pragma HLS ARRAY_PARTITION complete VARIABLE=in._M_instance
#pragma HLS PIPELINE
	return fdivconq<Add>(in);
}

int test_sum(){
	float output = 0, gold = 0, init = 0;
	std::array<int, LIST_LENGTH> input = genarr<-1000, 1000, LIST_LENGTH>();
	std::array<float, LIST_LENGTH> in;
	for(int i = 0; i < LIST_LENGTH; ++i){
		gold += input[i];
		in[i] = input[i];
	}
	
	output = hw_synth_divconq_add(in);
	if(output != gold){
		fprintf(stderr, "Error! Sum (reduce) returned the incorrect value. Output: %f, Gold: %f\n", output, gold);
		return -1;
	}
	printf("Sum (divconq) test passed!\n");

	output = hw_synth_for_add(in);
	if(output != gold){
		fprintf(stderr, "Error! Sum (for) returned the incorrect value. Output: %f, Gold: %f\n", output, gold);
		return -1;
	}
	printf("Sum (for) test passed!\n");
	
	printf("Sum Tests Passed!\n");
	return 0;
}
// -------------------- End Add --------------------

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
// -------------------- End Interleave --------------------

// -------------------- Begin Bit-reverse --------------------

std::array<std::size_t, LIST_LENGTH> hw_synth_divconq_bit_reverse(std::array<std::size_t, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
	return divconq<Interleave>(IN);
}


template<std::size_t LEN>
std::size_t bit_reverse(std::size_t i){
#pragma HLS INLINE
	int b = 0;
	static const std::size_t W = clog2(LEN);
	for(int j = 0; j < W; j++){
#pragma HLS_UNROLL
		b = b << 1;
		b |= ((i >> j) &1);
	}
	return b;
}

template<typename T, std::size_t LEN>
std::array<T, LEN> bit_reverse(std::array<T, LEN> IN){
	std::array<T, LEN> OUT;
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=OUT._M_instance
#pragma HLS INLINE
	for(std::size_t t = 0; t < LIST_LENGTH; t ++){
#pragma HLS UNROLL
		T temp;
		unsigned int b = bit_reverse<LEN>(t);
		OUT[t] = IN[b];
	}
	return OUT;
}

std::array<std::size_t, LIST_LENGTH> hw_synth_for_bit_reverse(std::array<std::size_t, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
	return bit_reverse(IN);
}


int test_bit_reverse(){
	auto in = range<LIST_LENGTH>();
	std::array<std::size_t, LIST_LENGTH> gold = bit_reverse(in);
	std::array<std::size_t, LIST_LENGTH> out;

	out = hw_synth_divconq_bit_reverse(in);
	for(int i= 0; i < LIST_LENGTH; ++i){
		if(out[i] != gold[i]){
			fprintf(stderr, "Error! Bit reverse (divconq) did not produce correct value at index %d\n", i);
			return -1;
		}
	}
	printf("Bit-Reverse (divconq) test passed!\n");

        out = hw_synth_for_bit_reverse(in);
	for(int i= 0; i < LIST_LENGTH; ++i){
		if(out[i] != gold[i]){
			fprintf(stderr, "Error! Bit reverse (for) did not produce correct value at index %d\n", i);
			return -1;
		}
	}
	printf("Bit-Reverse (for) test passed!\n");

	printf("Bit-Reverse Tests Passed!\n");
	return 0;
}

// -------------------- End Bit-Reverse --------------------
// -------------------- Begin Argmin/Argmax --------------------
template <typename T>
struct didx_t{
	T data;
	std::size_t idx, lev;
	didx_t<T> operator()(T data){
		return {data, 0, 0};
	}
};

template <typename T>
using argt = std::array<didx_t<T>, 1>;

class Argminop{
public:
	template <typename T>
	didx_t<T> operator()(didx_t<T> const& L, didx_t<T> const& R){
		bool b = R.data < L.data;
		std::size_t lev = L.lev;
		didx_t<T> result = {b ? R.data : L.data,
				    ((b & 1) << lev) | (b ? R.idx : L.idx),
				    lev +1};
		return result;
	}

	template <typename T>
	argt<T> operator()(argt<T> const& L, argt<T> const& R){
		return {this->operator()(L[0], R[0])};
	}
};

std::pair<int, std::size_t> hw_synth_divconq_argmin(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION variable=IN complete
#pragma HLS INLINE
#pragma HLS PIPELINE
	argt<int> out = divconq<Argminop>(map<didx_t<int>>(IN));
	return {out[0].data, out[0].idx};
}

std::pair<int, std::size_t> hw_synth_for_argmin(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION variable=IN complete
#pragma HLS INLINE
#pragma HLS PIPELINE
	didx_t<int> out = fdivconq<Argminop>(map<didx_t<int>>(IN));
	return {out.data, out.idx};
}

class Argmaxop{
public:
	template <typename T>
	didx_t<T> operator()(didx_t<T> const& L, didx_t<T> const& R){
		bool b = R.data > L.data;
		std::size_t lev = L.lev;
		didx_t<T> result = {b ? R.data : L.data,
				    ((b & 1) << lev) | (b ? R.idx : L.idx),
				    lev +1};
		return result;
	}

	template <typename T>
	argt<T> operator()(argt<T> const& L, argt<T> const& R){
		return {this->operator()(L[0], R[0])};
	}
};


std::pair<int, std::size_t> hw_synth_divconq_argmax(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION variable=IN complete
#pragma HLS INLINE
#pragma HLS PIPELINE
	argt<int> out = divconq<Argmaxop>(map<didx_t<int>>(IN));
	return {out[0].data, out[0].idx};
}

std::pair<int, std::size_t> hw_synth_for_argmax(std::array<int, LIST_LENGTH> IN){
#pragma HLS ARRAY_PARTITION variable=IN complete
#pragma HLS INLINE
#pragma HLS PIPELINE
	didx_t<int> out = fdivconq<Argmaxop>(map<didx_t<int>>(IN));
	return {out.data, out.idx};
}

int test_argmin(){
	std::pair<int, std::size_t> output;
	int gold = 1001, init = 0;
	std::size_t gold_idx = 0;
	std::array<int, LIST_LENGTH> in = genarr<-1000, 1000, LIST_LENGTH>();
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(in[i] < gold){
			gold = in[i];
			gold_idx = i;
		}
	}

	output = hw_synth_divconq_argmin(in);
	if((output.first != gold) || (output.second != gold_idx)){
		fprintf(stderr, "Error! Argmin (divconq) returned the incorrect value. Output: (%d, %d), Gold: (%d, %d)\n", output.first, (int)output.second, gold, (int)gold_idx);
		return -1;
	}
	printf("Argmin (divconq) test passed!\n");

	output = hw_synth_for_argmin(in);
	if((output.first != gold) || (output.second != gold_idx)){
		fprintf(stderr, "Error! Argmin (for) returned the incorrect value. Output: (%d, %d), Gold: (%d, %d)\n", output.first, (int)output.second, gold, (int)gold_idx);
		return -1;
	}
	printf("Argmin (for) test passed!\n");

	printf("Argmin Tests Passed!\n");
	return 0;
}

int test_argmax(){
	std::pair<int, std::size_t> output;
	int gold = -1001, init = 0;
	std::size_t gold_idx = 0;
	std::array<int, LIST_LENGTH> in = genarr<-1000, 1000, LIST_LENGTH>();
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(in[i] > gold){
			gold = in[i];
			gold_idx = i;
		}
	}

	output = hw_synth_divconq_argmax(in);
	if((output.first != gold) || (output.second != gold_idx)){
		fprintf(stderr, "Error! Argmax (divconq) returned the incorrect value. Output: (%d, %d), Gold: (%d, %d)\n", output.first, (int)output.second, gold, (int)gold_idx);
		return -1;
	}
	printf("Argmax (divconq) test passed!\n");

	output = hw_synth_for_argmax(in);
	if((output.first != gold) || (output.second != gold_idx)){
		fprintf(stderr, "Error! Argmax (for) returned the incorrect value. Output: (%d, %d), Gold: (%d, %d)\n", output.first, (int)output.second, gold, (int)gold_idx);
		return -1;
	}
	printf("Argmax (for) test passed!\n");

	printf("Argmax Tests Passed!\n");
	return 0;
}
// -------------------- Begin Argmin/Argmax --------------------


int main(){
	int err;
	if((err = test_sum())){
		return err;
	}

	if((err = test_min())){
		return err;
	}

	if((err = test_bit_reverse())){
		return err;
	}

	if((err = test_argmin())){
		return err;
	}

	if((err = test_argmax())){
		return err;
	}
	
	printf("Divconq tests passed\n");
	return 0;	
}

