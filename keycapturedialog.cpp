#include "keycapturedialog.h"
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QLabel>

KeyCaptureDialog::KeyCaptureDialog(QWidget *parent)
    : QDialog(parent), m_capturedKey(0)
{
    setWindowTitle("Нажмите кнопку");
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    resize(200, 100);

    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel("Нажмите нужную клавишу", this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    setLayout(layout);
}

void KeyCaptureDialog::keyPressEvent(QKeyEvent *event)
{
    // Используем nativeVirtualKey(), чтобы получить настоящий VK код
    m_capturedKey = event->nativeVirtualKey();
    accept();
}

int KeyCaptureDialog::capturedKey() const
{
    return m_capturedKey;
}