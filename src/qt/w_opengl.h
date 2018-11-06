/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef OGL_WIDGET_H
#define OGL_WIDGET_H

#include <QOpenGLFunctions_1_2>
#include <QOpenGLWidget>
#include <QWidget>
#include "../types.h"

/// NOTE: Uses deprecated OpenGL, since I can only run the program in a virtual
/// machine with limited GPU support.
class OGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_1_2
{
    Q_OBJECT

public:
    explicit OGLWidget(QWidget *parent = 0);

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
};

#endif
