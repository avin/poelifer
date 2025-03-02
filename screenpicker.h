#ifndef SCREENPICKER_H
#define SCREENPICKER_H

#include <QWidget>
#include <QPixmap>
#include <QPoint>

class ScreenPicker : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenPicker(const QPixmap &screenshot, QWidget *parent = nullptr);
signals:
    void pointPicked(const QPoint &pt);
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
private:
    QPixmap m_screenshot;
};

#endif // SCREENPICKER_H