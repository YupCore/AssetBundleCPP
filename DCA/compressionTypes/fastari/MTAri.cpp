#include "MTAri.h"

/* ============BLOCK FUNCTIONS============ */
int enc_blk(char* bin, char* bout, size_t ilen, size_t* olen, int par, void* workmem) {
	return FastAri::fa_compress_safe((unsigned char*)bin, (unsigned char*)bout, ilen, olen, workmem);
}
int dec_blk(char* bin, char* bout, size_t ilen, size_t* olen, void* workmem) {
	return FastAri::fa_decompress_safe((unsigned char*)bin, (unsigned char*)bout, ilen, olen, workmem);
}
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
	void** wbufs;
	/* Validate parameters */
	if (!fin) return EXIT_FAILURE;
	if (!fout) return EXIT_FAILURE;
	if (bsz < 1) return EXIT_FAILURE;
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
	if (!(ibufs = (char**)calloc(tcnt, sizeof(char*)))) return EXIT_FAILURE;
	if (!(obufs = (char**)calloc(tcnt, sizeof(char*)))) { free(ibufs); return EXIT_FAILURE; }
	if (!(ilens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); return EXIT_FAILURE; }
	if (!(olens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); free(ilens); return EXIT_FAILURE; }
	if (!(wbufs = (void**)calloc(tcnt, sizeof(void*)))) { free(ibufs); free(obufs); free(ilens); free(olens); return EXIT_FAILURE; }
	/* Allocate buffers */
	for (pi = 0; pi < tcnt; ++pi) {
		if (!(ibufs[pi] = (char*)malloc(bsz * sizeof(char))) || !(obufs[pi] = (char*)malloc(((bsz + (bsz / 10)) * sizeof(char)) + 1024)) || !(wbufs[pi] = malloc(FA_WORKMEM))) {
			if (ibufs[pi]) free(ibufs[pi]);
			if (obufs[pi]) free(obufs[pi]);
			for (mp = 0; mp < pi; ++mp) { free(ibufs[mp]); free(obufs[mp]); free(wbufs[mp]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			free(wbufs);
			return EXIT_FAILURE;
		}
	}
	/* Process input */
	while (1) {
		/* Read input */
		for (mp = 0; mp < tcnt; ++mp) {
			if ((ilens[mp] = fread(ibufs[mp], sizeof(char), bsz, fin)) < 1) break;
		}
		if (!mp) break;
		/* Process input */
		rc = 0;
#pragma omp parallel for
		for (pi = 0; pi < mp; ++pi) {
			if (ilens[pi] > 0) {
				rc += enc_blk(ibufs[pi], obufs[pi], ilens[pi], &(olens[pi]), par, wbufs[pi]);
			}
		}
		/* Check for processing errors */
		if (rc) {
			printf_s("Block compression failed...");
			for (pi = 0; pi < tcnt; ++pi) { free(ibufs[pi]); free(obufs[pi]); free(wbufs[pi]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			free(wbufs);
			return EXIT_FAILURE;
		}
		/* Write output */
		for (pi = 0; pi < mp; ++pi) {
			fwritesize(ilens[pi], fout);
			fwritesize(olens[pi], fout);
			fwrite(obufs[pi], sizeof(char), olens[pi], fout);
		}
	}
	/* Flush output */
	fwritesize(0, fout);
	fwritesize(0, fout);
	fflush(fout);
	/* Free memory */
	for (pi = 0; pi < tcnt; ++pi) { free(ibufs[pi]); free(obufs[pi]); free(wbufs[pi]); }
	free(ibufs);
	free(obufs);
	free(ilens);
	free(olens);
	free(wbufs);
	/* Return success */
	return EXIT_SUCCESS;
}
int MTAri::mtfa_decompress(FILE* fin, FILE* fout, int tcnt)
{
	/* Declare variables */
	char** ibufs;
	char** obufs;
	size_t* ilens;
	size_t* olens;
	void** wbufs;
	int mp, pi, rc, es = 0;
	/* Validate parameters */
	if (!fin) return EXIT_FAILURE;
	if (!fout) return EXIT_FAILURE;
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
	if (!(ibufs = (char**)calloc(tcnt, sizeof(char*)))) return EXIT_FAILURE;
	if (!(obufs = (char**)calloc(tcnt, sizeof(char*)))) { free(ibufs); return EXIT_FAILURE; }
	if (!(ilens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); return EXIT_FAILURE; }
	if (!(olens = (size_t*)calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); free(ilens); return EXIT_FAILURE; }
	if (!(wbufs = (void**)calloc(tcnt, sizeof(void*)))) { free(ibufs); free(obufs); free(ilens); free(olens); return EXIT_FAILURE; }
	/* Allocate work memory */
	for (pi = 0; pi < tcnt; ++pi) {
		if (!(wbufs[pi] = malloc(FA_WORKMEM))) {
			for (mp = 0; mp < pi; ++mp) { free(wbufs[mp]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			free(wbufs);
			return EXIT_FAILURE;
		}
	}
	/* Process input */
	while (1) {
		/* Read input */
		rc = 0;
		for (mp = 0; mp < tcnt; ++mp) {
			if (!freadsize(&(olens[mp]), fin)) { es = 1; break; }
			if (!freadsize(&(ilens[mp]), fin)) { free(ibufs[mp]); rc = 1; break; }
			if (!ilens[mp] || !olens[mp]) { es = 1; break; }
			if (!(ibufs[mp] = (char*) malloc(ilens[mp]))) { rc = 1; break; }
			if (!(obufs[mp] = (char*)malloc(olens[mp]))) { free(ibufs[mp]); rc = 1; break; }
			if (fread(ibufs[mp], sizeof(char), ilens[mp], fin) < ilens[mp]) { free(ibufs[mp]); free(obufs[mp]); rc = 1; break; }
		}
		if (rc) {
			for (pi = 0; pi < mp; ++pi) { free(ibufs[pi]); free(obufs[pi]); free(wbufs[pi]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			free(wbufs);
			return EXIT_FAILURE;
		}
		/* Process input */
#pragma omp parallel for
		for (pi = 0; pi < mp; ++pi) {
			if (ilens[pi] > 0) {
				rc += dec_blk(ibufs[pi], obufs[pi], ilens[pi], &(olens[pi]), wbufs[pi]);
			}
		}
		/* Check for processing errors */
		if (rc) {
			for (pi = 0; pi < mp; ++pi) { free(ibufs[pi]); free(obufs[pi]); free(wbufs[pi]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			free(wbufs);
			return EXIT_FAILURE;
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
	for (pi = 0; pi < tcnt; ++pi) { free(wbufs[pi]); }
	free(wbufs);
	/* Return success */
	return EXIT_SUCCESS;
}