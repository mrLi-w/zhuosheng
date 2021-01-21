#ifndef SYSTEMWINDOW_H
#define SYSTEMWINDOW_H

#include <QWidget>
#include "elementinterface.h"

namespace Ui {
class SystemWindow;
}

class SystemWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SystemWindow(QWidget *parent = 0);
    ~SystemWindow();
    void loadparam();
    void saveparam();

public slots:
    void screenCalibration();
//    void platformSelect(int i);
    void updateProgram();
    void setTime();
    void showVersion();
    void printData(int);
private slots:
    void on_pushButton_clicked();
    void on_ScreenCalibration_clicked();

private:
    Ui::SystemWindow *ui;
    //ElementInterface*element;
};

#endif // SYSTEMWINDOW_H
