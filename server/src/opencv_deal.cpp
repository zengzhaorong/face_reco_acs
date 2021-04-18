#include "opencv_deal.h"
#include "main.h"
#include "usermngr.h"

/* C++ ���� C */
#ifdef __cplusplus
	extern "C" {
#endif

#include "svrsocket.h"

#ifdef __cplusplus
	}
#endif

FaceImageDeal				gFaceDeal;
FaceRecogn					gFaceRec;

extern struct mainWorkStru 	gstMainWork;
extern struct socketInfo	svrSocketInfo;
extern struct userMngr_Stru	gstUsrMngr;;


/* ͼ����/��ʽת���߳� */
void* faceopencv_handle(void *arg)
{
	printf("%s: enter.\n", __FUNCTION__);
	
	int ret = 0;
	int v4l2Flag = 0;

	// update csv file
	Create_CSV((char *)FACES_LIB_PATH);

	ret = gFaceDeal.faceDeal_init();
	if(ret < 0)
	{
		printf("%s: faceDeal_init() failed.\n", __FUNCTION__);
		return (void *)0;
	}

	ret = pthread_create(&gstMainWork.tid_facDetRec, NULL, faceDetRec_handle, NULL);
	if(ret < 0)
	{
		printf("pthread_create faceDetRec_handle failed.\n");
		return (void *)0;
	}
	
	while(gstMainWork.workStatus)
	{
		if(gstMainWork.opcvDealReboot)		// restart pthread
			break;
		
		if(!svrSocketInfo.connectSta)	// no tcp connect
		{
			usleep(1000);
			continue;
		}
		
		pthread_mutex_lock(&svrSocketInfo.recvLock);
		v4l2Flag = svrSocketInfo.recvBLen;
		pthread_mutex_unlock(&svrSocketInfo.recvLock);
		if(v4l2Flag > 0)
		{
			/* ��V4L2ͼ������ת��ΪQImage */
			pthread_mutex_lock(&svrSocketInfo.recvLock);
			pthread_mutex_lock(&gFaceDeal.imgV4Lock);
			gFaceDeal.QImgV4l2 = gFaceDeal.v4l22QIamge(svrSocketInfo.recvBuf, svrSocketInfo.recvBLen);
			if(gFaceDeal.QImgV4l2.isNull())
			{
				pthread_mutex_unlock(&gFaceDeal.imgV4Lock);
				pthread_mutex_unlock(&svrSocketInfo.recvLock);
				
				printf("loadFromData failed[QImage is null].\n");
				continue;
//				return (void *)0;
			}
			pthread_mutex_unlock(&gFaceDeal.imgV4Lock);
			pthread_mutex_unlock(&svrSocketInfo.recvLock);

			/* ��QImageת��ΪcvMat */
			pthread_mutex_lock(&gFaceDeal.imgV4Lock);
			pthread_mutex_lock(&gFaceDeal.matV4Lock);
			gFaceDeal.MatV4l2 = gFaceDeal.QImage2cvMat(gFaceDeal.QImgV4l2).clone();
			if(gFaceDeal.MatV4l2.empty())
			{
				pthread_mutex_unlock(&gFaceDeal.imgV4Lock);
				pthread_mutex_unlock(&gFaceDeal.matV4Lock);
				
				printf("QImage2cvMat failed[MatV4l2 is empty].\n");
				continue;
//				return (void *)0;
			}
			pthread_mutex_unlock(&gFaceDeal.imgV4Lock);
			pthread_mutex_unlock(&gFaceDeal.matV4Lock);

/*
			pthread_mutex_lock(&gFaceDeal.imgOpLock);
			gFaceDeal.detectAndDraw(gFaceDeal.MatOpencv, gFaceDeal.face_cascade);
			pthread_mutex_unlock(&gFaceDeal.imgOpLock);
*/
/*
			pthread_mutex_lock(&gFaceDeal.imgOpLock);
//			gFaceDeal.QImgOpencv = gFaceDeal.cvMat2QImage(gFaceDeal.detectAndDraw(gFaceDeal.MatV4l2, gFaceDeal.face_cascade));
			gFaceDeal.QImgOpencv = gFaceDeal.cvMat2QImage(gFaceDeal.MatOpencv);
			pthread_mutex_unlock(&gFaceDeal.imgOpLock);
*/			

		}
				
	}
	
	gFaceDeal.faceDeal_deInit();

	gstMainWork.opcvDealReboot ++;		// �߳��˳� �ź� + 1
	printf("%s: exit ---------.\n", __FUNCTION__);
	
	return (void *)0;
}

/* �������ʶ���߳� */
void* faceDetRec_handle(void *arg)
{
	printf("%s: enter.\n", __FUNCTION__);

	int i;
	int ret = 0;
	Mat	detectMat;
	unsigned char tmpBuf[1024];
	int tmpInt = 0;
	int len = 0;
	
	ret = gFaceRec.faceRec_init();
	if(ret < 0)
	{
		printf("%s: faceRec_init() failed.\n", __FUNCTION__);
		return (void *)0;
	}
	
	while(gstMainWork.workStatus)
	{
		if(gstMainWork.opcvDealReboot)		// restart pthread
			break;
		
		if(gFaceRec.predChang)	// ��ʶ��������δ����
		{
			usleep(1000);	// ��Ҫ���޳���
			continue;
		}

		pthread_mutex_lock(&gFaceDeal.matV4Lock);
		detectMat = gFaceDeal.MatV4l2.clone();
		pthread_mutex_unlock(&gFaceDeal.matV4Lock);
		
		if(detectMat.empty())
		{
			usleep(10000);
			continue;
		}
		
		ret = gFaceDeal.detectAndDraw(detectMat, gFaceDeal.face_cascade, MAT_SCALE);
		
		/* ret>0��⵽���� (������detectFlag�жϣ������ѱ�mainwindow��0) */
		if(ret > 0)
		{
			memset(tmpBuf, 0, sizeof(tmpBuf));
			len = 0;
			
//			ret = 1;	// ��ʱ֧��һ��
			tmpBuf[0] = ret;	// ����
			len += 1;
			
			for(i=0; i<ret; i++)
			{
				// ������������
				tmpInt = gFaceDeal.vFacesRect[i].x;
				memcpy(tmpBuf+len, &tmpInt, 4);
				len += 4;
				tmpInt = gFaceDeal.vFacesRect[i].y;
				memcpy(tmpBuf+len, &tmpInt, 4);
				len += 4;
				tmpInt = gFaceDeal.vFacesRect[i].width;
				memcpy(tmpBuf+len, &tmpInt, 4);
				len += 4;
				tmpInt = gFaceDeal.vFacesRect[i].height;
				memcpy(tmpBuf+len, &tmpInt, 4);
				len += 4;
			}

			socket_send(svrSocketInfo.consockfd, CMD_DETECT, tmpBuf, len);

			ret = gFaceRec.face_recognize();
			if(ret == 0)	// ʶ��ɹ�: �ҵ��˻���޴��ˣ������ͽ��
			{
				memset(tmpBuf, 0, sizeof(tmpBuf));
				len = 0;

				tmpBuf[len] = 1;	// ����,��ʱ֧��һ��
				len += 1;
				
				memcpy(tmpBuf+len, &gFaceRec.predFace, 4);		// �������
				len += 4;
				
				memcpy(tmpBuf+len, gFaceRec.predName, USER_NAME_LEN);	// name
				len += USER_NAME_LEN;

				tmpBuf[len] = gFaceRec.faceConfd;	// ���ƶ�/���Ŷ�
				len += 1;

				socket_send(svrSocketInfo.consockfd, CMD_RECOGN, tmpBuf, len);
			}

			gFaceDeal.vFacesRect.clear();
		}
		else	// ������
		{
			gFaceRec.faceConfd = 0;
		}

	}

	gFaceRec.faceRec_deInit();

	gstMainWork.opcvDealReboot ++;		// �߳��˳� �ź� + 1
	printf("%s: exit ---------.\n", __FUNCTION__);
	
	return (void *)0;
}

/* ����opencv�����߳�: �������������� */
int opencvDeal_restart(void)
{
	printf("\n------------------ %s: ------------------\n", __FUNCTION__);
	int ret = 0;

	gstMainWork.opcvDealReboot = 1;
	
	while(gstMainWork.opcvDealReboot <= 2)		// wait 2 opencv deal pthread exit
	{
		usleep(1000);
	}

	get_UserList(&gstUsrMngr.pstUserInfo, &gstUsrMngr.userSum);

	gstMainWork.opcvDealReboot = 0;
	
	ret = pthread_create(&gstMainWork.tid_facOpen, NULL, faceopencv_handle, NULL);
	if(ret < 0)
	{
		printf("faceopencv_handle() failed.\n");
		return -1;
	}

	return 0;
}

FaceImageDeal::FaceImageDeal(void)
{
	
	printf("%s: enter.\n", __FUNCTION__);
}

int FaceImageDeal::faceDeal_init(void)
{
	int ret = 0;

	/* ��ʼ���� */
	pthread_mutex_init(&this->imgV4Lock, NULL);
	pthread_mutex_init(&this->matV4Lock, NULL);
	pthread_mutex_init(&this->imgOpLock, NULL);

	this->detectFlag = 0;
	
	ret = this->face_cascade.load("haarcascade_frontalface_alt.xml");
	if( !ret )
	{
		printf("%s: load xml failed[ret=%d].\n", __FUNCTION__, ret);
		return -1;
	}

	return 0;
}

int FaceImageDeal::detectAndDraw( Mat& img, CascadeClassifier& cascade, double scale)
{
    vector<Rect> faces;
    Mat GrayImg_0;
    Mat GrayImg;
    double t = 0;
    double fx = 1 / scale;
	
	cvtColor( img, GrayImg_0, COLOR_BGR2GRAY ); // ��Դͼ��תΪ�Ҷ�ͼ

	resize( GrayImg_0, GrayImg, Size(), fx, fx, INTER_LINEAR);	//����ͼ��
	
	equalizeHist( GrayImg, GrayImg );	// ֱ��ͼ���⻯�����ͼ������

    t = (double)getTickCount();

	cascade.detectMultiScale( GrayImg, faces,
								1.1, 2, 0
								//|CASCADE_FIND_BIGGEST_OBJECT
								//|CASCADE_DO_ROUGH_SEARCH
								|CASCADE_SCALE_IMAGE,
								Size(30, 30) );
	// ��ȡ�������β���
	Rect	tmpRect;
	for(int i =0; i<(int)faces.size(); i++)
	{
		gFaceRec.facesMat = GrayImg(faces[i]);	// ��������Rect >> Mat
//		rectangle(img, Point(faces[i].x, faces[i].y), Point(faces[i].x + faces[i].width, 
//			faces[i].y + faces[i].height), Scalar(0, 255, 0), 1, 8);	 // �������  
/*
		gFaceDeal.vFacesRect.x = faces[i].x * scale;
		gFaceDeal.vFacesRect.y = faces[i].y * scale;
		gFaceDeal.vFacesRect.width = faces[i].width * scale;
		gFaceDeal.vFacesRect.height = faces[i].height * scale;
*/
		tmpRect.x = faces[i].x * scale;
		tmpRect.y = faces[i].y * scale;
		tmpRect.width = faces[i].width * scale;
		tmpRect.height = faces[i].height * scale;
		gFaceDeal.vFacesRect.push_back(tmpRect);
		
//		printf("face[%d]: %d, %d, %d, %d\n", i, gFaceDeal.vFacesRect[i].x, gFaceDeal.vFacesRect[i].y,
//					gFaceDeal.vFacesRect[i].width, gFaceDeal.vFacesRect[i].height);
		gFaceDeal.detectFlag = 1;
	}
    t = (double)getTickCount() - t;
//    printf( "%s: detection time = %g ms, face size: %d\n", __FUNCTION__, t*1000/getTickFrequency(), (int)faces.size());

	return (int)faces.size();
}


QImage FaceImageDeal::v4l22QIamge(uchar *data, int len)
{
	QImage 	qimage;

	if(NULL==data || len <= 0)
	{
		printf("%s: param is illegal.\n", __FUNCTION__);
		return qimage;
	}

	qimage.loadFromData(data, len);
	if(qimage.isNull())
	{
		printf("loadFromData failed[camImage is null].\n");
		return qimage;
	}

	return qimage;
}

/* ����Ҫ�� .clone()��������� */
Mat FaceImageDeal::QImage2cvMat(QImage image)
{
    cv::Mat mat;
	
    //qDebug() << image.format();
    switch(image.format())
    {
	    case QImage::Format_ARGB32:
	    case QImage::Format_RGB32:
	    case QImage::Format_ARGB32_Premultiplied:
	        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.bits(), image.bytesPerLine());
	        break;
	    case QImage::Format_RGB888:
	        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.bits(), image.bytesPerLine());
	        cv::cvtColor(mat, mat, CV_BGR2RGB);
	        break;
	    case QImage::Format_Indexed8:
	        mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.bits(), image.bytesPerLine());
	        break;

		default:
	        printf("%s: ================ default ================\n", __FUNCTION__);		
    }

	if(mat.empty())
	{
        printf("%s: ================ mat is empty ================ \n", __FUNCTION__);				
	}

    return mat;
}

QImage FaceImageDeal::cvMat2QImage(const Mat& mat)
{

	if(mat.empty())
		printf("Mat is empty.\n");

    // 8-bits unsigned, NO. OF CHANNELS = 1
    if(mat.type() == CV_8UC1)
    {
//        qDebug() << "CV_8UC1";
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        //printf("set colors\n");
        image.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }
    // 8-bits unsigned, NO. OF CHANNELS = 3
    else if(mat.type() == CV_8UC3)
    {
//        qDebug() << "CV_8UC3";
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
//        qDebug() << "CV_8UC4";
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
//        qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }

}

void FaceImageDeal::faceDeal_deInit(void)
{
	
	pthread_mutex_destroy(&this->imgV4Lock);
	pthread_mutex_destroy(&this->matV4Lock);
	pthread_mutex_destroy(&this->imgOpLock);
	
}



/* =========================== class FaceRecogn ============================ */

static void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';') {
    std::ifstream file(filename.c_str(), ifstream::in);
    if (!file) {
        string error_message = "No valid input file was given, please check the given filename.";
        CV_Error(Error::StsBadArg, error_message);
    }
    string line, path, classlabel;
    while (getline(file, line)) {
        stringstream liness(line);
        getline(liness, path, separator);
        getline(liness, classlabel);
        if(!path.empty() && !classlabel.empty()) {
            images.push_back(imread(path, 0));
            labels.push_back(atoi(classlabel.c_str()));
        }
    }
}


FaceRecogn::FaceRecogn()
{
	printf("%s: enter.\n", __FUNCTION__);
}

int FaceRecogn::faceRec_init(void)
{

	this->predictLBPH = 0;
	this->predictFisher = 0;
	this->predFace = 0;
	this->faceConfd = 0;
	this->predChang = 0;
	memset(predName, 0, sizeof(predName));
	
    // These vectors hold the images and corresponding labels.
    vector<Mat> images;
    vector<int> labels;
    // Read in the data. This can fail if no valid
    // input filename is given.
    try {
        read_csv(fn_csv, images, labels);
    } catch (cv::Exception& e) {
        cerr << "Error opening file \"" << fn_csv << "\". Reason: " << e.msg << endl;
        // nothing more we can do
        exit(1);
    }
    // Quit if there are not enough images for this demo.
    if(images.size() <= 1) {
        string error_message = "This demo needs at least 2 images to work. Please add more images to your data set!";
        CV_Error(Error::StsError, error_message);
    }

	// ѵ��
	/* LBPH */
	this->modelLBPH = createLBPHFaceRecognizer();  
	this->modelLBPH->train(images, labels);
	
	modelLBPH->setThreshold(LBPH_REC_THRES_0);	// LBPH������ֵ
	
    this->modelFisher = createFisherFaceRecognizer();
    this->modelFisher->train(images, labels);

	/* ������PCA */
#if 0
	// Let's say we want to keep 10 Eigenfaces and have a threshold value of 10.0
	int num_components = 10;
	double threshold = 10.0;
	// Then if you want to have a cv::FaceRecognizer with a confidence threshold,
	// create the concrete implementation with the appropiate parameters:
	this->modelPCA = createEigenFaceRecognizer(num_components, threshold);
	this->modelPCA->train(images, labels);
#else
	  this->modelPCA = createEigenFaceRecognizer();
	  this->modelPCA->train(images, labels);
#endif
	
	printf("%s: --------- Face model train succeed ---------\n", __FUNCTION__);

	return 0;
}

/* ����ʶ�𣬷��ؽ��: 0-����  -1-���� */
int FaceRecogn::face_recognize()
{
    double tim = 0;
    double btim = 0;
	double confidence = 0.0;
	int i;

	predictLBPH = 0;
	predictPCA = 0;
	predictFisher = 0;
	predFace = 0;
	
	Mat face_resize;  
	if (facesMat.rows >= ROW_MIN)  // ������������̫С(Զ)
	{  
		resize(facesMat, face_resize, Size(92, 112));
	}
	else
	{
		return -1;
	}

	if (!face_resize.empty())
	{
		//����ͼ��Ӧ���ǻҶ�ͼ	LBPH
//		predictLBPH = modelLBPH->predict(face_resize);	
		modelLBPH->predict(face_resize, predictLBPH, confidence);	
		if(predictLBPH > 0)
		{
			// �������ƶ�
			if(confidence < LBPH_REC_THRES_100)
			{
				this->faceConfd = 100;
			}
			else if(confidence < LBPH_REC_THRES_80)
			{
				this->faceConfd = 80 + (LBPH_REC_THRES_80 - confidence) * 20 /
									(LBPH_REC_THRES_80 - LBPH_REC_THRES_100);
			}
			else if(confidence < LBPH_REC_THRES_0)
			{
				this->faceConfd = (LBPH_REC_THRES_0 - confidence) * 80 /
									(LBPH_REC_THRES_0 - LBPH_REC_THRES_80);
			}
			else
			{
				this->faceConfd = 0;
			}
			
//			printf("confidence: ========== %f \t predictLBPH: =========== %d\n", confidence, predictLBPH);
		}
		else
		{
			this->faceConfd = 0;
		}
		
//		predictFisher = modelFisher->predict(face_resize);	

#if	0
		int predictedLabel = -1;
		double confidence = 0.0;
		modelPCA->predict(face_resize, predictedLabel, confidence);
		printf(" ========= PCA_predict   : %d --- %f ========= \n", predictedLabel, confidence);
#else
//		predictPCA = modelPCA->predict(face_resize);  
#endif
		
		tim = (double)getTickCount() - btim;
//		printf( "predict time = %g ms\n", tim*1000/getTickFrequency());


		/* ����ʶ����:���ƶ�80���� */
		if(predictLBPH > 0 && confidence < LBPH_REC_THRES_80)
		{
			predFace = predictLBPH;		// label

			// user name
			for(i=0; i<gstUsrMngr.userSum; i++)
			{
				if((gstUsrMngr.pstUserInfo+i)->seq == predFace)
				{
					strncpy(predName, (gstUsrMngr.pstUserInfo+i)->name, USER_NAME_LEN);
				}
			}
				
			predChang = 1; 			// ʶ��������
		}
		else
		{
			predFace = 0;
			memset(predName, 0, sizeof(predName));
		}
	}  

	return 0;
}

void FaceRecogn::faceRec_deInit(void)
{
	
}

void opencvDeal_deInit(void)
{

	gFaceDeal.faceDeal_deInit();
	gFaceRec.faceRec_deInit();

}

