#pragma once

#ifndef FLY_SCORE_PLUGIN
#define FLY_SCORE_PLUGIN

#include<QWidget>

class FlyScorePlugin {
public:
    FlyScorePlugin();
    ~FlyScorePlugin();

    bool init();

private:

    QWidget *dock = nullptr;

};

#endif