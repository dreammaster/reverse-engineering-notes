#include "files/data_file.h"

#include "common/config-manager.h"
#include "common/file.h"
#include "common/savefile.h"
#include "common/system.h"

namespace Ultima6 {

// Paths that start with SAVE_PREFIX are saved games (kept by the save file manager under the name of the target, the rest of the path with its slashes turned into dashes); every other path is
// a file of the game's own data, found through the search manager (so whatever the case of its name and wherever the game was added from).
static const char SAVE_PREFIX[] = "@save/";

static Common::String saveName(const std::string &path) {
	Common::String name = ConfMan.getActiveDomainName() + "-";
	for (size_t i = sizeof(SAVE_PREFIX) - 1; i < path.size(); ++i)
		name += path[i] == '/' || path[i] == '\\' ? '-' : path[i];
	return name;
}

static bool isSave(const std::string &path) {
	return path.compare(0, sizeof(SAVE_PREFIX) - 1, SAVE_PREFIX) == 0;
}

bool readFile(const std::string &path, Bytes &out) {
	Common::SeekableReadStream *stream;
	Common::File file;
	if (isSave(path)) {
		stream = g_system->getSavefileManager()->openForLoading(saveName(path));
		if (!stream)
			return false;
	} else {
		if (!file.open(Common::Path(path.c_str(), '/')))
			return false;
		stream = &file;
	}
	out.resize((size_t)stream->size());
	bool ok = out.empty() || stream->read(&out[0], (uint32)out.size()) == out.size();
	if (stream != &file)
		delete stream;
	return ok;
}

bool writeFile(const std::string &path, const Bytes &data) {
	if (!isSave(path))
		return false; // the game's own files are never written
	Common::OutSaveFile *file = g_system->getSavefileManager()->openForSaving(saveName(path), false);
	if (!file)
		return false;
	if (!data.empty())
		file->write(&data[0], (uint32)data.size());
	file->finalize();
	bool ok = !file->err();
	delete file;
	return ok;
}

std::string joinPath(const std::string &dir, const std::string &name) {
	if (dir.empty() || dir[dir.size() - 1] == '/' || dir[dir.size() - 1] == '\\')
		return dir + name;
	return dir + "/" + name;
}

} // namespace Ultima6
