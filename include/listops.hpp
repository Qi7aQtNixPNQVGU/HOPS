// ----------------------------------------------------------------------
// Copyright) 2016, The Regents of the University of California All
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
#ifndef __LISTOPS_HPP
#define __LISTOPS_HPP
#include <utility>
#include <array>
#include "constops.hpp"

template<std::size_t STOP, std::size_t START = 0, std::size_t STEP = 1>
auto range() -> std::array<std::size_t, (STOP-START)/STEP>{
#pragma HLS INLINE
	std::array<std::size_t, (STOP-START)/STEP> temp;
#pragma HLS ARRAY_PARTITION complete VARIABLE=temp._M_instance
range_loop: 
	for(std::size_t v = START, idx = 0; v < STOP; v +=STEP, ++idx){
#pragma HLS UNROLL
		temp[idx] = v;
	}
	return temp;
};

struct Range{
	template<int STOP, int START = 0, int STEP = 1>
	auto operator()() -> decltype(range<STOP, START, STEP>()){
#pragma HLS INLINE
		return range<STOP, START, STEP>();
	}
};

template<typename TA, std::size_t LEN>
auto cons(TA const& H, const std::array<TA, LEN>& T) -> std::array<TA, LEN+1>{
#pragma HLS ARRAY_PARTITION complete VARIABLE=T._M_instance
#pragma HLS INLINE
	std::array<TA, LEN+1> temp;
#pragma HLS ARRAY_PARTITION complete VARIABLE=temp._M_instance
	temp[0] = H;
concat_loop:
	for(int idx = 0; idx < LEN; ++idx){
#pragma HLS UNROLL
		temp[idx+1] = T[idx];
	}
	return temp;
};

template<typename TA, std::size_t LEN>
auto reverse(const std::array<TA, LEN>& L) -> std::array<TA, LEN>{
	std::array<TA, LEN> res;
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION complete VARIABLE=L._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=res._M_instance
reverse_loop:
	for(int idx = 0; idx < LEN; ++idx){
#pragma HLS UNROLL
		res[idx] = L[LEN-(idx+1)];
	}
	return res;
};

struct Cons{
	template<typename TA, std::size_t LEN>
	auto operator()(TA const& H, const std::array<TA, LEN>& T) -> decltype(cons(H, T)) {
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION complete VARIABLE=T._M_instance
		return cons(H, T);
	}
};

// How do we make this constexpr? Should we?
template<typename TA, std::size_t LEN>
auto rcons(const std::array<TA, LEN>& T, TA const& H) -> std::array<TA, LEN+1>{
#pragma HLS INLINE
	std::array<TA, LEN+1> temp;
#pragma HLS ARRAY_PARTITION complete VARIABLE=temp._M_instance
rcons_loop:
	for(int idx = 0; idx < LEN; ++idx){
#pragma HLS UNROLL
		temp[idx] = T[idx];
	}
	temp[LEN] = H;
	return temp;
};

struct Rcons{
	template<typename TA, std::size_t LEN>
	auto operator()(const std::array<TA, LEN>& T, TA const& H) -> decltype(rcons(T, H)) {
#pragma HLS INLINE
		return rcons(T, H);
	}
};

// How do we make this constexpr? Should we?
template<typename TA, std::size_t LLEN, std::size_t RLEN>
auto merge(const std::array<TA, LLEN> LIN, const std::array<TA, RLEN> RIN) -> std::array<TA, LLEN+RLEN>{
#pragma HLS ARRAY_PARTITION complete VARIABLE=LIN._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=RIN._M_instance
#pragma HLS INLINE
	std::array<TA, LLEN+RLEN> temp;
#pragma HLS ARRAY_PARTITION complete VARIABLE=temp._M_instance
merge_left_loop:
	for(int idx = 0; idx < LLEN; ++idx){
#pragma HLS UNROLL
		temp[idx] = LIN[idx];
	}
merge_right_loop:
	for(int idx=0; idx < RLEN; ++idx){
#pragma HLS UNROLL
		temp[idx + LLEN] = RIN[idx];
	}
	return temp;
}

template<typename TA, std::size_t LLEN, std::size_t RLEN>
auto operator+(const std::array<TA, LLEN> LIN, const std::array<TA, RLEN> RIN) -> std::array<TA, LLEN+RLEN>{
#pragma HLS ARRAY_PARTITION complete VARIABLE=LIN._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=RIN._M_instance
#pragma HLS INLINE
	return merge(LIN, RIN);
}

struct Merge{
	template<typename TA, std::size_t LLEN, std::size_t RLEN>
	auto operator()(const std::array<TA, LLEN>& LIN, const std::array<TA, RLEN>& RIN) -> decltype(merge(LIN, RIN)){
#pragma HLS ARRAY_PARTITION complete VARIABLE=LIN._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=RIN._M_instance
#pragma HLS INLINE
		return merge(LIN, RIN);
	}
};

// How do we make this constexpr? Should we?
template<std::size_t LEN, typename T>
auto replicate(const T& v) -> std::array<T, LEN>{
#pragma HLS INLINE
	std::array<T, LEN> temp;
#pragma HLS ARRAY_PARTITION complete VARIABLE=temp._M_instance
replicate_loop:
	for(int idx = 0; idx < LEN; ++idx){
#pragma HLS UNROLL
		temp[idx] = v;
	}
	return temp;
};

struct Replicate{
	template<typename T, std::size_t LEN>
	auto operator()(const T& v) -> decltype(replicate<T, LEN>(v)){
#pragma HLS INLINE
		return replicate<T, LEN>(v);
	}
};

// How do we make this constexpr? Should we?
template<std::size_t IDX, typename TA, std::size_t LEN>
auto splitat(const std::array<TA, LEN>& IN) -> std::pair<std::array<TA, (IDX > LEN)? LEN : IDX>, std::array<TA, (IDX > LEN)? 0 : LEN - IDX > >{
#pragma HLS INLINE
	static const std::size_t llen = (IDX > LEN)? LEN : IDX;
	static const std::size_t rlen = (IDX > LEN)? 0 : LEN - IDX;
	std::array<TA, llen> l;
#pragma HLS ARRAY_PARTITION complete VARIABLE=l._M_instance
	std::array<TA, rlen> r;
#pragma HLS ARRAY_PARTITION complete VARIABLE=r._M_instance
	split_left_loop:
	for(int idx = 0; idx < llen; ++idx){
#pragma HLS UNROLL
		l[idx] = IN[idx];
	}
	split_right_loop:
	for(int ridx = llen, idx=0; idx < rlen; ++ridx, ++idx){
#pragma HLS UNROLL
		r[idx] = IN[ridx];
	}
	return {l, r};
};

struct Splitat{
	template<std::size_t IDX, typename TA, std::size_t LEN>
	auto operator()(const std::array<TA, LEN>& IN) -> decltype(splitat<IDX, TA, LEN>(IN)){
#pragma HLS INLINE
		return splitat<IDX, TA, LEN>(IN);
	}
};

template<typename TA, std::size_t LEN>
TA head(const std::array<TA, LEN>& t){
#pragma HLS INLINE
	return t[0];
}

struct Head{
	template<typename TA, std::size_t LEN>
	TA operator()(const std::array<TA, LEN>& t){
#pragma HLS INLINE
		return head(t);
	}
};

// How do we make this constexpr?
template<typename TA, std::size_t LEN>
auto tail(std::array<TA, LEN> const & L) -> std::array<TA, LEN-1>{
#pragma HLS INLINE
	std::array<TA, LEN-1> temp;
#pragma HLS ARRAY_PARTITION complete VARIABLE=temp._M_instance
tail_loop:
	for(int idx = 0; idx < LEN-1; ++idx){
#pragma HLS UNROLL
		temp[idx] = L[idx+1];
	}
	return temp;
};

struct Tail{
	template<typename TA, std::size_t LEN>
	auto operator()(std::array<TA, LEN> const& L) -> decltype(tail(L)){
#pragma HLS INLINE
		return tail(L);
	}
};

template <typename TA, std::size_t LEN> 
std::array<TA, LEN> shiftr (TA const& H, std::array<TA, LEN> const& T){
#pragma HLS INLINE
	return cons(H, splitat<LEN-1>(T).first);
}

struct Shiftr{
	template <typename TA, std::size_t LEN> 
	std::array<TA, LEN> operator()(TA const& H, std::array<TA, LEN> const& T){
#pragma HLS INLINE
		return shiftr(H, T);
	}
};

template <typename TA, std::size_t LEN>
std::array<TA, LEN> shiftl(std::array<TA, LEN> const& T, TA const& H){
#pragma HLS INLINE
	return rcons(tail(T), H);
}

struct Shiftl{
	template <typename TA, std::size_t LEN> 
	std::array<TA, LEN> operator()(std::array<TA, LEN> const& T, TA const& H){
#pragma HLS INLINE
		return shiftl(T, H);
	}
};

struct Array{
	template<typename T>
	std::array<T, 2> operator()(T L, T R){
#pragma HLS INLINE
		return {L, R};
	}

/*
	// Someday we'll be able to use the make_array method...
	template<typename... TS>
	auto operator()(TS... INS) -> decltype(make_array(INS...)){
		return {INS...};
		}*/
};

template<std::size_t LEN, typename T>
auto convert(const T L[LEN]) -> std::array<T, LEN>{
	std::array<T, LEN> temp;
#pragma HLS ARRAY_PARTITION complete VARIABLE=temp._M_instance
	for(int idx = 0; idx < LEN; ++idx){
#pragma HLS UNROLL
		temp[idx] = L[idx];
	}
	return temp;
}


#endif


