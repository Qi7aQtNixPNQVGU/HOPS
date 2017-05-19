// ----------------------------------------------------------------------
// Copyright (c) 2017, The Regents of the University of California All
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
#ifndef __REDUCE_HPP
#define __REDUCE_HPP
#include <array>
#include "listops.hpp"
template <class FTOR, std::size_t LEV>
struct _rHelp{
	template<typename TI, typename TA>
	static auto reduce(TI const& INIT, std::array<TA, LEV> const& IN)
		-> decltype(_rHelp<FTOR, LEV-1>().reduce(FTOR()(INIT, head(IN)), tail(IN))){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
		return _rHelp<FTOR, LEV-1>::reduce(FTOR()(INIT, head(IN)), tail(IN));
	}

	template<typename TI, typename TA>
	static auto rreduce(std::array<TA, LEV> const& IN, TI const& INIT)
		-> decltype(FTOR()(head(IN), _rHelp<FTOR, LEV-1>().rreduce(tail(IN), INIT))){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
		return FTOR()(head(IN), _rHelp<FTOR, LEV-1>::rreduce(tail(IN), INIT));
	}
};

template <class FTOR>
struct _rHelp<FTOR, 0>{
	template<typename TI, typename TA>
	static auto reduce(TI const& INIT, std::array<TA, 0> const& IN) -> TI{
#pragma HLS INLINE
		return INIT;
	}

	template<typename TI, typename TA>
	static auto rreduce(std::array<TA, 0> const& IN, TI const& INIT) -> TI{
#pragma HLS INLINE
		return INIT;
	}
};

template <class FTOR, typename TI, typename TA, std::size_t LEN>
auto reduce(TI const& INIT, std::array<TA, LEN> const& IN) -> decltype(_rHelp<FTOR, LEN>::reduce(INIT, IN)){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
#pragma HLS PIPELINE
	return _rHelp<FTOR, LEN>::reduce(INIT, IN);
}

template <class FTOR>
struct Reduce{
	template <typename TI, typename TA, std::size_t LEN>
	auto operator()(TI const& INIT, std::array<TA, LEN> const& IN) -> decltype(reduce<FTOR>(INIT, IN)){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
#pragma HLS PIPELINE
		return reduce<FTOR>(INIT, IN);
	}
};


template <class FTOR, typename TI, typename TA, std::size_t LEN>
auto rreduce(std::array<TA, LEN> const& IN, TI const& INIT) -> decltype(_rHelp<FTOR, LEN>().rreduce(IN, INIT)){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
#pragma HLS PIPELINE
	return _rHelp<FTOR, LEN>::rreduce(IN, INIT);
}

template <class FTOR>
struct Rreduce{
	template <typename TI, typename TA, std::size_t LEN>
	auto operator()(std::array<TA, LEN> const& IN, TI const& INIT) -> decltype(rreduce<FTOR>(IN, INIT)){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
#pragma HLS PIPELINE
		return rreduce<FTOR>(IN, INIT);
	}
};
#endif // __REDUCE_HPP

