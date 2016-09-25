#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>

template<class NodeId>
struct GraphNode {
	GraphNode() = default;
	GraphNode(NodeId _id)
		: id(_id)
	{}

	NodeId id;
	std::vector<NodeId> outEdges;
	std::vector<NodeId> inEdges;
};

template<class NodeId>
struct GraphEdge {
	GraphEdge() = default;
	GraphEdge(NodeId _srcId, NodeId _destId)
		: srcId(_srcId)
		, destId(_destId)
	{}
	NodeId srcId;
	NodeId destId;
};

template<class NodeId>
class Graph {
public:
	int getNodeCount() const noexcept {
		return static_cast<int>(edges.size());
	}

	int getNodeOutEdgeCount(NodeId id) const noexcept {
		const auto& indexIt = indexMap.find(id);
		if (indexIt != indexMap.end()) {
			return static_cast<int>(nodes[indexIt->second].outEdges.size());
		}
		return 0;
	}

	int getNodeInEdgeCount(NodeId id) const noexcept {
		const auto& indexIt = indexMap.find(id);
		if (indexIt != indexMap.end()) {
			return static_cast<int>(nodes[indexIt->second].inEdges.size());
		}
		return 0;
	}

	std::vector<NodeId> getNodeList() const noexcept {
		std::vector<NodeId> retval;
		for (const auto& nodeIt : nodes) {
			retval.push_back(nodeIt.id);
		}
		return retval;
	}

	void addNode(NodeId id) {
		// protect from readding the same id!
		if (indexMap.find(id) == indexMap.end()) {
			const int idx = static_cast<int>(nodes.size());
			indexMap[id] = idx;
			nodes.push_back(GraphNode<NodeId>(id));
		}
	}

	void removeNode(NodeId id) {
		const auto& indexIt = indexMap.find(id);
		if (indexIt != indexMap.end()) {
			const int idx = indexIt->second;
			// first remove all edges of the node
			const GraphNode<NodeId>& rNode = nodes[idx];
			// make copies of the in and out edges map, because they will be changed
			// TODO : this may be optimized by removing the edges directly from the connected nodes
			const std::vector<NodeId> inEdges(rNode.inEdges);
			for (const auto& ie : inEdges) {
				removeEdge(ie, id);
			}
			const std::vector<NodeId> outEdges(rNode.outEdges);
			for (const auto& oe : outEdges) {
				removeEdge(id, oe);
			}
			nodes.erase(nodes.begin() + idx);
			indexMap.erase(id);
			// the index map has to be reevaluated to account for the removed node
			for (int i = 0; i < nodes.size(); ++i) {
				indexMap[nodes[i].id] = i;
			}
		}
	}

	std::vector<GraphEdge<NodeId> > getEdgeList() const {
		std::vector<GraphEdge> retval;
		retval.reserve(nodes.size());
		for (const auto& n : nodes) {
			const GraphNode<NodeId>& node = n;
			for (const auto& e : node.outEdges) {
				retval.push_back(GraphEdge(node.id, e));
			}
		}
		return retval;
	}

	std::vector<NodeId> getOutEdgeList(NodeId id) const {
		std::vector<NodeId> retval;
		const auto& indexIt = indexMap.find(id);
		if (indexIt != indexMap.end()) {
			retval = nodes[indexIt->second].outEdges;
		}
		return retval;
	}

	std::vector<NodeId> getInEdgeList(NodeId id) const {
		std::vector<NodeId> retval;
		const auto& indexIt = indexMap.find(id);
		if (indexIt != indexMap.end()) {
			retval = nodes[indexIt->second].inEdges;
		}
		return retval;
	}

	void addEdge(NodeId srcId, NodeId destId) {
		auto& srcIt = indexMap.find(srcId);
		auto& destIt = indexMap.find(destId);
		if (srcIt != indexMap.end() && destIt != indexMap.end()) {
			GraphNode<NodeId>& srcNode = nodes[srcIt->second];
			bool connected = false;
			for (auto& e : srcNode.outEdges) {
				if (e == destId) {
					connected = true;
					break;
				}
			}
			if (!connected) {
				srcNode.outEdges.push_back(destId);
				GraphNode<NodeId>& destNode = nodes[destIt->second];
				destNode.inEdges.push_back(srcId);
			}
		}
	}

	void removeEdge(NodeId srcId, NodeId destId) {
		auto& srcIt = indexMap.find(srcId);
		auto& destIt = indexMap.find(destId);
		if (srcIt != indexMap.end() && destIt != indexMap.end()) {
			GraphNode<NodeId>& srcNode = nodes[srcIt->second];
			for (auto& it = srcNode.outEdges.begin(); it != srcNode.outEdges.end(); ++it) {
				if (*it == destId) {
					srcNode.outEdges.erase(it);
					break;
				}
			}
			// now remove the inward edge
			GraphNode<NodeId>& destNode = nodes[destIt->second];
			for (auto& it = destNode.inEdges.begin(); it != destNode.inEdges.end(); ++it) {
				if (*it == srcId) {
					destNode.inEdges.erase(it);
					break;
				}
			}
		}
	}

	std::vector<NodeId> listNodesInValency(int valency) const {
		std::vector<NodeId> res;
		for (const auto& n : nodes) {
			if (n.inEdges.size() == valency) {
				res.push_back(n.id);
			}
		}
		return res;
	}

	std::vector<NodeId> listNodesOutValency(int valency) const {
		std::vector<NodeId> res;
		for (const auto& n : nodes) {
			if (n.outEdges.size() == valency) {
				res.push_back(n.id);
			}
		}
		return res;
	}

	bool toposort(std::vector<NodeId>& res) const {
		if (nodes.size() == 0)
			return false;
		// the root list will contain all nodeId that have 0 valency
		std::vector<NodeId> rootsList;
		// and this map will contain a vector of out input edges of remaining nodes
		std::unordered_map<NodeId, std::vector<NodeId> > inEdges;
		for (const auto& node : nodes) {
			const GraphNode<NodeId>& n = node;
			if (n.inEdges.size() == 0) {
				rootsList.push_back(n.id);
			} else {
				inEdges[n.id] = n.inEdges;
			}
		}
		// check if there are any nodes with zero inputs
		if (rootsList.empty()) {
			return false;
		} else if (inEdges.empty()) {
			// the graph is not connected -> any ordering is a valid sorting
			res = rootsList;
			return true;
		}
		res.clear();
		res.reserve(nodes.size());
		std::queue<NodeId> nodeQueue;
		for (const auto& rootNode : rootsList) {
			nodeQueue.push(rootNode);
		}

		bool cycle = false;
		// now start the toposort itself
		while (!nodeQueue.empty() && !cycle) {
			const NodeId n = nodeQueue.front();
			nodeQueue.pop();
			res.push_back(n);
			// for each node m with an edge from n to m
			const auto nIdx = indexMap.find(n)->second;
			const std::vector<NodeId>& nOutEdges = nodes[nIdx].outEdges;
			for (const auto& m : nOutEdges) {
				// check if there is an m id in the inEdges - if there is not - there is a cycle
				auto & ieIt = inEdges.find(m);
				if (ieIt == inEdges.end()) {
					cycle = true;
					break;
				}
				// remove that edge from the inEdges map
				std::vector<NodeId>& mIns = inEdges[m];
				for (auto& it = mIns.begin(); it != mIns.end(); ++it) {
					if (*it == n) {
						mIns.erase(it);
						break;
					}
				}
				// check if the vector of edges is empty
				if (mIns.empty()) {
					// if it is -> add m to the queue and remove the key from the indexMap
					nodeQueue.push(m);
					inEdges.erase(m);
				}
			}
		}
		if (cycle || !inEdges.empty()) {
			// loop encountered - there are still edges in the inward edge map
			return false;
		} else {
			return true;
		}
	}

	// although this may be done with the toposort this is presumably faster and less resource exhausting
	bool isDAG() const {
		const int nodeCount = static_cast<int>(nodes.size());
		if (nodeCount <= 1) {
			// either empty or just one node
			return true;
		}
		// get a list of all the root nodes (with no inEdges)
		const std::vector<NodeId> rootsList(listNodesInValency(0));
		if (rootsList.empty()) {
			// no roots but non-empty node list - definitely a loop exists
			return false;
		} else if (nodeCount == rootsList.size()) {
			// unconnected graph - therefore no loops
			return true;
		}
		// the DAG check is perfermed as DFS form each edge that keeps a stack and looks if a loop is encoutered
		struct StackInfo {
			NodeId nId;
			int arrayIdx;
			int currentChild; //!< how far the iteration for the node has gone
			StackInfo(NodeId _nId = NodeId(), int _arrayIdx = -1, int _currentChild = 0)
				: nId(_nId)
				, arrayIdx(_arrayIdx)
				, currentChild(_currentChild)
			{}
		};
		std::unique_ptr<StackInfo[]> stackInfo(new StackInfo[nodeCount]);
		static const int UNVISITED = -1;
		// will hold -1 if the node was not visited, otherwise it will hold the iteration of the root node that first visited it
		std::vector<int> visited;
		visited.resize(nodeCount);
		for (auto& v : visited) {
			v = UNVISITED;
		}
		// now start the actual DFS from each root idx
		bool loopFound = false;
		for (int rootIdx = 0; rootIdx < rootsList.size() && !loopFound; ++rootIdx) {
			int stackTop = 0;
			int nodeIdx = indexMap.find(rootsList[rootIdx])->second;
			stackInfo[stackTop] = StackInfo(rootsList[rootIdx], nodeIdx, 0);
			// visit the node
			visited[nodeIdx] = rootIdx;
			stackTop++;
			// now starts the DFS loop over the stack
			while (stackTop > 0 && !loopFound) {
				stackTop--;
				StackInfo& sti = stackInfo[stackTop];
				// check if this node has more childs to iterate
				if (sti.currentChild < nodes[sti.arrayIdx].outEdges.size()) {
					// make a forward step keeping this node on the stack but iterating the child index
					const NodeId childId = nodes[sti.arrayIdx].outEdges[sti.currentChild];
					sti.currentChild++;
					stackTop++; // just iterate the stack top, the info is till there
					const int childArrayIdx = indexMap.find(childId)->second;
					// if the child is not visited - add it to the stack
					if (visited[childArrayIdx] == UNVISITED) {
						stackInfo[stackTop] = StackInfo(childId, childArrayIdx, 0);
						visited[childArrayIdx] = rootIdx; // visit the child
						stackTop++;
						// and continue with the next iteration
					} else if (visited[childArrayIdx] == rootIdx) {
						// if this child was visited by the same root idx - we should check if it is present currently in the stack
						// if it is this would mean a loop has been found - otherwise it was checked from a previous node and should
						// not be rechecked
						for (int i = 0; i < stackTop; ++i) {
							if (stackInfo[i].nId == childId) {
								loopFound = true;
								break;
							}
						}
						// just continue - the loopFound will break from all loops
					} // else this child was found from another rootIdx and is not going to a loop
				}
			}
		}
		return !loopFound;
	}

	// fills res with a BFS list starting from the node (WITH the node itself as the first element)
	// additionally returns true if any loops were encounterd (comes free so why not?) - still this is NOT a full DAG check
	bool BFS(std::vector<NodeId>& res, NodeId nid) const {
		const auto& indexIt = indexMap.find(nid);
		res.clear();
		bool retval = false;
		if (indexIt != indexMap.end()) {
			std::vector<bool> visited;
			visited.resize(nodes.size());
			for (auto& v : visited) {
				v = false;
			}
			std::queue<NodeId> searchList;
			searchList.push(nid);
			while (!searchList.empty()) {
				const NodeId nodeId = searchList.front();
				searchList.pop();
				const int nodeIdx = indexMap.find(nodeId)->second;
				// check if the node has already been visited
				if (visited[nodeIdx]) {
					retval = true;
					// continue with iterations skipping this node - it was visited
				} else {
					visited[nodeIdx] = true;
					// add it to the resulting list and add its children to the queue
					res.push_back(nodeId);
					for (const auto& childId : nodes[nodeIdx].outEdges) {
						searchList.push(childId);
					}
				}
			}
		}
		return retval;
	}

	// TODO - DFS if necessary

private:
	std::vector<GraphNode<NodeId> > nodes;
	std::unordered_map<NodeId, int> indexMap;
};

#endif // __GRAPH_H__
