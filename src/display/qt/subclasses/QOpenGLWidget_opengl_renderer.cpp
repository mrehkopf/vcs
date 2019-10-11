/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * Uses Qt's OpenGL implementation to draw the contents of the VCS frame buffer
 * to screen. I.e. just draws a full-window textured quad.
 *
 */

#include <QCoreApplication>
#include <QOpenGLWidget>
#include <QMatrix4x4>
#include "display/qt/subclasses/QOpenGLWidget_opengl_renderer.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "scaler/scaler.h"

// The texture into which we'll stream the captured frames.
GLuint FRAMEBUFFER_TEXTURE;

// The texture in which we'll display the current output overlay, if any.
GLuint OVERLAY_TEXTURE;

// A function that returns the current overlay as a QImage.
std::function<QImage()> OVERLAY_AS_QIMAGE_F;

OGLWidget::OGLWidget(std::function<QImage()> overlay_as_qimage, QWidget *parent) : QOpenGLWidget(parent)
{
    OVERLAY_AS_QIMAGE_F = overlay_as_qimage;

    return;
}

void OGLWidget::initializeGL()
{
    DEBUG(("Initializing OpenGL..."));

    this->initializeOpenGLFunctions();

    DEBUG(("OpenGL is reported to be version %s.", glGetString(GL_VERSION)));

    this->glDisable(GL_DEPTH_TEST);
    this->glEnable(GL_TEXTURE_2D);

    this->glGenTextures(1, &FRAMEBUFFER_TEXTURE);
    this->glBindTexture(GL_TEXTURE_2D, FRAMEBUFFER_TEXTURE);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    this->glGenTextures(1, &OVERLAY_TEXTURE);
    this->glBindTexture(GL_TEXTURE_2D, OVERLAY_TEXTURE);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // For alpha-blending the overlay image.
    this->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    this->glEnable(GL_BLEND);

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
    // Draw the output frame.
    const resolution_s r = ks_output_resolution();
    const u8 *const fb = ks_scaler_output_as_raw_ptr();
    if (fb != nullptr)
    {
        this->glDisable(GL_BLEND);

        this->glBindTexture(GL_TEXTURE_2D, FRAMEBUFFER_TEXTURE);
        this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, r.w, r.h, 0, GL_BGRA, GL_UNSIGNED_BYTE, fb);

        glBegin(GL_TRIANGLES);
            glTexCoord2i(0, 0); glVertex2i(0,             0);
            glTexCoord2i(0, 1); glVertex2i(0,             this->height());
            glTexCoord2i(1, 1); glVertex2i(this->width(), this->height());

            glTexCoord2i(1, 1); glVertex2i(this->width(), this->height());
            glTexCoord2i(1, 0); glVertex2i(this->width(), 0);
            glTexCoord2i(0, 0); glVertex2i(0,             0);
        glEnd();
    }

    // Draw the overlay, if any.
    const QImage image = OVERLAY_AS_QIMAGE_F();
    if (!image.isNull())
    {
        this->glEnable(GL_BLEND);

        this->glBindTexture(GL_TEXTURE_2D, OVERLAY_TEXTURE);
        this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width(), image.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, image.constBits());

        glBegin(GL_TRIANGLES);
            glTexCoord2i(0, 0); glVertex2i(0,             0);
            glTexCoord2i(0, 1); glVertex2i(0,             this->height());
            glTexCoord2i(1, 1); glVertex2i(this->width(), this->height());

            glTexCoord2i(1, 1); glVertex2i(this->width(), this->height());
            glTexCoord2i(1, 0); glVertex2i(this->width(), 0);
            glTexCoord2i(0, 0); glVertex2i(0,             0);
        glEnd();
    }

    this->glFlush();

    return;
}
