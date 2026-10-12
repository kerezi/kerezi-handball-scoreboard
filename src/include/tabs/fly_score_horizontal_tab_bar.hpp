#ifndef FLY_SCORE_HORIZONTAL_TAB_BAR
#define FLY_SCORE_HORIZONTAL_TAB_BAR

#pragma once

#include <QTabBar>
#include <QStylePainter>
#include <QStyleOptionTab>
#include <QTabWidget>

class FlyHorizontalTabBar : public QTabBar {
public:
    explicit FlyHorizontalTabBar(QWidget *parent = nullptr) : QTabBar(parent) {}

protected:
    QSize tabSizeHint(int index) const override {
        QSize size = QTabBar::tabSizeHint(index);
        if (size.width() < size.height()) {
            size.transpose();
        }
        return size;
    }

    void paintEvent(QPaintEvent *event) override {
        QStylePainter painter(this);
        QStyleOptionTab option;

        painter.setFont(this->font()); 
        for (int i = 0; i < count(); ++i) {
            initStyleOption(&option, i);
            
            painter.drawControl(QStyle::CE_TabBarTabShape, option);
            
            painter.drawText(tabRect(i), Qt::AlignCenter | Qt::TextDontClip, tabText(i));
        }
    }
};

class FlyHorizontalTabWidget : public QTabWidget {
public:
    explicit FlyHorizontalTabWidget (QWidget *parent = nullptr) : QTabWidget(parent) {
        setTabBar(new FlyHorizontalTabBar(this)); 
    }
};


#endif
