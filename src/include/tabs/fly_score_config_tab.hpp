#ifndef FLY_SCORE_CONFIG_TAB
#define FLY_SCORE_CONFIG_TAB

#pragma once

#include <QWidget>

class FlyConfigTab : public QWidget {
public:
    explicit FlyConfigTab(QWidget *parent = nullptr) : QWidget(parent);

    ~FlyConfigTab() override;

};

#endif
