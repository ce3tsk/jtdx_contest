// -*- Mode: C++ -*-
#ifndef GETFILE_H
#define GETFILE_H
#include <QString>
#include <QFile>
#include <QDebug>

void getfile(QString fname, int ntrperiod);
float gran();

extern "C" {
void wav12_(short d2[], short d1[], int* nbytes, short* nbitsam2);
}


#endif // GETFILE_H
