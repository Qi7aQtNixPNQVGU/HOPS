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
#ifndef __FFT_HPP
#define __FFT_HPP
#include "zip.hpp"
#include "map.hpp"
#include "listops.hpp"
#include "hof.hpp"
#include "divconq.hpp"
#include "reduce.hpp"
#include "constops.hpp"
#include <complex>
#include <stdio.h>
#ifdef BIT_ACCURATE
//#include "hls_math.h"
#include <cmath>
#else
#include <cmath>
#endif

template <typename T>
using FFT_t = std::complex<T>;

typedef std::pair<std::size_t, std::size_t> ctx_t;

template <typename T>
using twid_t = std::pair<T, T>;

template <typename T>
using data_t = std::pair<FFT_t<T>, FFT_t<T>>;

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
std::array<T, LEN> bitreverse(std::array<T, LEN> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
	return divconq<Interleave>(IN);
}


template <typename T, std::size_t LEV>
struct CalcAngle{
	auto operator()(std::size_t const& IDX) -> std::pair<T, T> {
#pragma HLS INLINE
		auto c = cos((((T)M_PI)*-2*IDX)/(1<<(LEV)));
		auto s = sin((((T)M_PI)*-2*IDX)/(1<<(LEV)));
		return {c, s};
	}
};

template <class FTOR>
struct NPtFFT{
	template <typename T, std::size_t LEN>
	std::array<FFT_t<T>, 2*LEN> operator()(std::array<FFT_t<T>, LEN> L, std::array<FFT_t<T>, LEN> R){
#pragma HLS INLINE
		static const size_t LEV = hlog2(LEN)+1;
		auto twiddles = map<CalcAngle<T, LEV>>(range<LEN>());
		auto pairs = zip(L, R);
		auto outputs = unzip(zipWith<FTOR>(twiddles, pairs));
		return outputs.first + outputs.second;
	}
};

class FFTOP{
public:
	template <typename T> 
	auto operator()(twid_t<T> const& TWID, data_t<T> const& IN) -> data_t<T>{
#pragma HLS INLINE
		FFT_t<T> ti = IN.first, bi = IN.second, to, bo;
		T c = TWID.first;
		T s = TWID.second;
		T temp_r = c*std::real(bi) + s*std::imag(bi);
		T temp_i = c*std::imag(bi) - s*std::real(bi);
		to.real(ti.real() + temp_r);
		to.imag(ti.imag() + temp_i);
		bo.real(ti.real() - temp_r);
		bo.imag(ti.imag() - temp_i);
		return {to, bo};
	}
};

template<typename T, std::size_t LEN>
std::array<std::complex<T>, LEN>fft(std::array<std::complex<T>, LEN> IN){
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
#pragma HLS INLINE
	return divconq<NPtFFT<FFTOP>>(bitreverse(IN));
}

namespace imperative{
	template <typename T, std::size_t LEN>
	std::array<T, LEN> bitreverse(std::array<T, LEN> IN){
#pragma HLS INLINE
		static const size_t LEV = hlog2(LEN);
		for(std::size_t t = 0; t < LEN; t++){
#pragma HLS UNROLL
			T temp;
			unsigned int b = 0;
			for(std::size_t j = 0; j < LEV; j++){
#pragma HLS UNROLL
				b = b << 1;
				b |= ((t >> j) &1);
			}
			if(t > b){
				temp = IN[t];
				IN[t] = IN[b];
				IN[b] = temp;
			}
		}
		return IN;
	}

	template <class FTOR, std::size_t LEN, typename T>
	std::array<FFT_t<T>, LEN> _butterfly(std::array<FFT_t<T>, LEN> IN){
#pragma HLS INLINE
		static const int LEV = hlog2(LEN);
		std::array<std::array<FFT_t<T>, LEN>, LEV + 1> stagearr;
#pragma HLS ARRAY_PARTITION complete VARIABLE=stagearr._M_instance
#pragma HLS ARRAY_PARTITION complete VARIABLE=stagearr._M_instance[0]
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
		stagearr[0]  = IN;
	bfly_level:
		for(std::size_t l = 0; l < LEV; ++l){
#pragma HLS UNROLL
			std::size_t stride = 1<<l;
			std::size_t mask = stride - 1;
		bfly_pair:
			for(std::size_t i = 0; i < (1 << (LEV - 1)); ++i){
#pragma HLS UNROLL
				std::size_t grp = i >> l;
				std::size_t top = i & mask;
				std::size_t base = top + (grp<<(l+1));
				data_t<T> inputs = {stagearr[l][base], 
						    stagearr[l][base + stride]};
				T c = cos((((T)M_PI)*-2*top)/(1<<(l + 1)));
				T s = sin((((T)M_PI)*-2*top)/(1<<(l + 1)));
				data_t<T> o = FTOR()({c,s}, inputs);
				stagearr[l+1][base]= o.first;
				stagearr[l+1][base + stride] = o.second;
			}
		}

		return stagearr[LEV];
	}

	template<typename T, std::size_t LEN>
	std::array<std::complex<T>, LEN> fft(std::array<std::complex<T>, LEN> IN){
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION complete VARIABLE=IN._M_instance
		auto res = _butterfly<FFTOP>(bitreverse(IN));
#pragma HLS ARRAY_PARTITION complete VARIABLE=res._M_instance
		return res;
	}
}
namespace software{

	template <std::size_t LEN, typename T>
	void bitreverse(std::array<T, LEN>& ARR){
		for(unsigned int t = 0; t < LEN; t++){
			T temp;
			unsigned int b = 0;
			for(int j = 0; j < hlog2(LEN); j++){
				b = b << 1;
				b |= ((t >> j) &1);
			}
			if(t > b){
				temp = ARR[t];
				ARR[t] = ARR[b];
				ARR[b] = temp;
			}
		}
	}

	template <typename T, std::size_t LEN>
	std::array<std::complex<T>, LEN> fft(std::array<std::complex<T>, LEN> IN) {
		std::complex<T> temp;
		int stage, fftid, fftidx, pairidx; // Loop Indicies
		int npts, offset;

		bitreverse(IN);

		T a, e, c, s;
		int counter;
	stages:for(stage=1; stage<= hlog2(LEN); stage++)
		{
			// Number of points in an FFT = 2^stage
			npts = 1 << stage;
			// Offset to the index of the input pair
			offset = npts/2;

			e = (T)M_PI*-2/npts;

			a = 0.0;
			counter =0;
			// Perform fft's for j-th stage
		ffts:for(fftid=0; fftid<offset; fftid++)
			{
				c = cos(a);
				s = sin(a);
				// Compute butterflies that use same W**k
			NPtFFT:for(fftidx=fftid; fftidx<LEN; fftidx += npts)
				{
					counter ++;
					pairidx = fftidx + offset;
					temp = {IN[pairidx].real()*c+ IN[pairidx].imag()*s, 
						IN[pairidx].imag()*c- IN[pairidx].real()*s};

					IN[pairidx] = {IN[fftidx].real() - temp.real(), 
						       IN[fftidx].imag() - temp.imag()};
					IN[fftidx] = {IN[fftidx].real() + temp.real(), 
						      IN[fftidx].imag() + temp.imag()};
				}
				a = a + e;
			}
		}
		return IN;
	}
}

#endif
