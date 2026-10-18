#include <cstdio>
#include <cstring>
#include <vector>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "vsplayer/particlesGame.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static std::vector<unsigned char> readAll(const char *name) {
	std::vector<unsigned char> data;
	FILE *f = fopen(name, "rb");

	if (!f)
		return data;
	unsigned char block[4096];
	size_t n;

	while ((n = fread(block, 1, sizeof(block), f)) > 0)
		data.insert(data.end(), block, block + n);
	fclose(f);
	return data;
}

// The file of an emitter (the editor's): written, read into another particle object, and the older versions.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	TVisionaireGame game;
	TVisObjRef noParent;
	TVisObjRef a = game.CreateObject(22, noParent, -1);
	TVisObjRef b = game.CreateObject(22, noParent, -1);

	a.SetValue(kParticleMaterialMode, 3, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleTilesX, 4, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleTilesY, 5, TSendEventEnum::kNoEvent);
	a.SetValue(kParticlePosX, 77, TSendEventEnum::kNoEvent);
	a.SetValue(kParticlePosY, 88, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleMaxParticles, 123, TSendEventEnum::kNoEvent);
	a.SetValue(kParticlePerTime, 2.5f, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleMaxSize, 9.0f, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleMinLife, 40, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleBlendColor3, 0x11223344, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleForceType, 2, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleForceStrength, 1.5f, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleForceBlend, 0.25f, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleActiveStages, 6, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleBlendMaxTile, 11, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleTexture, wxFileName(L"pics/spark.png"), TSendEventEnum::kNoEvent);

	std::vector<float> curve(100);
	std::vector<int> points(100, 0);

	for (int i = 0; i < 100; i++)
		curve[i] = (float)i / 10.0f;
	points[0] = 1;
	points[50] = 1;
	points[99] = 1;
	a.SetValue(kParticleBlendColorCurve1, curve, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleColorControlPoints1, points, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleBlendTilesCurve, curve, TSendEventEnum::kNoEvent);
	a.SetValue(kParticleBlendTilesControlPoints, points, TSendEventEnum::kNoEvent);

	CHECK(TGParticleEmitter::Save("emitter.part", a));
	CHECK(!TGParticleEmitter::Save("no/such/dir/emitter.part", a));
	CHECK(!TGParticleEmitter::Load("no-such-file.part", b));

	// the size of the file: the version, 28 header fields, the four curves, the force, the size curve, the path with its
	// null, the stages, the tiles curve and the largest tile (and not the camera)
	std::vector<unsigned char> data = readAll("emitter.part");
	size_t pathUnits = wcslen(L"pics/spark.png") + 1;
	size_t expected = 4 * (1 + 28 + 4 * 200 + 8 + 200 + pathUnits + 1 + 200 + 1);

	printf("file size %d, expected %d\n", (int)data.size(), (int)expected);
	CHECK(data.size() == expected);
	CHECK(data[0] == 6);

	CHECK(TGParticleEmitter::Load("emitter.part", b));
	CHECK(b.GetInt(kParticleMaterialMode) == 3);
	CHECK(b.GetInt(kParticleTilesX) == 4);
	CHECK(b.GetInt(kParticleTilesY) == 5);
	// (the position is written from the int fields kParticlePosX/Y and read into kParticleXPos/YPos, which are floats since
	// the upgrade 0x97 and refuse an int: the original logs the same error and the position is not restored. The same for
	// the two int rotations of the force, which are read as floats.)
	CHECK(b.GetInt(kParticlePosX) == -1);  // (its default)
	CHECK(b.GetFloat(kParticleXPos) == 0.0f);
	CHECK(b.GetInt(kParticleMaxParticles) == 123);
	CHECK(b.GetFloat(kParticlePerTime) == 2.5f);
	CHECK(b.GetFloat(kParticleMaxSize) == 9.0f);
	CHECK(b.GetInt(kParticleMinLife) == 40);
	CHECK(b.GetInt(kParticleBlendColor3) == 0x11223344);
	CHECK(b.GetInt(kParticleForceType) == 2);
	CHECK(b.GetFloat(kParticleForceStrength) == 1.5f);
	CHECK(b.GetFloat(kParticleForceBlend) == 0.25f);
	CHECK(b.GetInt(kParticleActiveStages) == 6);
	CHECK(b.GetInt(kParticleBlendMaxTile) == 11);
	CHECK(b.GetPath(kParticleTexture).GetFullPath().ToStdWstring() == L"pics/spark.png");

	std::vector<float> gotCurve;
	std::vector<int> gotPoints;

	b.GetFloats(kParticleBlendColorCurve1, gotCurve);
	b.GetInts(kParticleColorControlPoints1, gotPoints);
	CHECK(gotCurve == curve);
	CHECK(gotPoints == points);
	b.GetFloats(kParticleBlendTilesCurve, gotCurve);
	b.GetInts(kParticleBlendTilesControlPoints, gotPoints);
	CHECK(gotCurve == curve);
	CHECK(gotPoints == points);

	// a curve that is not set is written as zeros with the first and the last point, and the size curve as ones
	b.GetFloats(kParticleBlendColorCurve0, gotCurve);
	b.GetInts(kParticleColorControlPoints0, gotPoints);
	CHECK(gotCurve.size() == 100 && gotCurve[10] == 0.0f);
	CHECK(gotPoints.size() == 100 && gotPoints[0] == 1 && gotPoints[99] == 1 && gotPoints[1] == 0);
	b.GetFloats(kParticleBlendSizeCurve, gotCurve);
	CHECK(gotCurve.size() == 100 && gotCurve[10] == 1.0f);

	// an older file: the version 3 file has no tiles curve (nor the largest tile): the defaults
	data[0] = 3;
	FILE *f = fopen("old.part", "wb");

	fwrite(data.data(), 1, data.size(), f);
	fclose(f);

	TVisObjRef c = game.CreateObject(22, noParent, -1);

	c.SetValue(kParticleBlendMaxTile, 99, TSendEventEnum::kNoEvent);
	CHECK(TGParticleEmitter::Load("old.part", c));
	CHECK(c.GetInt(kParticleActiveStages) == 6);
	CHECK(c.GetInt(kParticleBlendMaxTile) == 99);
	c.GetFloats(kParticleBlendTilesCurve, gotCurve);
	c.GetInts(kParticleBlendTilesControlPoints, gotPoints);
	CHECK(gotCurve.size() == 100 && gotCurve[30] == 0.0f);
	CHECK(gotPoints[0] == 1 && gotPoints[99] == 1 && gotPoints[50] == 0);

	data[0] = 2;
	f = fopen("old2.part", "wb");
	fwrite(data.data(), 1, data.size(), f);
	fclose(f);
	CHECK(TGParticleEmitter::Load("old2.part", c));
	CHECK(c.GetInt(kParticleActiveStages) == 1);

	data[0] = 1;
	f = fopen("old1.part", "wb");
	fwrite(data.data(), 1, data.size(), f);
	fclose(f);
	CHECK(TGParticleEmitter::Load("old1.part", c));
	CHECK(c.GetPath(kParticleTexture).GetFullPath().IsEmpty());
	CHECK(c.GetInt(kParticleActiveStages) == 1);

	// version 0: only the defaults after the header-less file
	data[0] = 0;
	f = fopen("old0.part", "wb");
	fwrite(data.data(), 1, data.size(), f);
	fclose(f);
	TVisObjRef d = game.CreateObject(22, noParent, -1);

	d.SetValue(kParticleMaterialMode, 8, TSendEventEnum::kNoEvent);
	CHECK(TGParticleEmitter::Load("old0.part", d));
	CHECK(d.GetInt(kParticleMaterialMode) == 8);
	CHECK(d.GetInt(kParticleActiveStages) == 1);

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
