#include <osg/Camera>

namespace osg
{
    class GraphicsContext;
}
namespace osgViewer
{
    class View;
}

/** Camera pre configured for deferred shading.
*/
class DeferredCamera : public osg::Camera
{
public:
    DeferredCamera();
    virtual ~DeferredCamera();

    /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
    DeferredCamera(const Camera&,const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

    osg::Camera* getSlaveCamera();
    void handleResize(osg::GraphicsContext& graphicsContext);

private:
    void constructorInit();

private:
    struct PImpl;
    osg::ref_ptr<PImpl> pImpl_;
};

DeferredCamera* install(osgViewer::View& view);
