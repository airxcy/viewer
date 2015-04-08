#include "trackers/klttracker.h"

#include <cmath>
#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;
#define REPLACE
#define MAX_FEATURE_NUMBER  5000
#define PI 3.14159265
#define dirN 8

unsigned char _line_colos[6][3] =
{
	{0,255,0},
	{0,0,255},
	{255,255, 0},
	{255,0,255},
	{0,255,255},
	{255,0,0},
};
double directions[dirN][2];

int KLTtracker::init(int bsize,int w,int h)
{
	nFeatures = 1000;
	FOREGROUND_THRESHOLD = 0;
	isTracking = (int*)malloc(sizeof(int) * nFeatures);
	trkIndex = (int*)malloc(sizeof(int) * nFeatures);
    trackBuff=std::vector<FeatBuff>(nFeatures);

	for (int i=0;i<nFeatures;i++)
	{
		isTracking[i]=0;
		trkIndex[i]=0;
		//trackBuff[i].init(3,10);
		trackBuff[i].init(1,100);
	}

	frame_width = w;
	frame_height = h;
	frameidx=0;
	tc = KLTCreateTrackingContext();
	fl = KLTCreateFeatureList(nFeatures);
	tc->sequentialMode = TRUE;
	tc->writeInternalImages = FALSE;
	tc->affineConsistencyCheck = -1;  /* set this to 2 to turn on affine consistency check */
	//tc->min_determinant=0.01;
	tc->window_width=7;
	tc->window_height=7;
	//tc->lighting_insensitive=TRUE;
	KLTSetVerbosity(0);
	drawW=frame_width,drawH=frame_height;
	bgMod=0;
	delay=0;
	gt_inited = false;
    for(int di=0;di<dirN;di++)
    {
        double rad=((double)di)/((double)dirN)*PI*2;
        directions[di][0]=cos(rad);
        directions[di][1]=sin(rad);
    }
    gt_N=-1;
	return 1;
}
int KLTtracker::selfinit(unsigned char* framedata)
{
	curframedata=framedata;
	bgdata = new unsigned char[frame_width*frame_height];
	memset(bgdata,0,frame_width*frame_height);
	preframedata = new unsigned char[frame_width*frame_height];
	memcpy(preframedata,framedata,frame_width*frame_height);
	KLTSelectGoodFeatures(tc, framedata, frame_width, frame_height, fl);
	KLTUpdateTCBorder(tc);
	for (int k = 0; k < nFeatures; k++)
	{
		if(true)// (checkFG( x, y))
		{
            pttmp.x = (PntT)(fl->feature[k]->x + 0.5);
            pttmp.y = (PntT)(fl->feature[k]->y + 0.5);
            pttmp.t = frameidx;
			trackBuff[k].updateAFrame(&pttmp);
		}
	}
	return true;
}
int getAvgVec(FeatBuff& trk,REAL * dir,int dly)
{
    if(trk.len<=8)return 0;
    int startidx=0,endidx=trk.len-1;
    FeatPts * aptr = trk.getPtr(startidx);
    FeatPts * bptr = trk.getPtr(endidx);
    dir[0]=bptr->x - aptr->x;
    dir[1]=bptr->y - aptr->y;
    REAL norm=sqrt(dir[0]*dir[0]+dir[1]*dir[1]);
    dir[0]/=norm;
    dir[1]/=norm;
    return 1;
    /*
    for (int j = 1; j <strk.len; ++j)
    {
        aptr = strk.getPtr(j - 1);
        bptr = strk.getPtr(j);
        dtmp = sqrt(pow(bptr->x - aptr->x, 2.0) + pow(bptr->y - aptr->y, 2.0));
        totlen += dtmp;
        if (dtmp>maxdist)maxdist = dtmp;
    }
    */
}
int PredictTrk(FeatPts* point,FeatBuff& trk)
{
    if(trk.len<2)
    {
        memcpy(point,trk.cur_frame_ptr,sizeof(FeatPts));
        return 0;
    }
    int startidx=max(trk.len-8,0),endidx=trk.len-1;
    double len=endidx-startidx+1;
    FeatPts* aptr=trk.getPtr(startidx);
    FeatPts* bptr=trk.getPtr(endidx);
    point->x=(bptr->x-aptr->x)/len+bptr->x;
    point->y=(bptr->y-aptr->y)/len+bptr->y;
    point->t=bptr->t+1;
}
int KLTtracker::checkFG(int x,int y)
{
	if(bgMod==1)
	{
		return (abs(curframedata[y*frame_width+x]-bgdata[y*frame_width+x])>=FOREGROUND_THRESHOLD);
	}
	else if(bgMod==2)
	{
		return (int)bgdata[y*frame_width+x];
	}
	else 
		return true;
}
void KLTtracker::updateFGMask(unsigned char* bgptr)
{
	bgdata=bgptr;
}

bool KLTtracker::checkTrackMoving(FeatBuff &strk)
{
	bool isTrkValid = true;

    int Movelen=10,minlen=5,startidx=max(strk.len-Movelen,0);
    if(strk.len>Movelen)
    {
        double maxdist = .0, dtmp = .0,totlen=.0;

        FeatPts* aptr = strk.getPtr(startidx), *bptr = aptr;
        //int xa=(*aptr),ya=(*(aptr+1)),xb=*(strk.cur_frame_ptr),yb=*(strk.cur_frame_ptr+1);
        REAL xa=aptr->x,ya=aptr->y,xb=strk.cur_frame_ptr->x,yb=strk.cur_frame_ptr->y;
        double displc=sqrt( pow(xb-xa, 2.0) + pow(yb-ya, 2.0));
        /*
        for(int posi=0;posi<strk.len;posi++)
        {
            bptr=strk.getPtr(posi);
            xb=bptr->x,yb=bptr->y;
            dtmp = sqrt( pow(xb-xa, 2.0) + pow(yb-ya, 2.0));
            totlen+=dtmp;
            if (dtmp > maxdist && posi>=startidx) maxdist = dtmp;
            xa=xb,ya=yb;
        }
        */
        //if(strk.len>100&&totlen*0.5>displc){strk.isCurved=true;}
        if((strk.len -startidx)*0.2>displc)
        {
            isTrkValid = false;
        }
        //if (maxdist < 1.4 && strk.len>30){isTrkValid = false;}
        //if (maxdist <=0.1 && strk.len>=minlen){isTrkValid = false;}
    }
	return isTrkValid;
}

int KLTtracker::updateAframe(unsigned char* framedata,int fidx)
{
    frameidx=fidx;
	curframedata=framedata;
	KLTTrackFeatures(tc, preframedata, framedata, frame_width, frame_height, fl);

	KLTReplaceLostFeatures(tc, framedata, frame_width, frame_height, fl);

	for (int k = 0; k < nFeatures; k++)
	{
		if ( fl->feature[k]->val == 0)
		{
            pttmp.x = fl->feature[k]->x;//(PntT)(fl->feature[k]->x + 0.5);
            pttmp.y = fl->feature[k]->y;//(PntT)(fl->feature[k]->y + 0.5);
            pttmp.t = frameidx;
			trackBuff[k].updateAFrame(&pttmp);
			bool isMoving=checkTrackMoving(trackBuff[k]);
			if (!isMoving) 
			{
				isTracking[k]=0;
				fl->feature[k]->val=-2;
			}
		}

		if ( fl->feature[k]->val > 0)
		{
			trackBuff[k].clear();
            pttmp.x = fl->feature[k]->x;//(PntT)(fl->feature[k]->x + 0.5);
            pttmp.y = fl->feature[k]->y;//(PntT)(fl->feature[k]->y + 0.5);
            pttmp.t = frameidx;
			//trackBuff[k].updateAFrame(x,y,frameidx);
			trackBuff[k].updateAFrame(&pttmp);
			isTracking[k]=1;
            /*
            for(int bb_i=0;bb_i<gt_N;bb_i++)
            {
                bbxft[bb_i*nFeatures+k]=0;
            }
            */
		}

		if ( fl->feature[k]->val < 0)
		{
			trackBuff[k].clear();
            /*
            for(int bb_i=0;bb_i<gt_N;bb_i++)
            {
                bbxft[bb_i*nFeatures+k]=0;
            }
            */

		}
	}
    //if(gt_inited) updateBB();
	return 1;
}


int checkidx=720, checkx=0,checky=0,checkt=0;
vector<int> idxvec;
bool KLTtracker::checkCurve(FeatBuff &strk)
{
	double maxdist = .0, dtmp = .0, totlen = .0;
    FeatPts * aptr = strk.getPtr(0), *bptr = aptr;
    REAL xa = aptr->x, ya = aptr->y, xb = strk.cur_frame_ptr->x, yb = strk.cur_frame_ptr->y;
	double displc = sqrt(pow(xb - xa, 2.0) + pow(yb - ya, 2.0));
	for (int j = 1; j <strk.len; ++j)
	{
		aptr = strk.getPtr(j - 1);
		bptr = strk.getPtr(j);
		dtmp = sqrt(pow(bptr->x - aptr->x, 2.0) + pow(bptr->y - aptr->y, 2.0));
		totlen += dtmp;
		if (dtmp>maxdist)maxdist = dtmp;
	}
	return (strk.len > 20 && totlen*0.5 > displc);
}
int KLTtracker::endTraking()
{
  return 0;
}
