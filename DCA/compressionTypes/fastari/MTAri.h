#pragma once
//Its broken please FIX
#define _CRT_SECURE_NO_WARNINGS
#define BUFSIZE 0x400000
#define _OPENMP
#ifdef _OPENMP
#include <omp.h>
#endif
#include "FastAri.h"
#include <string>
#include <streambuf>
#include <iostream>
#include <sstream>

class MTAri
{
public:
	static int mtfa_compress(std::string& ibuf, std::string& obuf, int tcnt, size_t bsz);
	static int mtfa_decompress(std::string& ibuf, std::string& obuf, int tcnt);
};