#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "sdg.h"

int main_reformat(int argc, char *argv[])
{
	int c;
	sdg_graph_t *g;
	while ((c = getopt(argc, argv, "")) >= 0) {
	}

	if (optind + 1 > argc) {
		fprintf(stderr, "Usage: sdg reformat <in.sdg>\n");
		return 1;
	}
	g = sdg_g_read(argv[optind]);
	sdg_g_write(g, stdout);
	sdg_g_destroy(g);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "\n");
		fprintf(stderr, "Usage:   sdg <command> <arguments>\n\n");
		fprintf(stderr, "Command: reformat    reformat a side graph\n");
		fprintf(stderr, "\n");
		return 1;
	}
	if (strcmp(argv[1], "reformat") == 0) return main_reformat(argc-1, argv+1);
	else {
		fprintf(stderr, "ERROR: unrecognized command '%s'\n", argv[1]);
		return 1;
	}
}
