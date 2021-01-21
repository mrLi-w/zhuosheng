#ifndef M4_20MADLG_H
#define M4_20MADLG_H

#include <QFrame>

namespace Ui {
class Calib420mA;
}

struct V420mA
{
    int vle5mA;
    int vle10mA;
    int vle15mA;
    int vle19mA;

    float k1;
    float b1;
    float k2;
    float b2;
    float k3;
    float b3;
};


class Calib420mA : public QFrame
{
    Q_OBJECT
    
public:
    explicit Calib420mA(QWidget *parent = 0);
    ~Calib420mA();

    void renew420mAWnd();
    void save420mAParam();
    void load420mAParam();

public Q_SLOTS:
    void slot_pb420mACar();

Q_SIGNALS:
    void sig_5mA();
    void sig_10mA();
    void sig_15mA();
    void sig_20mA();

private:
    Ui::Calib420mA *ui;
    V420mA *v420mA;
};

#endif // M4_20MADLG_H
