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
#ifndef __ZIPPER_HPP
#define __ZIPPER_HPP
#include <utility>
#include "listops.hpp"
template <class TL, class TR, std::size_t LEN>
auto unzip(const std::array<std::pair<TL, TR>, LEN> IN) ->
	std::pair<std::array<TL, LEN>, std::array<TR, LEN> >{
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
	std::array<TL, LEN> left, right;
#pragma HLS ARRAY_PARTITION complete VARIABLE=left._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=right._M_instance
#pragma HLS INLINE
unzip_loop:
	for(int i = 0; i < LEN; ++i){
#pragma HLS UNROLL
		left[i] = IN[i].first;
		right[i] = IN[i].second;
		//TODO: std::tie(left[i], right[i]) = IN[i];
	}
	return {left, right};
}

struct Unzip{
	template <class TL, class TR, std::size_t LEN>
	auto operator ()(const std::array<std::pair<TL, TR>, LEN> IN) ->decltype(unzip(IN)){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
		return unzip(IN);
	}
};

template <class TL, class TR, std::size_t LEN>
auto zip(const std::array<TL, LEN>& L, const std::array<TR, LEN>& R) -> std::array<std::pair<TL, TR>, LEN>{
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=R._M_instance
#pragma HLS INLINE
	std::array<std::pair<TL, TR>, LEN> temp;
#pragma HLS ARRAY_PARTITION complete VARIABLE=temp._M_instance
zip_loop:
	for(int i = 0; i < LEN; ++i){
#pragma HLS UNROLL
		temp[i] = {L[i], R[i]};
	}
	return temp;
}

struct Zip{
	template <class TL, class TR, std::size_t LEN>
	auto operator ()(const std::array<TL, LEN>& L, const std::array<TR, LEN>& R) -> decltype(zip(L, R)) {
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=R._M_instance
#pragma HLS INLINE
		return zip(L, R);
	}
};

template <class FTOR, class TL, class TR, std::size_t LEN>
auto zipWith(const std::array<TL, LEN>& L, const std::array<TR, LEN>& R) -> std::array<decltype(FTOR()(L[0], R[0])), LEN>{
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=R._M_instance
#pragma HLS INLINE
	std::array<decltype(FTOR()(L[0], R[0])), LEN> temp;
#pragma HLS ARRAY_PARTITION complete VARIABLE=temp._M_instance
zipWith_loop:
	for(int i = 0; i < LEN; ++i){
#pragma HLS UNROLL
		temp[i] = FTOR()(L[i], R[i]);
	}
	return temp;
}

template <class FTOR>
struct ZipWith{
	template <class TL, class TR, std::size_t LEN>
	auto operator ()(const std::array<TL, LEN>& L, const std::array<TR, LEN>& R) -> decltype(zipWith<FTOR>(L, R)) {
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=R._M_instance
#pragma HLS INLINE
		return zipWith<FTOR>(L, R);
	}
};

#endif // __ZIPPER_HPP
