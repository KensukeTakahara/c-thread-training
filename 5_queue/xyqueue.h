#ifndef XYQUEUE_H
#define XYQUEUE_H

#include <stdlib.h>

typedef struct XYQueue_ XYQueue;

extern XYQueue *XYQueueCreate(size_t sz);
extern void XYQueueDestroy(XYQueue *que);
extern size_t XYQueueGetSize(XYQueue *que);
extern size_t XYQueueGetCount(XYQueue *que);
extern size_t XYQueueGetFreeCount(XYQueue *que);
extern int XYQueueAdd(XYQueue *que, double x, double y);
extern int XYQueueGet(XYQueue *que, double *x, double *y);
extern int XYQueueWait(XYQueue *que, long msec);

#endif  // XYQUEUE_H
