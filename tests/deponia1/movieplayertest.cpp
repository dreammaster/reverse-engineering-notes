#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AppGlobals.h"
#include "SdlStub.h"
#include "TMovie.h"
#include "graphicslib/softwareGraphics.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// The player of the movies on a real movie of the game (VP8 in Matroska): run this from the game directory, else only the
// answers for a file that is not a movie are tested.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 0;

	TSoftwareGraphics *backend = new TSoftwareGraphics(480, 270);

	graphics = backend;

	// (the first events of the headless SDL are that the window is hidden and closed)
	SDL_Event event;

	while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, 0, 0xFFFF) > 0) {
	}

	TMovieSettings settings;

	settings.videoResolution.x = 480;
	settings.videoResolution.y = 270;

	TMoviePlayer *player = CreateMatroskaMoviePlayer();

	CHECK(player != nullptr);
	CHECK(!player->Open(L"movietest_not_there.mkv", settings));
	CHECK(!player->Frame(nullptr));

	if (!player->Open(L"videos/video.vv001", settings)) {
		printf("(no real movie here)\n%d failures\n", failures);
		delete player;
		return failures ? 1 : 0;
	}

	CHECK(player->GetWidth() == 1920 && player->GetHeight() == 1080);
	CHECK(player->GetDuration() > 1.0f);

	// the pictures: the first one is drawn at once, in the middle of the black window; the clock goes on by itself
	CHECK(player->Frame(nullptr));
	CHECK(backend->GetDrawnCount() >= 1);

	auto brightness = [&]() {
		const std::vector<uint8_t> &shown = backend->GetFrame();
		long long sum = 0;

		for (size_t i = 0; i < shown.size(); i += 4)
			sum += shown[i] + shown[i + 1] + shown[i + 2];
		return sum;
	};

	printf("brightness at the start: %lld\n", brightness());

	wxMilliSleep(300);
	CHECK(player->Frame(nullptr));
	CHECK(player->GetTime() >= 0.25f && player->GetTime() < 2.0f);
	CHECK(player->GetCompletition() > 0.0f);

	// pause: the clock stands; resume: it goes on
	player->Pause();

	float paused = player->GetTime();

	wxMilliSleep(150);
	CHECK(player->GetTime() == paused);
	player->Resume();
	wxMilliSleep(100);
	CHECK(player->GetTime() > paused);

	// seek: the time is the one asked for, and the movie goes on from there
	player->Seek(1.0f);
	CHECK(player->GetTime() >= 1.0f && player->GetTime() < 1.5f);
	CHECK(player->Frame(nullptr));
	printf("brightness at 1 s: %lld\n", brightness());
	CHECK(brightness() > 0);

	// a picture that a script draws itself: at the place and size it gives
	CHECK(player->Draw(10.0f, 20.0f, 100.0f, 50.0f));
	player->Close();
	delete player;

	// a movie that can be skipped ends with a key; one that cannot goes on
	settings.skippable = false;
	player = CreateMatroskaMoviePlayer();
	CHECK(player->Open(L"videos/video.vv001", settings));
	event.type = SDL_KEYDOWN;
	SDL_PushEvent(&event);
	CHECK(player->Frame(nullptr));
	player->Close();
	delete player;

	settings.skippable = true;
	player = CreateMatroskaMoviePlayer();
	CHECK(player->Open(L"videos/video.vv001", settings));
	event.type = SDL_KEYDOWN;
	SDL_PushEvent(&event);
	CHECK(!player->Frame(nullptr));
	player->Close();
	delete player;

	// the movie plays to its end, and with the loop it does not end
	settings.skippable = false;
	player = CreateMatroskaMoviePlayer();
	CHECK(player->Open(L"videos/video.vv002", settings));

	float length = player->GetDuration();

	player->Seek(length - 0.1f);

	int frames = 0;

	while (player->Frame(nullptr) && frames < 200) {
		frames++;
		wxMilliSleep(20);
	}
	CHECK(frames < 200);
	CHECK(!player->Frame(nullptr));
	player->Close();
	delete player;

	player = CreateMatroskaMoviePlayer();
	CHECK(player->Open(L"videos/video.vv002", settings));
	player->SetLoop(true);
	player->Seek(player->GetDuration() - 0.1f);
	for (int i = 0; i < 30; i++) {
		CHECK(player->Frame(nullptr));
		wxMilliSleep(20);
	}
	CHECK(player->GetTime() < player->GetDuration());
	player->Close();
	delete player;

	// all the movies of the data: each opens, shows a picture and has a length
	if (getenv("MOVIETEST_ALL")) {
		int opened = 0;

		for (int volume = 0; volume < 42; volume++) {
			char name[64];

			snprintf(name, sizeof(name), "videos/video.vv%03d", volume);

			std::wstring path(name, name + strlen(name));
			TMoviePlayer *each = CreateMatroskaMoviePlayer();

			if (!each->Open(wxString(path.c_str()), settings)) {
				printf("%s: does not open\n", name);
				failures++;
				delete each;
				continue;
			}

			printf("%s: %dx%d, %.1f s\n", name, each->GetWidth(), each->GetHeight(), each->GetDuration());
			CHECK(each->GetWidth() > 0 && each->GetDuration() > 0.0f);
			each->Seek(each->GetDuration() / 2.0f);
			CHECK(each->Frame(nullptr));
			opened++;
			each->Close();
			delete each;
		}
		printf("%d movies opened\n", opened);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
