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
#include <utility>
#include <array>
#include "listops.hpp"
#include "testops.hpp"
#include "zip.hpp"
#include "map.hpp"
#include "hof.hpp"
#define LOG_LIST_LENGTH 6
#define LIST_LENGTH (1<<LOG_LIST_LENGTH)

class Add{
public:
	int operator()(int L, int R){
#pragma HLS INLINE
		return L + R;
	}
};

std::array<int, LIST_LENGTH> hw_synth_zipwith(std::array<int, LIST_LENGTH> &L, std::array<int, LIST_LENGTH> R){
#pragma HLS ARRAY_PARTITION variable=R complete
#pragma HLS ARRAY_PARTITION variable=L complete
#pragma HLS INLINE
#pragma HLS PIPELINE
	return zipWith<Add>(L, R);
}

std::array<std::pair<int, int>, LIST_LENGTH> hw_synth_zip(std::array<int, LIST_LENGTH> &L, std::array<int, LIST_LENGTH> R){
#pragma HLS ARRAY_PARTITION variable=R complete
#pragma HLS ARRAY_PARTITION variable=L complete
#pragma HLS INLINE
#pragma HLS PIPELINE
	return zip(L, R);
}

std::array<int, LIST_LENGTH> hw_synth_zip_add(std::array<int, LIST_LENGTH> &L, std::array<int, LIST_LENGTH> R){
#pragma HLS ARRAY_PARTITION variable=R complete
#pragma HLS ARRAY_PARTITION variable=L complete
#pragma HLS INLINE
#pragma HLS PIPELINE
	return map<Unpair<Add>>(zip(L, R));
}

std::array<int, 2* LIST_LENGTH> hw_synth_unzip(std::array<std::pair<int, int>, LIST_LENGTH> &IN){
#pragma HLS ARRAY_PARTITION variable=IN complete
#pragma HLS INLINE
#pragma HLS PIPELINE
	auto uzip = unzip(IN);
	return uzip.first + uzip.second;
}

int test_zip(){
	std::array<int, LIST_LENGTH> left = genarr<-1000, 1000, LIST_LENGTH>();
	std::array<int, LIST_LENGTH> right = genarr<-1000, 1000, LIST_LENGTH>();

	std::array<std::pair<int, int>, LIST_LENGTH> output;
	output = hw_synth_zip(left, right);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(left[i] != output[i].first){
			printf("Error! Zip output incorrect at index %d\n", i);
			return -1;
		}
		if(right[i] != output[i].second){
			printf("Error! Zip output incorrect at index %d\n", i);
			return -1;
		}
	}
	printf("Zip Test Passed!\n");
	return 0;
}
int test_zipwith(){
	std::array<int, LIST_LENGTH> left = genarr<-1000, 1000, LIST_LENGTH>();
	std::array<int, LIST_LENGTH> right = genarr<-1000, 1000, LIST_LENGTH>();

	std::array<int, LIST_LENGTH> output;
	output = hw_synth_zipwith(left, right);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(left[i] + right[i] != output[i]){
			printf("Error! Zip (map-unpair-add) output incorrect at index %d\n", i);
			return -1;
		}
	}

	printf("ZipWith Add Test Passed!\n");
	return 0;
}
int test_zip_add(){
	std::array<int, LIST_LENGTH> left = genarr<-1000, 1000, LIST_LENGTH>();
	std::array<int, LIST_LENGTH> right = genarr<-1000, 1000, LIST_LENGTH>();
	std::array<int, LIST_LENGTH> output;
	output = hw_synth_zip_add(left, right);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(left[i] + right[i] != output[i]){
			printf("Error! Zip (map-unpair-add) output incorrect at index %d\n", i);
			return -1;
		}
	}

	printf("Zip Add Test Passed!\n");
	return 0;
}

int test_unzip(){
	std::array<int, LIST_LENGTH> left = genarr<-1000, 1000, LIST_LENGTH>();
	std::array<int, LIST_LENGTH> right = genarr<-1000, 1000, LIST_LENGTH>();
	std::array<std::pair<int, int>, LIST_LENGTH> in = zip(left, right);
	auto output = hw_synth_unzip(in);
	for(int i = 0; i < LIST_LENGTH; ++i){
		if(left[i] != output[i]){
			printf("Error! unzip output incorrect at index %d\n", i);
			return -1;
		}
		if(right[i] != output[LIST_LENGTH + i]){
			printf("Error! unzip output incorrect at index %d\n", i);
			return -1;
		}
	}

	printf("Unzip Test Passed!\n");
	return 0;
}

int main(){
	int err;
	if((err = test_zip_add())){
		return err;
	}
	if((err = test_zip())){
		return err;
	}
	if((err = test_zipwith())){
		return err;
	}
	if((err = test_unzip())){
		return err;
	}
	printf("Zip Tests passed\n");
	return 0;	
}

