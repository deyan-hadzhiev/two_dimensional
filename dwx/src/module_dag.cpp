#include "graph.h"
#include "module_manager.h"
#include "module_dag.h"
#include "progress.h"

bool ModuleConnector::getInput(Bitmap & bmp, int idx) const {
	bool res = false;
	if (idx >= 0 && idx < srcModules.size() && srcModules[idx] != nullptr) {
		const ModuleId mid = srcModules[idx]->module->getModuleId();
		res = getBitmap(bmp, mid);
	}
	return res;
}

void ModuleConnector::setOutput(const Bitmap & bmp, ModuleId mid) {
	{
		std::lock_guard<std::mutex> lk(bmpMapMutex);
		bmpMap[mid] = std::shared_ptr<Bitmap>(new Bitmap(bmp));
	}
	// check for external output manager
	const auto& it = externalOutputMap.find(mid);
	if (it != externalOutputMap.end() && it->second) {
		it->second->setOutput(bmp, mid);
	}
	// TODO check if all the inputs are ready
	destModule->module->update();
}

bool ModuleConnector::getBitmap(Bitmap & bmp, ModuleId mid) const {
	bool res = false;
	const auto& it = bmpMap.find(mid);
	if (it != bmpMap.end() && it->second) {
		std::lock_guard<std::mutex> lk(bmpMapMutex);
		bmp = *(it->second);
		res = true;
	}
	return res;
}

void ModuleConnector::setDestModule(ModuleNode * moduleNode) {
	destModule = moduleNode;
	moduleNode->module->setInputManager(this);
}

void ModuleConnector::setSrcModule(ModuleNode * moduleNode, int srcIdx) {
	// can not have a source module without a dest one
	if (destModule != nullptr) {
		// now check if the vector is resize properly
		if (srcModules.size() != destModule->moduleDesc.inputs) {
			srcModules.resize(destModule->moduleDesc.inputs, nullptr);
		}
		srcModules[srcIdx] = moduleNode;
		// before setting the output manager check the module for external outputs
		OutputManager * externalOman = moduleNode->module->getOutputManager();
		if (externalOman != nullptr) {
			// set it as an external output manager
			setExternalOutput(moduleNode->module->getModuleId(), externalOman);
		}
		moduleNode->module->setOutputManager(this);
	}
}

bool ModuleConnector::removeConnection(int destSrcIdx) {
	ModuleNode * srcNode = srcModules[destSrcIdx];
	if (srcNode != nullptr) {
		srcModules[destSrcIdx] = nullptr;
		const ModuleId mid = srcNode->module->getModuleId();
		// if the module had an external output manager - set it instead of this - otherwise set to null
		const auto& extOmanIt = externalOutputMap.find(mid);
		if (extOmanIt != externalOutputMap.end()) {
			srcNode->module->setOutputManager(extOmanIt->second);
			// remove the output manager from the map
			externalOutputMap.erase(mid);
		} else {
			srcNode->module->setOutputManager(nullptr);
		}
		// also free memory from this input
		const auto& bmpIt = bmpMap.find(mid);
		if (bmpIt != bmpMap.end()) {
			std::lock_guard<std::mutex> lk(bmpMapMutex);
			// just erase the entry in the map - the shared_ptr will take care of the rest
			bmpMap.erase(mid);
		}
		// invalidate the module output connector id
		srcNode->outputConnector = InvalidConnectorId;
	}
	// now check if the connector is left without any source modules
	// if there is no source modules left - invalidate the destination node as well
	bool hasSrcNodes = false;
	for (int i = 0; i < srcModules.size() && !hasSrcNodes; ++i) {
		hasSrcNodes |= srcModules[i] != nullptr;
	}
	if (!hasSrcNodes) {
		// invalidate the destination module since this connector will be deleted (probably)
		destModule->module->setInputManager(nullptr);
		destModule->inputConnector = InvalidConnectorId;
		// just in case this doesnt get destroyed
		destModule = nullptr;
	}
	return !hasSrcNodes;
}

void ModuleConnector::setExternalOutput(ModuleId mid, OutputManager * oman) {
	externalOutputMap[mid] = oman;
	// check if there is already any output and add it to the OutputManager
	const auto& bmpIt = bmpMap.find(mid);
	if (oman != nullptr && bmpIt != bmpMap.end() && bmpIt->second->isOK()) {
		std::lock_guard<std::mutex> lk(bmpMapMutex);
		oman->setOutput(*(bmpIt->second), mid);
	}
}

// ModuleDAG

ModuleDAG::ModuleDAG()
	: connectorCount(0)
	, edgeCount(0)
{}

ModuleDAG::~ModuleDAG() {
	for (auto& node : moduleMap) {
		delete node.second;
		node.second = nullptr;
	}
	for (auto& connector : connectorMap) {
		delete connector.second;
		connector.second = nullptr;
	}
}

void ModuleDAG::addModule(ModuleBase * moduleInstance, const ModuleDescription& md) {
	if (!moduleInstance)
		return;
	// create a new module node
	ModuleNode * node = new ModuleNode(moduleInstance, md);
	const ModuleId mid = moduleInstance->getModuleId();
	moduleMap[mid] = node;
	// add the module to the graph
	graph.addNode(mid);
}

void ModuleDAG::removeModule(ModuleId mid) {
	const auto& mnIt = moduleMap.find(mid);
	if (mnIt != moduleMap.end()) {
		// remove the module from the graph
		graph.removeNode(mid);

		ModuleNode * mn = mnIt->second;
		// first remove all connected edges
		if (mn->output != InvalidEdgeId) {
			removeConnector(mn->output);
		}
		// also all the inputs
		for (int i = 0; i < mn->moduleDesc.inputs; ++i) {
			if (mn->inputs[i] != InvalidEdgeId) {
				removeConnector(mn->inputs[i]);
			}
		}
		// now remove the module itself
		delete mn;
		moduleMap[mid] = nullptr;
		moduleMap.erase(mid);
	}
}

EdgeId ModuleDAG::addConnector(ModuleId srcId, ModuleId destId, int destSrcIdx) {
	// check if the connection will retain the dag valid (no cycles - remember?)
	const auto& srcNodeIt = moduleMap.find(srcId);
	const auto& destNodeIt = moduleMap.find(destId);
	if (srcNodeIt == moduleMap.end() || destNodeIt == moduleMap.end()) {
		// invalid module ids
		return InvalidEdgeId;
	}
	Graph<ModuleId> checkGraph(graph);
	ModuleNode * srcNode = srcNodeIt->second;
	if (srcNode->output != InvalidEdgeId) {
		// so there is an edge out of this module - remove it from the check graph
		const SimpleConnector& sc = edgeMap[srcNode->output];
		checkGraph.removeEdge(sc.srcId, sc.destId);
	}
	ModuleNode * destNode = destNodeIt->second;
	if (destSrcIdx >= 0 &&
		destSrcIdx < destNode->moduleDesc.inputs &&
		destNode->inputs[destSrcIdx] != InvalidEdgeId
		) {
		const SimpleConnector& sc = edgeMap[destNode->inputs[destSrcIdx]];
		checkGraph.removeEdge(sc.srcId, sc.destId);
	}
	// now add the new edge to the check graph
	checkGraph.addEdge(srcId, destId);
	// directly check if the graph remains a DAG
	if (!checkGraph.isDAG()) {
		return InvalidEdgeId;
	}

	// remove existing edges on the source node
	if (srcNode->output != InvalidEdgeId) {
		removeConnector(srcNode->output);
	}
	// remove exising edges on the destination node
	// still check the src dest idx
	if (destSrcIdx >= 0 &&
		destSrcIdx < destNode->moduleDesc.inputs &&
		destNode->inputs[destSrcIdx] != InvalidEdgeId
		) {
		removeConnector(destNode->inputs[destSrcIdx]);
	}

	// add the edge to the graph (overlapping edges should have been removed by removeConnector(..)
	graph.addEdge(srcId, destId);

	EdgeId res = edgeCount;
	SimpleConnector sc(srcId, destId, destSrcIdx);
	ConnectorId connId = addModuleConnection(srcId, destId, destSrcIdx);
	if (connId != InvalidConnectorId) {
		sc.connId = connId;
		// add the edge id to the nodes
		moduleMap[srcId]->output = res;
		moduleMap[destId]->inputs[destSrcIdx] = res;
		edgeMap[res] = sc;
		// final
		edgeCount++;
	} else {
		res = InvalidEdgeId;
	}
	return res;
}

void ModuleDAG::removeConnector(EdgeId eid) {
	// check if it is not already removed (possible with gui updates)
	const auto& eIt = edgeMap.find(eid);
	if (eIt != edgeMap.end()) {
		const SimpleConnector& sc = eIt->second;
		// remove the simple connection from the graph
		graph.removeEdge(sc.srcId, sc.destId);
		// first detach the module connector
		ModuleConnector * mc = connectorMap[sc.connId];
		const bool removeConnector = mc->removeConnection(sc.destSrcIdx);
		if (removeConnector) {
			delete mc;
			connectorMap[sc.connId] = nullptr;
			connectorMap.erase(sc.connId);
		}
		// invalidate the modules edge ids
		ModuleNode * srcNode = moduleMap[sc.srcId];
		srcNode->output = InvalidEdgeId;
		ModuleNode * destNode = moduleMap[sc.destId];
		destNode->inputs[sc.destSrcIdx] = InvalidEdgeId;
		// and finally remove the edge from the map
		edgeMap.erase(eid);
	}
}

void ModuleDAG::invalidateModules(const std::vector<ModuleId>& invalidateList) {
	// TODO - create a dependency list to update all depending modules and connectors
	for (const auto& it : invalidateList) {
		ModuleNode * mn = moduleMap[it];
		// manually reset the abort flag (TODO - in a separate loop, because they should be reset before any updates are triggered)
		ProgressCallback * mcb = mn->module->getProgressCallback();
		if (mcb) {
			mcb->reset();
		}
		mn->module->update();
	}
}

void ModuleDAG::setExternalOutput(ModuleId mid, OutputManager * oman) {
	ModuleNode * mn = moduleMap[mid];
	if (mn->outputConnector != InvalidConnectorId) {
		connectorMap[mn->outputConnector]->setExternalOutput(mid, oman);
	} else {
		// directly attach it to the module
		mn->module->setOutputManager(oman);
		// reevaluate the module so it can report its output to the new manager
		if (oman != nullptr) {
			// TODO - optimize with a temporary output manager held by the DAG
			// to avoid module reevaluation
			mn->module->update();
		}
	}
}

ConnectorId ModuleDAG::addModuleConnection(ModuleId srcId, ModuleId destId, int destSrcIdx) {
	// first get the dest id
	const auto& destIt = moduleMap.find(destId);
	const auto& srcIt = moduleMap.find(srcId);
	if (destIt == moduleMap.end() || srcIt == moduleMap.end())
		return InvalidConnectorId;
	ModuleNode * srcNode = srcIt->second;
	ModuleNode * destNode = destIt->second;
	// TODO check for multiple inputs -> later observation (what - is this still neccessary?)
	ModuleConnector * mc = nullptr;
	ConnectorId connId = InvalidConnectorId;
	if (destNode->inputConnector != InvalidConnectorId) {
		connId = destNode->inputConnector;
		mc = connectorMap[connId];
	} else {
		mc = new ModuleConnector();
		connId = connectorCount;
		connectorMap[connId] = mc;
		connectorCount++; // increase the connector id count
	}
	// set the nodes in the connector
	mc->setDestModule(destNode);
	mc->setSrcModule(srcNode, destSrcIdx);
	// set the connector in the nodes
	srcNode->outputConnector = connId;
	destNode->inputConnector = connId;
	// after this is done - update the src module
	// TODO - cache the source result and only invalidate the dest
	srcNode->module->runModule();

	return connId;
}
