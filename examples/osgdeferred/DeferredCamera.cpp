# include "DeferredCamera.h"

using namespace osg;

DeferredCamera::DeferredCamera()
    : Camera()
{
    constructorInit();
}

DeferredCamera::DeferredCamera(const Camera& other,const CopyOp& copyop)
    : Camera(other, copyop)
{
    constructorInit();
}

void DeferredCamera::constructorInit()
{
    osg::StateSet* stateset = getOrCreateStateSet();
    stateset->setGlobalDefaults();
    setClearColor(osg::Vec4(0.2f, 0.0f, 0.3f, 0.0f));
    setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
