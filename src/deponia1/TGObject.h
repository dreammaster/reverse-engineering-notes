// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 258158-259640, all 13 manifest-listed methods; the
// manifest's range runs on to 259458 only, the rest of the constructor is the lines
// after it): an object of a scene - what the player clicks and sees. It is a TMObject
// (see there) with what the running game adds: the object's hit area (polygon from
// kObjectPolygon, moved by its offset), its position and centre, a scroll factor that
// makes it move slower or faster than the scene (parallax), the settings for drawing it
// (shader, matrix id, rotation, scale), an optional particle effect while it is active,
// and an optional "snoop" animation that is shown on top while the object is looked at.
//
// Not reconstructed (see TODO.md): the particle effect from a script expression
// (kParticleContainerSettings), the drawing of particles and of the shader, matrix and
// rotation settings (they go to the graphics backend), and the matrix transform of
// IsInside()/DrawSnoopAnimation().
//
// Original layout (TMObject's fields end at +0x1AC): +0x1AC whether the object scrolls
// at its own speed, +0x1B0/+0x1B4 how much faster (percent over 100), +0x1B8 a
// TPictureIO (cleared when the object is deactivated), +0x2A0 whether it has a
// particle effect, +0x2A1 whether that one is a script container, +0x2A8 the
// TGParticleSystem, +0x300 the ParticleContainer, +0x308 the particle timer, +0x318 the
// snoop animation. The settings read from the data (+0x17C rotation, +0x180 rotation
// centre, +0x188 shader, +0x18C matrix id, +0x190/+0x194 scale) are before them.
#pragma once

#include "TGParticleSystem.h"
#include "TMObject.h"
#include "TTimer.h"

class TGDetectInfo;

class TGObject : public TMObject {
public:
	explicit TGObject(const TVisObjRef &ref);
	~TGObject() override;

	/** Shows or hides the snoop animation (the one shown while the player looks at the
	 *  object with the "snoop" cursor). */
	void ShowSnoopAnimation(bool show) override;
	/** The object's own centre moved by its offset (for the drawing order). */
	int GetCenter() const override;
	/** Draws the object (see TManagedObject::Draw()) with the scene's brightness and its
	 *  shader, and its particle effect. */
	void Draw() override;
	/** Whether a character can walk on it (kObjectIsWalkable). */
	bool IsWalkable() const override;
	/** Its position moved by its offset. */
	wxPoint GetPosition() const override;
	/** Turns a character to the direction the object is to be used from. */
	void AlignCharacter(TVisObjRef &character) const override;
	void AnimationStopped(TGAnimation *animation) override;
	/** A point is in the object when the detection looks for objects, and it is in the
	 *  object's polygon, after the offset and the scroll factor are taken away. */
	bool IsInside(const wxPoint &position, const TGDetectInfo &info) const override;
	using TManagedObject::IsInside;
	/** Draws the snoop animation, when it runs. */
	void DrawSnoopAnimation() override;
	/** Activating also starts the particle effect (when it has one); deactivating ends it. */
	void SetActive(bool active) override;

protected:
	float _rotation;                    // +0x17C
	wxPoint _rotationCenter;            // +0x180
	int _shader;                        // +0x188
	int _matrixId;                      // +0x18C
	float _scaleX;                      // +0x190
	float _scaleY;                      // +0x194
	bool _scrolls;                      // +0x1AC
	int _scrollX;                       // +0x1B0
	int _scrollY;                       // +0x1B4
	bool _hasParticles;                 // +0x2A0
	bool _particlesFromScript;          // +0x2A1
	TGParticleSystem _particleSystem;   // +0x2A8
	ParticleContainer *_particleContainer;  // +0x300
	TTimer _particleTimer;              // +0x308
	TGAnimation *_snoopAnimation;       // +0x318
};
