#include "Qts/viewqt.h"
#include "Qts/modelsqt.h"
#include "Qts/streamthread.h"

#include <iostream>

#include <QPainter>
#include <QBrush>
#include <QPixmap>
#include <cmath>
#include <QGraphicsSceneEvent>
#include <QMimeData>
#include <QByteArray>
#include <QFont>
char viewstrbuff[200];
void DefaultScene::mousePressEvent ( QGraphicsSceneMouseEvent * event )
{
    emit clicked(event);
}
void DefaultScene::drawBackground(QPainter * painter, const QRectF & rect)
{
    QPen pen;
    QFont txtfont("Roman",40);
    txtfont.setBold(true);
    pen.setColor(QColor(255,255,255));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setWidth(10);
    painter->setPen(QColor(243,134,48,150));
    painter->setFont(txtfont);
    painter->drawText(rect, Qt::AlignCenter,"打开文件\nOpen File");
}
TrkScene::TrkScene(const QRectF & sceneRect, QObject * parent):QGraphicsScene(sceneRect, parent)
{
    streamThd=NULL;
    focusidx=-1;
    bb_N=0;
    pendingN=0;
    txtfont=QFont("System", 11);
    lineDone=0;
    bbDone=false;
    linepen.setColor(Qt::blue);
}
TrkScene::TrkScene(qreal x, qreal y, qreal width, qreal height, QObject * parent):QGraphicsScene( x, y, width, height, parent)
{
    streamThd=NULL;
    focusidx=-1;
    bb_N=0;
    pendingN=0;
    txtfont=QFont("System", 12);
    roidone=false;
    lineDone=0;
    bbDone=false;
    linepen.setColor(Qt::blue);
}
void TrkScene::drawBackground(QPainter * painter, const QRectF & rect)
{
    if(streamThd!=NULL&&streamThd->inited)
    {
        updateFptr(streamThd->frameptr, streamThd->frameidx);
    }
    painter->setBrush(bgBrush);
    painter->drawRect(rect);
    painter->setPen(QColor(120,180,150));
    painter->setFont(txtfont);
    sprintf(viewstrbuff,"Frame %d\0",frameidx);
    painter->drawText(QRectF (0,0,width(),height()), Qt::AlignLeft|Qt::AlignTop, viewstrbuff);

    if(roivec.size()>0)
    {
        painter->setPen(Qt::blue);
        DragDots* dot = roivec[0];
        int prex=dot->coord[0],prey=dot->coord[1];
        for(int i=0;i<roivec.size();i++)
        {
            dot = roivec[i];
            int x=dot->coord[0],y=dot->coord[1];
            painter->drawLine(prex,prey,x,y);
            prex=x,prey=y;
        }
        if(roidone)
        {
            DragDots* dot = roivec[0];
            int x=dot->coord[0],y=dot->coord[1];
            painter->drawLine(prex,prey,x,y);
        }
    }
    /*
    if(lineDone>=2)
    {
        painter->setPen(linepen);
        painter->drawLine(lineDots[0].coord[0],lineDots[0].coord[1],lineDots[1].coord[0],lineDots[1].coord[1]);
    }
    */

    if(streamThd->lineDone)
    {
        painter->setPen(Qt::red);
        //painter->drawLine(streamThd->pointA[0],streamThd->pointA[1],streamThd->pointB[0],streamThd->pointB[1]);
        for(int i=0;i<streamThd->y_idx.size();i++)
        {
            painter->drawPoint(streamThd->x_idx[i],streamThd->y_idx[i]);
        }
    }
    views().at(0)->update();
    //update();

}
/*
void TrkScene::drawItems(QPainter *painter, int numItems,QGraphicsItem *items[],const QStyleOptionGraphicsItem options[],QWidget *widget)
{

    QGraphicsScene::drawItems(painter,numItems,items,options,widget);
    views().at(0)->update();
    update();

}
*/
/*
void TrkScene::drawForeground(QPainter * painter, const QRectF & rect)
{
    update(sceneRect());
    //views().at(0)->updateScene(sceneRect());

}
*/
void TrkScene::updateFptr(unsigned char * fptr,int fidx)
{
    bgBrush.setTextureImage(QImage(fptr,streamThd->framewidth,streamThd->frameheight,QImage::Format_RGB888));
    frameidx=fidx;
}
int TrkScene::init()
{
    if(streamThd!=NULL)
    {
        while(bb_N<streamThd->bb_N)
        {
            int bb_i=bb_N;
            addADot(0,0);
            dotvec[bb_i]->setVisible(false);
            dotvec[bb_i]->ismovable=false;
            dotvec[bb_i]->isGood=true;
            //dotvec[bb_i]->setVisible(true);
            dotvec[bb_i]->setClr(feat_colos[bb_i%6][0],feat_colos[bb_i%6][1],feat_colos[bb_i%6][2]);
            dotvec[bb_i]->pid=bb_i;
        }
        if(!roidone&&streamThd->roidone)
        {
            for(int i=0;i<streamThd->roi.size();i++)
            {
                cv::Point2i& t = streamThd->roi[i];
                int x = t.x,y=t.y;
                DragDots* newdot = new DragDots(x,y);
                newdot->setClr(0,0,0);
                newdot->pid = roivec.size();
                sprintf(newdot->txt,"%d\0",newdot->pid);
                roivec.push_back(newdot);
            }
            roidone=true;
        }
        return 1;
    }
    return 0;
}
void TrkScene::startTrk(int bb_i)
{
    dotvec[bb_i]->isGood=true;
    dotvec[bb_i]->setVisible(true);
}
void TrkScene::endTrk(int bb_i)
{
    dotvec[bb_i]->setVisible(true);
    dotvec[bb_i]->isGood=false;
}
void TrkScene::clear()
{

    for(int bb_i=0;bb_i<bb_N;bb_i++)
    {
        dotvec[bb_i]->ismovable=false;
        dotvec[bb_i]->isGood=false;
        //dotvec[bb_i]->unlit();
        dotvec[bb_i]->setVisible(false);
    }
    bb_N=0;
    //roidone=false;
}
void TrkScene::startEdit()
{
    if(streamThd!=NULL)
    {
    }
}
void TrkScene::addADot(int x,int y)
{
    if(bb_N<dotvec.size())
    {
        DragDots* newdot= dotvec[bb_N];
        newdot->setClr(feat_colos[bb_N%6][0],feat_colos[bb_N%6][1],feat_colos[bb_N%6][2]);
        newdot->pid=bb_N;
        sprintf(newdot->txt,"%d\0",bb_N);

        newdot->setCoord(width(),height());
        newdot->setVisible(true);
        newdot->setCoord(width()/2,height()/2);
        newdot->setVisible(true);
        update();
    }
    else
    {
        DragDots* newdot = new DragDots(x,y);
        newdot->setClr(feat_colos[bb_N%6][0],feat_colos[bb_N%6][1],feat_colos[bb_N%6][2]);
        newdot->pid=bb_N;
        sprintf(newdot->txt,"%d\0",bb_N);
        newdot->trkscene=this;

        dotvec.push_back(newdot);
        addItem(newdot);
        newdot->setCoord(width(),height());
        newdot->setVisible(true);
        newdot->setCoord(width()/2,height()/2);
        newdot->setVisible(true);
        update();
    }
    bb_N++;

}
void TrkScene::setUpbb()
{
    if(!bbDone&&streamThd!=NULL&&streamThd->persDone)
    {
        std::cout<<"setUPbb"<<std::endl;
        for(int i=0;i<bb_N;i++)
        {
            if(i<bbvec.size())
            {
                BBox* newbb = bbvec[i];
                DragDots* dot = dotvec[i];
                unsigned char r=dot->rgb[0],g=dot->rgb[1],b=dot->rgb[2];
                newbb->setClr(r,g,b);
                newbb->bbid=dot->pid;
                sprintf(newbb->txt,"%d\0",dot->pid);
                newbb->setVisible(false);
            }
            else
            {
                BBox* newbb = new BBox(0,0,0,0);
                DragDots* dot = dotvec[i];
                unsigned char r=dot->rgb[0],g=dot->rgb[1],b=dot->rgb[2];
                newbb->setClr(r,g,b);
                newbb->bbid=dot->pid;
                sprintf(newbb->txt,"%d\0",dot->pid);
                bbvec.push_back(newbb);
                addItem(newbb);
                newbb->setVisible(false);
            }
        }
        std::cout<<"in"<<std::endl;
        for(int i=0;i<dragbbvec.size();i++)
        {
            dragbbvec[i]->setVisible(false);
        }
        bbDone=true;
    }
}
void TrkScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button()==Qt::LeftButton)
    {
        int x = event->scenePos().x(),y=event->scenePos().y();

        if(lineDone<2)
        {
            lineDots[lineDone].setCoord(x,y);
            lineDots[lineDone].ismovable=true;
            lineDots[lineDone].setClr(255,255,255);
            lineDots[lineDone].pid=lineDone;
            sprintf(lineDots[lineDone].txt,"%c\0",lineDone+'A');
            addItem(lineDots+lineDone);
            lineDots[lineDone].setVisible(true);
            lineDone++;
            if(lineDone>=2)
            {
                linepen.setColor(Qt::white);
                linepen.setWidth(1);
            }
        }
    }
    QGraphicsScene::mousePressEvent(event);
}
void TrkScene::dragBBclicked(int pid)
{
    if(streamThd->paused&&lineDone>=2&&pid<bb_N)
    {
        DragDots* dot = dotvec[pid];
        int x=dot->coord[0],y=dot->coord[1],range=2*dot->range;
        std::cout<<"pid:"<<pid<<"|"<<x<<","<<y<<std::endl;
        DragBBox* newbb = new DragBBox(x-range,y-range,x+range,y+range);
        newbb->bbid=pid;
        newbb->setClr(dot->rgb[0],dot->rgb[1],dot->rgb[2]);
        sprintf(newbb->txt,"%d\0",pid);
        dragbbvec.push_back(newbb);
        addItem(newbb);
    }
}
RefScene::RefScene(const QRectF & sceneRect, QObject * parent):QGraphicsScene(sceneRect, parent)
{
    streamThd=NULL;
    trkscene=NULL;
    frameidx=0;
    bb_N=0;


}
RefScene::RefScene(qreal x, qreal y, qreal width, qreal height, QObject * parent):QGraphicsScene( x, y, width, height, parent)
{
    streamThd=NULL;
    trkscene=NULL;
    frameidx=0;
    bb_N=0;


}
void RefScene::init()
{
    if(streamThd!=NULL)
    {
        setSceneRect(0,0,streamThd->slicelen,streamThd->linelen);
        updateFptr(streamThd->sliceptr, streamThd->frameidx);
        txtfont.setPixelSize(10);
        txtfont.setFamily("PreCursive");
        txtfont.setBold(true);
        lvlpen.setWidth(10);
    }
}

void RefScene::drawBackground(QPainter * painter, const QRectF & rect)
{

    if(streamThd!=NULL&&streamThd->lineDone)
    {
        updateFptr(streamThd->sliceptr, streamThd->frameidx);
    }
    painter->setBrush(bgBrush);
    painter->drawRect(rect);
    if(streamThd!=NULL&&streamThd->lineDone)
    {
        int updatepos=streamThd->slicetail;
        if(streamThd->slicetail<streamThd->slicelen)
            updatepos=streamThd->slicetail;
        else
            updatepos=streamThd->slicelen-1;
        for(int i=0;i<updatepos+1;i++)
        {
            for(int j=1;j<streamThd->linelen;j++)
            {
                int value=*(streamThd->sliceGT+j*streamThd->slicelen+i);
                if(value!=0)
                {
                    int dirsgn = ((value>0)-(value<0));
                    int absval=dirsgn*value-1;
                    painter->setPen(QColor(feat_colos[absval%6][0],feat_colos[absval%6][1],feat_colos[absval%6][2],200));
                    painter->setFont(txtfont);

                    painter->drawText(QRectF(i-2,j-2,5,5),Qt::AlignCenter|Qt::AlignCenter|Qt::TextDontClip,QString::number(absval));
                    //painter->drawPoint(i,j);
                    painter->drawLine(i,j,i+dirsgn*20,j);
                    painter->drawLine(i+dirsgn*16,j-4,i+dirsgn*20,j);
                    painter->drawLine(i+dirsgn*16,j+4,i+dirsgn*20,j);
                }
                if(streamThd->persDone)
                {
                    int bbvalue=*(streamThd->sliceBB+j*streamThd->slicelen+i);
                    if(bbvalue!=0)
                    {
                        int dirsgn = ((bbvalue>0)-(bbvalue<0));
                        int absval=dirsgn*bbvalue-1;
                        painter->setPen(QColor(feat_colos[absval%6][0],feat_colos[absval%6][1],feat_colos[absval%6][2],200));
                        painter->setFont(txtfont);
                        painter->drawPoint(i,j);
                    }
                }
            }
        }

        int* gtptr=streamThd->sliceGT;
        int slicelen=streamThd->slicelen;
        for(int bb_i=0;bb_i<bb_N;bb_i++)
        {
            Anchor* a1=anchor1[bb_i];
            Anchor* a2=anchor2[bb_i];
            painter->setPen(QColor(a1->rgb[0],a1->rgb[1],a1->rgb[2]));
            int x1=a1->coord[0],x2=a2->coord[0],y=a1->coord[1];
            if(x1>x2)std::swap(x1,x2);
            //std::cout<<"drawline"<<std::endl;
            painter->drawLine(x1,0,x1,height());
            painter->drawLine(x2,0,x2,height());

            int count=0;

            for(int i=0;i<streamThd->linelen;i++)
                for(int j=x1;j<x2;j++)
                {
                    count+=(gtptr[i*slicelen+j]!=0);
                }
            int level = (height()-(count+0.0)/100*height());
            painter->setPen(QPen(QColor(a1->rgb[0],a1->rgb[1],a1->rgb[2],150),20,Qt::SolidLine,Qt::FlatCap));
            painter->drawLine(x1,level,x2,level);
            painter->setPen(QColor(255,255,255));
            painter->setFont(QFont("Precursive",20,1));
            painter->drawText(QRectF(x1,level-10,x2-x1,20),Qt::AlignCenter|Qt::AlignCenter|Qt::TextDontClip,QString::number(count));

        }
    }
    views().at(0)->update();
}

void RefScene::updateFptr(unsigned char * fptr,int fidx)
{
    bgBrush.setTextureImage(QImage(fptr,streamThd->slicelen,streamThd->linelen,QImage::Format_RGB888));
    frameidx=fidx;

}
void RefScene::clear()
{
    bgBrush.setStyle(Qt::NoBrush);
    frameidx=0;
}
void RefScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{

    if(event->button()==Qt::RightButton)
    {
        int x = event->scenePos().x(),y=event->scenePos().y();
        Anchor* aanchor=  new Anchor(x,y);
        aanchor->setClr(feat_colos[bb_N%6][0],feat_colos[bb_N%6][1],feat_colos[bb_N%6][2]);
        aanchor->pid=bb_N;
        anchor1.push_back(aanchor);
        addItem(aanchor);

        aanchor=  new Anchor(x,y);
        aanchor->setClr(feat_colos[bb_N%6][0],feat_colos[bb_N%6][1],feat_colos[bb_N%6][2]);
        aanchor->pid=bb_N;
        anchor2.push_back(aanchor);
        addItem(aanchor);
        bb_N++;
        std::cout<<bb_N<<std::endl;
        pressed=true;
    }
    QGraphicsScene::mousePressEvent(event);
}
void RefScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if ((event->buttons() & Qt::RightButton)&&pressed)
    {
        int x = event->scenePos().x(),y=event->scenePos().y();
        anchor2[bb_N-1]->setX(x);
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void RefScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button()==Qt::RightButton&&pressed)
    {
        int x = event->scenePos().x(),y=event->scenePos().y();
        anchor2[bb_N-1]->setX(x);
        pressed=false;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}
