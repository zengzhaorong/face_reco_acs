#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QLabel>
#include <QWidget>
#include <QPushButton>
#include <QTimer>
#include <QTime>
#include <QString>
#include <time.h>


#define QTIMER_INTERV		1		// ms
#define REC_OK_SHOWTIME		3	// ʶ��ɹ���ʾʱ�� S

#define PRED_FACE_LABEL			"Face Label"
#define CONFIDENCE_LABEL		"Confidence"

#define WIN_BACKGRD_IMG				"images/gdut.jpg"		// ���汳��ͼ
#define REC_OK_IMG					"images/rec_ok.jpg"
#define COPYRIGHT_IMG				"images/Copyright-1.png"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void showImage();

private:
	QWidget 		*cenWidget;			// ���Ĵ���
	QLabel 			*imgLabel;			// ͼ����ʾ
	QImage 			camImage;			// ����ͷͼ��
	QLabel			*copyrightLab;		// copyright info label
	QLabel 			*predLabel;			// ʶ����
	QString			predString;			// ʶ������ʾ�ַ�
	QLabel 			*confdLabel;		// ʶ�����Ŷ�/���ƶ�
	QString			confdString;		// ʶ�����Ŷ���ʾ�ַ�
	QLabel			*recOKLabel;		// ��֤ͨ��ͼ��
	QTimer 			*timer;				// ˢ����ʾ��ʱ��
	QLabel			*QLabMess;			// some message 
	QPushButton 	*button;
};

#endif
