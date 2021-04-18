#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QWidget>
#include <QPushButton>
#include <QTimer>
#include <QString>
#include <time.h>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>

#include "opencv_deal.h"
#include "usermngr.h"


#define QTIMER_INTERV		1		// ms
#define REC_OK_SHOWTIME		3		// ʶ��ɹ���ʾʱ�� S

#define PRED_FACE_LABEL			"Face Label"
#define CONFIDENCE_LABEL		"Confidence"
#define	WORKS_MESSAGE			"Works : Video Access Control System on Embedded Linux"
#define	AUTHOR_MESSAGE			"Author: ZengZhaorong (GDUT)"

#define WIN_BACKGRD_IMG				"images/gdut.jpg"		// ���汳��ͼ
#define WIN_FACE_IMG				"images/face.png"
#define REC_OK_IMG					"images/rec_ok.jpg"
#define ADD_FACE_OK					"images/addFace_ok.jpg"
#define COPYRIGHT_IMG				"images/copyright-0.png"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void showImage();
	void addFace();
	void delFace();

private:
	QWidget 		*cenWidget;			// ���Ĵ���
	QLabel 			*imgLabel;			// ͼ����ʾ
	QLabel			*faceLabel;			// ������ʾ��
	QImage			QImgWinLa;			// ��ʼ����ͼ
	QImage 			QImgFaceLa;			// ��ʼ������ͼ
	QLabel			*copyrightLab;		// copyright info label
	QLabel 			*predLabel;			// ʶ����
	QString			predString;			// ʶ������ʾ�ַ�
	QLabel 			*confdLabel;		// ʶ�����Ŷ�/���ƶ�
	QString			confdString;		// ʶ�����Ŷ���ʾ�ַ�
	QLabel			*PopUpLabel;		// ������ʾ����
	QPixmap			QPixRecOK;			// ��֤ͨ��ͼ��
	QPixmap			QPixAddFaOK;		// ¼��ɹ�ͼ��
	QTimer 			*timer;				// ˢ����ʾ��ʱ��
	QLabel			*QLabMess;			// some message 
	QLineEdit		*LEditUser;			// edit add user name
	QPushButton 	*QButAddUser;		// add face key
	QComboBox		*QComDelUser;		// delete user list box
	QPushButton 	*QButDelUser;		// delete user key
};

#endif
