#pragma once
#define _OPENMP
#define _CRT_SECURE_NO_WARNINGS
#define BUFSIZE 0x400000
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "FastAri.h"

class MTAri
{
public:

	static int mtfa_compress(FILE* fin, FILE* fout, int tcnt, size_t bsz, int par = 0);
	static int mtfa_decompress(FILE* fin, FILE* fout, int tcnt);
};