#include <chrono>
#include <cstdio>
#include <vector>

#include "AppGlobals.h"
#include "TTimer.h"
#include "graphicslib/graphics.h"
#include "graphicslib/picture.h"
#include "vscommon/canimation.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

static long now() {
	return (long)std::chrono::duration_cast<std::chrono::milliseconds>(
	    std::chrono::steady_clock::now().time_since_epoch()).count();
}

// The playing of the pictures of an animation: the order (forwards, backwards, by chance), the loops, the pauses of the
// pictures, the wait between the loops of an animation that loops by chance, and the questions that can be asked of it.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/a.png", 10, 10);
	writeFakePicture("pics/b.png", 10, 10);
	writeFakePicture("pics/c.png", 10, 10);

	graphics = new NullGraphics();

	TVisionaireGame game;
	TVisObjRef gameRef = game.GetGame();

	gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);

	TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
	TVisObjRef object = game.CreateObject(6, scene, kSceneObjects);

	auto makeData = [&](const char *name, int replay, int loops, int pause) {
		TVisObjRef animation = game.CreateObject(9, object, kObjectAnimations);

		animation.SetName(TCharHolder(name));

		std::vector<TSprite> sprites(3);

		sprites[0].SetPath(TCharHolder("pics/a.png"));
		sprites[1].SetPath(TCharHolder("pics/b.png"));
		sprites[2].SetPath(TCharHolder("pics/c.png"));
		animation.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
		animation.SetValue(kAnimationPosition, wxPoint{10, 20}, TSendEventEnum::kNoEvent);
		animation.SetValue(kAnimationCenter, wxPoint{1, 2}, TSendEventEnum::kNoEvent);
		animation.SetValue(kAnimationReplay, replay, TSendEventEnum::kNoEvent);
		animation.SetValue(kAnimationNumberOfLoops, loops, TSendEventEnum::kNoEvent);
		animation.SetValue(kAnimationPause, pause, TSendEventEnum::kNoEvent);
		return animation;
	};
	// the indexes of the pictures that are shown one after the other, until the animation is over (or `limit` of them)
	auto play = [&](TCAnimation &animation, bool reverse, size_t limit) {
		std::vector<int> shown;

		animation.Start(reverse, 100.0f);

		long deadline = now() + 3000;

		while (!animation.IsFinished() && shown.size() < limit && now() < deadline) {
			animation.SetCurrentSprite(false);

			if (animation.IsFinished())
				break;

			int index = animation.GetCurrentSpriteIndex();

			// (a picture is noted when it changes: no two pictures one after the other are the same)
			if (index >= 0 && (shown.empty() || shown.back() != index))
				shown.push_back(index);

			wxMilliSleep(1);
		}
		return shown;
	};

	// forwards: the three pictures once
	{
		TVisObjRef data = makeData("Forward", 0, 1, 15);
		TVisObjRef state = game.CreateActiveObject(26, data);
		TCAnimation animation(state, data);

		CHECK(animation.IsSpriteAnimation() && !animation.IsModelAnimation() && !animation.IsBonesAnimation());
		CHECK(animation.GetFrameCount() == 3);
		CHECK(animation.IsFinished());
		CHECK(!animation.IsRandomOrder() && !animation.IsLoopRandom() && !animation.IsMoveAnimation());
		CHECK(animation.WaitBetweenLoops());
		CHECK(!animation.IsSpriteIndexValid());

		std::vector<int> shown = play(animation, false, 10);

		printf("forward:");
		for (int index : shown)
			printf(" %d", index);
		printf("\n");
		CHECK(shown == (std::vector<int>{0, 1, 2}));
		CHECK(animation.IsFinished());
		CHECK(animation.GetCurrentSprite() == nullptr);
		CHECK(animation.GetAnimationLoops() == 1);
		CHECK(!animation.IsEndless());
		animation.PreloadSprites(true);
		animation.PreloadSprites(false);
		animation.RefreshSprites();
		animation.RemoveSprites();
		animation.NextSpriteSelected();
	}

	// backwards, by the animation or by the call
	{
		TVisObjRef data = makeData("Backward", 1, 1, 15);
		TVisObjRef state = game.CreateActiveObject(26, data);
		TCAnimation animation(state, data);

		CHECK(play(animation, false, 10) == (std::vector<int>{2, 1, 0}));

		TVisObjRef data2 = makeData("Forward2", 0, 1, 15);
		TVisObjRef state2 = game.CreateActiveObject(26, data2);
		TCAnimation reversed(state2, data2);

		CHECK(play(reversed, true, 10) == (std::vector<int>{2, 1, 0}));

		// both: forwards again
		TVisObjRef state3 = game.CreateActiveObject(26, data);
		TCAnimation both(state3, data);

		CHECK(play(both, true, 10) == (std::vector<int>{0, 1, 2}));
	}

	// two loops, and the loops that are left
	{
		TVisObjRef data = makeData("Twice", 0, 2, 10);
		TVisObjRef state = game.CreateActiveObject(26, data);
		TCAnimation animation(state, data);

		animation.Start(false, 100.0f);
		CHECK(animation.GetAnimationLoops() == 2);
		CHECK(animation.GetCurrentSpriteIndex() == -1);

		std::vector<int> shown = play(animation, false, 10);

		CHECK(shown == (std::vector<int>{0, 1, 2, 0, 1, 2}));
		CHECK(animation.IsFinished());

		// a negative number of loops is none: it never ends
		TVisObjRef negative = makeData("Negative", 0, -3, 10);
		TVisObjRef negativeState = game.CreateActiveObject(26, negative);
		TCAnimation endless(negativeState, negative);

		endless.Start(false, 100.0f);
		CHECK(endless.IsEndless());
		CHECK(endless.GetAnimationLoops() == 0);
	}

	// by chance: every picture once in a pass (not the one before), then the end
	{
		TVisObjRef data = makeData("Random", 2, 1, 10);
		TVisObjRef state = game.CreateActiveObject(26, data);
		TCAnimation animation(state, data);

		CHECK(animation.IsRandomOrder());

		std::vector<int> shown = play(animation, false, 10);

		printf("random: %d pictures\n", (int)shown.size());
		CHECK(shown.size() >= 3);
		for (int index : shown)
			CHECK(index >= 0 && index < 3);
		for (size_t i = 1; i < shown.size(); i++)
			CHECK(shown[i] != shown[i - 1]);
		CHECK(animation.IsFinished());
	}

	// the pause of each picture
	{
		TVisObjRef data = makeData("Individual", 0, 0, 100);
		std::vector<TSprite> sprites(3);

		sprites[0].SetPath(TCharHolder("pics/a.png"));
		sprites[0].SetPause(30);
		sprites[1].SetPath(TCharHolder("pics/b.png"));
		sprites[1].SetPause(-1);
		sprites[2].SetPath(TCharHolder("pics/c.png"));
		sprites[2].SetPause(70);
		data.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);

		TVisObjRef state = game.CreateActiveObject(26, data);
		TCAnimation animation(state, data);

		animation.Start(false, 100.0f);
		animation.SetCurrentSprite(false);
		CHECK(animation.GetCurrentSpriteIndex() == 0);
		// (the pause of the animation, as the pictures do not have their own)
		CHECK(animation.GetCurrentPause() == 100);
		CHECK(animation.GetPauseCompletion() >= 0.0f && animation.GetPauseCompletion() <= 1.0f);
		wxMilliSleep(120);
		CHECK(animation.GetPauseCompletion() == 1.0f);

		data.SetValue(kAnimationUseIndividualPause, true, TSendEventEnum::kNoEvent);
		CHECK(animation.GetCurrentPause() == 30);
		animation.SetCurrentSprite(true);
		CHECK(animation.GetCurrentSpriteIndex() == 1);
		CHECK(animation.GetCurrentPause() == 100);
		animation.SetCurrentSprite(true);
		CHECK(animation.GetCurrentSpriteIndex() == 2);
		CHECK(animation.GetCurrentPause() == 70);

		// a pause of nothing is complete at once... or not started
		data.SetValue(kAnimationPause, 0, TSendEventEnum::kNoEvent);
		data.SetValue(kAnimationUseIndividualPause, false, TSendEventEnum::kNoEvent);
		CHECK(animation.GetPauseCompletion() == 0.0f);
	}

	// a picture that is released when it is replaced: only in the last loop of a pass that is not by chance
	{
		TVisObjRef data = makeData("Releasing", 0, 1, 10);
		TVisObjRef state = game.CreateActiveObject(26, data);
		TCAnimation animation(state, data);

		CHECK(!animation.CanRemoveCurrentSprite());
		animation.Start(false, 100.0f);
		animation.SetCurrentSprite(false);
		CHECK(animation.GetCurrentSprite() != nullptr);
		CHECK(animation.CanRemoveCurrentSprite());

		// the position of the picture: in the animation, scaled
		animation.SetPosition(wxPoint{100, 200}, 50.0f);
		printf("position %d,%d\n", animation.GetCurrentSpritePosition().x, animation.GetCurrentSpritePosition().y);
		CHECK((animation.GetCurrentSpritePosition() == wxPoint{100, 200}) || animation.GetCurrentSpritePosition().x != 0);
		animation.SetPosition(wxPoint{5, 6}, -1.0f);
		CHECK(state.GetFloat(kAnimationSize) == 50.0f);
	}

	// a loop by chance waits (5 to 20 seconds) between the passes: here shortened by changing the time
	{
		TVisObjRef data = makeData("Waiting", 0, 0, 10);
		data.SetValue(kAnimationLoopRandom, true, TSendEventEnum::kNoEvent);

		TVisObjRef state = game.CreateActiveObject(26, data);
		TCAnimation animation(state, data);

		CHECK(animation.IsLoopRandom());
		animation.Start(false, 100.0f);
		CHECK(animation.IsWaiting());

		int waiting = state.GetInt(kAnimationWaitingTime);

		printf("waits %d ms\n", waiting);
		CHECK(waiting >= 5000 && waiting < 20000);
		animation.SetCurrentSprite(false);
		CHECK(animation.GetCurrentSprite() == nullptr);
		CHECK(animation.GetCurrentSpriteIndex() == -1);

		// (the wait is over)
		state.SetValue(kAnimationWaitingTime, 20, TSendEventEnum::kNoEvent);
		wxMilliSleep(40);
		animation.SetCurrentSprite(false);
		CHECK(!animation.IsWaiting());
		CHECK(animation.GetCurrentSpriteIndex() == 0);

		// a pass later it waits again
		long deadline = now() + 2000;

		while (!animation.IsWaiting() && now() < deadline) {
			animation.SetCurrentSprite(false);
			wxMilliSleep(2);
		}
		CHECK(animation.IsWaiting());
		CHECK(animation.GetCurrentSpriteIndex() == -1);
		CHECK(animation.IsEndless());

		// the loops that are counted wait too, and the last one ends
		TVisObjRef counted = makeData("Counted", 0, 2, 10);
		counted.SetValue(kAnimationLoopRandom, true, TSendEventEnum::kNoEvent);

		TVisObjRef countedState = game.CreateActiveObject(26, counted);
		TCAnimation twice(countedState, counted);

		twice.Start(false, 100.0f);
		countedState.SetValue(kAnimationWaitingTime, 0, TSendEventEnum::kNoEvent);

		int waits = 0;

		deadline = now() + 3000;

		while (!twice.IsFinished() && now() < deadline) {
			twice.SetCurrentSprite(false);
			if (twice.IsWaiting() && countedState.GetInt(kAnimationWaitingTime) > 0) {
				waits++;
				countedState.SetValue(kAnimationWaitingTime, 0, TSendEventEnum::kNoEvent);
			}
			wxMilliSleep(1);
		}
		printf("waited %d times\n", waits);
		CHECK(twice.IsFinished());
		CHECK(waits == 1);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
