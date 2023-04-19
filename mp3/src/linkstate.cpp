#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <iostream>
#include <set>

using namespace std;

FILE *fpOut;
set<int> nodes;

unordered_map<int, unordered_map<int, int>> read_topo(char *filename) {
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Error: cannot open file %s\n", filename);
		exit(EXIT_FAILURE);
	}

	unordered_map<int, unordered_map<int, int>> topo;
	char linebuf[BUFSIZ];
	while (fgets(linebuf, BUFSIZ, fp) != NULL) {
		int src, dest, cost, num_scanned;
		num_scanned = sscanf(linebuf, "%d %d %d", &src, &dest, &cost);
		if (num_scanned < 3) {
			printf("scanned %d items from line: %s", num_scanned, linebuf);
			continue;
		}
		topo[src][dest] = cost;
		topo[dest][src] = cost;
		nodes.insert(src);
		nodes.insert(dest);
	}

	fclose(fp);
	return topo;
}

int main(int argc, char** argv) {
	//printf("Number of arguments: %d", argc);
	if (argc != 4) {
		printf("Usage: ./linkstate topofile messagefile changesfile\n");
		return EXIT_FAILURE;
	}

	unordered_map<int, unordered_map<int, int>> topo = read_topo(argv[1]);

	fpOut = fopen("output.txt", "w");
	fclose(fpOut);

	return EXIT_SUCCESS;
}
