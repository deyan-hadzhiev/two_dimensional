#include "module_dag.h"

ModuleDAG::ModuleDAG()
	: idCount(0)
{}

ModuleDAG::~ModuleDAG() {
	for (auto node : moduleMap) {
		delete node.second;
	}
}

int ModuleDAG::addModule(ModuleBase * moduleInstance, int id) {
	if (!moduleInstance)
		return -1;
	if (-1 == id) {
		// create a new module node
		ModuleNode * node = new ModuleNode;
		node->id = idCount;
		node->module = moduleInstance;
		moduleMap[idCount] = node;
		idCount++;
		// TODO generate name of the module
	} else {
		
	}
	return 0;
}
