#include "Picture_Stitch.h"

using namespace std;
using namespace cv;


#define DETAIL
bool try_use_gpu = false;
Stitcher::Mode mode = Stitcher::PANORAMA;


int last_id=0;
string result_status="idle";  //0 idle, 1 busy ,2 finish, 3  Can't  find images  4Can't stitch images
string result_name = "";
string result_path = "";


int httpserver_bindsocket(int port, int backlog);
int httpserver_start(int port, int nthreads, int backlog);
void* httpserver_Dispatch(void *arg);
void httpserver_GenericHandler(struct evhttp_request *req, void *arg);
void httpserver_ProcessRequest(struct evhttp_request *req);
int parseCmdArgs(string path, string type, vector<Mat> *images, vector<String> *imgs);
void getPath(char * path);
string get_current_path();
string Find_para(string cmd,string key_word);
string Stitch_Picture(string path,string result_path,string result_name);

struct HTTPST
{
    struct event_base *base;
    struct evhttp *httpd;
};

int httpserver_bindsocket(int port, int backlog) {
  int r;
  int nfd;
  nfd = socket(AF_INET, SOCK_STREAM, 0);
  if (nfd < 0) {
    printf("socket fail. \n");
    return -1;
  }

  int one = 1;
  r = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  r = bind(nfd, (struct sockaddr*)&addr, sizeof(addr));
  if (r < 0) {
    printf("bind fail. \n");
    return -1;
  }
  r = listen(nfd, backlog);
  if (r < 0) {
    printf("listen fail. \n");  
    return -1;
  }

  int flags;
  if ((flags = fcntl(nfd, F_GETFL, 0)) < 0
      || fcntl(nfd, F_SETFL, flags | O_NONBLOCK) < 0)
    return -1;

  return nfd;
}

int httpserver_start(int port, int nthreads, int backlog) {
  int r, i;
  int nfd = httpserver_bindsocket(port, backlog);
  if (nfd < 0) {
    printf("http server bind socket fail. \n");
    return -1;
  }


  printf("http server start now \n");
  pthread_t ths[nthreads];
  for (i = 0; i < nthreads; i++) {
    HTTPST *httppara= new HTTPST;
    httppara->base = event_init();
    if (httppara->base == NULL) return -1;
    httppara->httpd = evhttp_new(httppara->base);
    if (httppara->httpd == NULL) return -1;
    r = evhttp_accept_socket(httppara->httpd, nfd);
    if (r != 0) return -1;
    evhttp_set_gencb(httppara->httpd, httpserver_GenericHandler, NULL);
    r = pthread_create(&ths[i], NULL, httpserver_Dispatch, httppara);
    if (r != 0) return -1;
  }
  for (i = 0; i < nthreads; i++) {
    pthread_join(ths[i], NULL);
  }
  return 0;
}

void* httpserver_Dispatch(void *arg) {
  HTTPST *httpimp=(struct HTTPST*)arg;
  printf("http server start OK! \n");
  event_base_dispatch(httpimp->base);
  evhttp_free(httpimp->httpd);
  delete httpimp;
  httpimp=NULL;
  return NULL;
}

void httpserver_GenericHandler(struct evhttp_request *req, void *arg) {
      httpserver_ProcessRequest(req);
}

void httpserver_ProcessRequest(struct evhttp_request *req) {
    struct evbuffer *buf = evbuffer_new();
    if (buf == NULL) return;

    std::string URL_ORG=evhttp_request_get_uri(req);
    std::string URL=evhttp_decode_uri(URL_ORG.c_str());
    size_t pos1=URL.find('/',0);
    size_t pos2=URL.find("/?",0);
    std::string cmd=URL.substr(pos1+1,pos2-1);
    std::string para=URL.substr(pos2+2);

    //here comes the magic
    std::string org_picture_path="";
    std::string org_picture_num="1";
    std::string org_picture_type="";
    string error_code="no error";

    struct evkeyvalq *headers = evhttp_request_get_input_headers(req);
    for (struct evkeyval *header = headers->tqh_first; header;
        header = header->next.tqe_next) {
        printf("  %s: %s\n", header->key, header->value);
    }



    if((cmd=="StartStitch")||(cmd=="StartStitchWithVideo"))
    {
        string video_name=Find_para(para,"org_video_name=");

        int cut_number=std::atoi(Find_para(para,"cut_number=").c_str());
        result_name=Find_para(para,"result_name=");
        result_path=Find_para(para,"result_path=");
        org_picture_path=Find_para(para,"org_picture_path=");
        org_picture_num=Find_para(para,"org_picture_num=");
        org_picture_type=Find_para(para,"org_picture_type=");

        if(cmd=="StartStitchWithVideo")
        {
            if(video_to_picture(video_name,org_picture_path,cut_number)==-1)
                error_code="open video failed";
        }
        if(error_code=="no error")
            error_code=Stitch_Picture(org_picture_path,result_path,result_name);
        evbuffer_add_printf(buf,
                            "{\
                            \n\"command\":\"%s\",\
                            \n\"code\":\"%s\",\
                            \n\"stauts\":\"%s\",\
                            \n\"result_name\":\"%s\"\
                            \n}",
                            cmd.c_str(),error_code.c_str(),result_status.c_str(),result_name.c_str());
    }
    else if(cmd=="GetStitchRestult")
    {
        evbuffer_add_printf(buf,
                            "{\
                            \n\"command\":\"%s\",\
                            \n\"code\":\"%s\",\
                            \n\"stauts\":\"%s\",\
                            \n\"result_name\":\"%s\"\
                            \n}",
                            cmd.c_str(),error_code.c_str(),result_status.c_str(),result_name.c_str());
    }
    else if(cmd=="DeletStitchRestult")
    {
        string delete_file=Find_para(para,"result_name=");
        string delete_path=Find_para(para,"result_path=");
        if((delete_file!="")&&(delete_path!=""))
        {
            delete_file="/"+delete_file;
            delete_file=delete_path+delete_file;
        }

        if(remove(delete_file.c_str())!=0)
            error_code="can not find this file";
        evbuffer_add_printf(buf,
                            "{\
                            \n\"command\":\"%s\",\
                            \n\"code\":\"%s\"\
                            \n}",
                            cmd.c_str(),error_code.c_str());

    }
    else
    {
          //add picture to client
        string image;

        if(cmd.find('/',0)==string::npos)
        {
           image=get_current_path()+"/"+cmd;
        }
        else
        {
           image="/"+cmd;
        }

        cout << "Open image: "<<image<<endl;
        int fd=open(image.c_str(),O_RDONLY);
        if(fd>-1)
        {
            struct stat st;
            fstat(fd,&st);
            evbuffer_add_file(buf,fd,0,st.st_size);
            cmd="DownloadPicture";
        }
        else
        {
            evbuffer_add_printf(buf,
                            "{\
                            \n\"command\":\"unknown\",\
                            \n\"code\":\"%d\"\
                            \n}",
                            20);
        }
    }
    evhttp_add_header(req->output_headers, "Server", "Sony test server");
    if(cmd!="DownloadPicture")
        evhttp_add_header(req->output_headers, "Content-Type", "application/json; charset=UTF-8");
    else
        evhttp_add_header(req->output_headers, "Content-Type", "image/jpeg");
    evhttp_add_header(req->output_headers, "Connection", "keep-alive");
    evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");

    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);

    return;

}

int main(void) {
    printf("stitcher start ... \n");
    last_id=0;
    result_status="idle";  //0 idle, 1 busy ,2 finish, 3  Can't  find images  4Can't stitch images
    result_name = "";
    result_path = "";
    Paras_All_Init();
    httpserver_start(8081, 10, 10240);
    printf("stitcher end ... \n");
}

int parseCmdArgs(string path,string type,vector<Mat> *images,vector<String> *imgs) {
    cout << "Find "<< type.c_str()<<" in "<<path.c_str()<<endl;

    DIR* pDir = NULL;
    struct dirent* ent = NULL;
    pDir = opendir(path.c_str());
    if (NULL == pDir){
        printf("Source folder not exists!");
        return 0;
    }

    int flag=0;
    while (NULL != (ent=readdir(pDir)))  {
        if(ent->d_type==8)
        {
            string name= ent->d_name;
            //if((int)name.find(type,0)==(name.length()-type.length()))
            {
                imgs->push_back(path+"/"+name);
                Mat img = imread(path+"/"+name);
                if(!img.empty())
                {
                    flag=1;
                    printf("%s ", name.c_str());
                    images->push_back(img);
                }
            }
        }
    }
    if(flag==0)
        return 0;
    return 1;
}


string Stitch_Picture(string path,string result_path,string result_name) {
    string error_code="busy error";

    while(result_status!="busy")
    {
        result_status="busy";


        Mat pano;
        vector<Mat> images;
        vector<String> imgs;
        int retval = parseCmdArgs(path, "",&images,&imgs);
        if (retval==0)
        {
            cout << "Can't  find images"<< endl;
            result_status="Can't  find images";
            break;
        }


#ifdef  DETAIL
        error_code=stitch_detail(imgs,pano);
        if(error_code!="no error")
            break;
#else

        Ptr<Stitcher> stitcher = Stitcher::create(mode, try_use_gpu);
        Stitcher::Status status = stitcher->stitch(images, pano);

        if (status != Stitcher::OK)
        {
            cout << "Can't stitch images, error code = " << int(status) << endl;
            result_status="Can't stitch images";
            switch (status) {
                case Stitcher::ERR_NEED_MORE_IMGS:
                    error_code="need more images";
                    break;
                case Stitcher::ERR_HOMOGRAPHY_EST_FAIL:
                    error_code="homography est fail";
                    break;
                case Stitcher::ERR_CAMERA_PARAMS_ADJUST_FAIL:
                    error_code="camera params adjust fail";
                    break;
                default:
                    break;
            }
            break;
        }
#endif


        if((result_path!="")&&(result_name!=""))
        {
            result_name="/"+result_name;
            result_name=result_path+result_name;
        }
        else if(result_name=="")
        {
            result_status="Can't stitch images";
            error_code="result name error";
            break;
        }

        error_code="no error";
        cout << "Result name is "<<result_name<< endl;
        imwrite(result_name, pano);
        pano.release();
        break;
    }
    if(error_code!="busy error")
        result_status="finish";
    return error_code;
}

string get_current_path() {
    char path[256];
    FILE * fp;
    fp=popen("pwd","r");
    fgets(path,sizeof(path),fp);
    pclose(fp);
    string path111=path;
    path111.resize(path111.size()-1);
    return path111;
}

string Find_para(string cmd,string key_word) {
    int length=key_word.length();
    if(cmd.find(key_word)==string::npos)
        return "";
    return cmd.substr(cmd.find(key_word)+length,cmd.find("&",cmd.find(key_word)+2)-cmd.find(key_word)-length);
}
