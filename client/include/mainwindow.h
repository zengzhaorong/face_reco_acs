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
#define REC_OK_SHOWTIME		3	// 识别成功显示时间 S

#define PRED_FACE_LABEL			"Face Label"
#define CONFIDENCE_LABEL		"Confidence"

#define WIN_BACKGRD_IMG				"images/gdut.jpg"		// 界面背景图
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
	QWidget 		*cenWidget;			// 中心窗口
	QLabel 			*imgLabel;			// 图像显示
	QImage 			camImage;			// 摄像头图像
	QLabel			*copyrightLab;		// copyright info label
	QLabel 			*predLabel;			// 识别结果
	QString			predString;			// 识别结果显示字符
	QLabel 			*confdLabel;		// 识别置信度/相似度
	QString			confdString;		// 识别置信度显示字符
	QLabel			*recOKLabel;		// 验证通过图标
	QTimer 			*timer;				// 刷新显示定时器
	QLabel			*QLabMess;			// some message 
	QPushButton 	*button;
};

#endif
