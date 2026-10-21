#pragma once
// Console host for headless script runs: prints text, auto-answers prompts (yes / ESC).
#include "fod/vm.h"
#include <cstdio>
#include <vector>

class TextHost : public fod::Host {
public:
	std::vector<int> keys;
	void print(const std::string &t) override {
		for (char c : t) std::putchar(c == '\r' ? '\n' : c);
	}
	void message(const std::string &t) override {
		std::printf("[message] ");
		print(t);
		std::putchar('\n');
	}
	int readKey() override {
		if (keys.empty()) return 0xFF;
		const int k = keys.front();
		keys.erase(keys.begin());
		return k;
	}
	bool yesNo(const std::string &p) override {
		std::printf("[yes/no] %s -> yes\n", p.c_str());
		return true;
	}
	std::string readLine() override { return ""; }
	void sound(int n) override { std::printf("[sound %d]\n", n); }
	void log(const std::string &m) override { std::printf("[log] %s\n", m.c_str()); }
	void showMenu(const std::string &t, bool) override {
		std::printf("[menu] ");
		print(t);
		std::putchar('\n');
	}
};
