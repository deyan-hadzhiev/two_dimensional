#include <iostream>
#include <string>

#include "util.h"
#include "graph.h"

using namespace std;

void fillGraph(Graph<int>& g) {
	g.addNode(0);
	g.addNode(3);
	g.addNode(7);
	g.addNode(2);
	g.addNode(4);
	g.addNode(5);
	g.addNode(8);
	g.addNode(1);
	g.addNode(6);
	// add eges
	g.addEdge(0, 3);
	g.addEdge(7, 2);
	g.addEdge(2, 3);
	g.addEdge(3, 4);
	g.addEdge(1, 4);
	g.addEdge(5, 6);
	g.addEdge(8, 4);
	g.addEdge(1, 8);
	g.addEdge(5, 4);
}

void printGraph(Graph<int>& g) {
	const vector<int>& nodeList = g.getNodeList();
	for (const auto& n : nodeList) {
		cout << n << ": ";
		const vector<int>& inEdges = g.getInEdgeList(n);
		for (const auto& e : inEdges) {
			cout << e << " ";
		}
		cout << " | ";
		const vector<int>& outEdges = g.getOutEdgeList(n);
		for (const auto& e : outEdges) {
			cout << e << " ";
		}
		cout << endl;
	}
}

void printTopoSort(const Graph<int>& g) {
	vector<int> order;
	if (g.toposort(order)) {
		for (const auto& n : order) {
			cout << n << " ";
		}
		cout << endl;
	} else {
		cout << "ERROR: Graph is not a DAG" << endl;
	}
}

void printBFS(const Graph<int>& g, int startId) {
	std::vector<int> bfsRes;
	const bool loop = g.BFS(bfsRes, startId);
	cout << "BFS (loops? - " << (loop ? "true" : "false") << ") : ";
	for (const auto& nid : bfsRes) {
		cout << nid << " ";
	}
	cout << endl;
}

int main() {
	Graph<int> g;
	fillGraph(g);
	printGraph(g);

	cout << "===========================" << endl;
	cout << "graph " <<  (g.isDAG() ? "is" : "is NOT") << " a DAG!" << endl;
	printTopoSort(g);
	printBFS(g, 2);
	cout << "---------------------------" << endl;

	g.removeEdge(5, 4);
	g.removeNode(8);
	printGraph(g);

	cout << "===========================" << endl;
	cout << "graph " << (g.isDAG() ? "is" : "is NOT") << " a DAG!" << endl;
	printTopoSort(g);
	printBFS(g, 2);
	cout << "---------------------------" << endl;

	// make a deliberate cycle
	g.addEdge(4, 5);
	g.addEdge(6, 7);
	g.addEdge(7, 4);

	cout << "===========================" << endl;
	const bool dag = g.isDAG();
	cout << "graph " << (g.isDAG() ? "is" : "is NOT") << " a DAG!" << endl;
	printTopoSort(g);
	printBFS(g, 2);
	printBFS(g, 7);
	printBFS(g, 1);
	cout << "---------------------------" << endl;

	g.removeNode(2);

	cout << "===========================" << endl;
	cout << "graph " << (g.isDAG() ? "is" : "is NOT") << " a DAG!" << endl;
	printTopoSort(g);
	printBFS(g, 2);
	cout << "---------------------------" << endl;

	// make some more cycles
	g.addNode(2);
	g.addEdge(2, 3);
	g.addEdge(7, 2);
	g.addEdge(6, 0);
	g.addEdge(4, 1);
	g.addEdge(5, 0);
	printGraph(g);
	cout << "===========================" << endl;
	cout << "graph " << (g.isDAG() ? "is" : "is NOT") << " a DAG!" << endl;
	printTopoSort(g);
	printBFS(g, 2);
	cout << "---------------------------" << endl;

	return 0;
}
