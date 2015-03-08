#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>
#include "sdg.h"

/*******************
 * jpos operations *
 *******************/

#include "kbtree.h"
#define join_cmp(a, b) ((a).sp - (b).sp)
KBTREE_INIT(j, sdg_jpos_t, join_cmp)
typedef kbtree_t(j) jtree_t;

sdg_jpos_t *sdg_s_add_jpos(sdg_seq_t *s, int64_t sp)
{
	uint32_t i;
	if (s->n_jpos < SG_TREE_JOINS) { // then s->p is an array
		sdg_jpos_t *a = (sdg_jpos_t*)s->jpos;
		for (i = 0; i < s->n_jpos; ++i)
			if (a[i].sp >= sp) break;
		if (i == s->n_jpos || a[i].sp != sp) { // a new side to add
			if (s->n_jpos == s->m_jpos) {
				s->m_jpos = s->m_jpos? s->m_jpos<<1 : 2;
				s->jpos = a = realloc(a, s->m_jpos * sizeof(sdg_jpos_t));
			}
			if (i < s->n_jpos) // make room for insertion
				memmove(&a[i+1], &a[i], (s->n_jpos - i) * sizeof(sdg_jpos_t));
			++s->n_jpos;
			memset(&a[i], 0, sizeof(sdg_jpos_t));
			a[i].sp = sp;
		}
		return &a[i];
	} else { // then s->p is or will be a tree
		jtree_t *t;
		sdg_jpos_t *a = (sdg_jpos_t*)s->jpos, tmp, *r;
		if (s->n_jpos == SG_TREE_JOINS) { // then convert s->p to a tree
			t = kb_init(j, 512);
			for (i = 0; i < s->n_jpos; ++i) kb_putp(j, t, &a[i]);
			free(s->jpos);
			s->jpos = t;
			s->m_jpos = 0;
		} else t = (jtree_t*)s->jpos;
		memset(&tmp, 0, sizeof(sdg_jpos_t));
		tmp.sp = sp;
		if ((r = kb_getp(j, t, &tmp)) == 0)
			r = kb_putp(j, t, &tmp);
		s->n_jpos = kb_size(t);
		return r;
	}
}

sdg_jpos_t *sdg_s_get_jpos(const sdg_seq_t *s, int64_t sp)
{
	if (s->n_jpos <= SG_TREE_JOINS) {
		uint32_t i;
		sdg_jpos_t *a = (sdg_jpos_t*)s->jpos;
		for (i = 0; i < s->n_jpos; ++i)
			if (a[i].sp == sp) return &a[i];
		return 0;
	} else {
		jtree_t *t = (jtree_t*)s->jpos;
		sdg_jpos_t tmp;
		tmp.sp = sp;
		return kb_getp(j, t, &tmp);
	}
}

/*****************
 * jpos iterator *
 *****************/

struct sdg_ji_t {
	sdg_seq_t *s;
	kbitr_t itr;
	int32_t i;
};

sdg_ji_t *sdg_ji_first(sdg_seq_t *s)
{
	sdg_ji_t *itr;
	itr = calloc(1, sizeof(sdg_ji_t));
	itr->s = s;
	if (s->n_jpos > SG_TREE_JOINS) { // s->jpos is a B-tree
		itr->i = -1;
		kb_itr_first_j((jtree_t*)s->jpos, &itr->itr);
	} else itr->i = 0;
	return itr;
}

int sdg_ji_next(sdg_ji_t *itr)
{
	if (itr->i >= 0) {
		if (++itr->i >= itr->s->n_jpos)
			return 0;
		else return 1;
	} else return kb_itr_next_j((jtree_t*)itr->s->jpos, &itr->itr);
}

sdg_jpos_t *sdg_ji_at(sdg_ji_t *itr)
{
	if (itr->i >= 0) {
		if (itr->i >= itr->s->n_jpos) return 0;
		sdg_jpos_t *a = (sdg_jpos_t*)itr->s->jpos;
		return &a[itr->i];
	} else return &kb_itr_key(sdg_jpos_t, &itr->itr);
}

/********************
 * Other operations *
 ********************/

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
		if (s->n_jpos <= SG_TREE_JOINS) free(s->jpos);
		else kb_destroy(j, ((jtree_t*)s->jpos));
		free(s->name);
	}
	free(g->seqs);
	kh_destroy(n, g->hash);
	free(g);
}

sdg_seq_t *sdg_g_get_seq(const sdg_graph_t *g, const char *name)
{
	nhash_t *h = (nhash_t*)g->hash;
	khint_t k;
	k = kh_get(n, h, name);
	return k == kh_end(h)? 0 : &g->seqs[kh_val(h, k)];
}

sdg_seq_t *sdg_g_add_seq(sdg_graph_t *g, const char *name)
{
	sdg_seq_t *s;
	nhash_t *h = (nhash_t*)g->hash;
	khint_t k;
	int absent;
	k = kh_put(n, h, name, &absent);
	if (!absent) return &g->seqs[kh_val(h, k)]; // added before
	if (g->n_seqs == g->m_seqs) {
		g->m_seqs = g->m_seqs? g->m_seqs<<1 : 16;
		g->seqs = realloc(g->seqs, g->m_seqs * sizeof(sdg_seq_t));
	}
	s = &g->seqs[g->n_seqs++];
	s->len = -1;
	kh_key(h, k) = s->name = strdup(name);
	kh_val(h, k) = s->id   = g->n_seqs - 1;
	return s;
}

void sdg_g_add_join1(sdg_graph_t *g, const sdg_side_t s1, const sdg_side_t s2)
{
	sdg_jpos_t *j;
	j = sdg_s_add_jpos(&g->seqs[s1.id], s1.sp);
	sdg_j_add_side(j, s2);
}

int sdg_g_add_join(sdg_graph_t *g, const sdg_side_t s1, const sdg_side_t s2)
{
	if (s1.id > g->n_seqs || s2.id > g->n_seqs) return -1;
	sdg_g_add_join1(g, s1, s2);
	sdg_g_add_join1(g, s2, s1);
	return 0;
}
