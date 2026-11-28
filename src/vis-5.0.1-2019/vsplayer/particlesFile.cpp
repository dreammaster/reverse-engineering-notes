#include "vsplayer/particlesGame.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

namespace {

const int kBufferSize = 0x100000;
const int kCurveEntries = 100;
const int kFileVersion = 6;

/** The fields in the order of the file, up to the four colour curves. */
const struct {
	bool isFloat;
	int field;
} kHeaderFields[] = {
	{false, kParticleMaterialMode}, {false, kParticleTilesX}, {false, kParticleTilesY}, {false, kParticleType},
	{false, kParticlePosX}, {false, kParticlePosY}, {false, kParticleRotationX}, {false, kParticleRotationZ},
	{false, kParticleEmitterType}, {false, kParticlePhiX}, {false, kParticlePhiY}, {false, kParticleEmitterSizeX},
	{false, kParticleEmitterSizeY}, {false, kParticleEmitterSizeZ}, {false, kParticleMaxParticles}, {true, kParticlePerTime},
	{true, kParticleMinSize}, {true, kParticleMaxSize}, {true, kParticleMinVelocity}, {true, kParticleMaxVelocity},
	{false, kParticleMinLife}, {false, kParticleMaxLife}, {false, kParticleMinColor}, {false, kParticleMaxColor},
	{false, kParticleBlendColor0}, {false, kParticleBlendColor1}, {false, kParticleBlendColor2}, {false, kParticleBlendColor3},
};
const int kHeaderCount = sizeof(kHeaderFields) / sizeof(kHeaderFields[0]);

/** The field that Load() sets at a place of the header: the same as Save() reads, except that the position is set in the
 *  fields kParticleXPos and kParticleYPos (Save() reads kParticlePosX and kParticlePosY there). */
int loadField(int index) {
	if (index == 4)
		return kParticleXPos;
	if (index == 5)
		return kParticleYPos;
	return kHeaderFields[index].field;
}

const int kForceFields[] = {kParticleForceStrength, kParticleForceRotationX, kParticleForceRotationZ, kParticleForceXPos,
                            kParticleForceYPos, kParticleForceZPos, kParticleForceBlend};

/** Puts a curve (a float and an int for each of the 100 control points). A curve of another size is `fillValue` for every
 *  point, and only the first and the last point are set. */
void putCurve(uint8_t *&at, const TVisObjRef &particle, int floats, int ints, float fillValue) {
	std::vector<float> curve;
	std::vector<int> points;

	particle.GetFloats(floats, curve);
	particle.GetInts(ints, points);

	bool fromData = curve.size() == kCurveEntries && points.size() >= kCurveEntries;

	for (int i = 0; i < kCurveEntries; i++) {
		float value = fromData ? curve[i] : fillValue;
		int flag = fromData ? points[i] : ((i == 0 || i == kCurveEntries - 1) ? 1 : 0);

		memcpy(at, &value, 4);
		memcpy(at + 4, &flag, 4);
		at += 8;
	}
}

void getCurve(const uint8_t *&at, TVisObjRef &particle, int floats, int ints) {
	std::vector<float> curve(kCurveEntries);
	std::vector<int> points(kCurveEntries);

	for (int i = 0; i < kCurveEntries; i++) {
		memcpy(&curve[i], at, 4);
		memcpy(&points[i], at + 4, 4);
		at += 8;
	}
	particle.SetValue(floats, curve, TSendEventEnum::kSendEvent);
	particle.SetValue(ints, points, TSendEventEnum::kSendEvent);
}

void defaultCurve(TVisObjRef &particle, int floats, int ints) {
	std::vector<float> curve(kCurveEntries, 0.0f);
	std::vector<int> points(kCurveEntries, 0);

	points[0] = 1;
	points[kCurveEntries - 1] = 1;
	particle.SetValue(floats, curve, TSendEventEnum::kSendEvent);
	particle.SetValue(ints, points, TSendEventEnum::kSendEvent);
}

} // End of anonymous namespace

// Confirmed (asm lines 1378389-1378934). The file is the int array of the original: [0] the version, the header fields,
// the four colour curves (0x320 bytes each), the force (an int, seven floats), the size curve, the texture path (32-bit
// characters, null terminated, so what follows moves with its length), the active stages, the tiles curve and the largest
// tile. The two floats after that (camera z and zoom; the second is written over the first) are in the buffer but not in
// the file: the count of the write ends after the largest tile.
bool TGParticleEmitter::Save(const char *filename, const TVisObjRef &particle) {
	std::vector<uint8_t> buffer(kBufferSize, 0);
	uint8_t *at = buffer.data();
	int version = kFileVersion;

	memcpy(at, &version, 4);
	at += 4;

	for (int i = 0; i < kHeaderCount; i++) {
		if (kHeaderFields[i].isFloat) {
			float value = particle.GetFloat(kHeaderFields[i].field);

			memcpy(at, &value, 4);
		} else {
			int value = particle.GetInt(kHeaderFields[i].field);

			memcpy(at, &value, 4);
		}
		at += 4;
	}

	for (int curve = 0; curve < 4; curve++)
		putCurve(at, particle, kParticleBlendColorCurve0 + curve, kParticleColorControlPoints0 + curve, 0.0f);

	int forceType = particle.GetInt(kParticleForceType);

	memcpy(at, &forceType, 4);
	at += 4;
	for (int field : kForceFields) {
		float value = particle.GetFloat(field);

		memcpy(at, &value, 4);
		at += 4;
	}
	putCurve(at, particle, kParticleBlendSizeCurve, kParticleSizeControlPoints, 1.0f);

	std::wstring path = particle.GetPath(kParticleTexture).GetFullPath().ToStdWstring();

	for (wchar_t character : path) {
		uint32_t unit = (uint32_t)character;

		memcpy(at, &unit, 4);
		at += 4;
	}
	at += 4; // (the null character: the buffer is zero)

	int stages = particle.GetInt(kParticleActiveStages);

	memcpy(at, &stages, 4);
	at += 4;
	putCurve(at, particle, kParticleBlendTilesCurve, kParticleBlendTilesControlPoints, 1.0f);

	int maxTile = particle.GetInt(kParticleBlendMaxTile);

	memcpy(at, &maxTile, 4);
	at += 4;

	FILE *file = fopen(filename, "wb");

	if (!file)
		return false;

	fwrite(buffer.data(), 4, (size_t)(at - buffer.data()) / 4, file);
	fclose(file);
	return true;
}

// Confirmed (asm lines 1379800-1380801). An older version ends earlier: 1 has no path (it is empty), 2 has no active
// stages (1), 3 has no tiles curve (the defaults: zeros, the first and the last point set), 4 has no largest tile, 5 no
// camera. Version 0 (or less) has only the defaults. The force and the size curve are read for every version above 0.
bool TGParticleEmitter::Load(const char *filename, TVisObjRef &particle) {
	std::vector<uint8_t> buffer(kBufferSize, 0);
	FILE *file = fopen(filename, "rb");

	if (!file)
		return false;

	fread(buffer.data(), 1, kBufferSize, file);

	int version;

	memcpy(&version, buffer.data(), 4);
	const uint8_t *at = buffer.data() + 4;

	auto readInt = [&](int field) {
		int value;

		memcpy(&value, at, 4);
		particle.SetValue(field, value, TSendEventEnum::kSendEvent);
	};
	auto readFloat = [&](int field, int offset) {
		float value;

		memcpy(&value, at + offset, 4);
		particle.SetValue(field, value, TSendEventEnum::kSendEvent);
	};

	if (version > 0) {
		for (int i = 0; i < kHeaderCount; i++) {
			if (kHeaderFields[i].isFloat)
				readFloat(loadField(i), 0);
			else
				readInt(loadField(i));
			at += 4;
		}

		for (int curve = 0; curve < 4; curve++)
			getCurve(at, particle, kParticleBlendColorCurve0 + curve, kParticleColorControlPoints0 + curve);

		readInt(kParticleForceType);
		at += 4;
		for (int field : kForceFields) {
			readFloat(field, 0);
			at += 4;
		}
		getCurve(at, particle, kParticleBlendSizeCurve, kParticleSizeControlPoints);
	}

	if (version <= 1) {
		particle.SetValue(kParticleTexture, TCharHolder(), TSendEventEnum::kSendEvent);
		particle.SetValue(kParticleActiveStages, 1, TSendEventEnum::kSendEvent);
	} else {
		std::wstring path;
		size_t length = 0;

		for (;; length++) {
			uint32_t unit;

			memcpy(&unit, at + length * 4, 4);
			if (!unit)
				break;
			path += (wchar_t)unit;
		}
		at += (length + 1) * 4;

		wxFileName name(path);

		name.NormalizePath();
		particle.SetValue(kParticleTexture, TCharHolder(name), TSendEventEnum::kSendEvent);

		if (version == 2) {
			particle.SetValue(kParticleActiveStages, 1, TSendEventEnum::kSendEvent);
		} else {
			readInt(kParticleActiveStages);
			at += 4;
		}
	}

	if (version > 3)
		getCurve(at, particle, kParticleBlendTilesCurve, kParticleBlendTilesControlPoints);
	else
		defaultCurve(particle, kParticleBlendTilesCurve, kParticleBlendTilesControlPoints);

	if (version > 4) {
		readInt(kParticleBlendMaxTile);
		if (version > 5) {
			readFloat(kParticleContainerCameraZPosition, 4);
			readFloat(kParticleContainerCameraZoom, 8);
		}
	}

	fclose(file);
	return true;
}
