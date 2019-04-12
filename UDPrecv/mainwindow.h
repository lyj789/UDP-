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
#include<QMap>

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
    void recevie_file();
     void continue_transfer(qint64);
     void findfile();
     void onTimeout();

private:
    Ui::MainWindow *ui;
    QUdpSocket *receive;
    QString fileName;
    QFile *file;
    QFile *filerequest;
    /* 已接受数据，总数据，文件名长度 */
    qint64 gotBytes, fileBytes,restBytes,sentBytes;
    qint64 f;
    qint64 pp;
    int testa,testb;
    QByteArray alldata;
    QTimer *time;
    QHostAddress address;
    quint16 port;
//    THREAD *th;
    QMap<quint16 ,int >map;

};

#endif // MAINWINDOW_H
