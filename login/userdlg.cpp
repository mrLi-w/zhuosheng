#include <QFile>
#include "userdlg.h"
#include "ui_userdlg.h"
#include <QMessageBox>
#include <QDebug>
#include "loginmanage.h"

UserDlg::UserDlg(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::UserDlg),
    gpLoginManage(new LoginManage)
{
    ui->setupUi(this);

//    setTabOrder(ui->leUser,ui->lePassword);
//    setTabOrder(ui->lePassword,ui->pbLogin);
//    setTabOrder(ui->pbLogin,ui->leUser);
    //connect(ui->pBback,SIGNAL(clicked()),this,SLOT(slot_back()));
    connect(ui->pbLogout,SIGNAL(clicked()),this,SLOT(slot_Logout()));
//    connect(ui->pbReturnCG,SIGNAL(clicked()),this,SLOT(slot_showMain()));
//    connect(ui->pbReturnFP,SIGNAL(clicked()),this,SLOT(slot_showMain()));
//    ui->lbFPW_Code->setText(QString("%1").arg(gpLoginManage->getMacCode()));
}

UserDlg::~UserDlg()
{
    delete ui;
}

void UserDlg::showcentral(int t1,int t2,int t3,int t4)
{

    int x,y,t5,t6;
    t5=this->width();
    t6=this->height();
    x=t1+t3/2-t5/2;
    y=t2+t4/2-t6/2;
    this->move(x,y);
    this->show();

}

void UserDlg::slot_back()
{
    emit back();
    this->hide();
}

void UserDlg::slot_Logout()
{
    loginLevel = al_notlogin;
    login(loginLevel);
    this->hide();
}

//void UserDlg::on_lbChangePW_linkActivated(const QString &)
//{
//    ui->stackedWidget->setCurrentIndex(1);
//}

//void UserDlg::on_lbForgetPW_linkActivated(const QString &)
//{
//    ui->stackedWidget->setCurrentIndex(2);
//}

void UserDlg::showEvent(QShowEvent *)
{
    //ui->stackedWidget->setCurrentIndex(0);
}

void UserDlg::on_pbLogin_clicked()
{
    int iUser = ui->cbUser->currentIndex();
    QString sPassword = ui->lePassword->text();
    if(iUser==0)
    {
        if(sPassword == "0000"){
            QMessageBox::about(this,tr("提示"),tr("登录成功！"));
            loginLevel = al_operation;
            emit login(loginLevel);
            ui->lePassword->clear();
            hide();
            return;
        }
        else
        {
           QMessageBox::warning(this,tr("警告"),tr("登录失败，用户名或者密码错误！"));
           ui->lePassword->clear();
        }
    }
    if(iUser==1)
    {
        if(sPassword == "1111"){
            QMessageBox::about(this,tr("提示"),tr("登录成功！"));
            loginLevel = al_admin;
            emit login(loginLevel);
            ui->lePassword->clear();
            hide();
            return;
        }
        else if(sPassword == "1210"){
            QMessageBox::about(this,tr("提示"),tr("登录成功！"));
            loginLevel = al_supper;
            emit login(loginLevel);
            ui->lePassword->clear();
            hide();
            return;
        }
        else if(sPassword == "2580"){
            QMessageBox::about(this,tr("提示"),tr("登录成功！"));
            loginLevel = al_kaifa;
            emit login(loginLevel);
            ui->lePassword->clear();
            hide();
            return;
        }
        else
        {
           QMessageBox::warning(this,tr("警告"),tr("登录失败，用户名或者密码错误！"));
           ui->lePassword->clear();
        }
    }
//    if(iUser==2)
//    {
//        if(sPassword == "1210"){
//            QMessageBox::about(this,tr("提示"),tr("登录成功！"));
//            loginLevel = al_supper;
//            emit login(loginLevel);
//            ui->lePassword->clear();
//            hide();
//            return;
//        }
//        else
//        {
//           QMessageBox::warning(this,tr("警告"),tr("登录失败，用户名或者密码错误！"));
//           ui->lePassword->clear();
//        }
//    }
//    QString strUser = ui->cbUser->currentIndex()?"admin":"guest";
//    QString strPassword = ui->lePassword->text();

//    if(gpLoginManage->check(strUser.toAscii().data(),strPassword.toAscii().data())){
//        QMessageBox::about(this,tr("提示"),tr("登录成功！"));
//        if(strUser == "admin")
//            loginLevel = al_admin;
//        else if(strUser == "guest")
//            loginLevel = al_operation;
//        emit login(loginLevel);
//        hide();
//    }
//    else
//        QMessageBox::warning(this,tr("警告"),tr("登录失败，用户名或者密码错误！"));


    //ui->lePassword->clear();
}

//void UserDlg::on_pbChangePW_clicked()
//{
//    QString strUser = ui->cbMDUser->currentIndex()==0?"guest":"admin";
//    QString strPassword = ui->leCPW_Password->text();
//    QString strNewPassword1 = ui->leCPW_Password1->text();
//    QString strNewPassword2 = ui->leCPW_Password2->text();

//    if(strNewPassword1!=strNewPassword2){
//        QMessageBox::warning(this,tr("警告"),tr("密码不一致，请重新输入！"));
//        return;
//    }
//    if(gpLoginManage->modify(strUser.toAscii().data(),strPassword.toAscii().data()
//                  ,strNewPassword1.toAscii().data()))
//        QMessageBox::about(this,tr("提示"),tr("修改成功！"));
//    else
//        QMessageBox::warning(this,tr("警告"),tr("密码错误！"));
//}

//void UserDlg::on_pbForgetPW_clicked()
//{
//    QString strCode = ui->leFPW_Decode->text();
//    int i = strCode.toInt();
//    if(gpLoginManage->checkVerificationCode(i)){
//        QMessageBox::about(this,tr("提示"),tr("密码重置成功！"));
//        ui->lbFPW_Code->setText(QString("%1").arg(gpLoginManage->getMacCode()));
//    }
//    else
//        QMessageBox::warning(this,tr("警告"),tr("密码重置失败！"));
//}

//void UserDlg::slot_showMain()
//{
//    ui->stackedWidget->setCurrentIndex(0);
//}

//传随机数
//                    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
//                    int v = qrand()%10-5;
//                    double v1=v/1000.0;
//                    double v2=1.0+v1;
//                    double v3=conc*v2;
//                    lastValue=QString::number(v3,'f',3);
//                    return;
