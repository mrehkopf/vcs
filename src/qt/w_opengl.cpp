/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * Uses Qt's OpenGL implementation to draw the contents of the VCS frame buffer
 * to screen. I.e. just draws a full-window textured quad.
 *
 * NOTE: Uses deprecated OpenGL, since I can only run the program in a virtual
 * machine with limited GPU support.
 *
 */

#ifdef USE_OPENGL

#include <QCoreApplication>
#include <QOpenGLWidget>
#include <QMatrix4x4>
#include "../scaler.h"
#include "../common.h"
#include "w_opengl.h"

GLuint FRAMEBUFFER_TEXTURE;

OGLWidget::OGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    return;
}

void OGLWidget::initializeGL()
{
    DEBUG(("Initializing OpenGL..."));

    this->initializeOpenGLFunctions();

    DEBUG(("OpenGL is reported to be version %s.", glGetString(GL_VERSION)));

    this->glDisable(GL_DEPTH_TEST);
    this->glEnable(GL_TEXTURE_2D);

    // Initialize the texture into which we'll stream the renderer's frame buffer.
    this->glGenTextures(1, &FRAMEBUFFER_TEXTURE);
    this->glBindTexture(GL_TEXTURE_2D, FRAMEBUFFER_TEXTURE);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    k_assert(!this->glGetError(), "OpenGL initialization failed.");

    return;
}

void OGLWidget::resizeGL(int w, int h)
{
    QMatrix4x4 m;
    m.setToIdentity();
    m.ortho(0, w, h, 0, -1, 1);

    this->glLoadMatrixf(m.constData());

    return;
}

void OGLWidget::paintGL()
{
    const resolution_s r = ks_output_resolution();
    const u8 *const fb = ks_scaler_output_as_raw_ptr();
    if (fb == nullptr)
    {
        return;
    }

    // Update the OpenGL texture with the current contents of the frame buffer.
    /// FIXME: Would it be better to use glTexSubImage2D()?
    this->glBindTexture(GL_TEXTURE_2D, FRAMEBUFFER_TEXTURE);
    this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, r.w, r.h, 0, GL_BGRA, GL_UNSIGNED_BYTE, fb);

    // Simple textured screen quad.
    glBegin(GL_TRIANGLES);
        glTexCoord2i(0, 0); glVertex2i(0,             0);
        glTexCoord2i(0, 1); glVertex2i(0,             this->height());
        glTexCoord2i(1, 1); glVertex2i(this->width(), this->height());

        glTexCoord2i(1, 1); glVertex2i(this->width(), this->height());
        glTexCoord2i(1, 0); glVertex2i(this->width(), 0);
        glTexCoord2i(0, 0); glVertex2i(0,             0);
    glEnd();

    this->glFlush();

    return;
}

#endif
