#ifndef OPENCV_DEAL_H
#define OPENCV_DEAL_H

#include <opencv2/opencv.hpp>  
#include "opencv2/core.hpp"
#include "opencv2/face.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <QImage>
#include <QDebug>
#include <pthread.h>
#include "usermngr.h"

using namespace cv;
using namespace std;
using namespace cv::face;


#define ROW_MIN			60
#define MAT_SCALE		3


#define LBPH_REC_THRES_0		125.0	// LBPH�趨��ֵ/���ƶ�0
#define LBPH_REC_THRES_80		85.0	// LBPHʶ�������ֵ/���ƶ�80
#define LBPH_REC_THRES_100		65.0	// LBPH���ƶ�100��ֵ

class FaceImageDeal
{
public:
	FaceImageDeal(void);
	int faceDeal_init(void);
    int detectAndDraw( Mat& img, CascadeClassifier& cascade, double scale);
//	int face_recognize();

	QImage v4l22QIamge(uchar *data, int len);
	Mat QImage2cvMat(QImage image);
	QImage cvMat2QImage(const Mat& mat);
	void faceDeal_deInit(void);

public:
//	Ptr<LBPHFaceRecognizer> modelLBPH;		// ѵ���������������
	CascadeClassifier 	face_cascade;		//������������
//	Mat 				facesMat;			// �����ȡ������
//	int 				predictLBPH;		// ʶ����
//	string 				fn_csv;				// csv�ļ�
	QImage				QImgV4l2;			// ��V4L2����ת���ɵ�QImage
	Mat					MatV4l2;			// v4l2->QImage->MatV4l2
	Mat					MatOpencv;			// MatV4l2 >> opencv >> MatOpencv
	QImage				QImgOpencv;			// ��opnecv�����ת����QImage
	pthread_mutex_t		imgV4Lock;			// QImgV4l2������
	pthread_mutex_t		imgOpLock;			// QImgOpencv������
	pthread_mutex_t		matV4Lock;			// MatV4l2������
	int 				detectFlag;			// ��⵽������� 1-��
	vector<Rect>		vFacesRect;			// ������������

private:
	
};

class FaceRecogn
{
public:
	FaceRecogn(void);
	int faceRec_init(void);
	int face_recognize(void);
	void faceRec_deInit(void);

public:
	string						fn_csv; 					// csv�ļ�
	Mat 						facesMat;					// �����ȡ������
	int 						predFace;					// ������� 0-���޴��ˣ�>0-��Ӧ���
	char						predName[USER_NAME_LEN];	// user name
	uchar						faceConfd;					// ���ƶ�/���Ŷ�
	int							predChang;					// ���½��

private:
	Ptr<LBPHFaceRecognizer> 	modelLBPH;			// LBPHѵ��������ģ��
	Ptr<BasicFaceRecognizer> 	modelPCA;			// PCAѵ��������ģ��
	Ptr<BasicFaceRecognizer> 	modelFisher;		// Fisherѵ��������ģ��
	int 						predictLBPH;		// LBPHʶ����
	int 						predictPCA;			// PCAʶ����
	int 						predictFisher;		// Fisherʶ����
	
	
};


/* ͼ����/��ʽת���߳� */
void* faceopencv_handle(void *arg);

/* �������ʶ���߳� */
void* faceDetRec_handle(void *arg);

/* opencv_dealģ��ȥ��ʼ�� */
void opencvDeal_deInit(void);

/* ����opencv�����߳� */
int opencvDeal_restart(void);

#endif // OPENCV_DEAL_H
