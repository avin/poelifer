#ifndef KEYCAPTUREDIALOG_H
#define KEYCAPTUREDIALOG_H

#include <QDialog>

class QKeyEvent;

class KeyCaptureDialog : public QDialog
{
    Q_OBJECT
public:
    explicit KeyCaptureDialog(QWidget *parent = nullptr);
    int capturedKey() const;
protected:
    void keyPressEvent(QKeyEvent *event) override;
private:
    int m_capturedKey;
};

#endif // KEYCAPTUREDIALOG_H