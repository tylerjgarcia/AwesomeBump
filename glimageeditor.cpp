/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "glimageeditor.h"



GLImage::GLImage(QWidget *parent)
    : QGLWidget(QGLFormat::defaultFormat(), parent)

{
    bShadowRender         = false;
    bRecalculateOcclusion = false;
    bSkipProcessing       = false;
    conversionType        = CONVERT_NONE;

    fboRatio = 1;

    // initialize position of the corners
    cornerPositions[0] = QVector2D(-0.0,-0);
    cornerPositions[1] = QVector2D( 1,-0);
    cornerPositions[2] = QVector2D( 1, 1);
    cornerPositions[3] = QVector2D(-0, 1);
    draggingCorner = -1;
    gui_perspective_mode = 1;
    gui_seamless_mode    = 0;
    setCursor(Qt::OpenHandCursor);
    cornerCursors[0] = QCursor(QPixmap(":/content/corner1.png"));
    cornerCursors[1] = QCursor(QPixmap(":/content/corner2.png"));
    cornerCursors[2] = QCursor(QPixmap(":/content/corner3.png"));
    cornerCursors[3] = QCursor(QPixmap(":/content/corner4.png"));
    setMouseTracking(true);
}
//! [0]

//! [1]
GLImage::~GLImage()
{
    glDeleteBuffers(sizeof(vbos)/sizeof(GLuint), &vbos[0]);
    delete program;
}
//! [1]

//! [2]
QSize GLImage::minimumSizeHint() const
{
    return QSize(360, 360);

}
//! [2]

//! [3]
QSize GLImage::sizeHint() const
//! [3] //! [4]
{
    return QSize(500, 400);
}


//! [6]
void GLImage::initializeGL()
{

    initializeOpenGLFunctions();
    qglClearColor(QColor::fromCmykF(0.79, 0.79, 0.79, 0.0).dark());
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);


    qDebug() << "Loading filters (fragment shader)";
    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceFile(":/content/filters.vert");
    if (!vshader->log().isEmpty()) qDebug() << vshader->log();
    else qDebug() << "done";

    qDebug() << "Loading filters (vertex shader)";
    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceFile(":/content/filters.frag");
    if (!fshader->log().isEmpty()) qDebug() << fshader->log();
    else qDebug() << "done";

    program = new QOpenGLShaderProgram(this);
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("positionIn", PROGRAM_VERTEX_ATTRIBUTE);
    program->link();

    program->bind();
    program->setUniformValue("layerA" , 0);
    program->setUniformValue("layerB" , 1);
    program->setUniformValue("layerC" , 2);

    makeScreenQuad();
    subroutines["mode_normal_filter"]         = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_filter" );
    subroutines["mode_overlay_filter"]        = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_overlay_filter" );
    subroutines["mode_invert_filter"]         = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_invert_filter" );
    subroutines["mode_gauss_filter"]          = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_gauss_filter" );
    subroutines["mode_seamless_filter"]       = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_seamless_filter" );
    subroutines["mode_dgaussians_filter"]     = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_dgaussians_filter" );
    subroutines["mode_constrast_filter"]      = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_constrast_filter" );
    subroutines["mode_small_details_filter"]  = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_small_details_filter" );
    subroutines["mode_gray_scale_filter"]     = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_gray_scale_filter" );
    subroutines["mode_medium_details_filter"] = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_medium_details_filter" );
    subroutines["mode_height_to_normal"]      = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_height_to_normal" );
    subroutines["mode_sharpen_blur"]          = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_sharpen_blur" );
    subroutines["mode_normals_step_filter"]   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normals_step_filter" );
    subroutines["mode_invert_components_filter"]= glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_invert_components_filter" );
    subroutines["mode_normal_to_height"]        = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_to_height" );
    subroutines["mode_sobel_filter"]            = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_sobel_filter" );
    subroutines["mode_normal_expansion_filter"] = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_expansion_filter" );
    subroutines["mode_normalize_filter"]        = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normalize_filter" );
    subroutines["mode_smooth_filter"]           = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_smooth_filter" );
    subroutines["mode_occlusion_filter"]        = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_occlusion_filter" );
    subroutines["mode_combine_normal_height_filter"]= glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_combine_normal_height_filter" );
    subroutines["mode_perspective_transform_filter"]= glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_perspective_transform_filter" );


}
//! [6]

//! [7]
void GLImage::paintGL()
{
    resizeGL(width(),height());
    render();
    //emit rendered();
}



void GLImage::render(){

    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glVertexAttribPointer(PROGRAM_VERTEX_ATTRIBUTE,3,GL_FLOAT,GL_FALSE,sizeof(float)*3,(void*)0);

    QGLFramebufferObject* activeFBO     = activeImage->fbo;
    QGLFramebufferObject* activeAuxFBO  = activeImage->aux_fbo;
    QGLFramebufferObject* activeAux2FBO = activeImage->aux2_fbo;

    if(!bSkipProcessing == true){


    // resizing the FBOs in case of convertion procedure
    switch(conversionType){
        case(CONVERT_FROM_H_TO_N):
        FBOImages::resize(targetImageNormal->ref_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageNormal->fbo      ,activeImage->ref_fbo);
        FBOImages::resize(targetImageNormal->aux_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageNormal->aux2_fbo ,activeImage->ref_fbo);
        activeFBO     = targetImageNormal->fbo;
        activeAuxFBO  = targetImageNormal->aux_fbo;
        activeAux2FBO = targetImageNormal->aux2_fbo;
        break;
        case(CONVERT_FROM_N_TO_H):
        FBOImages::resize(targetImageHeight->ref_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageHeight->fbo      ,activeImage->ref_fbo);
        FBOImages::resize(targetImageHeight->aux_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageHeight->aux2_fbo ,activeImage->ref_fbo);
        activeFBO     = targetImageHeight->fbo;
        activeAuxFBO  = targetImageHeight->aux_fbo;
        activeAux2FBO = targetImageHeight->aux2_fbo;
        break;
        case(CONVERT_FROM_D_TO_O):
        FBOImages::resize(targetImageHeight->ref_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageHeight->fbo      ,activeImage->ref_fbo);
        FBOImages::resize(targetImageHeight->aux_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageHeight->aux2_fbo ,activeImage->ref_fbo);
        FBOImages::resize(targetImageNormal->ref_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageNormal->fbo      ,activeImage->ref_fbo);
        FBOImages::resize(targetImageNormal->aux_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageNormal->aux2_fbo ,activeImage->ref_fbo);
        FBOImages::resize(targetImageSpecular->ref_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageSpecular->fbo      ,activeImage->ref_fbo);
        FBOImages::resize(targetImageSpecular->aux_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageSpecular->aux2_fbo ,activeImage->ref_fbo);
        // copy diffuse to specular image
        applyNormalFilter(activeImage->ref_fbo,targetImageSpecular->ref_fbo);
        applyNormalFilter(activeImage->ref_fbo,targetImageSpecular->fbo);

        FBOImages::resize(targetImageOcclusion->ref_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageOcclusion->fbo      ,activeImage->ref_fbo);
        FBOImages::resize(targetImageOcclusion->aux_fbo  ,activeImage->ref_fbo);
        FBOImages::resize(targetImageOcclusion->aux2_fbo ,activeImage->ref_fbo);
        break;
        default:
        break;
    }

    program->bind();
    program->setUniformValue("gui_image_type", activeImage->imageType);
    program->setUniformValue("gui_depth", float(1.0));
    program->setUniformValue("gui_mode_dgaussian", 1);


    if(activeImage->bFirstDraw){
        resetView();
        qDebug() << "Doing first draw";

        activeImage->bFirstDraw = false;
        // Updating ref FBO...

        activeImage->ref_fbo->bind();
            glViewport(0,0,activeImage->ref_fbo->width(),activeImage->ref_fbo->height());
            program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
            program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
            glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, activeImage->scr_tex_id);
            glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
        activeImage->ref_fbo->bindDefault();

    }

    if(bRecalculateOcclusion == true){           
           // applyCombineNormalHeightFilter(targetImageNormal->fbo,targetImageHeight->fbo,activeImage->ref_fbo);
            bRecalculateOcclusion = false;
    }


    if(activeImage->imageType == OCCLUSION_TEXTURE){
        applyCombineNormalHeightFilter(targetImageNormal->fbo,targetImageHeight->fbo,activeImage->ref_fbo);
        applyOcclusionFilter(activeImage->ref_fbo,activeFBO);
    }else{
        // Updating FBO...
        activeFBO->bind();
            glViewport(0,0,activeFBO->width(),activeFBO->height());
            program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
            program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
            glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]);
            glBindTexture(GL_TEXTURE_2D, activeImage->ref_fbo->texture());
            glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
        activeFBO->bindDefault();

        if(conversionType == CONVERT_NONE ){
            applyPerspectiveTransformFilter(activeFBO,activeAuxFBO);
            copyFBO(activeAuxFBO,activeFBO);
        }

        // Making seamless...
        if(FBOImageProporties::bMakeSeamless && conversionType == CONVERT_NONE){
            applySeamlessFilter(activeFBO,activeAuxFBO,FBOImageProporties::MakeSeamlessRadius);
            copyFBO(activeAuxFBO,activeFBO);
        }
    }

    applyInvertComponentsFilter(activeFBO,activeAuxFBO);

    if(activeImage->bGrayScale){
        applyGrayScaleFilter(activeAuxFBO,activeFBO);
    }else{
        copyFBO(activeAuxFBO,activeFBO);
    }
    // specular manipulation
    if(activeImage->bSpeclarControl){
        applyDGaussiansFilter(activeFBO,activeAux2FBO,activeAuxFBO);
        applyContrastFilter(activeAuxFBO,activeFBO);
    }


    // Removing shading...
    if(activeImage->bRemoveShading){
        applyGaussFilter(activeFBO,activeAux2FBO,activeAuxFBO,activeImage->noRemoveShadingGaussIter);
        applyInverseColorFilter(activeAuxFBO,activeAux2FBO);
        copyFBO(activeAux2FBO,activeAuxFBO);
        applyOverlayFilter(activeFBO,activeAuxFBO,activeAux2FBO);
        copyFBO(activeAux2FBO,activeFBO);

    }

    if(activeImage->noBlurPasses > 0){
        for(int i = 0 ; i < activeImage->noBlurPasses ; i++ ){
            applyGaussFilter(activeFBO,activeAux2FBO,activeAuxFBO,1);
            applyOverlayFilter(activeFBO,activeAuxFBO,activeAux2FBO);
            copyFBO(activeAux2FBO,activeFBO);
        }
    }

    if( activeImage->smallDetails  > 0.0){
        applySmallDetailsFilter(activeFBO,activeAux2FBO,activeAuxFBO);
        copyFBO(activeAuxFBO,activeFBO);
    }


    if( activeImage->mediumDetails > 0.0){
        applyMediumDetailsFilter(activeFBO,activeAux2FBO,activeAuxFBO);
        copyFBO(activeAuxFBO,activeFBO);
    }

    if(activeImage->sharpenBlurAmount != 0){
        applySharpenBlurFilter(activeFBO,activeAux2FBO,activeAuxFBO);
        copyFBO(activeAuxFBO,activeFBO);
    }

    if(activeImage->imageType == NORMAL_TEXTURE){
        applyNormalsStepFilter(activeFBO,activeAuxFBO);
        copyFBO(activeAuxFBO,activeFBO);
    }





    if(activeImage->imageType == HEIGHT_TEXTURE && activeImage->bConversionHN){
        applyHeightToNormal(activeFBO,activeAuxFBO);
        copyFBO(activeAuxFBO,activeFBO);
    }

    if(activeImage->imageType == NORMAL_TEXTURE && activeImage->bConversionNH){
        applyNormalToHeight(activeImage,activeFBO,activeAux2FBO,activeAuxFBO);
        applyCPUNormalizationFilter(activeAuxFBO,activeFBO);
    }

    if(activeImage->imageType == DIFFUSE_TEXTURE && activeImage->bConversionBaseMap){
        applyBaseMapConversion(activeFBO,activeAux2FBO,activeAuxFBO);

        if(conversionType == CONVERT_FROM_D_TO_O){
            applyNormalToHeight(targetImageNormal,activeAuxFBO,activeFBO,activeAux2FBO);
            applyCPUNormalizationFilter(activeAux2FBO,activeFBO);
        }

        copyFBO(activeFBO,activeAux2FBO);
        copyFBO(activeAuxFBO,activeFBO);
        if(conversionType == CONVERT_FROM_D_TO_O) applyCombineNormalHeightFilter(activeFBO,activeAux2FBO,targetImageOcclusion->ref_fbo);
    }


    switch(conversionType){
        case(CONVERT_FROM_H_TO_N):
        copyFBO(activeFBO,targetImageNormal->ref_fbo);
        break;
        case(CONVERT_FROM_N_TO_H):
        copyFBO(activeFBO,targetImageHeight->ref_fbo);
        break;
        case(CONVERT_FROM_D_TO_O):
        //copyFBO(activeFBO,targetImageHeight->ref_fbo);
        copyFBO(activeFBO,targetImageNormal->fbo);
        copyFBO(activeFBO,targetImageNormal->ref_fbo);
        copyFBO(activeAux2FBO,targetImageHeight->ref_fbo);
        copyFBO(activeAux2FBO,targetImageHeight->fbo);
        break;
        default:
        break;
    }


    activeFBO   = activeImage->fbo;

    program->setUniformValue("gui_clear_alpha",1);
    applyNormalFilter(activeFBO,activeAuxFBO);
    program->setUniformValue("gui_clear_alpha",0);
    copyFBO(activeAuxFBO,activeFBO);


    }// end of skip processing





    if(!bShadowRender){
        // Displaying new image
        activeFBO->bindDefault();
        program->setUniformValue("quad_draw_mode", 1);

        glViewport(0,0,width(),height());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, activeFBO->texture());

        QMatrix4x4 m;        
        m.ortho(0,orthographicProjWidth,0,orthographicProjHeight,-1,1);
        program->setUniformValue("ProjectionMatrix", m);
        m.setToIdentity();
        m.translate(xTranslation,yTranslation,0);
        program->setUniformValue("ModelViewMatrix", m);
        glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]);
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
        program->setUniformValue("quad_draw_mode", 0);
    }
    bSkipProcessing = false;
    conversionType  = CONVERT_NONE;
}

void GLImage::showEvent(QShowEvent* event){
    QWidget::showEvent( event );
    resetView();
}
void GLImage::resetView(){

    zoom = 0;
    windowRatio = float(width())/height();
    fboRatio    = float(activeImage->fbo->width())/activeImage->fbo->height();
    // openGL window dimensions
    orthographicProjHeight = (1+zoom)/windowRatio;
    orthographicProjWidth = (1+zoom)/fboRatio;

    if(orthographicProjWidth < 1.0) { // fitting x direction
        zoom = fboRatio - 1;
        orthographicProjHeight = (1+zoom)/windowRatio;
        orthographicProjWidth = (1+zoom)/fboRatio;
    }
    if(orthographicProjHeight < 1.0) { // fitting y direction
        zoom = windowRatio - 1;
        orthographicProjHeight = (1+zoom)/windowRatio;
        orthographicProjWidth = (1+zoom)/fboRatio;
    }
    // setting the image in the center
    xTranslation = orthographicProjWidth /2;
    yTranslation = orthographicProjHeight/2;
}

//! [8]
void GLImage::resizeGL(int width, int height)
{
    windowRatio     = float(width)/height;
    glViewport(0, 0, width, height);

    fboRatio = float(activeImage->fbo->width())/activeImage->fbo->height();
    orthographicProjHeight = (1+zoom)/windowRatio;
    orthographicProjWidth = (1+zoom)/fboRatio;
}


void GLImage::setActiveImage(FBOImageProporties* ptr){
        activeImage = ptr;
        updateGL();
}
void GLImage::enableShadowRender(bool enable){
        bShadowRender = enable;
}
void GLImage::enableRecalculateOcclusion(bool enable){
        bRecalculateOcclusion = enable;
}
void GLImage::setConversionType(ConversionType type){
    conversionType = type ;
}
void GLImage::updateCornersPosition(QVector2D dc1,QVector2D dc2,QVector2D dc3,QVector2D dc4){

    cornerPositions[0] = QVector2D(0,0) + dc1;
    cornerPositions[1] = QVector2D(1,0) + dc2;
    cornerPositions[2] = QVector2D(1,1) + dc3;
    cornerPositions[3] = QVector2D(0,1) + dc4;
    updateGL();
}
void GLImage::selectPerspectiveTransformMethod(int method){
    gui_perspective_mode = method;
    updateGL();
}

void GLImage::selectSeamlessModeBlending(bool enable){
    FBOImageProporties::bMakeSeamless = enable;
    gui_seamless_mode = 0;
    updateGL();
}
void GLImage::selectSeamlessModeMirror(bool enable){
    FBOImageProporties::bMakeSeamless = enable;
    gui_seamless_mode = int(enable);
    updateGL();
}


void GLImage::applyNormalFilter(QGLFramebufferObject* inputFBO,
                         QGLFramebufferObject* outputFBO){

    outputFBO->bind();
    glViewport(0,0,outputFBO->width(),outputFBO->height());    
    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();
}

void GLImage::applyPerspectiveTransformFilter(  QGLFramebufferObject* inputFBO,
                                                QGLFramebufferObject* outputFBO){

    outputFBO->bind();

    glViewport(0,0,outputFBO->width(),outputFBO->height());
    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_perspective_transform_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    program->setUniformValue("corner1"  , cornerPositions[0]);
    program->setUniformValue("corner2"  , cornerPositions[1]);
    program->setUniformValue("corner3"  , cornerPositions[2]);
    program->setUniformValue("corner4"  , cornerPositions[3]);

    program->setUniformValue("gui_perspective_mode"  , gui_perspective_mode);


    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();


}

void GLImage::applyCompressedFormatFilter(QGLFramebufferObject* baseFBO,
                                          QGLFramebufferObject* alphaFBO,
                                          QGLFramebufferObject* outputFBO){


    outputFBO->bind();
    glViewport(0,0,outputFBO->width(),outputFBO->height());
    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_compressed_type_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, baseFBO->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, alphaFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    glActiveTexture(GL_TEXTURE0);
    outputFBO->bindDefault();
}

void GLImage::applyGaussFilter(QGLFramebufferObject* sourceFBO,
                               QGLFramebufferObject* auxFBO,
                               QGLFramebufferObject* outputFBO,int no_iter){


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]);
    program->setUniformValue("gui_gauss_radius", no_iter);
    program->setUniformValue("gui_gauss_w", float(no_iter));


    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    glViewport(0,0,sourceFBO->width(),sourceFBO->height());
    program->setUniformValue("gauss_mode",1);

    auxFBO->bind();
    glBindTexture(GL_TEXTURE_2D, sourceFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    program->setUniformValue("gauss_mode",2);
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, auxFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    program->setUniformValue("gauss_mode",0);


}

void GLImage::applyInverseColorFilter(QGLFramebufferObject* inputFBO,
                                      QGLFramebufferObject* outputFBO){


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_invert_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();
}

void GLImage::applyOverlayFilter(QGLFramebufferObject* layerAFBO,
                                 QGLFramebufferObject* layerBFBO,
                                 QGLFramebufferObject* outputFBO){


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_overlay_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    outputFBO->bind();
    glViewport(0,0,outputFBO->width(),outputFBO->height());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, layerAFBO->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, layerBFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    glActiveTexture(GL_TEXTURE0);

    outputFBO->bindDefault();

}

void GLImage::applySeamlessFilter(QGLFramebufferObject* inputFBO,
                            QGLFramebufferObject* outputFBO,float radius){

    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_seamless_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    program->setUniformValue("make_seamless_radius"  , radius);
    program->setUniformValue("gui_seamless_mode"  , gui_seamless_mode);    
    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();
}

void GLImage::applyDGaussiansFilter(QGLFramebufferObject* inputFBO,
                                  QGLFramebufferObject* auxFBO,
                                  QGLFramebufferObject* outputFBO){


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]);
    program->setUniformValue("gui_gauss_radius", activeImage->specularRadius);
    program->setUniformValue("gui_gauss_w", activeImage->specularW1);

    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    glViewport(0,0,inputFBO->width(),inputFBO->height());
    program->setUniformValue("gauss_mode",1);

    auxFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    program->setUniformValue("gauss_mode",2);
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, auxFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

    program->setUniformValue("gui_gauss_w", activeImage->specularW2);
    auxFBO->bind();
    program->setUniformValue("gauss_mode",1);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    program->setUniformValue("gauss_mode",2);
    inputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, auxFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_dgaussians_filter"]);
    program->setUniformValue("gui_specular_amplifier", activeImage->specularAmplifier);


    auxFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, outputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    glActiveTexture(GL_TEXTURE0);
    program->setUniformValue("gauss_mode",0);

    copyFBO(auxFBO,outputFBO);
    outputFBO->bindDefault();

}

void GLImage::applyContrastFilter(QGLFramebufferObject* inputFBO,
                                  QGLFramebufferObject* outputFBO){

    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_constrast_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    program->setUniformValue("gui_specular_contrast", activeImage->specularContrast);
    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();
}

void GLImage::applySmallDetailsFilter(QGLFramebufferObject* inputFBO,
                                      QGLFramebufferObject* auxFBO,
                                    QGLFramebufferObject* outputFBO){


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]);
    program->setUniformValue("gui_depth", activeImage->detailDepth);
    program->setUniformValue("gui_gauss_radius", float(3.0));
    program->setUniformValue("gui_gauss_w", float(3.0));

    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    glViewport(0,0,inputFBO->width(),inputFBO->height());
    program->setUniformValue("gauss_mode",1);

    auxFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    program->setUniformValue("gauss_mode",2);
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, auxFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_dgaussians_filter"]);
    program->setUniformValue("gui_mode_dgaussian", 0);

    auxFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, outputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    glActiveTexture(GL_TEXTURE0);
    program->setUniformValue("gauss_mode",0);
    program->setUniformValue("gui_mode_dgaussian", 1);




    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_small_details_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    program->setUniformValue("gui_small_details", activeImage->smallDetails);
    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, auxFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    glActiveTexture(GL_TEXTURE0);
    outputFBO->bindDefault();
    program->setUniformValue("gui_depth", float(1.0));

}

void GLImage::applyMediumDetailsFilter(QGLFramebufferObject* inputFBO,
                                       QGLFramebufferObject* auxFBO,
                                       QGLFramebufferObject* outputFBO){


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]);
    program->setUniformValue("gui_depth", activeImage->detailDepth);
    program->setUniformValue("gui_gauss_radius", float(20.0));
    program->setUniformValue("gui_gauss_w", float(20.0));

    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    glViewport(0,0,inputFBO->width(),inputFBO->height());
    program->setUniformValue("gauss_mode",1);

    auxFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    program->setUniformValue("gauss_mode",2);
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, auxFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_dgaussians_filter"]);
    program->setUniformValue("gui_mode_dgaussian", 0);

    auxFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, outputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    glActiveTexture(GL_TEXTURE0);
    program->setUniformValue("gauss_mode",0);
    program->setUniformValue("gui_mode_dgaussian", 1);




    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_small_details_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    program->setUniformValue("gui_small_details", activeImage->mediumDetails);
    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, auxFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    glActiveTexture(GL_TEXTURE0);
    outputFBO->bindDefault();
    program->setUniformValue("gui_depth", float(1.0));


}


void GLImage::applyGrayScaleFilter(QGLFramebufferObject* inputFBO,
                             QGLFramebufferObject* outputFBO){

    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gray_scale_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();

}

void GLImage::applyInvertComponentsFilter(QGLFramebufferObject* inputFBO,
                             QGLFramebufferObject* outputFBO){

    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_invert_components_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    program->setUniformValue("gui_inverted_components"  , QVector3D(activeImage->bInvertR,activeImage->bInvertG,activeImage->bInvertB));

    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();

}

void GLImage::applySharpenBlurFilter(QGLFramebufferObject* inputFBO,
                                     QGLFramebufferObject* auxFBO,
                                     QGLFramebufferObject* outputFBO){

    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_sharpen_blur"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    program->setUniformValue("gui_sharpen_blur", activeImage->sharpenBlurAmount);

    glViewport(0,0,inputFBO->width(),inputFBO->height());

    auxFBO->bind();
    program->setUniformValue("gauss_mode",1);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bind();
    program->setUniformValue("gauss_mode",2);
    glBindTexture(GL_TEXTURE_2D, auxFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

    outputFBO->bindDefault();
    program->setUniformValue("gauss_mode",0);

}

void GLImage::applyNormalsStepFilter(QGLFramebufferObject* inputFBO,
                               QGLFramebufferObject* outputFBO){

    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normals_step_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    program->setUniformValue("gui_normals_step", activeImage->normalsStep);

    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();

}


void GLImage::applyPreSmoothFilter(  QGLFramebufferObject* inputFBO,
                                     QGLFramebufferObject* auxFBO,
                                     QGLFramebufferObject* outputFBO){


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]);
    program->setUniformValue("gui_gauss_radius", int(activeImage->conversionBaseMapPreSmoothRadius));
    program->setUniformValue("gui_gauss_w", float(activeImage->conversionBaseMapPreSmoothRadius));


    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    glViewport(0,0,inputFBO->width(),inputFBO->height());
    program->setUniformValue("gauss_mode",1);

    auxFBO->bind();
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    program->setUniformValue("gauss_mode",2);
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, auxFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    program->setUniformValue("gauss_mode",0);
    outputFBO->bindDefault();


}

void GLImage::applySobelToNormalFilter(QGLFramebufferObject* inputFBO,
                                       QGLFramebufferObject* outputFBO){


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_sobel_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    program->setUniformValue("gui_basemap_amp", activeImage->conversionBaseMapAmplitude);

    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();

}

void GLImage::applyHeightToNormal(QGLFramebufferObject* inputFBO,
                         QGLFramebufferObject* outputFBO){

    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_height_to_normal"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    program->setUniformValue("gui_hn_conversion_depth", activeImage->conversionHNDepth);
    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();

}


void GLImage::applyNormalToHeight(FBOImageProporties* image,QGLFramebufferObject* normalFBO,
                                  QGLFramebufferObject* heightFBO,
                                  QGLFramebufferObject* outputFBO){



    applyGrayScaleFilter(normalFBO,heightFBO);
    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_to_height"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));



    outputFBO->bind();
    glViewport(0,0,heightFBO->width(),heightFBO->height());
    program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,1.0));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightFBO->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

    for(int i = 0; i < image->conversionNHItersHuge ; i++){
        program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,5)));
        heightFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

        outputFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    }
    for(int i = 0; i < image->conversionNHItersVeryLarge ; i++){
        program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,4)));
        heightFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

        outputFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    }
    for(int i = 0; i < image->conversionNHItersLarge ; i++){
        program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,3)));
        heightFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

        outputFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    }
    for(int i = 0; i < image->conversionNHItersMedium; i++){
        program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,2)));
        heightFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

        outputFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    }
    for(int i = 0; i < image->conversionNHItersSmall; i++){
        program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,1)));
        heightFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

        outputFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    }
    for(int i = 0; i < image->conversionNHItersVerySmall; i++){
        program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,0)));
        heightFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

        outputFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    }
/*
    for(int k = 8; k >= 0 ; k--)
    for(int i = 0; i < no_conversion_iters ; i++){

        program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,k)));
        heightFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

        outputFBO->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightFBO->texture());
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    }
*/
    glActiveTexture(GL_TEXTURE0);
    outputFBO->bindDefault();
}

void GLImage::applySelectiveGaussFilter(QGLFramebufferObject* inputFBO,
                              QGLFramebufferObject* outputFBO){


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_expansion_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();

}

void GLImage::applyCPUNormalizationFilter(QGLFramebufferObject* inputFBO,
                                          QGLFramebufferObject* outputFBO){

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    GLint textureWidth, textureHeight;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight);

    float* img = new float[textureWidth*textureHeight*4];

    glGetTexImage(	GL_TEXTURE_2D,0,GL_RGBA,GL_FLOAT,img);

    float min[3] = {img[0],img[1],img[2]};
    float max[3] = {img[0],img[1],img[2]};

    for(int i = 0 ; i < textureWidth*textureHeight ; i++){
        for(int c = 0 ; c < 3 ; c++){
            if( max[c] < img[4*i+c] ) max[c] = img[4*i+c];
            if( min[c] > img[4*i+c] ) min[c] = img[4*i+c];
        }

    }

    qDebug() << "Image normalization:";
    qDebug() << "Min color = (" << min[0] << "," << min[1] << "," << min[2] << ")"  ;
    qDebug() << "Max color = (" << max[0] << "," << max[1] << "," << max[2] << ")"  ;


    delete[] img;


    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normalize_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    outputFBO->bind();
    glViewport(0,0,inputFBO->width(),inputFBO->height());
    program->setUniformValue("min_color",QVector3D(min[0],min[1],min[2]));
    program->setUniformValue("max_color",QVector3D(max[0],max[1],max[2]));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);


    outputFBO->bindDefault();


}

void GLImage::applyBaseMapConversion(QGLFramebufferObject* baseMapFBO,
                                     QGLFramebufferObject *auxFBO,
                                     QGLFramebufferObject* outputFBO){


        applyGrayScaleFilter(baseMapFBO,outputFBO);
        applySobelToNormalFilter(outputFBO,auxFBO);
        applyInvertComponentsFilter(auxFBO,baseMapFBO);
        applyPreSmoothFilter(baseMapFBO,auxFBO,outputFBO);

        program->setUniformValue("gui_combine_normals" , 0 );
        program->setUniformValue("gui_filter_radius" , activeImage->conversionBaseMapFilterRadius );

        for(int i = 0; i < activeImage->conversionBaseMapNoIters ; i ++){

            copyFBO(outputFBO,auxFBO);
            program->setUniformValue("gui_normal_flatting" , activeImage->conversionBaseMapFlatness );
            applySelectiveGaussFilter(auxFBO,outputFBO);
        }


        program->setUniformValue("gui_combine_normals" , 1 );
        program->setUniformValue("gui_mix_normals"   , activeImage->conversionBaseMapMixNormals );
        program->setUniformValue("gui_blend_normals" , activeImage->conversionBaseMapBlending );
        copyFBO(outputFBO,auxFBO);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, baseMapFBO->texture());
            applySelectiveGaussFilter(auxFBO,outputFBO);
        glActiveTexture(GL_TEXTURE0);
        program->setUniformValue("gui_combine_normals" , 0 );



}


void GLImage::applyOcclusionFilter(QGLFramebufferObject* inputFBO,
                          QGLFramebufferObject* outputFBO){

    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_occlusion_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));

    program->setUniformValue("gui_ssao_no_iters"   ,activeImage->ssaoNoIters);
    program->setUniformValue("gui_ssao_depth"      ,activeImage->ssaoDepth);
    program->setUniformValue("gui_ssao_bias"       ,activeImage->ssaoBias);
    program->setUniformValue("gui_ssao_intensity"  ,activeImage->ssaoIntensity);

    glViewport(0,0,inputFBO->width(),inputFBO->height());
    outputFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFBO->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, targetImageNormal->fbo->texture());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, targetImageHeight->fbo->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    outputFBO->bindDefault();
    glActiveTexture(GL_TEXTURE0);

}

void GLImage::applyCombineNormalHeightFilter(QGLFramebufferObject* normalFBO,
                                             QGLFramebufferObject* heightFBO,
                                             QGLFramebufferObject* outputFBO){

    glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_combine_normal_height_filter"]);
    program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    glViewport(0,0,normalFBO->width(),normalFBO->height());
    outputFBO->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, normalFBO->texture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, heightFBO->texture());
    glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);
    glActiveTexture(GL_TEXTURE0);
    outputFBO->bindDefault();
}

void GLImage::copyFBO(QGLFramebufferObject* src,QGLFramebufferObject* dst){
    src->blitFramebuffer(dst,QRect(QPoint(0,0),src->size()),src,QRect(QPoint(0,0),dst->size()));
}

void GLImage::makeScreenQuad()
{

    int size = 2;
    QVector<QVector2D>texCoords = QVector<QVector2D>(size*size);
    QVector<QVector3D> vertices  = QVector<QVector3D>(size*size);
    int iter = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            float offset = 0.5;
            float x = i/(size-1.0);
            float y = j/(size-1.0);
            vertices[iter]  = (QVector3D(x-offset,y-offset,0));
            texCoords[iter] = (QVector2D(x,y));
            iter++;
    }}



    glGenBuffers(3, &vbos[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float)*3, vertices.constData(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(PROGRAM_VERTEX_ATTRIBUTE);
    glVertexAttribPointer(PROGRAM_VERTEX_ATTRIBUTE,3,GL_FLOAT,GL_FALSE,sizeof(float)*3,(void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float)*2, texCoords.constData(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    glVertexAttribPointer(PROGRAM_TEXCOORD_ATTRIBUTE,2,GL_FLOAT,GL_FALSE,sizeof(float)*2,(void*)0);


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[2]);


    int no_triangles = 2*(size - 1)*(size - 1);
    QVector<GLuint> indices(no_triangles*3);
    iter = 0;
    for(int i = 0 ; i < size -1 ; i++){
    for(int j = 0 ; j < size -1 ; j++){
        GLuint i1 = i + j*size;
        GLuint i2 = i + (j+1)*size;
        GLuint i3 = i+1 + j*size;
        GLuint i4 = i+1 + (j+1)*size;
        indices[iter++] = (i1);
        indices[iter++] = (i3);
        indices[iter++] = (i2);
        indices[iter++] = (i2);
        indices[iter++] = (i3);
        indices[iter++] = (i4);
    }
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * no_triangles * 3 , indices.constData(), GL_STATIC_DRAW);
}


void GLImage::updateMousePosition(){
    QPoint p = mapFromGlobal(QCursor::pos());
    cursorPhysicalXPosition =       double(p.x())/width() * orthographicProjWidth  - xTranslation;
    cursorPhysicalYPosition = (1.0-double(p.y())/height())* orthographicProjHeight - yTranslation;
    bSkipProcessing = true;    
}

void GLImage::wheelEvent(QWheelEvent *event){

    if( event->delta() > 0) zoom+=0.1;
        else zoom-=0.1;
    if(zoom < -0.90) zoom = -0.90;

    updateMousePosition();
    resizeGL(width(),height());

    QPoint p = mapFromGlobal(QCursor::pos());//getting the global position of cursor
    // restoring the translation after zooming
    xTranslation =        double(p.x())/width() *orthographicProjWidth  - cursorPhysicalXPosition;
    yTranslation = ((1.0-double(p.y())/height())*orthographicProjHeight - cursorPhysicalYPosition );

    updateGL();
}

void GLImage::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastCursorPos.x();
    int dy = event->y() - lastCursorPos.y();
    lastCursorPos = event->pos();

    QVector2D defCorners[4];//default position of corners
    defCorners[0] = QVector2D(0,0) ;
    defCorners[1] = QVector2D(1,0) ;
    defCorners[2] = QVector2D(1,1) ;
    defCorners[3] = QVector2D(0,1) ;
    QVector2D mpos((cursorPhysicalXPosition+0.5),(cursorPhysicalYPosition+0.5));

    if(draggingCorner == -1){
    setCursor(Qt::OpenHandCursor);
    for(int i = 0; i < 4 ; i++){
        float dist = (mpos - defCorners[i]).length();
        if(dist < 0.2){
            //setCursor(Qt::SizeAllCursor);
            setCursor(cornerCursors[i]);
        }
    }
    }// end if dragging


    if (event->buttons() & Qt::RightButton){
        xTranslation += dx*(float(orthographicProjWidth)/width());
        yTranslation -= dy*(float(orthographicProjHeight)/height());
        setCursor(Qt::ClosedHandCursor);
    }else if(event->buttons() & Qt::LeftButton){

        // calculate distance from corners
        if(draggingCorner == -1){
        for(int i = 0; i < 4 ; i++){
            float dist = (mpos - defCorners[i]).length();
            if(dist < 0.2){
                draggingCorner = i;
            }
        }// end of for corners
        }// end of if
        if(draggingCorner >=0 && draggingCorner < 4) cornerPositions[draggingCorner] += QVector2D(-dx*(float(orthographicProjWidth)/width()),dy*(float(orthographicProjHeight)/height()));
        repaint();

    }else if(event->buttons() & Qt::MiddleButton){ // drag image
        setCursor(Qt::SizeAllCursor);
        // move all corners
        QVector2D averagePos(0.0,0.0);
        QVector2D dmouse = QVector2D(-dx*(float(orthographicProjWidth)/width()),dy*(float(orthographicProjHeight)/height()));
        for(int i = 0; i < 4 ; i++){
            averagePos += cornerPositions[i]*0.25;
            cornerPositions[i] += dmouse;
        }
        repaint();
    }
    updateMousePosition();
    updateGL();
}
void GLImage::mousePressEvent(QMouseEvent *event)
{

    lastCursorPos = event->pos();
    bSkipProcessing = true;
    draggingCorner = -1;
    // change cursor
    if (event->buttons() & Qt::RightButton) {
        setCursor(Qt::ClosedHandCursor);
    }
    updateGL();

}
void GLImage::mouseReleaseEvent(QMouseEvent *event){
    setCursor(Qt::OpenHandCursor);
    draggingCorner = -1;
}
