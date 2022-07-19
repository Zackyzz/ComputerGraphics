#include "ads/glos.h"
#include "ads/gl.h"
#include "ads/glu.h"
#include "ads/glut.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#pragma pack(1)
struct TGAHEADER
{
    GLbyte identsize;              // Size of ID field that follows header (0)
    GLbyte colorMapType;           // 0 = None, 1 = paletted
    GLbyte imageType;              // 0 = none, 1 = indexed, 2 = rgb, 3 = grey, +8=rle
    unsigned short colorMapStart;  // First colour map entry
    unsigned short colorMapLength; // Number of colors
    unsigned char colorMapBits;    // bits per palette entry
    unsigned short xstart;         // image x origin
    unsigned short ystart;         // image y origin
    unsigned short width;          // width in pixels
    unsigned short height;         // height in pixels
    GLbyte bits;                   // bits per pixel (8 16, 24, 32)
    GLbyte descriptor;             // image descriptor
};
#pragma pack(8)

static GLbyte *pImage = NULL;
static GLbyte *sImage = NULL;
static GLint iWidth, iHeight, iComponents;
static GLenum eFormat;
static int level = 512;
static int step = 256;
static int mode = 0;

GLbyte *gltLoadTGA(const char *szFileName, GLint *iWidth, GLint *iHeight, GLint *iComponents, GLenum *eFormat)
{
    FILE *pFile;
    TGAHEADER tgaHeader;
    unsigned long lImageSize;
    short sDepth;
    GLbyte *pBits = NULL;

    *iWidth = 0;
    *iHeight = 0;
    *eFormat = GL_BGR_EXT;
    *iComponents = GL_RGB8;

    pFile = fopen(szFileName, "rb");
    if (pFile == NULL)
        return NULL;

    fread(&tgaHeader, sizeof(TGAHEADER), 1, pFile);

    *iWidth = tgaHeader.width;
    *iHeight = tgaHeader.height;
    sDepth = tgaHeader.bits / 8;

    lImageSize = tgaHeader.width * tgaHeader.height * sDepth;
    pBits = (GLbyte *)malloc(lImageSize * sizeof(GLbyte));
    if (pBits == NULL)
        return NULL;

    if (fread(pBits, lImageSize, 1, pFile) != 1)
    {
        free(pBits);
        return NULL;
    }

    switch (sDepth)
    {
    case 3:
        *eFormat = GL_BGR_EXT;
        *iComponents = GL_RGB8;
        break;
    case 4:
        *eFormat = GL_BGRA_EXT;
        *iComponents = GL_RGBA8;
        break;
    case 1:
        *eFormat = GL_LUMINANCE;
        *iComponents = GL_LUMINANCE8;
        break;
    };

    fclose(pFile);
    return pBits;
}

void SetupRC(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    pImage = gltLoadTGA("horse.tga", &iWidth, &iHeight, &iComponents, &eFormat);
    sImage = gltLoadTGA("lena.tga", &iWidth, &iHeight, &iComponents, &eFormat);
}

void ShutdownRC(void)
{
    free(pImage);
    free(sImage);
}

int get_step_size(int level)
{
    int log_level = log2(1 + log2(level));
    return (int)pow(2, log_level);
}

void RenderScene(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glRasterPos2i(0, 0);

    GLbyte pattern[iWidth * iHeight];
    int step_size = get_step_size(level);
    switch (mode)
    {
    case 0:
        for (int i = 0; i < iHeight; i++)
        {
            for (int j = 0; j < iWidth; j++)
            {
                if (i % level == 0)
                {
                    pattern[(i * 512) + j] = 1;
                }
                else
                {
                    pattern[(i * 512) + j] = 0;
                }
            }
        }
        break;

    case 1:
        for (int i = 0; i < iHeight; i++)
        {
            for (int j = 0; j < iWidth; j++)
            {
                if (j % level == 0)
                {
                    pattern[(i * 512) + j] = 1;
                }
                else
                {
                    pattern[(i * 512) + j] = 0;
                }
            }
        }
        break;
    case 2:
        for (int i = 0; i < iHeight; i += step_size)
        {
            for (int j = 0; j < iWidth; j += step_size)
            {
                for (int a = 0; a < step_size; a++)
                {
                    for (int b = 0; b < step_size; b++)
                    {
                        if (i % level == 0 && j % level == 0)
                            pattern[((i + a) * 512) + j + b] = 1;
                        else
                            pattern[((i + a) * 512) + j + b] = 0;
                    }
                }
            }
        }
        break;
    case 3:
        for (int i = 0; i < iHeight; i++)
        {
            for (int j = 0; j < iWidth; j++)
            {
                if (i < step && j < step && i > iHeight - step && j > iWidth - step)
                    pattern[(i * 512) + j] = 1;
                else
                    pattern[(i * 512) + j] = 0;
            }
        }
        break;
    }

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilMask(GL_TRUE);
    glDrawPixels(iWidth, iHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, pattern);
    glStencilMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, 0, 1);
    glDrawPixels(iWidth, iHeight, eFormat, GL_UNSIGNED_BYTE, pImage);
    glStencilFunc(GL_EQUAL, 1, 1);
    glDrawPixels(iWidth, iHeight, eFormat, GL_UNSIGNED_BYTE, sImage);

    glutSwapBuffers();
}

void ChangeSize(int w, int h)
{
    if (h == 0)
        h = 1;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluOrtho2D(0.0f, (GLfloat)w, 0.0, (GLfloat)h);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'w':
        if (mode == 3)
        {
            step -= 16;
            if (step < 256)
                step = 256;
        }
        else
        {
            level >>= 1;
            if (level == 0)
                level = 1;
        }
        glutPostRedisplay();
        break;
    case 's':
        if (mode == 3)
        {
            step += 16;
            if (step > 512)
                step = 512;
        }
        else
        {
            level <<= 1;
            if (level > 512)
                level = 512;
        }
        glutPostRedisplay();
        break;
    case 27:
        ShutdownRC();
        exit(0);
    }
}

void ProcessMenu(int value)
{
    level = 512;
    step = 256;
    mode = value;
    glutPostRedisplay();
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GL_DOUBLE);
    glutInitWindowSize(512, 512);
    glutCreateWindow("Dissolving Images");
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);

    glutKeyboardFunc(keyboard);
    glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("Mode H", 0);
    glutAddMenuEntry("Mode V", 1);
    glutAddMenuEntry("Mode All", 2);
    glutAddMenuEntry("Mode IO", 3);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    SetupRC();
    glutMainLoop();
    ShutdownRC();

    return 0;
}
