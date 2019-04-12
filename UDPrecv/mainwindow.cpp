#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <QDataStream>
#include <QFile>
#include <QHostAddress>
#include<QUdpSocket>
#include<QtGlobal>
#include<QTime>

const quint16 PORT = 3333;
#define TIMER_TIMEOUT   (5*1000)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    time = new QTimer(this);

    /*设置天选之子*/
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
    testa =qrand()%1000;
    testb =qrand()%1000;
    qDebug()<<testa;
    qDebug()<<testb;

    ui->recvProg->setValue(0);
    ui->sendProg->setValue(0);

    receive = new QUdpSocket;
    receive->bind(QHostAddress("127.0.0.1"), PORT);
    qDebug()<<receive->localAddress();
    qDebug()<<receive->localPort();

    /*列出共享文件目录*/
    findfile();

    connect(receive, SIGNAL(readyRead()), this, SLOT(recevie_file()));
    connect(time, SIGNAL(timeout()), this, SLOT(onTimeout()));

    fileBytes = gotBytes= 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

int i=0;
int tempa,tempb;

/*--- 接收文件 ---*/
void MainWindow::recevie_file()
{
    QByteArray ba;
    while(receive->hasPendingDatagrams())
    {
        i++;
        qDebug()<<i;
        qDebug()<<"第"<<i-1<<"个数据包";

        ba.resize(receive->pendingDatagramSize());
        receive->readDatagram(ba.data(),ba.size(),&address,&port);
        gotBytes+=ba.size();

        qDebug()<<ba.size();
        qDebug()<<ba;
        qDebug()<<ba.mid(ba.size()-1,1);
        qDebug()<<ba.mid(0,ba.size()-1);


        QByteArray biaoshi=ba.mid(ba.size()-1,1);

        /*判断数据包标识进行不同处理*/
        /*响应发送文件的请求*/
        if(biaoshi=="3")
        {
            QString filename="E://udp//"+ba.mid(0,ba.size()-1);
            filerequest = new QFile(filename);
            if (!filerequest->open(QFile::ReadOnly))
            {   qDebug()<<"文件打开失败";
                return;
            }
            fileBytes = filerequest->size();
            restBytes=fileBytes;
            qDebug()<<"文件比特数"<<fileBytes;
            ui->sendProg->setValue(0);

            qDebug()<<"文件名"<<ba.mid(0,ba.size()-1);

            QByteArray buf;
            QDataStream out(&buf, QIODevice::WriteOnly);

            /* 首部 = 总大小 + 文件名 */
            out << qint64(0);//<< sfName;
            /* 总大小加上首部的大小 */
            buf+=ba.mid(0,ba.size()-1);
            buf+="0";   //数据包类型加在每个数据包最后，0表示是文件的大头
            fileBytes =fileBytes+ buf.size()+((fileBytes/510)+1)*2;
            qDebug()<<"加了头之后的报文比特数"<<fileBytes;
            /* 重写首部的前两个长度字段 */
            out.device()->seek(0);
            out << fileBytes;//<<sfName;
            /* 发送首部，计算剩余大小 */
            qDebug()<<"头长度"<<buf.size();
            qDebug()<<"首部信息"<<buf;
            restBytes = fileBytes - receive->writeDatagram(buf,QHostAddress::LocalHost,port);
            qDebug()<<"发送请求头之后正文长度"<<restBytes;
            sentBytes=0;
            ui->sendProg->setMaximum(restBytes);

            alldata=filerequest->readAll();
            QDateTime current_date_time =QDateTime::currentDateTime();
            QString current_date =current_date_time.toString("MM.dd hh:mm");
            QString ppp;
            ppp=QString::number(port);
            ui->textEdit_down->append(address.toString()+"   "+ppp+"        "+current_date+"      "+ba.mid(0,ba.size()-1));
            break;
        }

        /*响应ACK*/
        if(biaoshi=="2")
        {
            qDebug()<<"收到ACK,继续发数据了";
            //首先把消息分组


            qDebug()<<"ack"<<ba.mid(4,1).toHex().toInt(0,16);
            qDebug()<<"i"<<i-2;

            if( ba.mid(4,1).toHex().toInt(0,16)==(i-2)%256)
            {
                qDebug()<<"对方已收到"<<i-2<<"号数据包";
                time->stop();
                qDebug()<<restBytes;
                if(restBytes>0)
                    continue_transfer(restBytes);
                else
                {   i=0;
                    sentBytes=restBytes=0;
                }

            }
            else
                qDebug()<<"wrong ack";

        }


        /*响应文件头*/
        if(biaoshi=="0")
        {
            QByteArray fb=ba.mid(0,sizeof(qint64));
            f=fb.toHex().toInt(Q_NULLPTR,16);
            qDebug()<<"filebytes"<<f;
            ui->recvProg->setMaximum(f);


            QByteArray fn=ba.mid(sizeof(qint64),ba.size()-1-sizeof(qint64));
            qDebug()<<fn;
            fileName="E:\\udp\\"+fn;

            file=new QFile(fileName);
            if(file->open(QIODevice::Append))
                qDebug()<<"open sucess";


            QByteArray ACK=" ack";
            ACK.append(i-1);
            ACK+="2";
            receive->writeDatagram(ACK,QHostAddress::LocalHost,port);
            qDebug()<<"接收文件头  ack："<<ACK;

            /*设置日志信息*/
            QDateTime current_date_time =QDateTime::currentDateTime();
            QString current_date =current_date_time.toString("MM.dd hh:mm");
            QString ppp;
            ppp=QString::number(port);
            ui->textEdit_up->append(address.toString()+"   "+ppp+"        "+current_date+"      "+fn);
        }


        /*响应核心数据包*/
        if(biaoshi=="1")
        {
            qDebug()<<"seq:"<<ba.mid(0,1);
            QByteArray ACK=" ack";

            qDebug()<<ba.mid(0,1).toHex().toInt(0,16);
            if(ba.mid(0,1).toHex().toInt(0,16)==(i-1)%256) //是想要的
            {
                qDebug()<<"正确ack";
                ACK.append(i-1);
            }
            else
            { qDebug()<<"错误ack";
                ACK.append(i-2);}

            /*处理天选之子*/
            if(i-1==testa)
            {

                tempa=i-1;
                qDebug()<<"ACK丢失的天选之子"<<i-1;
                gotBytes-=ba.size();
                testa+=10000000;
                i-=1;
            }
            else   if(i-1==testb)
            {

                tempa=i-1;
                qDebug()<<"数据包丢失的天选之子"<<i-1;
                testb+=10000000;
                i-=1;
                gotBytes-=ba.size();
                ba.fill('0');
                qDebug()<<ba;

            }


           /*处理正常核心数据包*/
            else
            {
                ACK+="2";    //ACK的数据标识为2
                qDebug()<<"total"<<f;
                qDebug()<<"alreadly received"<<gotBytes;
                qDebug()<<ACK;
                ui->recvProg->setValue(gotBytes);
                file->write(ba.mid(1,ba.size()-1-1));
                receive->writeDatagram(ACK,QHostAddress::LocalHost,port);
                qDebug()<<"写文件完成";
            }

        }
    }


    /*文件接收完毕*/
    if(gotBytes==f)
    {
        qDebug()<<"over";
        findfile();
        file->close();
        fileBytes = gotBytes= 0;
        i=0;

    }

}


/*发送核心数据包*/
void MainWindow::continue_transfer(qint64 sentSize)
{
    qDebug()<<"第"<<i-1<<"次发数据";

    QByteArray line = (i-1)+alldata.mid(510*(i-2),510)+"1";   //在传输具体数据的数据包的最后添加标识1，代表是数据
    time->start(TIMER_TIMEOUT);
    receive->writeDatagram(line,QHostAddress::LocalHost,port);
    qDebug()<< "第"<< i - 1 <<"个数据包发送完毕"<< line.size();
    sentBytes+=line.size();
    restBytes=sentSize-line.size();
    qDebug()<<"剩余比特数"<<restBytes;
    ui->sendProg->setValue(sentBytes);
    if(line.size() <512){
        QByteArray yes;
        yes.append("over");
        QMessageBox::warning(this,tr("通知"),tr("发送成功！"),QMessageBox::Yes);
        ui->textEdit->clear();
        findfile();


    }
}



/*获取共享文件目录下的所有文件*/
void MainWindow::findfile()
{
    ui->textEdit->clear();
    qDebug()<<"filelook";
    QDir *dir=new QDir("E://udp");
    QStringList filter;
    QList<QFileInfo> *fileInfo=new QList<QFileInfo>(dir->entryInfoList(filter));
    qDebug()<<fileInfo->count();   //文件夹下文件数
    int i = 0;
    for(i = 2;i<fileInfo->count(); i++)
    {
        ui->textEdit->append(fileInfo->at(i).fileName());
    }

}



/*超时重传函数*/
void MainWindow::onTimeout()
{
    qDebug()<<"timeout";
    qDebug()<<"第"<<i<<"个数据包丢失";
    qDebug()<<"还剩"<<restBytes<<"比特";
    qDebug()<<"重传";
    QByteArray sec=i+alldata.mid(510*(i-1),510)+"1";
    receive->writeDatagram(sec,QHostAddress::LocalHost,port);


}


