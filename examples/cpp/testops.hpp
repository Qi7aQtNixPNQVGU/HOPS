#ifndef __UTILITY_HPP
#define __UTILITY_HPP
#include <stdio.h>
#include <random>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <algorithm>
#include <array>
template <int RN_MIN, int RN_MAX, std::size_t LEN>
std::array<int, LEN> genarr() {
	std::array<int, LEN> lst;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> distribution(RN_MIN,RN_MAX);
	std::uniform_int_distribution<unsigned char> sparsifier(0,255);
	for(uint i = 0; i < LEN; ++i) {
		lst[i] = distribution(gen);
	}
	return lst;
}

void print(int * list, unsigned int length){
	printf("[ ");
	for(unsigned int i = 0; i < length; i++){
		printf("%d ", list[i]);
	}
	printf("]\n");
}

#endif // __UTILITY_HPP
