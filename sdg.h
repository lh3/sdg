#ifndef SIDEGRAPH_H
#define SIDEGRAPH_H

/*
 * A graph (sdg_graph_t) consists of sequences (sdg_seq_t). A sequence consists
 * of join positions (sdg_jpos_t). These are sides on this sequence that
 * involved in joins. The adjacent sides are kept in sdg_jpos_t::{nei,neis}.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef kroundup64
#define kroundup64(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, (x)|=(x)>>32, ++(x))
#endif

#ifndef kroundup32
#define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#endif

#define SG_TREE_JOINS 8

typedef struct {
	int64_t id; // sequence id
	int64_t sp; // stranded pos: pos<<1|strand
} sdg_side_t;

typedef struct {
	int64_t sp; // stranded pos
	uint32_t n_sides, m_sides;
	union {     // if $n_sides==1, we use x.nei; otherwise, use x.neis[]
		sdg_side_t nei;
		sdg_side_t *neis;
	} x;
} sdg_jpos_t;

typedef struct {
	int64_t id;  // sequence ID
	int64_t len; // len<0 if length is not available
	char *name;  // sequence name
	uint32_t n_jpos, m_jpos;
	void *jpos;  // an array if n_jpos<SG_TREE_JOINS; otherwise a B-tree
} sdg_seq_t;

typedef struct {
	int64_t n_seqs, m_seqs;
	sdg_seq_t *seqs;
	void *hash;  // translate name strings to integer ids
} sdg_graph_t;

struct sdg_ji_t;
typedef struct sdg_ji_t sdg_ji_t;

extern int sdg_verbose;

#ifdef __cplusplus
extern "C" {
#endif

	// basic operations

	sdg_graph_t *sdg_g_init(void);
	void sdg_g_destroy(sdg_graph_t *g);

	sdg_seq_t *sdg_g_get_seq(const sdg_graph_t *g, const char *name);
	sdg_seq_t *sdg_g_add_seq(sdg_graph_t *g, const char *name, int *absent);
	int sdg_g_add_join(sdg_graph_t *g, const sdg_side_t s1, const sdg_side_t s2);

	sdg_jpos_t *sdg_s_get_jpos(const sdg_seq_t *s, int64_t sp);
	sdg_jpos_t *sdg_s_add_jpos(sdg_seq_t *s, int64_t sp);

	// jpos iterator

	sdg_ji_t *sdg_ji_first(sdg_seq_t *s);
	int sdg_ji_next(sdg_ji_t *itr);
	sdg_jpos_t *sdg_ji_at(sdg_ji_t *itr);

	// I/O

	sdg_graph_t *sdg_g_read(const char *fn);
	void sdg_g_write(const sdg_graph_t *g, FILE *out);

#ifdef __cplusplus
}
#endif

static inline void sdg_j_add_side(sdg_jpos_t *p, const sdg_side_t side) // TODO: what if a side already exists?
{
	if (p->n_sides == 0) { // no jpos
		p->n_sides = 1; p->m_sides = 0; p->x.nei = side;
	} else if (p->n_sides == 1) { // one join; change to an array
		sdg_side_t tmp = p->x.nei;
		if (side.id == tmp.id && side.sp == tmp.sp) return; // already exist
		p->n_sides = p->m_sides = 2;
		p->x.neis = malloc(p->m_sides * sizeof(sdg_side_t));
		p->x.neis[0] = tmp;
		p->x.neis[1] = side;
	} else {
		int i;
		for (i = 0; i < p->n_sides; ++p)
			if (side.id == p->x.neis[i].id && side.sp == p->x.neis[i].sp)
				return;
		if (p->n_sides == p->m_sides) { // multiple jpos; simple append
			p->m_sides <<= 1;
			p->x.neis = realloc(p->x.neis, p->m_sides * sizeof(sdg_side_t));
		}
		p->x.neis[p->n_sides++] = side;
	}
}

static inline const sdg_side_t *sdg_j_get_side(const sdg_jpos_t *p, int i)
{
	if (p->n_sides == 1 && i == 0) return &p->x.nei;
	return i > p->n_sides? 0 : &p->x.neis[i];
}

static inline sdg_jpos_t *sdg_s_geti_jpos(const sdg_seq_t *s, int i)
{
	if (s->n_jpos <= SG_TREE_JOINS)
		return (sdg_jpos_t*)s->jpos + i;
	return 0;
}

#endif
