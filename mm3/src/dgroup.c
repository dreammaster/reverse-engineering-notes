#include "dgroup.h"

#include <stdio.h>
#include <stdlib.h>

int mm3_dgroup_load(Mm3Dgroup *dg, const char *path) {
	FILE *f = fopen(path, "rb");
	dg->data = NULL;
	if (!f)
		return -1;
	dg->data = malloc(MM3_DGROUP_SIZE);
	if (!dg->data || fread(dg->data, 1, MM3_DGROUP_SIZE, f) != MM3_DGROUP_SIZE) {
		free(dg->data);
		dg->data = NULL;
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

void mm3_dgroup_free(Mm3Dgroup *dg) {
	free(dg->data);
	dg->data = NULL;
}
