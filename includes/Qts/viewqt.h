#ifndef VIEWQT
#define VIEWQT

#include <QGraphicsScene>
#include <QGraphicsView>
#include <vector>
#include <QThread>
#include "Qts/modelsqt.h"
class StreamThread;
class Dot;
class RefScene;
class DragVtx;
class DragDots;
class DefaultScene : public QGraphicsScene
{
    Q_OBJECT
public:
    DefaultScene(const QRectF & sceneRect, QObject * parent = 0):QGraphicsScene(sceneRect, parent)
    {

    }
    DefaultScene(qreal x, qreal y, qreal width, qreal height, QObject * parent = 0):QGraphicsScene( x, y, width, height, parent)
    {

    }
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event ) Q_DECL_OVERRIDE;
    virtual void drawBackground(QPainter * painter, const QRectF & rect) Q_DECL_OVERRIDE;
signals:
    void clicked(QGraphicsSceneMouseEvent * event);
};
class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsView(QGraphicsScene *scene) : QGraphicsView(scene)
    {
        setMouseTracking(true);
        setAcceptDrops(true);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    }
    GraphicsView(QGraphicsScene *scene, QWidget * parent = 0) : QGraphicsView(scene,parent)
    {
        setMouseTracking(true);
        setAcceptDrops(true);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    }
};

class TrkScene :public QGraphicsScene
{
    Q_OBJECT
public:
    TrkScene(const QRectF & sceneRect, QObject * parent = 0);
    TrkScene(qreal x, qreal y, qreal width, qreal height, QObject * parent = 0);

    StreamThread* streamThd;
    RefScene* refscene;
    int bb_N,focusidx,pendingN;
    std::vector<DragDots *> dotvec;
    std::vector<DragBBox *> dragbbvec;
    std::vector<BBox *> bbvec;
    QBrush bgBrush;
    QBrush fgBrush;
    QFont txtfont;
    QPen linepen;

    std::vector<DragDots *> roivec;
    DragDots lineDots[2];
    int frameidx,lineDone;
    bool roidone;
    bool bbDone;
    virtual void drawBackground(QPainter * painter, const QRectF & rect) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event)Q_DECL_OVERRIDE;
    //virtual void drawItems(QPainter *painter, int numItems,QGraphicsItem *items[],const QStyleOptionGraphicsItem options[],QWidget *widget)Q_DECL_OVERRIDE;
    //virtual void drawForeground(QPainter * painter, const QRectF & rect) Q_DECL_OVERRIDE;
    int init();
    void updateFptr(unsigned char * fptr,int fidx);
    void clear();
public slots:
    void dragBBclicked(int pid);
    void startEdit();
    void setUpbb();

    void addADot(int x,int y);
    void startTrk(int bb_i);
    void endTrk(int bb_i);
signals:
    void litDot(int bb_i);
};

class RefScene :public QGraphicsScene
{
    Q_OBJECT
 public:
    RefScene(const QRectF & sceneRect, QObject * parent = 0);
    RefScene(qreal x, qreal y, qreal width, qreal height, QObject * parent = 0);
    StreamThread* streamThd;
    TrkScene* trkscene;
    int frameidx;
    QBrush bgBrush;
    QFont txtfont;
    QPen lvlpen;
    int bb_N;
    bool pressed;
    std::vector<Anchor *> anchor1,anchor2;
    void init();
    void updateFptr(unsigned char * fptr,int fidx);
    virtual void drawBackground(QPainter * painter, const QRectF & rect) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event)Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) Q_DECL_OVERRIDE;
    void clear();
};

#endif // VIEWQT

