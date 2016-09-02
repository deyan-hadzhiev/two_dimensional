#ifndef __MODULE_DAG_H__
#define __MODULE_DAG_H__

#include <memory>
#include <unordered_map>

#include "module_base.h"

struct ModuleNode {
	ModuleNode()
		: id(-1)
		, module(nullptr)
	{}
	int id;
	ModuleBase * module;
	std::vector<int> inputs;
	std::vector<int> outputs;
};

class ModuleDAG {
public:
	ModuleDAG();
	~ModuleDAG();

	int addModule(ModuleBase * moduleInstance, int id = -1);

private:
	int idCount; // used for assigning unique ids to new modules
	std::unordered_map<std::string, int> moduleIdMap;
	std::unordered_map<int, ModuleNode *> moduleMap;
};

#endif // __MODULE_DAG_H__
