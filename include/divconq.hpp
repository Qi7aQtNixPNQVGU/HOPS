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
#ifndef __DIVCONQ_HPP
#define __DIVCONQ_HPP
#include <utility>
#include <array>
#include "listops.hpp"
#include "constops.hpp"
#include "hof.hpp"
template <class FTOR, std::size_t LEV>
struct _dcHelp{
	template<typename TA, std::size_t FHLEN>
	static auto divconq(std::array<TA, FHLEN> const& IN) -> 
		decltype(FTOR()(_dcHelp<FTOR, LEV-1>::divconq(std::array<TA, (FHLEN)/2>()),
				_dcHelp<FTOR, LEV-1>::divconq(std::array<TA, (FHLEN)/2>()))) {
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
		auto p = splitat<FHLEN/2>(IN);
		return FTOR()(
			_dcHelp<FTOR, LEV-1>::divconq(p.first), 
			_dcHelp<FTOR, LEV-1>::divconq(p.second));
	}
	// TODO: In order to handle Non-power-of-2-length lists, 
	// we need overloading (and there's a bug in GCC)
};

template <class FTOR>
struct _dcHelp<FTOR, 1>{
	template <typename TA>
	static auto divconq(std::array<TA, 2> const& IN) -> decltype(unpair<FTOR>(splitat<1>(IN))){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
		return unpair<FTOR>(splitat<1>(IN));
	}
	// TODO: In order to handle Non-power-of-2-length lists, 
	// we need overloading (and there's a bug in GCC, so we can't for now)
};

template <class FTOR, typename TA, std::size_t LEN>
auto divconq(std::array<TA, LEN> const& IN) -> decltype(_dcHelp<FTOR, clog2(LEN)>::divconq(IN)){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
#pragma HLS PIPELINE
	return _dcHelp<FTOR, clog2(LEN)>::divconq(IN);
}

template <class FTOR>
struct Divconq{
	template <typename TA, std::size_t LEN>
	auto operator()(std::array<TA, LEN> const& IN) -> decltype(divconq(IN)){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
#pragma HLS PIPELINE
		return divconq<FTOR>(IN);
	}
};
#endif
