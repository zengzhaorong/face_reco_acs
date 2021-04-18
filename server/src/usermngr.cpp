#include "usermngr.h"
#include "opencv_deal.h"
#include "mainwindow.h"


struct userMngr_Stru		gstUsrMngr;

extern FaceImageDeal		gFaceDeal;


void userMngr_Init(struct userMngr_Stru *userMnStru)
{

	// gstUsrMngr 
	userMnStru->photoFlag = 0;
	userMnStru->addFaceOK = 0;

	userMnStru->userChange = 0;
	userMnStru->userSum = 0;
	userMnStru->pstUserInfo = NULL;
	
	pthread_mutex_init(&userMnStru->MatCapLock, NULL);
	pthread_mutex_init(&userMnStru->QImgFaceLock, NULL);

	get_UserList(&userMnStru->pstUserInfo, &userMnStru->userSum);
	
}

// ��ȡ�û������б�, ppUserList���ָ��, Count����
int get_UserList(struct userInfo_Stru **ppUserList, int *Count)
{
	struct stat statbuf;
	DIR *dir;
	struct dirent *dirp;
	char 	tmpDirPath[128] = {0};
	char	label[10] = {0};
	struct userInfo_Stru 	tmpUserInfo;
	struct userInfo_Stru 	*pUserMem;
	int 	dirCnt = 0;
	int		dirNum = 0;
	int 	i;

	// ��ȡ�ļ�����
	if(lstat(FACES_LIB_PATH, &statbuf) < 0)
	{
		printf("%s: lstat(%s) failed !\n", __FUNCTION__, FACES_LIB_PATH);
		return -1;
	}

	// �ж��Ƿ�ΪĿ¼
	if(S_ISDIR(statbuf.st_mode) != 1)
	{
		printf("%s: %s is not dir !\n", __FUNCTION__, FACES_LIB_PATH); 
		return -1;
	}

	// ��Ŀ¼
	dir = opendir(FACES_LIB_PATH);
	if( dir ==NULL)
	{
		printf("opendir failed.\n");
		return -1;
	}
	// ����һ��Ŀ¼��ͳ������
	while((dirp = readdir(dir)) != NULL)
	{
		// ���� '.' '..'�ļ���linux��
		if( strncmp(dirp->d_name, ".", strlen(dirp->d_name))==0 || 
			 strncmp(dirp->d_name, "..", strlen(dirp->d_name))==0 )
			continue;
		
		dirCnt ++;
	}
	closedir(dir);

	*Count = dirCnt;	// total dir count
	
	pUserMem = (struct userInfo_Stru *)malloc((*Count) *sizeof(struct userInfo_Stru));
	if(pUserMem == NULL)
	{
		printf("%s: malloc for pUserMem failed.\n", __FUNCTION__);
		return -1;
	}

	// ��Ŀ¼
	dir = opendir(FACES_LIB_PATH);
	if( dir ==NULL)
	{
		free(pUserMem);
		printf("opendir failed.\n");
		return -1;
	}
	// ����һ��Ŀ¼����ȡ��Ϣ
	while((dirp = readdir(dir)) != NULL)
	{
		// ���� '.' '..'�ļ���linux��
		if( strncmp(dirp->d_name, ".", strlen(dirp->d_name))==0 || 
			 strncmp(dirp->d_name, "..", strlen(dirp->d_name))==0 )
			continue;

		// ��1��Ŀ¼��2��Ŀ¼���
		memset(tmpDirPath, 0, sizeof(tmpDirPath));
		strcat(tmpDirPath, FACES_LIB_PATH);
		strcat(tmpDirPath, "/");
		strcat(tmpDirPath, dirp->d_name);

		if(lstat(tmpDirPath, &statbuf) < 0)
		{
			printf("%s: lstat(%s) failed !\n", __FUNCTION__, tmpDirPath);
			continue;
		}

		if(S_ISDIR(statbuf.st_mode) != 1)
		{
			printf("%s: %s is not dir !\n", __FUNCTION__, tmpDirPath); 
			continue;
		}

		memset(label, 0, sizeof(label));
		for(i=0; i<10; i++) 	// ȡ���
		{
			if(dirp->d_name[i] != '_')
				label[i] = dirp->d_name[i];
			else
				break;
		}
		if(i == 10)
			continue; 

		// get seq
		memset(&tmpUserInfo, 0, sizeof(tmpUserInfo));
		tmpUserInfo.seq = atoi(label);
		strncpy(tmpUserInfo.name, dirp->d_name + i+1, strlen(dirp->d_name)-(i+1));

		memcpy(pUserMem+dirNum, &tmpUserInfo, sizeof(struct userInfo_Stru));
		dirNum ++;
//		printf("seq: %d\t name: %s\n", (pUserMem+dirNum)->seq , (pUserMem+dirNum)->name);
	}

	closedir(dir);

	if(*ppUserList != NULL)		// free last time source
	{
		free(*ppUserList);
		*ppUserList = NULL;
	}
	*ppUserList = pUserMem;

	gstUsrMngr.userChange = 1;
	
	return 0;
}

int del_UserFace(char *facePath)
{
	struct stat statbuf;
	int ret;

	if(facePath == NULL)
		return -1;

	// ��ȡ�ļ�����
	if(lstat(facePath, &statbuf) < 0)
	{
		printf("lstat(%s) failed !\n", facePath);
		return -1;
	}

	// �ж��Ƿ�ΪĿ¼
	if(S_ISDIR(statbuf.st_mode) != 1)
	{
		printf("%s is not dir !\n", facePath); 
		return -1;
	}

	ret = remove_dir(facePath);
	if(ret != 0)
	{
		printf("%s: remove_dir %s failed.\n", __FUNCTION__, facePath);
		return -1;
	}
	printf("remove_dir %s success.\n", facePath);

	return 0;
}

// ����: char *����
void* addFace_pthread(void *arg)
{
	struct stat statbuf;
	char 	chName[USER_NAME_LEN] = {0};
	char 	getDirPath[128] = {0};
	int		ret = 0;

	if(arg == NULL)
	{
		printf("%s: arg is null !\n", __FUNCTION__);
		return (void*)0;
	}

	pthread_mutex_lock(&gstUsrMngr.QImgFaceLock);
	gstUsrMngr.QImgFace.load(WIN_FACE_IMG);
	pthread_mutex_unlock(&gstUsrMngr.QImgFaceLock);

	pthread_mutex_lock(&gstUsrMngr.MatCapLock);
	gstUsrMngr.MatCapture = imread(WIN_BACKGRD_IMG);
	pthread_mutex_unlock(&gstUsrMngr.MatCapLock);
	
	gstUsrMngr.getFaceSta = 1;		// last init

	strncpy(chName, (const char*)arg, strlen((const char*)arg));
	printf("add user name: %s\n", chName);

	ret = get_dir((char *)FACES_LIB_PATH, chName, getDirPath);
	if(ret != 0)
	{
		printf("get_dir failed!\n");
		goto addFace_exit;
//		return (void*)0;
	}
	
	/* ���Ŀ¼�Ƿ񴴽��ɹ� */
	if(lstat(getDirPath, &statbuf) < 0)	// ��ȡ�ļ�����
	{
		printf("lstat(%s) failed !\n", getDirPath);
		goto addFace_exit;
	}
	if(S_ISDIR(statbuf.st_mode) != 1)	// �ж��Ƿ�ΪĿ¼
	{
		printf("%s is not dir !\n", getDirPath); 
		goto addFace_exit;
	}
	
	ret = get_face(getDirPath);
	if(ret < 0)
	{
		printf("get_face failed!\n");
		ret = remove(getDirPath);
		if(ret != 0)
			printf("remove %s failed!\n", getDirPath);
		
		goto addFace_exit;
	}

	gstUsrMngr.addFaceOK = 1;		// ¼��ɹ����

	ret = Create_CSV((char *)FACES_LIB_PATH);

	// restart 2 opencv deal pthread
	ret = opencvDeal_restart();

addFace_exit:
	gstUsrMngr.getFaceSta = 0;

	// �ر�����opencv����
	destroyAllWindows();

	printf("%s: exit ---------.\n", __FUNCTION__);
	
	return (void*)0;
}



// �ж�ͨ��Ŀ¼�Ƿ��Ѵ��ڣ���:1_* ��Ѱ����1_��ͷ��ֻ֧��ͨ�俪ͷ 
int checkDirExist(char *basePath, char *dirHeadStr)
{
	struct stat statbuf;
	DIR *dir;
	struct dirent *dirp;
	char 	tmpDirPath[128] = {0};

	if(basePath[strlen(basePath)-1] == '/')		// ͳһ���벻��'/'�������� "/mnt/" ��Ϊ "/mnt"
		basePath[strlen(basePath)-1] = 0;
	
	if(dirHeadStr[strlen(dirHeadStr)-1] == '*')
		dirHeadStr[strlen(dirHeadStr)-1] = 0;

	// ��ȡ�ļ�����
	if(lstat(basePath, &statbuf) < 0)
	{
		printf("lstat(%s) failed !\n", basePath);
		return -1;
	}
	
	// �ж��Ƿ�ΪĿ¼
	if(S_ISDIR(statbuf.st_mode) != 1)
	{
		printf("%s is not dir !\n", basePath); 
		return -1;
	}

	// ��Ŀ¼
	dir = opendir(basePath);
	if( dir ==NULL)
	{
		printf("opendir failed.\n");
		return -1;
	}

	// ����һ��Ŀ¼
	while((dirp = readdir(dir)) != NULL)
	{
		// ���� '.' '..'�ļ���linux��
		if( strncmp(dirp->d_name, ".", strlen(dirp->d_name))==0 || 
			 strncmp(dirp->d_name, "..", strlen(dirp->d_name))==0 )
			continue;
		
		// ��1��Ŀ¼��2��Ŀ¼���
		memset(tmpDirPath, 0, sizeof(tmpDirPath));
		strcat(tmpDirPath, basePath);
		strcat(tmpDirPath, "/");
		strcat(tmpDirPath, dirp->d_name);

		if(strncmp(dirp->d_name, dirHeadStr, strlen(dirHeadStr)) == 0)	// ƥ�䣬֤������
		{
			return -1;
		}

	}
	
	// ���˴�����dirHeadStr��ͷ��Ŀ¼����

	return 0;
}


// ��ȡ����Ŀ¼ ���_���� .../1_zengzhaorong ���Ψһ
int get_dir(char *inDir, char *inName, char *outDir)
{
	struct stat statbuf;
	char 	tmpDirPath[128] = {0};
	int		nDirSeq = 0;
	char 	chDirSeq[10] = {0};
	int 	ret = 0;


	if(inDir[strlen(inDir)-1] == '/') 	// ͳһ���벻��'/'�������� "/mnt/" ��Ϊ "/mnt"
		inDir[strlen(inDir)-1] = 0;

	if(lstat(inDir, &statbuf) < 0)	// ��ȡ�ļ�����
	{
		printf("lstat(%s) failed !\n", inDir);
		return -1;
	}
	
	if(S_ISDIR(statbuf.st_mode) != 1)	// �ж��Ƿ�ΪĿ¼
	{
		printf("%s is not dir !\n", inDir); 
		return -1;
	}

	// ��s1 s2 ...��ʼѰ��δ�õ�Ŀ¼��
	while(1)
	{
		nDirSeq ++;

		// ���ת����1_ 2_ ...
		memset(chDirSeq, 0, sizeof(chDirSeq));
		sprintf(chDirSeq, "%d_", nDirSeq);

		// �������Ƿ����
		ret = checkDirExist(inDir, chDirSeq);
		if(ret != 0)	// ����
		{
			continue;
		}

		// ���Ŀ¼
		memset(tmpDirPath, 0, sizeof(tmpDirPath));
		strcat(tmpDirPath, inDir);
		strcat(tmpDirPath, "/");
		strcat(tmpDirPath, chDirSeq);
		strcat(tmpDirPath, inName);

		if(lstat(tmpDirPath, &statbuf) < 0)		// �����ڣ�֤������
		{
			printf("lstat: %s is not used !\n", tmpDirPath); 
			memcpy(outDir, tmpDirPath, sizeof(tmpDirPath));
			break;
		}

		if(S_ISDIR(statbuf.st_mode) != 1)	// ����Ŀ¼��֤������
		{
			printf("S_ISDIR: %s is not used !\n", tmpDirPath); 
			memcpy(outDir, tmpDirPath, sizeof(tmpDirPath));
			break;
		}

	}

	// make directory
	ret = mkdir(tmpDirPath, 0777);
	if(ret == -1)
	{
		printf("mkdir failed.\n");
		return -1;
	}

	return 0;
}


// ��ȡ����ͼ��
int get_face(char *path)
{
	CascadeClassifier face_cascade;  
	VideoCapture camera;
	Mat	frame;
	int ret = 0;
	int faceNum = 1;
	vector<Rect> faces;  
	Mat img_gray;  
	Mat resize_gIMG;
	Mat faceImg;
    double scale = 3.00000;
    double fx = 1.00000 / scale;

	camera.open(0);		// ������ͷ
	if(!camera.isOpened())
	{
	  cout << "open camera failed. " << endl;
	  return -1;
	}
	cout << "open camera succeed. " << endl;

	// ��������������
	ret = face_cascade.load("haarcascade_frontalface_alt.xml");
	if( !ret )
	{
		printf("load xml failed.\n");
		return -1;
	}

	while (1)  
	{
		camera >> frame;
		if(frame.empty())
		{
			continue;
			usleep(100);
		}
		
		cvtColor(frame, img_gray, COLOR_BGR2GRAY);  
		
		resize( img_gray, resize_gIMG, Size(), fx, fx, INTER_LINEAR);	//����ͼ��
		
//		equalizeHist(img_gray, img_gray);  
		equalizeHist(resize_gIMG, resize_gIMG);  
		
		// ���Ŀ��
//		face_cascade.detectMultiScale(img_gray, faces, 1.1, 3, 0, Size(50, 50)); 
		face_cascade.detectMultiScale(resize_gIMG, faces, 1.1, 3, 0, Size(50, 50)); 

		for(size_t i =0; i<faces.size(); i++)  
		{
			// ����resize�������λ��
			faces[0].x *= scale;
			faces[0].y *= scale;
			faces[0].width *= scale;
			faces[0].height *= scale;
		
			 /* �����ο��Ŀ�� */
			rectangle(frame, Point(faces[0].x, faces[0].y), Point(faces[0].x + 
				faces[0].width, faces[0].y + faces[0].height),	Scalar(0, 255, 0), 1, 8);
		}

		pthread_mutex_lock(&gstUsrMngr.MatCapLock);
		gstUsrMngr.MatCapture = frame.clone();
		pthread_mutex_unlock(&gstUsrMngr.MatCapLock);
/*
		imshow("camera", frame);  // ��ʾ
		// delay, no too loog or short , not use WaitKey() (because qt)
		usleep(800);
*/
		if(gstUsrMngr.photoFlag)
		{
			// ֻ�޶����һ������
			if(faces.size() == 1)
			{
				faceImg = frame(faces[0]);
				ret = resize_save(faceImg, path, faceNum);	// ������С������
				if(ret == 0)
				{
					printf("resize_save %s/%d success.\n", path, faceNum);
					if(faceNum >= 10)
						return 0;
					faceNum ++;
				}
			}
			gstUsrMngr.photoFlag = 0;
		}
		
	}  
	
	camera.release();

	return 0;
}


// ������С������
int resize_save(Mat& faceIn, char *path, int FaceSeq)
{
	string strName;
	Mat image;
	Mat faceOut;  
	int ret;

	if(faceIn.empty())
	{  
    	printf("faceIn is empty.\n");
      	return -1;  
	}  

	if (faceIn.cols > 100)  
	{  
		resize(faceIn, faceOut, Size(92, 112));		// ������С����ѡ����ٷ����������
		strName = format("%s/%d.jpg", path, FaceSeq);	// ��Ҫ�����ļ���
		ret = imwrite(strName, faceOut);  // �ļ�����׺Ҫ��ȷ .jpg .bmp ...
		if(ret == false)	// ���ִ��������ļ�����׺���ļ�·���Ƿ����
		{
			printf("imwrite failed!\n");
			printf("please check filename[%s] is legal ?!\n", strName.c_str());
			return -1;
		}
//		imshow(strName, faceOut);  
		
		pthread_mutex_lock(&gstUsrMngr.QImgFaceLock);
		gstUsrMngr.QImgFace = gFaceDeal.cvMat2QImage(faceOut); // cvMat >> QImage
		pthread_mutex_unlock(&gstUsrMngr.QImgFaceLock);

	}  

    return 0;
}


// ����/����CSV�ļ� ����: ������·��
int Create_CSV(char *dir_path)
{
	struct stat statbuf;
	DIR *dir = NULL;
	DIR *dirFile = NULL;
	struct dirent *dirp;
	struct dirent *direntFile;
	char 	dir_path2[64] = {0};
	char 	fileName[128] = {0};
	char 	witeBuf[128] = {0};
	char 	label[10] = {0};
	int 	fd;
	int 	i;

	if(dir_path[strlen(dir_path)-1] == '/')		// ͳһ���벻��'/'�������� "/mnt/" ��Ϊ "/mnt"
		dir_path[strlen(dir_path)-1] = 0;

	// ��ȡ�ļ�����
	if(lstat(dir_path, &statbuf) < 0)
	{
		printf("lstat(%s) failed !\n", dir_path);
		return -1;
	}

	// �ж��Ƿ�ΪĿ¼
	if(S_ISDIR(statbuf.st_mode) != 1)
	{
		printf("%s is not dir !\n", dir_path); 
		return -1;
	}

	// ��Ŀ¼
	dir = opendir(dir_path);
	if( dir ==NULL)
	{
		printf("opendir failed.\n");
		return -1;
	}

	// �������csv�ļ�
	fd = open("faces.csv", O_RDWR | O_CREAT | O_TRUNC, 0777);
	if(fd < 0)
	{
		printf("open file faile.\n");
		return -1;
	}

	// ��λ��дλ��
	lseek(fd, 0, SEEK_SET);

	// ����һ��Ŀ¼
	while((dirp = readdir(dir)) != NULL)
	{
		// ���� '.' '..'�ļ���linux��
		if( strncmp(dirp->d_name, ".", strlen(dirp->d_name))==0 || 
			 strncmp(dirp->d_name, "..", strlen(dirp->d_name))==0 )
			continue;

		// ��1��Ŀ¼��2��Ŀ¼���
		memset(dir_path2, 0, sizeof(dir_path2));
		strcat(dir_path2, dir_path);
		strcat(dir_path2, "/");
		strcat(dir_path2, dirp->d_name);

		if(lstat(dir_path2, &statbuf) < 0)
		{
			printf("lstat(%s) failed !\n", dir_path2);
			continue;
		}

		if(S_ISDIR(statbuf.st_mode) != 1)
		{
			printf("%s is not dir !\n", dir_path2); 
			continue;
		}

		dirFile = opendir(dir_path2);
		if( dirFile ==NULL)
		{
			printf("opendir failed.\n");
			return -1;
		}

		// ��������Ŀ¼
		while((direntFile = readdir(dirFile)) != NULL)
		{
			if( strncmp(direntFile->d_name, ".", strlen(direntFile->d_name))==0 || 
				 strncmp(direntFile->d_name, "..", strlen(direntFile->d_name))==0 )
				continue;

			// ��ȡ�����ļ�·����
			memset(fileName, 0, sizeof(fileName));
			strcat(fileName, dir_path2);
			strcat(fileName, "/");
			strcat(fileName, direntFile->d_name);

			if(lstat(fileName, &statbuf) < 0)
			{
				printf("lstat(%s) failed !\n", fileName);
				continue;
			}
			if(S_ISREG(statbuf.st_mode) != 1)	// ������ͨ�ļ�
			{
				printf("%s is not reg file !\n", fileName); 
				continue;
			}

			memset(label, 0, sizeof(label));
			memset(witeBuf, 0, sizeof(witeBuf));
			memcpy(witeBuf, fileName, strlen(fileName));
			strcat(witeBuf, ";");

			for(i=0; i<10; i++)		// ȡ���
			{
				if(dirp->d_name[i] != '_')
					label[i] = dirp->d_name[i];
				else
					break;
			}
			if(i == 10)
				continue; 
			
			strcat(witeBuf, label);	// ��ǩ: '_'ǰ���ַ�
			strcat(witeBuf, "\n");

			// д����Ϣ
			if(write(fd, witeBuf, strlen(witeBuf)) != (ssize_t)strlen(witeBuf))
			{
				printf("%s: write failed!\n", __FUNCTION__);
				closedir(dirFile);
				closedir(dir);
				close(fd);
				return -1;
			}
		}
	}

	closedir(dirFile);
	closedir(dir);
	close(fd);

	return 0;
}

void userMngr_DeInit(struct userMngr_Stru *userMnStru)
{
	if(userMnStru->pstUserInfo != NULL)
	{
		free(userMnStru->pstUserInfo);
		userMnStru->pstUserInfo = NULL;
	}
}

// ɾ���ǿ�Ŀ¼
int remove_dir(const char *dir)
{
    char cur_dir[] = ".";
    char up_dir[] = "..";
    char dir_name[128];
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;

    // �������ݽ�����Ŀ¼�����ڣ�ֱ�ӷ���
    if ( 0 != access(dir, F_OK) ) {
        return 0;
    }

    // ��ȡĿ¼����ʧ�ܣ����ش���
    if ( 0 > stat(dir, &dir_stat) ) {
        perror("get directory stat error");
        return -1;
    }

    if ( S_ISREG(dir_stat.st_mode) ) {  // ��ͨ�ļ�ֱ��ɾ��
        remove(dir);
    } else if ( S_ISDIR(dir_stat.st_mode) ) {   // Ŀ¼�ļ����ݹ�ɾ��Ŀ¼������
        dirp = opendir(dir);
        while ( (dp=readdir(dirp)) != NULL ) {
            // ���� . �� ..
            if ( (0 == strcmp(cur_dir, dp->d_name)) || (0 == strcmp(up_dir, dp->d_name)) ) {
                continue;
            }

            sprintf(dir_name, "%s/%s", dir, dp->d_name);
            remove_dir(dir_name);   // �ݹ����
        }
        closedir(dirp);

        rmdir(dir);     // ɾ����Ŀ¼
    } else {
        perror("unknow file type!");    
    }

    return 0;
}

