#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include "sdg.h"
#include "kstring.h"

#include "kseq.h"
KSEQ_INIT(gzFile, gzread)

static inline sdg_side_t read_side(char *str, sdg_graph_t *g, char **r, int *absent)
{
	char *q, *p;
	sdg_seq_t *s;
	sdg_side_t side;
	for (q = p = str; *p && *p != '\t'; ++p);
	*p++ = 0;
	s = sdg_g_add_seq(g, q, absent);
	side.id = s->id;
	side.sp = strtol(p, &p, 10) << 1;
	if (*p == '>') side.sp |= 1;
	++p;
	*r = p;
	return side;
}

sdg_graph_t *sdg_g_read(const char *fn)
{
	gzFile fp;
	kstring_t str = {0,0,0};
	kstream_t *ks;
	int dret, absent;
	char *p, *q;
	int64_t x, lineno = 0;
	sdg_seq_t *s;
	sdg_graph_t *g;

	fp = fn && strcmp(fn, "-")? gzopen(fn, "r") : gzdopen(fileno(stdin), "r");
	if (fp == 0) return 0;
	ks = ks_init(fp);
	g = sdg_g_init();
	while (ks_getuntil(ks, KS_SEP_LINE, &str, &dret) >= 0) {
		++lineno;
		if (str.l < 2 || str.s[1] != '\t') continue;
		if (str.s[0] == 'S') {
			for (q = p = str.s + 2; *p && *p != '\t'; ++p);
			*p++ = 0; 
			s = sdg_g_add_seq(g, q, &absent);
			x = strtol(p, &p, 10); // TODO: check errors
			if (s->len >= 0 && s->len != x && sdg_verbose >= 2)
				fprintf(stderr, "WARNING: at line %ld, sequence '%s' was added before with a different length.\n", (long)lineno, s->name);
			if (s->len < 0 && x >= 0) s->len = x; // TODO: what if a sequence added twice with different lengths?
		} else if (str.s[0] == 'J') {
			sdg_side_t s1, s2;
			s1 = read_side(str.s + 2, g, &p, &absent);
			s2 = read_side(p + 1, g, &p, &absent);
			sdg_g_add_join(g, s1, s2);
		} else if (str.s[0] == 'I') {
			sdg_side_t s1, s2, s0;
			for (q = p = str.s + 2; *p && *p != '\t'; ++p);
			*p++ = 0; 
			s = sdg_g_add_seq(g, q, &absent);
			if (!absent && sdg_verbose >= 2)
				fprintf(stderr, "WARNING: at line %ld, sequence '%s' was added before.\n", (long)lineno, s->name);
			s->len = strtol(p, &p, 10);
			s1 = read_side(p + 1, g, &p, &absent);
			if (absent && sdg_verbose >= 2)
				fprintf(stderr, "WARNING: at line %ld, sequence '%s' was not added before.\n", (long)lineno, g->seqs[s1.id].name);
			s2 = read_side(p + 1, g, &p, &absent);
			if (absent && sdg_verbose >= 2)
				fprintf(stderr, "WARNING: at line %ld, sequence '%s' was not added before.\n", (long)lineno, g->seqs[s2.id].name);
			s0.id = s->id, s0.sp = 0;
			sdg_g_add_join(g, s0, s1);
			s0.sp = s->len<<1 | 1;
			sdg_g_add_join(g, s0, s2);
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
	kputc("<>"[sp&1], s);
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
		kputc('\n', &str);
		fwrite(str.s, 1, str.l, out);
		if (s->n_jpos == 0) continue;
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
		free(itr);
	}
	free(str.s);
}
