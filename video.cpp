#include "Picture_Stitch.h"

using namespace std;
using namespace cv;

int video_to_picture(string video,string picture_path,int maxcount)
{

    const string video_name=video;
    CvCapture *capture = NULL;
    IplImage *frame = NULL;


    struct stat stStat;
    if(stat(picture_path.c_str(), &stStat)!=0)
    {
        if(mkdir(picture_path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR)!=0)
            return -1;
    }

    capture = cvCaptureFromFile(video_name.c_str());
    //cvNamedWindow("AVI player",1);
    if(capture==NULL)
        return -1;

    long totalFrameNumber =cvGetCaptureProperty(capture,CV_CAP_PROP_FRAME_COUNT);
    cout << "整个视频共" << totalFrameNumber << "帧" << endl;
    int delay=totalFrameNumber/maxcount;
    int count_tmp=0;
    char tmpfile[100]={'\0'};

    while(true)
    {
        if(cvGrabFrame(capture))
        {
            if (count_tmp % delay == 0)
            {
                frame=cvRetrieveFrame(capture);
                //cvShowImage("AVI player",frame);//显示当前帧
                sprintf(tmpfile,"%s/image_%d.jpg",picture_path.c_str(),count_tmp);//使用帧号作为图片名
                cvSaveImage(tmpfile,frame);
                cout << "保存图片成功，图片名：" << tmpfile << endl;
            }
            ++count_tmp;
        }
        else
        {
            break;
        }
    }
    //关闭视频文件
    cvReleaseCapture(&capture);
    //waitKey(0);

    return 0;
}
