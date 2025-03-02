#include "screenpicker.h"
#include <QPainter>
#include <QMouseEvent>

ScreenPicker::ScreenPicker(const QPixmap &screenshot, QWidget *parent)
    : QWidget(parent), m_screenshot(screenshot)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose);
    resize(screenshot.size());
    setCursor(Qt::CrossCursor);
}

void ScreenPicker::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_screenshot);
}

void ScreenPicker::mousePressEvent(QMouseEvent *event)
{
    emit pointPicked(event->pos());
    close();
}