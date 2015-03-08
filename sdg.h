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
	int64_t n_seqs, m_seqs;
	sdg_seq_t *seqs;
	void *hash;
} sdg_graph_t;

#ifdef __cplusplus
extern "C" {
#endif

	sdg_graph_t *sdg_g_init(void);
	void sdg_g_destroy(sdg_graph_t *g);

	sdg_join_t *sdg_s_add_side(sdg_seq_t *s, int64_t sp);

	int64_t sdg_g_get_seq_id(const sdg_graph_t *g, const char *name);
	int64_t sdg_g_add_seq(sdg_graph_t *g, const char *name, int64_t len);
	int sdg_g_add_join(sdg_graph_t *g, const sdg_side_t s1, const sdg_side_t s2);

#ifdef __cplusplus
}
#endif

#endif
