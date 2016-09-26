#ifndef __MODULE_DAG_H__
#define __MODULE_DAG_H__

#include <memory>
#include <mutex>
#include <unordered_map>

#include "bitmap.h"
#include "graph.h"
#include "module_base.h"

using ConnectorId = int;
enum {
	InvalidConnectorId = -1,
};

using EdgeId = int;
enum {
	InvalidEdgeId = -1,
};

class SimpleConnector {
public:
	ModuleId srcId;
	ModuleId destId;
	int destSrcIdx;
	ConnectorId connId;
	SimpleConnector(ModuleId _srcId = InvalidModuleId, ModuleId _destId = InvalidModuleId, int _destSrcIdx = 0, ConnectorId _connId = InvalidConnectorId)
		: srcId(_srcId)
		, destId(_destId)
		, destSrcIdx(_destSrcIdx)
		, connId(_connId)
	{}
};

class ModuleNode {
public:
	ModuleNode(ModuleBase * _module, const ModuleDescription& _moduleDesc)
		: module(_module)
		, moduleDesc(_moduleDesc)
		, inputs(new EdgeId[_moduleDesc.inputs])
		, output(InvalidEdgeId)
		, inputConnector(InvalidConnectorId)
		, outputConnector(InvalidConnectorId)
	{
		for (int i = 0; i < moduleDesc.inputs; ++i) {
			inputs[i] = InvalidEdgeId;
		}
	}

	ModuleBase * module;
	ModuleDescription moduleDesc;
	std::unique_ptr<EdgeId[]> inputs;
	EdgeId output;
	ConnectorId inputConnector;
	ConnectorId outputConnector;
};

// used for temporary storing an output to be reused when new connections or external outputs are added
// to avoid unneccessary module updates - still should be kept only until a valid connection is made
class TemporaryOutput
	: public OutputManager
{
public:
	TemporaryOutput()
		: storedMid(InvalidModuleId)
		, externalOutput(nullptr)
	{}

	// from OutputManager
	void setOutput(const Bitmap& bmp, ModuleId mid) override final;

	// own functions
	void setBitmapToManager(OutputManager * external) const;

	void setExternalOutput(OutputManager * oman);

	// beware - nullptr
	OutputManager * getExternalOutput() const;

private:
	mutable std::mutex bmpMutex;
	ModuleId storedMid;
	Bitmap storedBmp;
	OutputManager * externalOutput;
};

class ModuleConnector
	: public InputManager
	, public OutputManager
{
public:
	ModuleConnector()
		: destModule(nullptr)
	{}

	// from InputManager
	bool getInput(Bitmap& bmp, int idx) const override final;

	// from OutputManager
	void setOutput(const Bitmap& bmp, ModuleId mid) override final;

	// own functions
	bool getBitmap(Bitmap& bmp, ModuleId mid) const;

	// add a destination module
	void setDestModule(ModuleNode * moduleNode);

	// add a source module
	void setSrcModule(ModuleNode * moduleNode, int srcIdx);

	// remove connection and provide a temporary output manager to save any valid outputs
	// returns true if this was the last connection held by the connector and it should be removed
	bool removeConnection(int destSrcIdx, TemporaryOutput * tempOman);

	// adds external output to the module
	void setExternalOutput(ModuleId mid, OutputManager * oman);

private:
	mutable std::mutex bmpMapMutex;
	std::unordered_map<ModuleId, std::shared_ptr<Bitmap> > bmpMap;
	// this map is used for storing external output managers and chain them when the module is finished
	std::unordered_map<ModuleId, OutputManager*> externalOutputMap;
	std::unordered_map<ModuleId, int> srcIndexMap;
	std::vector<bool> validInputs;
	std::vector<ModuleNode*> srcModules;
	ModuleNode* destModule;
};

class ModuleDAG {
public:
	ModuleDAG();
	~ModuleDAG();

	void addModule(ModuleBase * moduleInstance, const ModuleDescription& md);

	void removeModule(ModuleId mid);

	EdgeId addConnector(ModuleId srcId, ModuleId destId, int destSrcIdx);

	void removeConnector(EdgeId eid);

	void invalidateModules(const std::vector<ModuleId>& invalidateList);

	// this function connects an external output manager to the module
	// if the module is already connected - it attaches it to the connector
	// otherwise it sets it directly to the module
	void setExternalOutput(ModuleId mid, OutputManager * oman);

private:
	ConnectorId addModuleConnection(ModuleId srcId, ModuleId destId, int destSrcIdx);

	Graph<ModuleId> graph; //!< a graph containing the full connection for checks and lists

	std::unordered_map<ModuleId, ModuleNode*> moduleMap;
	std::unordered_map<ConnectorId, ModuleConnector*> connectorMap;
	ConnectorId connectorCount;
	std::unordered_map<EdgeId, SimpleConnector> edgeMap;
	EdgeId edgeCount;

	// temporary output holders
	std::unordered_map<ModuleId, std::shared_ptr<TemporaryOutput> > tempOutputMap;
};

#endif // __MODULE_DAG_H__
