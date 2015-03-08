#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include "sdg.h"
#include "kstring.h"

#include "kseq.h"
KSEQ_INIT(gzFile, gzread)

sdg_graph_t *sdg_g_read(const char *fn)
{
	gzFile fp;
	kstring_t str = {0,0,0};
	kstream_t *ks;
	int dret;
	char *p, *q;
	int64_t x;
	sdg_seq_t *s;
	sdg_graph_t *g;

	fp = fn && strcmp(fn, "-")? gzopen(fn, "r") : gzdopen(fileno(stdin), "r");
	if (fp == 0) return 0;
	ks = ks_init(fp);
	g = sdg_g_init();
	while (ks_getuntil(ks, KS_SEP_LINE, &str, &dret) >= 0) {
		if (str.l < 2 || str.s[1] != '\t') continue;
		if (str.s[0] == 'S') {
			for (q = p = str.s + 2; *p && *p != '\t'; ++p);
			*p++ = 0; 
			s = sdg_g_add_seq(g, q);
			x = strtol(p, &p, 10); // TODO: check errors
			if (s->len < 0 && x >= 0) s->len = x; // TODO: what if a sequence added twice with different lengths?
		} else if (str.s[0] == 'J') {
			sdg_side_t s1, s2;
			for (q = p = str.s + 2; *p && *p != '\t'; ++p);
			*p++ = 0; 
			s = sdg_g_add_seq(g, q);
			s1.id = s->id;
			s1.sp = strtol(p, &p, 10) << 1;
			if (*p == '-') s1.sp |= 1; // TODO: what if *p is neither '+' nor '-'
			for (q = p = p + 2; *p && *p != '\t'; ++p);
			*p++ = 0; 
			s = sdg_g_add_seq(g, q);
			s2.id = s->id;
			s2.sp = strtol(p, &p, 10) << 1;
			if (*p == '-') s2.sp |= 1; // TODO: what if *p is neither '+' nor '-'
			sdg_g_add_join(g, s1, s2);
		}
	}
	free(str.s);
	ks_destroy(ks);
	gzclose(fp);
	return g;
}

static inline void write_side(kstring_t *s, const sdg_graph_t *g, int64_t id, int64_t sp)
{
	kputc('\t', s);
	kputs(g->seqs[id].name, s);
	kputc('\t', s);
	kputl(sp>>1, s);
	kputc("+-"[sp&1], s);
}

void sdg_g_write(const sdg_graph_t *g, FILE *out)
{
	int64_t i;
	kstring_t str = {0,0,0};
	for (i = 0; i < g->n_seqs; ++i) {
		sdg_seq_t *s = &g->seqs[i];
		sdg_ji_t *itr;
		str.l = 0;
		kputsn("S\t", 2, &str);
		kputs(s->name, &str);
		kputc('\t', &str);
		kputl(s->len, &str);
		kputsn("\t*\n", 3, &str);
		fwrite(str.s, 1, str.l, out);
		itr = sdg_ji_first(s);
		do { // traverse all join pos on $
			sdg_jpos_t *p;
			int j;
			p = sdg_ji_at(itr);
			for (j = 0; j < p->n_sides; ++j) { // traverse all sides involving $s->id:$p->sp
				const sdg_side_t *d = sdg_j_get_side(p, j);
				if (d->id < s->id || (d->id == s->id && d->sp > p->sp)) {
					str.l = 0;
					kputc('J', &str);
					write_side(&str, g, s->id, p->sp);
					write_side(&str, g, d->id, d->sp);
					kputc('\n', &str);
					fwrite(str.s, 1, str.l, out);
				}
			}
		} while (sdg_ji_next(itr));
	}
	free(str.s);
}
