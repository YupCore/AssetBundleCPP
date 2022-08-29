#include "MTAri.h"

/* ============BLOCK FUNCTIONS============ */
int enc_blk(char* bin, char* bout, size_t ilen, size_t* olen) {
	return !FastAri::fa_compress((unsigned char*)bin, (unsigned char*)bout, ilen, olen);
}
int dec_blk(char* bin, char* bout, size_t ilen, size_t olen) {
	return !FastAri::fa_decompress((unsigned char*)bin, (unsigned char*)bout, ilen, olen);
}
/* ======================================= */

void fwritesize(size_t v, std::stringstream& f) {
	int s = 8;
	while (s)
	{
		f.put(v & 0xff);
		v >>= 8;
		--s;
	}
}
int freadsize(size_t* v, std::stringstream& f) {
	int s = 0;
	int c;
	*v = 0;
	while (s < 64)
	{
		if ((c = f.get()) == EOF) { *v = 0;  return 0; }
		*v |= c << s;
		s += 8;
	}
	return 1;
}

int MTAri::mtfa_compress(std::string& ibuf, std::string& obuf, int tcnt, size_t bsz)
{
	std::stringstream inpt(ibuf,std::ios_base::binary | std::ios::in);
	std::stringstream outf;
	/* Declare variables */
	char** ibufs;
	char** obufs;
	size_t* ilens;
	size_t* olens;
	int mp, pi, rc;
	/* Validate parameters */
	if (ibuf == "") return 0;
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
		for (pi = 0; pi < tcnt; ++pi)
		{
			inpt.read(ibufs[pi], bsz);
			inpt.seekg((size_t)inpt.tellg() + bsz);
			ilens[pi] = bsz;
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

			fwritesize(ilens[pi], outf);
			fwritesize(olens[pi], outf);
			outf.write(obufs[pi], olens[pi]);
		}
		if (rc) break;
	}
	/* Flush output */
	fwritesize(0, outf);
	fwritesize(0, outf);
	/* Free memory */
	for (pi = 0; pi < tcnt; ++pi) { free(ibufs[pi]); free(obufs[pi]); }
	free(ibufs);
	free(obufs);
	free(ilens);
	free(olens);
	/* Return success */
	obuf = outf.str();
	outf.clear();
	inpt.clear();
	return 1;
}
int MTAri::mtfa_decompress(std::string& ibuf, std::string& obuf, int tcnt)
{
	std::stringstream fin(ibuf);
	std::stringstream fout;
	/* Declare variables */
	char** ibufs;
	char** obufs;
	size_t* ilens;
	size_t* olens;
	int mp, pi, rc, es = 0;
	/* Validate parameters */
	if (ibuf == "") return 0;
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
			if (fin.readsome(ibufs[mp], ilens[mp]) < ilens[mp])
			{
				free(ibufs[mp]); free(obufs[mp]); rc = 1;
				break;
			}
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
			fout.write(obufs[pi], olens[pi]);
			free(obufs[pi]);
		}
		if (es) break;
	}
	/* Free memory */
	free(ibufs);
	free(obufs);
	free(ilens);
	free(olens);
	/* Return success */
	obuf = fout.str();
	fout.clear();
	fin.clear();
	return 1;
}