#include "mainwindow.h"
#include "opencv_deal.h"
#include "main.h"

/* C++ ���� C */
#ifdef __cplusplus
	extern "C" {
#endif

#include "v4l2_cap.h"
#include "clisocket.h"

#ifdef __cplusplus
	}
#endif

extern struct mainWorkStru 		gstMainWork;

extern struct v4l2capStru		gv4l2capInfo;
extern OpencvDeal				gOpcvDeal;
extern struct socketInfo		cliSocketInfo;


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	QImage	tmpQImg;
	QPixmap tmpQPix;

	setWindowTitle("mainwindow");

	resize(800, 480);

	cenWidget = new QWidget;	// ���Ĵ���
	setCentralWidget(cenWidget);

	imgLabel = new QLabel(cenWidget);
	imgLabel->setPixmap(QPixmap(WIN_BACKGRD_IMG));
	imgLabel->setGeometry(0,0,640,480);	// ��������
	imgLabel->show();

	// ��֤ͨ��ͼ��
	recOKLabel = new QLabel(cenWidget);
	recOKLabel->setGeometry(220,140,200,200); // ��������
	tmpQImg.load(REC_OK_IMG);
	tmpQPix = QPixmap::fromImage(tmpQImg);
	QPixmap fitpixmap = tmpQPix.scaled(recOKLabel->width(), recOKLabel->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);  // �������
	recOKLabel->setPixmap(fitpixmap);
	recOKLabel->hide();

	tmpQImg.load(COPYRIGHT_IMG);
	copyrightLab = new QLabel(cenWidget);
	copyrightLab->setGeometry(640,0,160,120);
	copyrightLab->setPixmap(QPixmap::fromImage(tmpQImg));
	copyrightLab->show();

	predString = QString("%1:  %2").arg(PRED_FACE_LABEL).arg(cliSocketInfo.predFace);
	predLabel = new QLabel(cenWidget);
	predLabel->setGeometry(640,150,160,50);	// ��������
	predLabel->setText(predString);

	confdString = QString("%1: %2\%").arg(CONFIDENCE_LABEL).arg(0);
	confdLabel = new QLabel(cenWidget);
	confdLabel->setGeometry(640,200,160,50);	// ��������
	confdLabel->setText(confdString);

	QLabMess = new QLabel(cenWidget);
	QLabMess->setGeometry(640,250,250,50);
	QLabMess->setText("User: ");
	
/*
	button = new QPushButton(cenWidget);
	button->setText(tr("Hellobutton"));
	button->setGeometry(640, 240, 100, 50);	// ��������
*/
	/* ��ʱ������ʾͼ�� */
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(showImage()));
	timer->start(QTIMER_INTERV);

}

MainWindow::~MainWindow()
{

}

void MainWindow::showImage()
{
	int 	i = 0;
	Mat 	detectMat;
	QImage	detectImg;
	static time_t 	beTim = 0;
	time_t 	curTim = 0;
	static int 	recOKshow = 0;

	curTim = time(NULL);
	timer->stop();
	
	if(gv4l2capInfo.frameLen > 0)
	{
		/* ��V4L2ͼ������ת��ΪQImage */
		pthread_mutex_lock(&gv4l2capInfo.frameLock);
		camImage.loadFromData(gv4l2capInfo.frameBuf, gv4l2capInfo.frameLen);
		gv4l2capInfo.frameLen = 0;
		pthread_mutex_unlock(&gv4l2capInfo.frameLock);
		if(camImage.isNull())
		{
			printf("loadFromData failed[camImage is null].\n");
			return ;
		}

		if(cliSocketInfo.detectFlag == 1)	// ��⵽������������ʾ
		{
			/* ��QImageת��ΪcvMat */
			gOpcvDeal.MatV4l2 = gOpcvDeal.QImage2cvMat(camImage).clone();
			
			/* ��ȡ��ǰͼ����Ͼ��ο򣬴�������: ��ʱ */
			detectMat = gOpcvDeal.MatV4l2.clone();

			/* �����ο������  */
			for(i=0; i<cliSocketInfo.faceCount; i++)
			{
				rectangle(detectMat, Point(cliSocketInfo.faceRect[i].x, cliSocketInfo.faceRect[i].y), 
				Point(cliSocketInfo.faceRect[i].x + cliSocketInfo.faceRect[i].w, 
					cliSocketInfo.faceRect[i].y + cliSocketInfo.faceRect[i].h), 
					Scalar(0, 0, 255), 3, 8);	 // �������
			}
			
			cliSocketInfo.detectFlag = 0;
			cliSocketInfo.faceCount = 0;
			memset(&cliSocketInfo.faceRect, 0, sizeof(struct rectStru));

			/* cmMat >> QImage */
			detectImg = gOpcvDeal.cvMat2QImage(detectMat);

			/* qt image show */
			imgLabel->setPixmap(QPixmap::fromImage(detectImg));
	
		}
		else	// ������
		{
			if(!cliSocketInfo.predChang)
			{
				cliSocketInfo.faceConfd = 0;
			}
			if(cliSocketInfo.connectSta)
			{
				imgLabel->setPixmap(QPixmap::fromImage(camImage));
			}
		}

		/* ˢ����ʾʶ���� */
		if(cliSocketInfo.predChang && !recOKshow)
		{
			QString prdNameStr =  QString("%1").arg(cliSocketInfo.predName);
			QLabMess->setText(prdNameStr);
			beTim = time(NULL);
			predString = QString("%1:  %2").arg(PRED_FACE_LABEL).arg(cliSocketInfo.predFace);
			predLabel->setText(predString);
			
			recOKLabel->show();
			recOKshow = 1;
			
			ioctl(gstMainWork.door_fd, DEV_ON, DOOR_DEV);		// open door 
		}
		else if(curTim - beTim >= REC_OK_SHOWTIME)		// ��ʾ3S,��ʱ��0
		{
			QLabMess->setText("User: ");
			beTim = curTim + 10000;		// ��ֹ��������
			cliSocketInfo.predChang = 0;
			cliSocketInfo.predFace = 0;
			recOKLabel->hide();
			recOKshow = 0;
			
			ioctl(gstMainWork.door_fd, DEV_OFF, DOOR_DEV);		// close door 
			
			predString = QString("%1:  %2").arg(PRED_FACE_LABEL).arg(cliSocketInfo.predFace);
			predLabel->setText(predString);
		}
		
		// ��ʾ���ƶ�
		confdString = QString("%1: %2\%").arg(CONFIDENCE_LABEL).arg(cliSocketInfo.faceConfd);
		confdLabel->setText(confdString);

	}

	timer->start(QTIMER_INTERV);

}

