#ifndef __MODULE_MANAGER_H__
#define __MODULE_MANAGER_H__

#include <string>
#include <vector>

#include "module_base.h"
#include "modules.h"

enum ModuleId {
	M_VOID = -1,
	M_IDENTITY = 0,
	M_NEGATIVE,
	M_TEXT_SEGMENTATION,
	M_SINOSOID,
	M_HOUGH_RO_THETA,
	M_ROTATION,
	M_HISTOGRAMS,
	M_THRESHOLD,
	M_FILTER,
	M_DOWNSCALE,
	M_RELOCATE,
	M_CROP,
	M_CHANNEL,
	M_FFT_DOMAIN,
	M_FFT_COMPRESSION,
	M_FFT_FILTER,
	M_COUNT, //!< must remain to be used for IDs and array sizes
};

typedef ModuleBase * (*ModuleCreator)();

template<class T>
ModuleBase * create() {
	return new T();
}

class ModuleDescription {
public:
	ModuleId id;
	ModuleCreator make;
	std::string name;
	std::string fullName;
	int inputs; //!< number of inputs required by the module
	int outputs; //!< number of outputs provided by the module // for now all provide a single output

	ModuleDescription(ModuleId _id = M_VOID, ModuleCreator _maker = create<IdentityModule>, const std::string& _name = "void", const std::string& _fullName = "Void", int _inputs = 0, int _outputs = 0)
		: id(_id)
		, make(_maker)
		, name(_name)
		, fullName(_fullName)
		, inputs(_inputs)
		, outputs(_outputs)
	{}
};

extern const ModuleDescription MODULE_DESC[ModuleId::M_COUNT];

// used for creating modules on the fly
// handles deallocation
class ModuleFactory {
public:
	ModuleFactory() = default;
	~ModuleFactory();

	ModuleFactory(const ModuleFactory&) = delete;
	ModuleFactory& operator=(const ModuleFactory&) = delete;

	// clears all allocated modules
	void clear();

	// allocates new module of the specified type and returns the base handle to it
	ModuleBase * getModule(ModuleId id);
private:
	std::vector<ModuleBase*> allocatedModules;
};

#endif // __MODULE_MANAGER_H__
