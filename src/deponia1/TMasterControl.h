#pragma once

class TGameController;

class TMasterControl {
public:
    TGameController* GetGameController();

private:
    TGameController* m_gameController = nullptr;
};
