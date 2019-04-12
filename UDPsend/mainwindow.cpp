#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QByteArray>
#include <QDataStream>
#include <QFileDialog>
#include <QHostAddress>
#include <QIODevice>
#include <QString>
#include <QUdpSocket>
#include<QTimer>
#include<QDir>
#include<QDateTime>

#define TIMER_TIMEOUT   (5*1000)
const quint16 PORT = 3333;

quint16 localport;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    time = new QTimer(this);
    send = new QUdpSocket(this);
    fileBytes = sentBytes = restBytes =gotBytes= 0;
    file = Q_NULLPTR;

    connect(send, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(time, SIGNAL(timeout()), this, SLOT(onTimeout()));

    ui->sendProg->setValue(0); // 进度条置零
    ui->recvProg->setValue(0);
    ui->sendBtn->setEnabled(false); // 禁用发送按钮

    //展示文件列表
    findfile();




}

MainWindow::~MainWindow()
{
    delete ui;
    delete send;
}

/*打开文件*/
void MainWindow::on_selectBtn_clicked()
{
    fileName = QFileDialog::getOpenFileName(this);
    if(!fileName.isEmpty())
    {
        ui->stLabel->setText(fileName);
        ui->sendBtn->setEnabled(true);
    }

}

/*封装文件头*/
void MainWindow::on_sendBtn_clicked()
{
    file = new QFile(fileName);
    if(!file->open(QFile::ReadOnly))
    {
        ui->stLabel->setText(QString("文件打开错误"));
        return;
    }
    fileBytes = file->size();
    restBytes=fileBytes;
    qDebug()<<"文件比特数"<<fileBytes;
    ui->sendProg->setValue(0);
    QString sfName = fileName.right(fileName.size() -
               fileName.lastIndexOf('/') - 1);
       qDebug()<<"文件名"<<sfName;

    QByteArray buf;
    QDataStream out(&buf, QIODevice::WriteOnly);

    /* 首部 = 总大小 + 文件名 */
    out << qint64(0);//<< sfName;
    /* 总大小加上首部的大小 */
    buf+=sfName;
    buf+="0";   //数据包类型加在每个数据包最后，0表示是文件头
   // qDebug()<<(fileBytes/512+1);
    fileBytes =fileBytes+ buf.size()+((fileBytes/510)+1)*2;
    qDebug()<<"加了头之后的报文比特数"<<fileBytes;
    /* 重写首部的前两个长度字段 */
    out.device()->seek(0);
    out << fileBytes;//<<sfName;
    /* 发送首部，计算剩余大小 */
    qDebug()<<"头长度"<<buf.size();
    qDebug()<<"首部信息"<<buf;
    restBytes = fileBytes - send->writeDatagram(buf,QHostAddress::LocalHost,PORT);
    qDebug()<<"发送请求头之后正文长度"<<restBytes;
    ui->sendProg->setMaximum(restBytes);
    alldata=file->readAll();
  //  continue_transfer(restBytes);
    ui->sendBtn->setEnabled(false);
 }



/*发送核心数据包*/
void MainWindow::continue_transfer(qint64 sentSize)
{
    qDebug()<<"第"<<i<<"次发数据";

        QByteArray line = i+alldata.mid(510*(i-1),510)+"1";   //在传输具体数据的数据包的最后添加标识1，代表是数据
        time->start(TIMER_TIMEOUT);
        send->writeDatagram(line,QHostAddress::LocalHost,PORT);
        qDebug()<< "第"<< i <<"个数据包发送完毕"<< line.size();
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

/*处理socket读到的数据*/
void MainWindow::readData()
{

     QByteArray ba;
     while(send->hasPendingDatagrams())
     {
         i++;
         qDebug()<<"第"<<i-1<<"次读数据";
         ba.resize(send->pendingDatagramSize());
         send->readDatagram(ba.data(),ba.size());
         qDebug()<<ba;
         qDebug()<<ba.mid(ba.size()-1,1);


         /*根据数据包标识做不同的处理*/
         /*发过来的如果是ack*/
         if(ba.mid(ba.size()-1,1)=="2")
         {
             qDebug()<<"ack"<<ba.mid(4,1).toHex().toInt(0,16);
             qDebug()<<"i"<<i-1;

             if( ba.mid(4,1).toHex().toInt(0,16)==(i-1)%256)
             {
                 qDebug()<<"对方已收到"<<i-1<<"号数据包";
                 time->stop();
                 qDebug()<<restBytes;
                if(restBytes>0)
                {
                 continue_transfer(restBytes);
                }
                else
                 {   i=0;
                    sentBytes=restBytes=0;
                }

             }
             else
                 qDebug()<<"wrong ack";
         }



         /*发过来的如果是文件头*/
         if(ba.mid(ba.size()-1,1)=="0")
         {
             gotBytes+=ba.size();
             QByteArray fb=ba.mid(0,sizeof(qint64));

             f=fb.toHex().toInt(Q_NULLPTR,16);
             qDebug()<<"filebytes"<<f;
             ui->recvProg->setMaximum(f);


             QByteArray fn=ba.mid(sizeof(qint64),ba.size()-1-sizeof(qint64));
             qDebug()<<fn;
             fileName="E:\\udp\\download-"+fn;


             qDebug()<<"total"<<f;
             qDebug()<<"alreadly received"<<gotBytes;

             filerespond=new QFile(fileName);
             if(filerespond->open(QIODevice::Append))
                 qDebug()<<"open sucess";

             QByteArray ACK=" ack";
             ACK+=i-1;
             ACK+="2";
             send->writeDatagram(ACK,QHostAddress::LocalHost,PORT);   //回传一个ack0


         }



         /*发过来的如果是核心数据*/
         if(ba.mid(ba.size()-1,1)=="1")
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


             ACK+="2";    //ACK的数据标识为2
             gotBytes+=ba.size();
             qDebug()<<"alreadly received"<<gotBytes;
             qDebug()<<ACK;
             ui->recvProg->setValue(gotBytes);
             filerespond->write(ba.mid(1,ba.size()-1-1));

             send->writeDatagram(ACK,QHostAddress::LocalHost,PORT);
             qDebug()<<"写文件完成";


             /*文件接收完毕*/
             if(gotBytes==f)
             {
                 qDebug()<<"over";
                 i=0;
                 gotBytes=restBytes=f=0;

                 filerespond->close();
                 findfile();

             }

              }

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
    send->writeDatagram(sec,QHostAddress::LocalHost,PORT);


}

/*获取共享目录下的文件信息*/
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


/*向服务器端获取文件的请求*/
void MainWindow::on_pushButton_clicked()
{
    QByteArray fname=ui->lineEdit->text().toLatin1()+"3";    //数据包标识为三代表请求服务器端发送该文件
    send->writeDatagram(fname,QHostAddress::LocalHost,PORT);
    qDebug()<<fname;


}

/*设置本地占用端口号（不设置随机分配也行）*/
void MainWindow::on_pushButton_2_clicked()
{
    localport=ui->lineEdit_2->text().toInt();
    qDebug()<<localport;
    send->bind(QHostAddress::LocalHost, localport);
}
