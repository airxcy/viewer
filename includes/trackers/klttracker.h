#ifndef KLTTRACKER_H
#define KLTTRACKER_H
#include "trackers/tracker.h"
#include "trackers/klt_c/klt.h"
//#include "trackers/klt_c/pnmio.h"
//#include "trackers/klt_c/trk.h"
#include <vector>
class KLTtracker : public Tracker
{
public:
	KLT_TrackingContext tc;
	KLT_FeatureList fl;
	KLT_FeatureTable ft;

	int *isTracking, *trkIndex;
    int bgMod;

	int frameidx;
	int nFeatures; /*** get frature number ***/
	int FOREGROUND_THRESHOLD;
	int curMaxTrk;
	int drawW, drawH, scaleW, scaleH;
	int delay;
	unsigned char* preframedata,* bgdata,*curframedata;

    std::vector<FeatBuff> trackBuff;
    FeatPts pttmp;
    //std::vector<FeatBuff> targetLoc;
    //std::vector<REAL> dlyBB,dirVec,comVec;

	int init(int bsize,int w,int h);
	int selfinit(unsigned char* framedata);
	int initBG(int mode,unsigned char* bgptr=NULL);
    int updateAframe(unsigned char* framedata,int fidx);
	void updateFGMask(unsigned char* bgptr);

	int checkFG(int x,int y);
    bool checkTrackMoving(FeatBuff &strk);
    bool checkCurve(FeatBuff &strk);

	int endTraking();
};
#endif
