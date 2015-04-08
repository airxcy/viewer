#include "Qts/streamthread.h"

#include "Qts/viewqt.h"
#include "Qts/modelsqt.h"
#include "trackers/utils.h"
#include <iostream>
#include <fstream>
//#include <stdlib.h>

//#include <direct.h>
#include "Qts/mainwindow.h"
#include <opencv2/opencv.hpp>

#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStringList>

using namespace cv;

VideoCapture cap;
Mat frame;
float fps=0;
char strbuff[100];
QDir qdirtmp;
Mat gray;
StreamThread::StreamThread(QObject *parent) : QThread(parent)
{
    restart = false;
    abort = false;
    pause = false;
    bb_N=0,bufflen=0;
    trkscene=NULL;
    paused=false;
    inited=false;
    framebuff=NULL;
    firsttime=true;
    roidone=false;
    nextframeidx=0;
    sliceptr=NULL;
    crossLog=NULL;
    prodlog=NULL;
    sliceGT=NULL;
    sliceBB=NULL;
    persDone=false;
    kltInited=false;
}
StreamThread::~StreamThread()
{
    /*
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();
    wait();
    */
    //QMutexLocker lock(&mutex);
    abort = true;
    cv0.wakeAll();
    wait();
}

bool StreamThread::init()
{
    //QMutexLocker lock(&mutex);

    restart=false,abort=false,pause=false,paused=false;
    bufflen=0,maxframe=0,maxskeyframe=0;
    //cap.open("C:/Users/xcy/Documents/CVProject/data/grandcentral/grandcentral.avi");
//    if(cap.isOpened())
//        cap.set(CV_CAP_PROP_POS_AVI_RATIO,0);
//    else

    if(!cap.isOpened())
    {
        cap.open(vidfname);
        std::cout<<"reopened"<<std::endl;
    }
    cap.set(CV_CAP_PROP_POS_FRAMES,nextframeidx);
    frameidx=nextframeidx;
    maxframe=cap.get(CV_CAP_PROP_FRAME_COUNT );
    int fps = cap.get(CV_CAP_PROP_FPS);
    parsefname();

    parsegt();

    cap>>frame;

    delay=25;
    bufflen=delay+10;
    maxskeyframe=maxframe;//delay;
    cvtColor(frame,frame,CV_BGR2RGB);
    framewidth=frame.size[1],frameheight=frame.size[0];
    cvtColor(frame,gray,CV_BGR2GRAY);

    if(framebuff==NULL)
    {
        framebuff = new FrameBuff();
        framebuff->init(frame.elemSize(),framewidth,frameheight,bufflen);
    }
    else
        framebuff->clear();

    frameByteSize=frame.size[0]*frame.size[1]*frame.elemSize();
    framebuff->updateAFrame(frame.data);
    frameptr=framebuff->cur_frame_ptr;

    delayedFrameptr=framebuff->headptr;
    inited=true;
    firsttime=false;
    tracker = new KLTtracker();
    tracker->init(10,framewidth,frameheight);
    tracker->selfinit(gray.data);
    prodvec.reserve(tracker->nFeatures);
    featlog.assign(tracker->nFeatures,0);
    kltInited=true;
    //if(!lineDone)
        //setUpLine(framewidth/2,0,framewidth/2,frameheight);
    return cap.isOpened();
}
void StreamThread::updateItems()
{
    if(trkscene!=NULL)
    {
    }
}
void StreamThread::parsefname()
{
    QFileInfo qvidfileinfo(vidfname.data());
    baseDirname=qvidfileinfo.path();
    vidid=qvidfileinfo.baseName();
    vidid=vidid+"_"+qvidfileinfo.completeSuffix();
    gtdir=baseDirname+"/"+vidid+"/";
    qdirstr=baseDirname+"/"+vidid+"/";
}
void StreamThread::getroi()
{
    if(trkscene!=NULL)
    {
        for(int i=0;i<trkscene->roivec.size();i++)
        {
            DragDots* dot = trkscene->roivec[i];
            int x=dot->coord[0],y=dot->coord[1];
            roi.push_back(Point2i(x,y));
        }
        qdirtmp.mkpath(qdirstr);
        QString savefname=qdirstr+"roi.txt";
        QFile qoutfile(savefname);
        qoutfile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream outts(&qoutfile);
        outts<< roi.size()<<"\n";
        int x,y;
        for(int i=0;i<roi.size();i++)
        {
            Point2i& ptr = roi[i];
            x=ptr.x,y=ptr.y;
            outts<<x<<","<<y<<"\n";
        }
        trkscene->roidone=true;
    }
}
double StreamThread::checkroi(int x, int y)
{
    if(roidone)
        return pointPolygonTest(roi,Point2f(x,y),1);
    return 1;
}
void StreamThread::parsegt()
{
    if(firsttime)
    {
        sprintf(strbuff,"%06d.txt\0",bb_N);
        QString savefname=qdirstr+strbuff;
        QFile qinfile(savefname);
        while(qinfile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream ints(&qinfile);
            int len=0;
            QString line = qinfile.readLine();
            len = line.toInt();
            TrackBuff * abbtrk =new TrackBuff();
            abbtrk->init(1,100);
            int x,y,fidx;
            for (int i = 0; i < len; i++)
            {
                line = qinfile.readLine();
                QStringList list = line.split(",", QString::SkipEmptyParts);
                pttmp.x=list[0].toInt(),pttmp.y=list[1].toInt(),pttmp.t=list[2].toInt();
                abbtrk->updateAFrame(&pttmp);
            }
            trackBuff.push_back(abbtrk);
            qinfile.close();
            bb_N++;
            sprintf(strbuff,"%06d.txt\0",bb_N);
            savefname=qdirstr+strbuff;
            qinfile.setFileName(savefname);
        }
        pdvec = std::vector<double>(bb_N,0);
        maxpd=1,minpd=1,avgpd=1;
        qinfile.close();
    }
    if(!roidone)
    {
        QString savefname=qdirstr+"roi.txt";
        QFile qinfile(savefname);
        if(qinfile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            int len=0;
            QString line = qinfile.readLine();
            len = line.toInt();
            int x,y,fidx;
            for (int i = 0; i < len; i++)
            {
                line = qinfile.readLine();
                QStringList list = line.split(",", QString::SkipEmptyParts);
                roi.push_back(Point2i(list[0].toInt(),list[1].toInt()));
            }
            roidone=true;
        }
    }
}
void StreamThread::updateRefscene()
{
    delayedFrameptr=frameptr;
    if(bb_N>0)
    {
        for(int bb_i=0;bb_i<bb_N;bb_i++)
        {
            TrackBuff* bbtrkptr = trackBuff[bb_i];
            if(trackBuff[bb_i]->len>0)
            {
                TrkPts_p trkptr=bbtrkptr->data;
                int startfidx = trkptr->t,len = bbtrkptr->len;
                DragDots* ddot = trkscene->dotvec[bb_i];
                int x=trkptr->x,y=trkptr->y;
                int updatepos=slicetail;
                if(slicetail<slicelen)
                    updatepos=slicetail;
                else
                    updatepos=slicelen-1;
                if(frameidx==startfidx)
                {
                    ddot->setVisible(true);
                    //emit trkStart(bb_i);
                    ddot->setCoord(x,y);
                    if(trkscene->bbDone&&bb_i<trkscene->bbvec.size())
                    {
                        int td=topa*y+topb,bd=bota*y+botb,wd=wa*y+wb;
                        int l=x-wd,t = y+td,r=x+wd,b=y+bd;
                        trkscene->bbvec[bb_i]->setVisible(true);
                        trkscene->bbvec[bb_i]->updateVtx(l,t,r,b);
                    }
                }
                else if(frameidx==startfidx+len)
                {
                    ddot->setVisible(false);
                    //emit trkEnd(bb_i);
                    if(trkscene->bbDone&&bb_i<trkscene->bbvec.size())
                    {
                        trkscene->bbvec[bb_i]->setVisible(false);
                    }
                }
                else if(frameidx>startfidx&&frameidx<startfidx+len)
                {

                    if(!ddot->isVisible())
                        ddot->setVisible(true);
                        //emit trkStart(bb_i);
                    int curidx = frameidx-startfidx;
                    trkptr=bbtrkptr->getPtr(curidx);
                    int curx=trkptr->x,cury=trkptr->y;
                    ddot->setCoord(curx,cury);

                    double td=topa*cury+topb,bd=bota*cury+botb,wd=wa*cury+wb;
                    int l=curx-wd,t = cury+td,r=curx+wd,b=cury+bd;
                    if(trkscene->bbDone&&bb_i<trkscene->bbvec.size())
                    {
                        if(!trkscene->bbvec[bb_i]->isVisible())
                            trkscene->bbvec[bb_i]->setVisible(true);
                        trkscene->bbvec[bb_i]->updateVtx(l,t,r,b);
                    }
                    if(lineDone&&curidx>0)
                    {
                        double x1=curx-pointA[0],y1=cury-pointA[1];
                        double norm1=sqrt(x1*x1+y1*y1);
                        double cproduct =(xn*x1+yn*y1);
                        double lproduct = xline*x1+yline*y1;
                        int pdsign=sgn(cproduct);
                        int crossed= pdsign*prodlog[bb_i*5];
                        if(crossLog[5*bb_i]==0&&crossed<=0)
                        {
                            crossLog[bb_i*5]=1;
                            prodlog[bb_i*5]=pdsign;
                            ddot->lit();
                            int pos=lproduct+0.5;
                            if(pos>=0&&pos<linelen)
                                sliceGT[pos*slicelen+updatepos]=-pdsign*(bb_i+1);
                        }
                        double product[4];
                                product[0]=cproduct-wd*xn+td*yn,
                                product[1]=cproduct-wd*xn+bd*yn,
                                product[2]=cproduct+wd*xn+bd*yn,
                                product[3]=cproduct+wd*xn+td*yn;
                        double projection[4];
                                projection[0]=lproduct-wd*xline+td*yline,
                                projection[1]=lproduct-wd*xline+bd*yline,
                                projection[2]=lproduct+wd*xline+bd*yline,
                                projection[3]=lproduct+wd*xline+td*yline;
                        for(int i=0;i<4;i++)
                        {
                            int start=i,end=(i+1)%4;
                            if(product[start]*product[end]<0&&projection[start]*projection[end]>0)
                            {
                                double pro=(projection[start]*product[end]-product[start]*projection[end])
                                        /(product[end]-product[start]);
                                int pos=pro+0.5;
                                if(pos>=0&&pos<linelen)
                                    sliceBB[pos*slicelen+updatepos]=(crossLog[bb_i*5]*2-1)*prodlog[bb_i*5]*(bb_i+1);
                            }
                        }
                    }
                }
            }
        }
    }
}
void StreamThread::checkKLTSlice()
{
    if(kltInited&&lineDone)
    {
        std::vector<FeatBuff>& featvec=tracker->trackBuff;
        prodvec.clear();
        for(int i=0;i<tracker->nFeatures;i++)
        {

            FeatBuff& feattrk = featvec[i];
            if(feattrk.len>0)
            {

                double x1=feattrk.cur_frame_ptr->x,y1=feattrk.cur_frame_ptr->y;
                double vx1=x1-pointA[0],vy1=y1-pointA[1];//vx0=x0-pointA[0],vy0=y0-pointA[1],;
                double norm1=sqrt(vx1*vx1+vy1*vy1);//norm0=sqrt(vx0*vx0+vy0*vy0),;
                //double cproduct0 = (vx0*xn+vy0*yn);
                double cproduct1 = (vx1*xn+vy1*yn);
                int prodsign=sgn(cproduct1);
                //double cproduct = cproduct0*cproduct1/norm0/norm1;
                prodvec.push_back(cproduct1);
                if(feattrk.len==1||featlog[i]==0)
                {
                    featlog[i]=prodsign;
                }
                if(feattrk.len>5&&prodsign*featlog[i]<=0)
                {

                    if(prodsign<=0)
                    featlog[i]=-1;
                    else
                    featlog[i]=1;
                    int updatepos=slicetail;
                    if(slicetail<slicelen)
                        updatepos=slicetail;
                    else
                        updatepos=slicelen-1;
                    double lproduct = xline*vx1+yline*vy1;
                    int pos = lproduct+0.5;
                    if(pos>=0&&pos<linelen)
                    {
                        double x0=feattrk.getPtr(feattrk.len-2)->x,y0=feattrk.getPtr(feattrk.len-2)->y;
                        double x=x1-x0,y=y1-y0;
                        double dirx = x*xn+y*yn,diry= x*xline+y*yline;
                        sliceKLT(pos,updatepos,0)=dirx;
                        sliceKLT(pos,updatepos,1)=diry;
                        double kltlen=sqrt((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0));
                    }
                }
            }
        }
        std::sort(prodvec.begin(),prodvec.end());
    }
}
void StreamThread::setPos(int stepPos)
{
    nextframeidx=stepPos*delay;
    //cap.set(CV)
    //cap.set(CV_CAP_PROP_POS_AVI_RATIO,nextframeidx);
    std::cout<<nextframeidx<<std::endl;
}

void StreamThread::setUpLine(int xa,int ya,int xb,int yb)
{
    lineDone=false;
    if(ya>=yb)
    {
        //swap
        int tmp=ya;ya=yb;yb=tmp;
        tmp=xa;xa=xb;xb=tmp;
    }
    pointA[0]=xa,pointA[1]=ya,pointB[0]=xb,pointB[1]=yb;
    slicelen=500;
    x_idx.clear();
    y_idx.clear();
    xn=-(pointB[1]-pointA[1]),yn=pointB[0]-pointA[0],
    xline=pointB[0]-pointA[0],yline=pointB[1]-pointA[1];
    normline=sqrt(xline*xline+yline*yline),normn=sqrt(xn*xn+yn*yn);
    xline=xline/normline,yline=yline/normline
    ,xn=xn/normn,yn=yn/normn;

    int prelen = linelen;
    //linelen = getLineIdx(x_idx,y_idx,pointA,pointB);
    linelen = getLineProp(x_idx,y_idx,pointA,pointB,normline);
    if(crossLog==NULL) crossLog=new char[bb_N*5];
    if(prodlog==NULL) prodlog=new char[bb_N*5];
    if(prelen!=linelen)
    {
        delete sliceptr;
        sliceptr = new unsigned char[linelen*slicelen*3];
        delete sliceGT;
        sliceGT = new int[linelen*slicelen];
        delete sliceBB;
        sliceBB = new int[linelen*slicelen];
        for(int i = 0;i<linelen*slicelen;i++)
        {
            sliceGT[i]=0;
            sliceBB[i]=0;
        }
        slicetail=0;
        for(int i=0;i<bb_N;i++)
        {
            for(int j=0;j<5;j++)
                crossLog[i*5+j]=0;
            TrkPts_p trkptr=trackBuff[i]->data;
            int x=trkptr->x,y=trkptr->y;
            double x0=x-pointA[0],y0=y-pointA[1];
            double norm0=sqrt(x0*x0+y0*y0);
            double  cproduct = (xn*x0+yn*y0)/norm0;
            prodlog[i*5]=sgn(cproduct);
        }
        sliceKLT=Map3D<double>(linelen,slicelen,2);
        memset(sliceKLT.data,0,linelen*slicelen*2*sizeof(double));
    }

    lineDone=true;
    if(trkscene)
        trkscene->updateDirPts();
    checkKLTSlice();
    setUpPers();
}
void StreamThread::setUpPers()
{
    if(!persDone&&trkscene->dragbbvec.size()==2)
    {
        int l0,t0,r0,b0;
        trkscene->dragbbvec[0]->getltrb(l0,t0,r0,b0);
        int pid0=trkscene->dragbbvec[0]->bbid;
        int x0=trkscene->dotvec[pid0]->coord[0],y0=trkscene->dotvec[pid0]->coord[1];
        double top0=t0-y0,bot0=b0-y0,w0=(r0-l0)/2;
        int l1,t1,r1,b1;
        trkscene->dragbbvec[1]->getltrb(l1,t1,r1,b1);
        int pid1=trkscene->dragbbvec[1]->bbid;
        int x1=trkscene->dotvec[pid1]->coord[0],y1=trkscene->dotvec[pid1]->coord[1];
        double top1=t1-y1,bot1=b1-y1,w1=(r1-l1)/2;
        topa=(top1-top0)/(y1-y0),bota=(bot1-bot0)/(y1-y0),wa=(w1-w0)/(y1-y0);
        topb=top0-y0*topa,botb=bot0-y0*bota,wb=w0-y0*wa;
        persDone=true;
        trkscene->setUpbb();
    }
}
void StreamThread::updateSlice()
{

    unsigned char * ptr=sliceptr;
    int* gtptr=sliceGT,*slbbptr=sliceBB;
    int step= slicelen*3,cpysize=(slicelen-1)*3;
    int updatepos=slicetail;
    double* kltptr;
    if(slicetail==slicelen)
    {
        ptr=sliceptr;
        gtptr=sliceGT;
        slbbptr=sliceBB;
        kltptr=sliceKLT.data;
        for(int i=0;i<linelen;i++)
        {
            memmove(ptr,ptr+3,cpysize);
            memmove(gtptr,gtptr+1,(slicelen-1)*sizeof(int));
            memmove(slbbptr,slbbptr+1,(slicelen-1)*sizeof(int));
            memmove(kltptr,kltptr+2,(slicelen-1)*sizeof(double)*2);
            *(gtptr+slicelen-1)=0;
            *(slbbptr+slicelen-1)=0;
            *(kltptr+slicelen*2-1)=0;
            *(kltptr+slicelen*2-2)=0;
            ptr+=step;
            gtptr+=slicelen;
            slbbptr+=slicelen;
            kltptr+=slicelen*2;
        }
        updatepos=slicelen-1;
    }
    ptr=sliceptr+updatepos*3;

    for(int i=0;i<linelen;i++)
    {
        int x=x_idx[i],y=y_idx[i];
        int frameoffset=(y*framewidth+x)*3;
        memcpy(ptr,frameptr+frameoffset,3);
        ptr+=step;
    }
    if(slicetail<slicelen)
        slicetail++;
    /*
    for(int bb_i=0;bb_i<trkscene->bb_N;bb_i++)
    {
        trkscene->dotvec[bb_i]->unlit();
    }
    */
}
void StreamThread::updateSliceLean()
{

    unsigned char * ptr=sliceptr;
    int step= slicelen*3,cpysize=(slicelen-1)*3;
    int updatepos=slicetail;
    std::vector<int> & steper=x_idx;
    if(abs(pointA[0]-pointB[0])<abs(pointA[1]-pointB[1]))
    {
         steper=x_idx;
    }
    else
    {
        //step=(slicelen+pointB[1]-pointA[1])*3;
         steper=y_idx;
    }
    if(slicetail==slicelen)
    {
        ptr=sliceptr;
        for(int i=0;i<linelen;i++)
        {
            if(ptr<sliceptr+linelen*slicelen*3)
            {
            memmove(ptr,ptr+3,cpysize);
            //ptr+=step;
            if(i+1<linelen)
                ptr+=(slicelen+steper[i+1]-steper[i])*3;
            }
        }
        updatepos=slicelen-1;
    }
    ptr=sliceptr+updatepos*3;

    for(int i=0;i<linelen;i++)
    {
        if(ptr<sliceptr+linelen*slicelen*3)
        {
        int x=x_idx[i],y=y_idx[i];
        int frameoffset=(y*framewidth+x)*3;
        memcpy(ptr,frameptr+frameoffset,3);
        if(i+1<linelen)
            ptr+=(slicelen+steper[i+1]-steper[i])*3;
        }
    }
    if(slicetail<slicelen)
        slicetail++;
    /*
    Mat sliceMat(linelen,slicelen,CV_8UC3,sliceptr);
    imshow("slice",sliceMat);
    waitKey(1);
    */
}
void StreamThread::streaming()
{
    forever
    {
        if(init())
        {
            emit initSig();
            //frameidx=0;
            lastframeidx=frameidx;
            while(!frame.empty())
            {

                if (restart)
                        break;
                if (abort)
                        return;
                if (pause)
                {
                    mutex.lock();
                    paused=true;
                    cv0.wait(&mutex);
                    paused=false;
                    mutex.unlock();
                }
                cap >> frame;
                if(frame.empty())
                    break;
                cvtColor(frame,gray,CV_BGR2GRAY);
                cvtColor(frame,frame,CV_BGR2RGB);
                if(kltInited)
                {
                    tracker->updateAframe(gray.data,frameidx);
                }
                framebuff->updateAFrame(frame.data);
                frameptr=framebuff->cur_frame_ptr;
                if(lineDone)
                {
                    updateSlice();

                }
                mwindow->slider->setValue(frameidx);
                mwindow->label->setText(QString::number(frameidx)+"/"+QString::number(maxframe));
                frameidx++;
                updateRefscene();
                if(kltInited)
                    checkKLTSlice();
                //msleep(10);
            }
        }
        else
        {
            //emit debug( "init Failed");
        }
        /*
        for(int i=0;i<bb_N;i++)
        {
            delete trackBuff[i];
        }
        trackBuff.clear();
        bb_N=0;
        */
        if(trkscene->lineDone)
        setUpLine(trkscene->lineDots[0].coord[0],
                trkscene->lineDots[0].coord[1],
                trkscene->lineDots[1].coord[0],
                trkscene->lineDots[1].coord[1]);
        for(int i=0;i<bb_N;i++)
        {
            for(int j=0;j<5;j++)
                crossLog[i*5+j]=0;
            TrkPts_p trkptr=trackBuff[i]->data;
            int x=trkptr->x,y=trkptr->y;
            double x0=x-pointA[0],y0=y-pointA[1];
            double norm0=sqrt(x0*x0+y0*y0);
            double  cproduct = (xn*x0+yn*y0)/norm0;
            prodlog[i*5]=sgn(cproduct);
        }
        trkscene->clear();
        inited=false;
        /*
        mutex.lock();
        if (!restart)
            condition.wait(&mutex);
        restart = false;
        mutex.unlock();
        */
    }
}
void StreamThread::run()
{
    streaming();
}

void StreamThread::streamStart(std::string & filename)
{
    QMutexLocker locker(&mutex);
    //QMessageBox::question(NULL, "Test", "msg",QMessageBox::Ok);
    if (!isRunning()) {
        vidfname=filename;
        start(InheritPriority);
    }
    else
    {
        restart=true;
    }
}
