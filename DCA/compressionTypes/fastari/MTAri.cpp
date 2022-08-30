#include "MTAri.h"

#pragma region ToFix
///* ============BLOCK FUNCTIONS============ */
//int enc_blk(char* bin, char* bout, size_t ilen, size_t* olen) {
//	return FastAri::fa_compress((unsigned char*)bin, (unsigned char*)bout, ilen, olen);
//}
//int dec_blk(char* bin, char* bout, size_t ilen, size_t olen) {
//	return FastAri::fa_decompress((unsigned char*)bin, (unsigned char*)bout, ilen, olen);
//}
///* ======================================= */
//
//void fwritesize(size_t v, std::stringstream& f) {
//	int s = 8;
//	while (s)
//	{
//		f.put(v & 0xff);
//		v >>= 8;
//		--s;
//	}
//}
//int freadsize(size_t* v, std::stringstream& f) {
//	int s = 0;
//	int c;
//	*v = 0;
//	while (s < 64)
//	{
//		if ((c = f.get()) == EOF) { *v = 0;  return 0; }
//		*v |= c << s;
//		s += 8;
//	}
//	return 1;
//}
//
//int MTAri::mtfa_compress(std::stringstream& inptStream, std::stringstream& outStream)
//{
//	int tcnt = 4;
//	/* Declare variables */
//	char** ibufs;
//	char** obufs;
//	size_t* ilens;
//	size_t* olens;
//	int mp, pi, rc;
//	auto bsz = BUFSIZE;
//	/* Setup thread count */
//	if (tcnt < 1) {
//#ifdef _OPENMP
//		tcnt = omp_get_num_procs();
//#else
//		tcnt = 1;
//#endif
//	}
//#ifdef _OPENMP
//	if (tcnt > omp_get_num_procs()) tcnt = omp_get_num_procs();
//	omp_set_num_threads(tcnt);
//#else
//	tcnt = 1;
//#endif
//	/* Allocate arrays */
//	if (!(ibufs = (char**)calloc(tcnt, sizeof(char*)))) return 0;
//	if (!(obufs = (char**)calloc(tcnt, sizeof(char*)))) { free(ibufs); return 0; }
//	if (!(ilens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); return 0; }
//	if (!(olens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); free(ilens); return 0; }
//	/* Allocate buffers */
//	for (pi = 0; pi < tcnt; pi++) {
//		if (!(ibufs[pi] = (char*)malloc(bsz * sizeof(char))) || !(obufs[pi] = (char*)malloc(((bsz + (bsz / 10)) * sizeof(char)) + 1024))) {
//			if (ibufs[pi]) free(ibufs[pi]);
//			for (mp = 0; mp < pi; ++mp) { free(ibufs[mp]); free(obufs[mp]); }
//			free(ibufs);
//			free(obufs);
//			free(ilens);
//			free(olens);
//			return 0;
//		}
//	}
//	printf_s("Reached compression stage..\n");
//	/* Process input */
//	while (1)
//	{
//		/* Read input */
//		for (pi = 0; pi < tcnt; pi++) 
//		{
//			ilens[pi] = inptStream.readsome(ibufs[pi],bsz);
//		}
//		/* Process input */
//		rc = 0;
//#pragma omp parallel for
//		for (pi = 0; pi < tcnt; pi++) 
//		{
//			if (ilens[pi] > 0) {
//				rc += enc_blk(ibufs[pi], obufs[pi], ilens[pi], &(olens[pi]));
//			}
//		}
//		/* Check for processing errors */
//		if (rc) {
//			for (pi = 0; pi < tcnt; pi++) { free(ibufs[pi]); free(obufs[pi]); }
//			free(ibufs);
//			free(obufs);
//			free(ilens);
//			free(olens);
//			return 0;
//		}
//		/* Write output */
//		rc = 0;
//		for (pi = 0; pi < tcnt; pi++) {
//			if (!ilens[pi]) { rc = 1; break; }
//			fwritesize(ilens[pi], outStream);
//			fwritesize(olens[pi], outStream);
//			outStream.write(obufs[pi], olens[pi]);
//		}
//		if (rc) break;
//	}
//	/* Flush output */
//	fwritesize(0, outStream);
//	fwritesize(0, outStream);
//	/* Free memory */
//	for (pi = 0; pi < tcnt; pi++) { free(ibufs[pi]); free(obufs[pi]); }
//	free(ibufs);
//	free(obufs);
//	free(ilens);
//	free(olens);
//	/* Return success */
//	return 1;
//}
//int MTAri::mtfa_decompress(std::stringstream& inptStream, std::stringstream& outStream)
//{
//	/* Declare variables */
//	int tcnt = 4;
//	char** ibufs;
//	char** obufs;
//	size_t* ilens;
//	size_t* olens;
//	int mp, pi, rc, es = 0;
//	/* Validate parameters */
//	/* Setup thread count */
//	if (tcnt < 1) {
//#ifdef _OPENMP
//		tcnt = omp_get_num_procs();
//#else
//		tcnt = 1;
//#endif
//	}
//#ifdef _OPENMP
//	if (tcnt > omp_get_num_procs()) tcnt = omp_get_num_procs();
//	omp_set_num_threads(tcnt);
//#else
//	tcnt = 1;
//#endif
//	/* Allocate arrays */
//	if (!(ibufs = (char**)calloc(tcnt, sizeof(char*)))) return 0;
//	if (!(obufs = (char**)calloc(tcnt, sizeof(char*)))) { free(ibufs); return 0; }
//	if (!(ilens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); return 0; }
//	if (!(olens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); free(ilens); return 0; }
//	printf_s("Allocated buffers\n");
//	/* Process input */
//	while (1) {
//		/* Read input */
//		rc = 0;
//		printf_s("Started input processing\n");
//		for (mp = 0; mp < tcnt; ++mp) {
//			if (!freadsize(&(olens[mp]), inptStream)) { es = 1; break; }
//			if (!freadsize(&(ilens[mp]), inptStream)) { free(ibufs[mp]); rc = 1; break; }
//			if (!ilens[mp] || !olens[mp]) { es = 1; break; }
//			if (!(ibufs[mp] = (char*)malloc(ilens[mp]))) { rc = 1; break; }
//			if (!(obufs[mp] = (char*)malloc(olens[mp]))) { free(ibufs[mp]); rc = 1; break; }
//			printf_s(("block size is: " + std::to_string(ilens[mp]) + "\n").c_str());
//			printf_s(("input stream size is: " + std::to_string(inptStream.str().size()) + "\n").c_str());
//			if (inptStream.readsome(ibufs[mp], ilens[mp]) < ilens[mp] && ilens[mp] != 0)
//			{ 
//				free(ibufs[mp]); free(obufs[mp]); rc = 1;
//				break;
//			}
//		}
//		if (rc) {
//			for (pi = 0; pi < mp; ++pi) { free(ibufs[pi]); free(obufs[pi]); }
//			free(ibufs);
//			free(obufs);
//			free(ilens);
//			free(olens);
//			return 0;
//		}
//		printf_s("decoding block..\n");
//		/* Process input */
//#pragma omp parallel for
//		for (pi = 0; pi < mp; ++pi) {
//			if (ilens[pi] > 0) {
//				rc += dec_blk(ibufs[pi], obufs[pi], ilens[pi], olens[pi]);
//			}
//		}
//		/* Check for processing errors */
//		if (rc) {
//			for (pi = 0; pi < mp; ++pi) { free(ibufs[pi]); free(obufs[pi]); }
//			free(ibufs);
//			free(obufs);
//			free(ilens);
//			free(olens);
//			return 0;
//		}
//		/* Write output */
//		rc = 0;
//		for (pi = 0; pi < mp; ++pi) {
//			free(ibufs[pi]);
//			outStream.write(obufs[pi], olens[pi]);
//			free(obufs[pi]);
//		}
//		if (es) break;
//	}
//	/* Free memory */
//	free(ibufs);
//	free(obufs);
//	free(ilens);
//	free(olens);
//	/* Return success */
//	return 1;
//}

#pragma endregion

/* ============BLOCK FUNCTIONS============ */
int enc_blk(char* bin, char* bout, size_t ilen, size_t* olen) {
	return !FastAri::fa_compress((uint8_t*)bin, (uint8_t*)bout, ilen, olen);
}
int dec_blk(char* bin, char* bout, size_t ilen, size_t olen) {
	return !FastAri::fa_decompress((uint8_t*)bin, (uint8_t*)bout, ilen, olen);
}
/* ======================================= */

/* ==============DRIVER CORE============== */
void fwritesize(size_t v, FILE* f) {
	int s = 8;
	while (s) {
		fputc(v & 0xff, f);
		v >>= 8;
		--s;
	}
}
int freadsize(size_t* v, FILE* f) {
	int s = 0;
	int c;
	*v = 0;
	while (s < 64) {
		if ((c = fgetc(f)) == EOF) { *v = 0;  return 0; }
		*v |= c << s;
		s += 8;
	}
	return 1;
}

int MTAri::mtfa_compress(FILE* fin, FILE* fout, int tcnt, size_t bsz, int par)
{
	/* Declare variables */
	char** ibufs;
	char** obufs;
	size_t* ilens;
	size_t* olens;
	int mp, pi, rc;
	/* Validate parameters */
	if (!fin) return 0;
	if (!fout) return 0;
	if (bsz < 1) return 0;
	/* Setup thread count */
	if (tcnt < 1) {
#ifdef _OPENMP
		tcnt = omp_get_num_procs();
#else
		tcnt = 1;
#endif
	}
#ifdef _OPENMP
	if (tcnt > omp_get_num_procs()) tcnt = omp_get_num_procs();
	omp_set_num_threads(tcnt);
#else
	tcnt = 1;
#endif
	/* Allocate arrays */
	if (!(ibufs = (char**)calloc(tcnt, sizeof(char*)))) return 0;
	if (!(obufs = (char**)calloc(tcnt, sizeof(char*)))) { free(ibufs); return 0; }
	if (!(ilens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); return 0; }
	if (!(olens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); free(ilens); return 0; }
	/* Allocate buffers */
	for (pi = 0; pi < tcnt; ++pi) {
		if (!(ibufs[pi] = (char*)malloc(bsz * sizeof(char))) || !(obufs[pi] = (char*)malloc(((bsz + (bsz / 10)) * sizeof(char)) + 1024))) {
			if (ibufs[pi]) free(ibufs[pi]);
			for (mp = 0; mp < pi; ++mp) { free(ibufs[mp]); free(obufs[mp]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			return 0;
		}
	}
	/* Process input */
	while (1) {
		/* Read input */
		for (pi = 0; pi < tcnt; ++pi) {
			ilens[pi] = fread(ibufs[pi], sizeof(char), bsz, fin);
		}
		/* Process input */
		rc = 0;
#pragma omp parallel for
		for (pi = 0; pi < tcnt; ++pi) {
			if (ilens[pi] > 0) {
				rc += !enc_blk(ibufs[pi], obufs[pi], ilens[pi], &(olens[pi]));
			}
		}
		/* Check for processing errors */
		if (rc) {
			for (pi = 0; pi < tcnt; ++pi) { free(ibufs[pi]); free(obufs[pi]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			return 0;
		}
		/* Write output */
		rc = 0;
		for (pi = 0; pi < tcnt; ++pi) {
			if (!ilens[pi]) { rc = 1; break; }
			fwritesize(ilens[pi], fout);
			fwritesize(olens[pi], fout);
			fwrite(obufs[pi], sizeof(char), olens[pi], fout);
		}
		if (rc) break;
	}
	/* Flush output */
	fwritesize(0, fout);
	fwritesize(0, fout);
	fflush(fout);
	/* Free memory */
	for (pi = 0; pi < tcnt; ++pi) { free(ibufs[pi]); free(obufs[pi]); }
	free(ibufs);
	free(obufs);
	free(ilens);
	free(olens);
	/* Return success */
	return 1;
}

int MTAri::mtfa_decompress(FILE* fin, FILE* fout, int tcnt)
{
	/* Declare variables */
	char** ibufs;
	char** obufs;
	size_t* ilens;
	size_t* olens;
	int mp, pi, rc, es = 0;
	/* Validate parameters */
	if (!fin) return 0;
	if (!fout) return 0;
	/* Setup thread count */
	if (tcnt < 1) {
#ifdef _OPENMP
		tcnt = omp_get_num_procs();
#else
		tcnt = 1;
#endif
	}
#ifdef _OPENMP
	if (tcnt > omp_get_num_procs()) tcnt = omp_get_num_procs();
	omp_set_num_threads(tcnt);
#else
	tcnt = 1;
#endif
	/* Allocate arrays */
	if (!(ibufs = (char**)calloc(tcnt, sizeof(char*)))) return 0;
	if (!(obufs = (char**)calloc(tcnt, sizeof(char*)))) { free(ibufs); return 0; }
	if (!(ilens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); return 0; }
	if (!(olens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); free(ilens); return 0; }
	/* Process input */
	while (1) {
		/* Read input */
		rc = 0;
		for (mp = 0; mp < tcnt; ++mp) {
			if (!freadsize(&(olens[mp]), fin)) { es = 1; break; }
			if (!freadsize(&(ilens[mp]), fin)) { free(ibufs[mp]); rc = 1; break; }
			if (!ilens[mp] || !olens[mp]) { es = 1; break; }
			if (!(ibufs[mp] = (char*)malloc(ilens[mp]))) { rc = 1; break; }
			if (!(obufs[mp] = (char*)malloc(olens[mp]))) { free(ibufs[mp]); rc = 1; break; }
			if (fread(ibufs[mp], sizeof(char), ilens[mp], fin) < ilens[mp]) { free(ibufs[mp]); free(obufs[mp]); rc = 1; break; }
		}
		if (rc) {
			for (pi = 0; pi < mp; ++pi) { free(ibufs[pi]); free(obufs[pi]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			return 0;
		}
		/* Process input */
#pragma omp parallel for
		for (pi = 0; pi < mp; ++pi) {
			if (ilens[pi] > 0) {
				rc += !dec_blk(ibufs[pi], obufs[pi], ilens[pi], olens[pi]);
			}
		}
		/* Check for processing errors */
		if (rc) {
			for (pi = 0; pi < mp; ++pi) { free(ibufs[pi]); free(obufs[pi]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			return 0;
		}
		/* Write output */
		rc = 0;
		for (pi = 0; pi < mp; ++pi) {
			free(ibufs[pi]);
			fwrite(obufs[pi], sizeof(char), olens[pi], fout);
			free(obufs[pi]);
		}
		if (es) break;
	}
	/* Flush output */
	fflush(fout);
	/* Free memory */
	free(ibufs);
	free(obufs);
	free(ilens);
	free(olens);
	/* Return success */
	return 1;
}