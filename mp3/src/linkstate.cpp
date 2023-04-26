#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <iostream>
#include <set>
#include <unordered_set>
#include <algorithm>

using namespace std;

unordered_map<int, unordered_map<int, int>> read_topo(char *filename, set<int> &nodes) {
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

unordered_map<int, pair<int, int>> dijkstra(unordered_map<int, unordered_map<int, int>> topo, int src) {
	unordered_map<int, pair<int, int>> dist; // dist[node] = (cost, prev)
	for (auto node : topo) {
		dist[node.first] = make_pair(INT_MAX, -1);
	}
	dist[src].first = 0;

	priority_queue<pair<int, int>> pq;
	pq.push(make_pair(0, src));

	while (!pq.empty()) {
		pair<int, int> p = pq.top();
		pq.pop();
		int d = p.first;
		int u = p.second;
		if (d > dist[u].first) {
			continue;
		}
		for (auto v : topo[u]) { // v = (node, cost)
			if (dist[v.first].first > dist[u].first + v.second || (dist[v.first].first == dist[u].first + v.second && u < dist[v.first].second)) {
				dist[v.first].first = dist[u].first + v.second;
				dist[v.first].second = u;
				pq.push(make_pair(dist[v.first].first, v.first));
			}
		}
	}

	return dist;
}

vector<int> get_path(unordered_map<int, pair<int, int>> dist, int dest) {
	vector<int> path;
	int u = dest;
	while (u != -1) {
		path.push_back(u);
		u = dist[u].second;
	}
	reverse(path.begin(), path.end());
	return path;
}

unordered_map<int, unordered_map<int, pair<int, int>>> compute_routing_tables(unordered_map<int, unordered_map<int, int>> topo, FILE *fpOut, set<int> nodes) {
	unordered_map<int, unordered_map<int, pair<int, int>>> dists;
	for (auto src : nodes) {
		auto dist = dijkstra(topo, src);
		dists[src] = dist;
		for (auto dst : nodes) {
			if (src == dst) {
				fprintf(fpOut, "%d %d %d\n", dst, dst, 0);
				continue;
			}
			if (dist[dst].first == INT_MAX) {
				continue;
			}
			auto path = get_path(dist, dst);
			fprintf(fpOut, "%d %d %d\n", dst, path[1], dist[dst].first);
		}
	}
	return dists;
}

void process_message_file(char *filename, FILE *fpOut, unordered_map<int, unordered_map<int, pair<int, int>>> dists) {
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Error: cannot open file %s\n", filename);
		exit(EXIT_FAILURE);
	}

	char linebuf[BUFSIZ];
	char *message;
	while (fgets(linebuf, BUFSIZ, fp) != NULL) {
		int src, dest, num_scanned;
		num_scanned = sscanf(linebuf, "%d %d", &src, &dest);
		if (num_scanned < 2) {
			printf("scanned %d items from line: %s", num_scanned, linebuf);
			continue;
		}
		strtok(linebuf, " ");
		strtok(NULL, " ");
		message = strtok(NULL, "\n");
		auto dist = dists[src];
		if (dist[dest].first == INT_MAX) {
			fprintf(fpOut, "from %d to %d cost infinite hops unreachable message %s\n", src, dest, message);
			continue;
		}
		auto path = get_path(dist, dest);
		fprintf(fpOut, "from %d to %d cost %d hops ", src, dest, dist[dest].first);
		for (int i = 0; i < path.size() - 1; i++) {
			fprintf(fpOut, "%d ", path[i]);
		}
		fprintf(fpOut, "message %s\n", message);
	}
	fclose(fp);
}

void process_change_file(char *filename, char *message_filename, FILE *fpOut, unordered_map<int, unordered_map<int, int>> topo, set<int> nodes) {
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Error: cannot open file %s\n", filename);
	}

	char linebuf[BUFSIZ];
	while (fgets(linebuf, BUFSIZ, fp) != NULL) {
		int src, dest, cost, num_scanned;
		num_scanned = sscanf(linebuf, "%d %d %d", &src, &dest, &cost);
		if (num_scanned < 3) {
			printf("scanned %d items from line: %s", num_scanned, linebuf);
			continue;
		}
		if (cost == -999) {
			topo[src].erase(dest);
			topo[dest].erase(src);
		}
		else {
			topo[src][dest] = cost;
			topo[dest][src] = cost;
		}
		nodes.insert(src);
		nodes.insert(dest);
		auto dists = compute_routing_tables(topo, fpOut, nodes);
		process_message_file(message_filename, fpOut, dists);
	}
	fclose(fp);
}

int main(int argc, char** argv) {
	//printf("Number of arguments: %d", argc);
	if (argc != 4) {
		printf("Usage: %s topofile messagefile changesfile\n", argv[0]);
		return EXIT_FAILURE;
	}

	set<int> nodes;
	auto topo = read_topo(argv[1], nodes);
	FILE *fpOut = fopen("output.txt", "w");
	if (fpOut == NULL) {
		printf("Error: cannot open file output.txt\n");
		return EXIT_FAILURE;
	}

	auto dists = compute_routing_tables(topo, fpOut, nodes);
	process_message_file(argv[2], fpOut, dists);
	process_change_file(argv[3], argv[2], fpOut, topo, nodes);

	fclose(fpOut);
	return EXIT_SUCCESS;
}
