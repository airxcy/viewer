#include "Qts/mainwindow.h"
#include "Qts/streamthread.h"
#include "Qts/viewqt.h"
#include "Qts/modelsqt.h"

//#include <direct.h>

#include <QtWidgets>
#include <QSizePolicy>
#include <iostream>
#include <QPalette>
#include <QKeySequence>
#include <QFontDatabase>
#include <QStringList>
//! [1]
char cbuff[200];
MainWindow::MainWindow()
{
    cWidget = new QWidget(this);
    setCentralWidget(cWidget);
    updaderidx=0;
    streamThd = new StreamThread(this);
    streamThd->mwindow=this;
    setStyleSheet("QWidget { background-color: rgb(105,210,231); }");
    cWidget->setStyleSheet("QWidget { background-color: rgb(105,210,231); }");
    setupLayout();
    makeConns();
    refscene=NULL,trkscene=NULL,featscene=NULL;
    //setWindowFlags(Qt::FramelessWindowHint);
    setStyleSheet(" QPushButton:disabled {background: rgba(0,0,0,100)}");
    setFixedSize(cWidget->minimumSize());
    //cWidget->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
    move(100, 0);
    firsttime=true;

}
void MainWindow::setupLayout()
{
    /** add font **/
    int fontid = QFontDatabase::addApplicationFont(":/fonts/Minecrafter.Alt.ttf");
    fontid = QFontDatabase::addApplicationFont(":/fonts/DJBAlmostPerfect.ttf");
    fontid = QFontDatabase::addApplicationFont(":/fonts/Precursive_1_FREE.otf");
    QStringList fontstr = QFontDatabase::applicationFontFamilies(fontid);
    for(int i =0;i<fontstr.length();i++)
    {
        std::cout<<fontstr.at(i).toStdString()<<std::endl;
    }

    /** layout **/
    layout=new QGridLayout(cWidget);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    cWidget->setLayout(layout);
    cWidget->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    int frameGridSize=1;

    /** scroll area **/
    scrollarea = new QScrollArea(cWidget);
    scrollarea->setFixedSize(500,240);
    layout->addWidget(scrollarea,0,0,1,frameGridSize,Qt::AlignCenter);
    scrollwidget = new QWidget(scrollarea);
    scrollarea->setWidget(scrollwidget);
    layout1 =new QGridLayout(scrollwidget);
    scrollwidget->setLayout(layout1);
    layout1->setSizeConstraint(QLayout::SetFixedSize);
    scrollwidget->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);


    /** views **/
    defaultscene = new DefaultScene(0, 0, 440, 240);
    gview = new GraphicsView(defaultscene,scrollwidget);
    gview->setFixedSize(defaultscene->width()+2,defaultscene->height()+2);
    gview->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    refview = new GraphicsView(new QGraphicsScene(0, 0, 440, 240),scrollwidget);
    refview->setFixedSize(defaultscene->width()+2,defaultscene->height()+2);
    refview->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    featview = new GraphicsView(new QGraphicsScene(0, 0, 440, 240),scrollwidget);
    featview->setFixedSize(defaultscene->width()+2,defaultscene->height()+2);
    featview->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    layout1->addWidget(gview,0,0,1,frameGridSize,Qt::AlignTop);
    layout1->addWidget(refview,0,frameGridSize,1,frameGridSize,Qt::AlignTop);
    layout1->addWidget(featview,0,frameGridSize*2,1,frameGridSize,Qt::AlignTop);
    layout1->setMargin(0);
    layout1->setSpacing(0);

    scrollarea->setFixedHeight(240+20);




    /** buttons **/
    blayout0 = new QBoxLayout(QBoxLayout::LeftToRight,cWidget);
    layout->addLayout(blayout0,1,0,1,frameGridSize);
    blayout = new QBoxLayout(QBoxLayout::LeftToRight,cWidget);

    QString btnstyle = "QPushButton { background: rgba(243,134,48,100); color:rgba(243,134,48); padding: 3px;} QPushButton:disabled{background: rgba(0,0,0,50)}";
    QFont btnfont("PreCursive",20);
    btnfont.setBold(true);

    int btnHt=30,startidx=0;
    editTag = new QPushButton(cWidget);
    editTag->setStyleSheet(btnstyle+"QPushButton { background: rgba(255,0,255,200);}");
    editTag->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    editTag->setText("▶ǁ");
    editTag->setShortcut(QKeySequence(" "));
    editTag->setFont(btnfont);
    blayout0->addWidget(editTag);
    relabelBtn = new QPushButton(cWidget);
    relabelBtn->setStyleSheet(btnstyle+"QPushButton { background: rgba(255,0,255,200);}");
    relabelBtn->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    relabelBtn->setText("replay");
    relabelBtn->setShortcut(QKeySequence("p"));
    relabelBtn->setFont(btnfont);
    blayout0->addWidget(relabelBtn);
    roiBtin = new QPushButton(cWidget);
    roiBtin->setStyleSheet(btnstyle);
    roiBtin->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    roiBtin->setText("setRoi");
    roiBtin->setFont(btnfont);
    blayout0->addWidget(roiBtin);
    framedoneBtn = new QPushButton(cWidget);
    framedoneBtn->setStyleSheet(btnstyle);
    framedoneBtn->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    framedoneBtn->setText("set mark");
    framedoneBtn->setFont(btnfont);
    blayout0->addWidget(framedoneBtn);

    layout->addLayout(blayout,2,0,1,frameGridSize);
    slider =new QSlider(Qt::Horizontal,this);
    slider->setMaximum(100);
    slider->setTickPosition(QSlider::TicksBelow);
    slider->setEnabled(false);
    slider->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
    blayout->addWidget(slider);
    label=new QLabel(this);
    label->setText("0/0");
    label->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    blayout->addWidget(label);

    //blayout0->addStretch();
    btnfont.setPixelSize(20);
    startidx=frameGridSize;
    btnHt=40;

    scrollarea->horizontalScrollBar()->setValue(0);
    scrollarea->verticalScrollBar()->setValue(0);
}
void MainWindow::makeConns()
{
    connect(defaultscene,SIGNAL(clicked(QGraphicsSceneMouseEvent *)),this,SLOT(gviewClicked(QGraphicsSceneMouseEvent *)));
    connect(streamThd,SIGNAL(initSig()),this,SLOT(initUI()),Qt::BlockingQueuedConnection);
}
void MainWindow::gviewClicked(QGraphicsSceneMouseEvent * event)
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open vid"), "/Users/xcy/Documents/CVProject/data/labelCross", tr("Vid Files (*.avi *.mp4 *.mkv *.mts)"));
    if(!fileName.isEmpty())
    {
        streamThd->streamStart(fileName.toStdString());
    }
}

void MainWindow::initUI()
{
    //std::cout<<"init UI"<<std::endl;
    int fw=streamThd->framewidth,fh=streamThd->frameheight;

    if(firsttime)
    {
        QDesktopWidget *dwsktopwidget = QApplication::desktop();
        deskrect = dwsktopwidget->availableGeometry();
        QRect screenrect = dwsktopwidget->screenGeometry();
        int scrcount = dwsktopwidget->screenCount();

        trkscene = new TrkScene(0, 0, fw, fh);
        trkscene->streamThd=streamThd;
        streamThd->trkscene=trkscene;
        double scalefactor = 1;//2.0/3.0;
        gview->scale(scalefactor,scalefactor);
        gview->setFixedSize(fw*scalefactor+3,fh*scalefactor+3);
        gview->setScene(trkscene);


        scrollwidget->setFixedHeight(layout1->minimumSize().height());
        int minh=std::min(scrollwidget->height()+13,deskrect.height()-200);
        int minw=std::min(scrollwidget->width()+13,deskrect.width()*2-100);
        scrollarea->setFixedHeight(minh);
        scrollarea->setFixedWidth(minw);
        scrollarea->horizontalScrollBar()->setValue(0);
        scrollarea->verticalScrollBar()->setValue(0);
        slider->setMaximum(streamThd->maxskeyframe);

        setWindowTitle(streamThd->vidid);
        move(50, 0);
        connect(editTag,SIGNAL(clicked()),this,SLOT(resume()));
        connect(streamThd,SIGNAL(stepReached()),this,SLOT(stepReached()),Qt::BlockingQueuedConnection);
        connect(framedoneBtn,SIGNAL(clicked()),this,SLOT(frameDone()));
        connect(relabelBtn,SIGNAL(clicked()),this,SLOT(relabel()));
        if(!streamThd->roidone)
            connect(roiBtin,SIGNAL(clicked()),this,SLOT(setRoi()));

        trkscene->updateFptr(streamThd->frameptr,streamThd->frameidx);



        firsttime=false;
    }

    setUpSlice();
    sprintf(cbuff,"%06d/%06d",0,streamThd->maxframe);
    slider->setValue(0);
    label->setText(cbuff);


    trkscene->init();
    editTag->setEnabled(true);
    relabelBtn->setEnabled(true);
    framedoneBtn->setEnabled(true);

}
void MainWindow::setUpSlice()
{

    std::cout<<streamThd->slicelen<<","<<streamThd->linelen<<std::endl;
    int slicelen = streamThd->slicelen,linelen = streamThd->linelen;

    if(refscene==NULL)
    {
        refscene = new RefScene(0,0,slicelen,linelen);
        refscene->streamThd=streamThd;
        streamThd->refscene=refscene;
        refview->setScene(refscene);

    }

    if(featscene==NULL)
    {
        featscene = new FeatScene(0,0,slicelen,linelen);
        featscene->streamThd=streamThd;
        featview->setScene(featscene);

    }

    refscene->init();
    featscene->init();
    refview->setFixedSize(refscene->width()+2,refscene->height()+2);
    featview->setFixedSize(featscene->width()+2,featscene->height()+2);

    scrollwidget->setFixedHeight(layout1->minimumSize().height());
    int minh=std::min(scrollwidget->height()+13,deskrect.height()-200);
    int minw=std::min(scrollwidget->width()+13,deskrect.width()*2-100);
    scrollarea->setFixedHeight(minh);
    scrollarea->setFixedWidth(minw);
    cWidget->setFixedSize(layout->minimumSize());
    setFixedSize(cWidget->minimumSize());
}
void MainWindow::resume()
{
    if(streamThd!=NULL&&streamThd->paused)
    {
        if(trkscene->lineDone>=2)
        {
            std::cout<<"ressume"<<std::endl;
            if(refscene!=NULL)
                refscene->clear();
            if(featscene!=NULL)
                featscene->clear();
            streamThd->setUpLine(trkscene->lineDots[0].coord[0],
                    trkscene->lineDots[0].coord[1],
                    trkscene->lineDots[1].coord[0],
                    trkscene->lineDots[1].coord[1]);
            setUpSlice();
        }
            streamThd->pause=false;
            gview->update();
            streamThd->cv0.wakeAll();
    }
    else if(streamThd!=NULL&&!streamThd->paused)
    {
            streamThd->pause=true;
    }
}
void MainWindow::setVidPos()
{
    if(streamThd!=NULL&&streamThd->inited)
    {
        streamThd->setPos(slider->value());
    }
}
void MainWindow::stepReached()
{
    if(streamThd!=NULL)
    {
        trkscene->startEdit();
        int pos = streamThd->frameidx/streamThd->delay;
        slider->setValue(pos);
        sprintf(cbuff,"%06d/%06d",streamThd->frameidx,streamThd->maxframe);
        label->setText(cbuff);
    }
}
void MainWindow::setRoi()
{
    if(streamThd!=NULL&&!streamThd->roidone&&trkscene->roivec.size()>0)
    {
        streamThd->getroi();
        trkscene->roidone=true;
    }
}
void MainWindow::frameDone()
{
    if(streamThd!=NULL&&streamThd->frameidx>20&&streamThd->paused)
    {
        streamThd->nextframeidx=streamThd->frameidx-15;
        framedoneBtn->setEnabled(false);
        trkscene->clear();
        streamThd->restart=true;
        streamThd->pause=false;
        streamThd->cv0.wakeAll();
    }
}
void MainWindow::relabel()
{
    if(streamThd!=NULL)//&&streamThd->isTagging)
    {
        refscene->clear();
        streamThd->restart=true;
        streamThd->pause=false;
        streamThd->cv0.wakeAll();
    }
}
