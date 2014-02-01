#include <osg/Camera>

/** Camera pre configured for deferred shading.
*/
class DeferredCamera : public osg::Camera
{
public:
    DeferredCamera();

    /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
    DeferredCamera(const Camera&,const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

private:
    void constructorInit();
};
