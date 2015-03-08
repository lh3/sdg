#ifndef SIDEGRAPH_H
#define SIDEGRAPH_H

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
	int64_t sp;
	uint32_t n_sides, m_sides;
	union {
		sdg_side_t nei;
		sdg_side_t *neis;
	} x;
} sdg_join_t;

typedef struct {
	int64_t id;  // sequence ID
	int64_t len; // len<0 if length is not available
	char *name;  // sequence name
	uint32_t n_joins, m_joins;
	void *joins; // an array if n_joins<SG_TREE_JOINS; otherwise a B-tree
} sdg_seq_t;

typedef struct {
	int64_t n_seqs, m_seqs, max_rank;
	sdg_seq_t *seqs;
	void *hash;
} sdg_graph_t;

#ifdef __cplusplus
extern "C" {
#endif

	sdg_graph_t *sdg_g_init(void);
	void sdg_g_destroy(sdg_graph_t *g);

	int64_t sdg_s_getid(const sdg_graph_t *g, const char *name);
	int64_t sdg_s_add(sdg_graph_t *g, const char *name, int64_t len);
	int64_t sdg_j_add(sdg_graph_t *g, sdg_side_t s1, sdg_side_t s2);

#ifdef __cplusplus
}
#endif

static inline void sdg_j_append_side(sdg_join_t *p, sdg_side_t side)
{
	if (p->n_sides == 0) { // no joins
		p->n_sides = 1; p->m_sides = 0; p->x.nei = side;
	} else if (p->n_sides == 1) { // one join; change to an array
		sdg_side_t tmp = p->x.nei;
		p->n_sides = p->m_sides = 2;
		p->x.neis = malloc(p->m_sides * sizeof(sdg_side_t));
		p->x.neis[0] = tmp;
		p->x.neis[1] = side;
	} else {
		if (p->n_sides == p->m_sides) { // multiple joins; simple append
			p->m_sides <<= 1;
			p->x.neis = realloc(p->x.neis, p->m_sides * sizeof(sdg_side_t));
		}
		p->x.neis[p->n_sides++] = side;
	}
}

#endif
