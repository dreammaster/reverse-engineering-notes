#include "vsplayer/particlesGame.h"

#include "datastruct/visionaireobject.h"
#include "datastruct/vlist.h"
#include "graphicslib/picture.h"
#include "vstables/fieldIds.h"

/** Degrees as radians, as the original does it: times pi, divided by 180. */
static float radians(int degrees) {
	return static_cast<float>(degrees) * 3.14159274f / 180.0f;
}

/** The 100 numbers of a curve of the data (null in `values` when the data has another number: `fill` is then the value
 *  of all of them). */
static void readCurve(const TVisObjRef &particle, int field, float *curve, float fill) {
	std::vector<float> values;

	particle.GetFloats(field, values);

	if (values.size() == 100) {
		for (int i = 0; i < 100; i++)
			curve[i] = values[i];
	} else {
		for (int i = 0; i < 100; i++)
			curve[i] = fill;
	}
}

// Confirmed (asm lines 1378934-1379800). The control points of the curves (kParticleColorControlPoints0 and the others) are
// for the editor; the player does not use them.
bool TGParticleEmitter::Init(const TVisObjRef &particle) {
	if (particle.IsEmpty())
		return false;

	int material = particle.GetInt(kParticleMaterialMode);

	if (material == 1)
		_materialMode = 1;
	else if (material == 2)
		_materialMode = 2;
	else if (material == 0)
		_materialMode = 0;

	_tilesX = particle.GetInt(kParticleTilesX);
	_tilesY = particle.GetInt(kParticleTilesY);

	int type = particle.GetInt(kParticleType);

	if (type == 1)
		_particleType = kRandomTile;
	else if (type == 0)
		_particleType = kBillboard;
	else if (type == 2)
		_particleType = kStreak;
	else if (type == 3)
		_particleType = kAnimatedTile;

	float posX = static_cast<float>(particle.GetInt(kParticlePosX));
	float posY = static_cast<float>(particle.GetInt(kParticlePosY));
	TVector3D direction(0.0f, 1.0f, 0.0f);

	direction.RotateX(radians(particle.GetInt(kParticleRotationX)));
	direction.RotateZ(radians(particle.GetInt(kParticleRotationZ)));

	int emitterType = particle.GetInt(kParticleEmitterType);

	if (emitterType == 2) {
		float halfX = static_cast<float>(particle.GetInt(kParticleEmitterSizeX)) * 0.5f;
		float halfY = static_cast<float>(particle.GetInt(kParticleEmitterSizeY)) * 0.5f;
		float halfZ = static_cast<float>(particle.GetInt(kParticleEmitterSizeZ)) * 0.5f;
		int phiY = particle.GetInt(kParticlePhiY);
		int phiX = particle.GetInt(kParticlePhiX);

		SetEmitter(TVector3D(posX + halfX, posY + halfY, halfZ), TVector3D(posX - halfX, posY - halfY, -halfZ), direction,
		           radians(phiX), radians(phiY));
	} else if (emitterType == 1) {
		int phiY = particle.GetInt(kParticlePhiY);
		int phiX = particle.GetInt(kParticlePhiX);

		SetEmitter(TVector3D(posX, posY, 0.0f), direction, radians(phiX), radians(phiY));
	}

	InitEmitter(particle.GetInt(kParticleMaxParticles), static_cast<ParticleType>(_particleType));
	_perTime = particle.GetFloat(kParticlePerTime);
	_minSize = particle.GetFloat(kParticleMinSize);
	_maxSize = particle.GetFloat(kParticleMaxSize);
	_minVelocity = particle.GetFloat(kParticleMinVelocity);
	_maxVelocity = particle.GetFloat(kParticleMaxVelocity);
	_minLife = particle.GetInt(kParticleMinLife);
	_maxLife = particle.GetInt(kParticleMaxLife);
	_minColor = particle.GetInt(kParticleMinColor);
	_maxColor = particle.GetInt(kParticleMaxColor);
	_activeStages = particle.GetInt(kParticleActiveStages);
	_stageColors[0] = particle.GetInt(kParticleBlendColor0);
	_stageColors[1] = particle.GetInt(kParticleBlendColor1);
	_stageColors[2] = particle.GetInt(kParticleBlendColor2);
	_stageColors[3] = particle.GetInt(kParticleBlendColor3);

	for (int stage = 0; stage < 4; stage++)
		readCurve(particle, kParticleBlendColorCurve0 + stage, _stageCurves[stage], 0.0f);

	int force = particle.GetInt(kParticleForceType);

	if (force == 1)
		_forceType = 1;
	else if (force == 2)
		_forceType = 2;
	else if (force == 0)
		_forceType = 0;

	_forceStrength = particle.GetFloat(kParticleForceStrength);
	_forceRotationX = radians(particle.GetInt(kParticleForceRotationX));
	_forceRotationZ = radians(particle.GetInt(kParticleForceRotationZ));

	TVector3D push(0.0f, -1.0f, 0.0f);

	push.RotateX(_forceRotationX);
	push.RotateZ(_forceRotationZ);
	push *= _forceStrength;
	_force = push;

	float forceZ = particle.GetFloat(kParticleForceZPos);
	float forceY = particle.GetFloat(kParticleForceYPos);
	float forceX = particle.GetFloat(kParticleForceXPos);

	_forcePosition.x = forceX;
	_forcePosition.y = forceY;
	_forcePosition.z = forceZ;
	_forceBlend = particle.GetFloat(kParticleForceBlend);

	readCurve(particle, kParticleBlendSizeCurve, _sizeCurve, 1.0f);
	_maxTile = particle.GetInt(kParticleBlendMaxTile);
	readCurve(particle, kParticleBlendTilesCurve, _tileCurve, 1.0f);

	IncTime();

	// the lead time (in seconds, 40 steps each) of the container lets the effect run before it is first shown
	TVisObjRef container = particle.GetParent();
	int steps = container.GetInt(kParticleContainerLeadTime) * 5 * 8;

	for (int i = 0; i < steps; i++)
		IncTime();

	return true;
}

// Confirmed (asm lines 1380801-1381041)
bool TGParticleSystem::Init(const TVisObjRef &container, const wxString &/*name*/) {
	Clear();

	if (container.IsEmpty())
		return false;

	_cameraZ = container.GetFloat(kParticleContainerCameraZPosition);
	_cameraZoom = container.GetFloat(kParticleContainerCameraZoom);

	TVList particles;

	container.GetLinks(kParticleContainerParticles, TypeOrder::kValue1, particles);

	for (TVisionaireObject *object : particles.items) {
		TGParticleEmitter *emitter = new TGParticleEmitter();

		emitter->Init(TVisObjRef(*object));
		_emitters.push_back(emitter);

		TPictureIO *texture = new TPictureIO();

		texture->CreateSpriteTexture(object->GetPath(kParticleTexture));
		_textures.push_back(texture);
	}

	return true;
}
