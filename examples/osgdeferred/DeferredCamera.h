#include <osg/Camera>

#include <memory>

/** Camera pre configured for deferred shading.
*/
class DeferredCamera : public osg::Camera
{
public:
    DeferredCamera();

    /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
    DeferredCamera(const Camera&,const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

    osg::Camera* getSlaveCamera();

private:
    void constructorInit();

private:
    struct PImpl;
    std::unique_ptr<PImpl> pImpl_;
};
