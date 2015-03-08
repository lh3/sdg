#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include "sdg.h"
#include "kstring.h"

#include "kseq.h"
KSEQ_INIT(gzFile, gzread)

sdg_graph_t *sdg_read(const char *fn)
{
	return 0;
}

static inline void write_side(kstring_t *s, const sdg_graph_t *g, int64_t id, int64_t sp)
{
	kputc('\t', s);
	kputs(g->seqs[id].name, s);
	kputc('\t', s);
	kputl(sp>>1, s);
	kputc("+-"[sp&1], s);
}

void sdg_write(const sdg_graph_t *g, FILE *out)
{
	int64_t i;
	kstring_t str = {0,0,0};
	for (i = 0; i < g->n_seqs; ++i) {
		sdg_seq_t *s = &g->seqs[i];
		sdg_ji_t *itr;
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
				if (d->id > s->id || (d->id == s->id && d->sp > p->sp)) {
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
