#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QFile>
#include<QFileDialog>
#include<QMessageBox>
#include<QTextStream>
#include<QByteArray>
#include<QDebug>
#include<QUdpSocket>
#include<QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
       void continue_transfer(qint64);
       void on_selectBtn_clicked();
       void on_sendBtn_clicked();
       void readData();
       void onTimeout();
       void findfile();


       void on_pushButton_clicked();

       void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
    QUdpSocket *send;
    QFile *file,*filerespond;
    QString fileName;
    int i=0;
    /* 总数据大小，已发送数据大小，剩余数据大小，每次发送数据块大小 */
    qint64 fileBytes, sentBytes, restBytes,gotBytes;
    QByteArray alldata;
    QTimer *time;
    qint64 f;
};

#endif // MAINWINDOW_H
