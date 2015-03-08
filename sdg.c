#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>
#include "sdg.h"

#include "kbtree.h"
#define join_cmp(a, b) ((a).sp - (b).sp)
KBTREE_INIT(j, sdg_join_t, join_cmp)
typedef kbtree_t(j) jtree_t;

sdg_join_t *sdg_s_add_side(sdg_seq_t *s, int64_t sp)
{
	uint32_t i;
	if (s->n_joins < SG_TREE_JOINS) { // then s->p is an array
		sdg_join_t *a = (sdg_join_t*)s->joins;
		for (i = 0; i < s->n_joins; ++i)
			if (a[i].sp >= sp) break;
		if (i == s->n_joins || a[i].sp != sp) { // a new side to add
			if (s->n_joins == s->m_joins) {
				s->m_joins = s->m_joins? s->m_joins<<1 : 2;
				s->joins = a = realloc(a, s->m_joins * sizeof(sdg_join_t));
			}
			if (i < s->n_joins) // make room for insertion
				memmove(&a[i+1], &a[i], (s->n_joins - i) * sizeof(sdg_join_t));
			++s->n_joins;
			memset(&a[i], 0, sizeof(sdg_join_t));
		}
		return &a[i];
	} else { // then s->p is or will be a tree
		jtree_t *t;
		sdg_join_t *a = (sdg_join_t*)s->joins, tmp, *r;
		if (s->n_joins == SG_TREE_JOINS) { // then convert s->p to a tree
			t = kb_init(j, 512);
			for (i = 0; i < s->n_joins; ++i) kb_putp(j, t, &a[i]);
			free(s->joins);
			s->joins = t;
			s->m_joins = 0;
		} else t = (jtree_t*)s->joins;
		memset(&tmp, 0, sizeof(sdg_join_t));
		tmp.sp = sp;
		if ((r = kb_getp(j, t, &tmp)) == 0)
			r = kb_putp(j, t, &tmp);
		s->n_joins = kb_size(t);
		return r;
	}
}

sdg_join_t *sdg_s_get_join(const sdg_seq_t *s, int64_t sp)
{
	if (s->n_joins <= SG_TREE_JOINS) {
		uint32_t i;
		sdg_join_t *a = (sdg_join_t*)s->joins;
		for (i = 0; i < s->n_joins; ++i)
			if (a[i].sp == sp) return &a[i];
		return 0;
	} else {
		jtree_t *t = (jtree_t*)s->joins;
		sdg_join_t tmp;
		tmp.sp = sp;
		return kb_getp(j, t, &tmp);
	}
}

#include "khash.h"
KHASH_MAP_INIT_STR(n, int64_t)
typedef khash_t(n) nhash_t;

sdg_graph_t *sdg_g_init(void)
{
	sdg_graph_t *g;
	g = calloc(1, sizeof(sdg_graph_t));
	g->hash = kh_init(n);
	return g;
}

void sdg_g_destroy(sdg_graph_t *g) // TODO
{
	int64_t i;
	if (g == 0) return;
	for (i = 0; i < g->n_seqs; ++i) {
		sdg_seq_t *s = &g->seqs[i];
		if (s->n_joins <= SG_TREE_JOINS) free(s->joins);
		else kb_destroy(j, ((jtree_t*)s->joins));
		free(s->name);
	}
	free(g->seqs);
	kh_destroy(n, g->hash);
	free(g);
}

int64_t sdg_g_get_seq_id(const sdg_graph_t *g, const char *name)
{
	nhash_t *h = (nhash_t*)g->hash;
	khint_t k;
	k = kh_get(n, h, name);
	return k == kh_end(h)? -1 : kh_val(h, k);
}

int64_t sdg_g_add_seq(sdg_graph_t *g, const char *name, int64_t len)
{
	sdg_seq_t *s;
	nhash_t *h = (nhash_t*)g->hash;
	khint_t k;
	int absent;
	k = kh_put(n, h, name, &absent);
	if (!absent) return kh_val(h, k); // added before
	if (g->n_seqs == g->m_seqs) {
		g->m_seqs = g->m_seqs? g->m_seqs<<1 : 16;
		g->seqs = realloc(g->seqs, g->m_seqs * sizeof(sdg_seq_t));
	}
	s = &g->seqs[g->n_seqs++];
	s->len = len;
	kh_key(h, k) = s->name = strdup(name);
	kh_val(h, k) = s->id   = g->n_seqs - 1;
	return s->id;
}

void sdg_g_add_join1(sdg_graph_t *g, const sdg_side_t s1, const sdg_side_t s2)
{
	sdg_join_t *j;
	j = sdg_s_add_side(&g->seqs[s1.id], s1.sp);
	sdg_j_add_side(j, s2);
}

int sdg_g_add_join(sdg_graph_t *g, const sdg_side_t s1, const sdg_side_t s2)
{
	if (s1.id > g->n_seqs || s2.id > g->n_seqs) return -1;
	sdg_g_add_join1(g, s1, s2);
	sdg_g_add_join1(g, s2, s1);
	return 0;
}
