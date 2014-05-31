/* OpenSceneGraph example, osgtexture2Dms.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

#include <osgUtil/UpdateVisitor>

#include <osgDB/ReadFile>

#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/PositionAttitudeTransform>

#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

namespace
{

class DisplayTextureCamera : public osg::Camera
{
public:
    DisplayTextureCamera(osg::ref_ptr<osg::Texture> textureImage, osg::ref_ptr<osg::Texture> textureDepth)
        : textureImage_(textureImage)
        , textureDepth_(textureDepth)
    {
        osg::StateSet* stateset = getOrCreateStateSet();
        stateset->setGlobalDefaults();
        setClearColor(osg::Vec4(0.3f, 0.2f, 0.1f, 1.f));
        setClearMask(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);
        setProjectionMatrix(osg::Matrixd::identity());
        setViewMatrix(osg::Matrixd::identity());
        setRenderOrder(osg::Camera::NESTED_RENDER);
        setReferenceFrame(osg::Transform::ABSOLUTE_RF);

        groupNode_ = new osg::Group();
        addChild(groupNode_.get());
    }
    void setup(unsigned int sizeX, unsigned int sizeY, osg::Vec2d& screenSize)
    {
        osg::ref_ptr<osg::Node> imageQuad = createTexturedQuad(textureImage_, osg::Vec2d(sizeX, sizeY), screenSize, 0);
        groupNode_->addChild(imageQuad.get());
        osg::ref_ptr<osg::Node> textureQuad = createTexturedQuad(textureDepth_, osg::Vec2d(sizeX, sizeY), screenSize, 1);
        groupNode_->addChild(textureQuad.get());
    }

private:
   static osg::ref_ptr<osg::Node> createTexturedQuad(osg::ref_ptr<osg::Texture> textureImage, osg::Vec2d quadSize, osg::Vec2d screenSize, unsigned int position)
   {
        osg::ref_ptr<osg::Geode> geometry = new osg::Geode();

        bool left = (position % 2) == 0;
        unsigned int line = position / 2;
        osg::Vec2d startCoordinates(
            left ? -1. : 1. - (quadSize.x() /screenSize.x()),
            -1. + line * (quadSize.y() / screenSize.y()));
        osg::Vec2d endCoordinates(startCoordinates + componentDivide(quadSize, screenSize));

        // counter-clockwise
        osg::ref_ptr<osg::Vec3Array> quadCoords = new osg::Vec3Array(); // vertex coords
        quadCoords->push_back(osg::Vec3d(startCoordinates.x(), startCoordinates.y(), 0));
        quadCoords->push_back(osg::Vec3d(endCoordinates.x(), startCoordinates.y(), 0));
        quadCoords->push_back(osg::Vec3d(endCoordinates.x(), endCoordinates.y(), 0));
        quadCoords->push_back(osg::Vec3d(startCoordinates.x(), endCoordinates.y(), 0));

        osg::ref_ptr<osg::Vec2Array> quadTexCoords = new osg::Vec2Array(); // texture coords
        quadTexCoords->push_back(osg::Vec2(0, 0));
        quadTexCoords->push_back(osg::Vec2(1, 0));
        quadTexCoords->push_back(osg::Vec2(1, 1));
        quadTexCoords->push_back(osg::Vec2(0, 1));

        osg::ref_ptr<osg::Geometry> quad = new osg::Geometry();
        quad->setSupportsDisplayList(false);

        quad->setVertexArray(quadCoords.get());
        quad->setTexCoordArray(0, quadTexCoords.get());
        quad->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));

        osg::StateSet* stateset = geometry->getOrCreateStateSet();
        stateset->setGlobalDefaults();
        stateset->setTextureAttributeAndModes(0, textureImage.get());
        geometry->addDrawable(quad.get());

        return geometry;
    }

private:
    osg::ref_ptr<osg::Texture> textureImage_;
    osg::ref_ptr<osg::Texture> textureDepth_;
    osg::ref_ptr<osg::Group> groupNode_;
};


class RTTCamera : public osg::Camera
{
public:
    void setup(unsigned int sizeX, unsigned int sizeY)
    {
        osg::StateSet* stateset = getOrCreateStateSet();
        stateset->setGlobalDefaults();
        setClearColor(osg::Vec4(0.1f, 0.2f, 0.3f, 1.f));
        setClearMask(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);
        setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        //setRenderOrder(osg::Camera::PRE_RENDER);
        texture2DImage_ = new osg::Texture2D();
        texture2DImage_->setTextureSize(sizeX, sizeY);
        texture2DImage_->setInternalFormat(GL_RGBA);
        texture2DImage_->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        texture2DImage_->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        texture2DDepth_ = new osg::Texture2D();
        texture2DDepth_->setTextureSize(sizeX, sizeY);
        texture2DDepth_->setInternalFormat(GL_DEPTH_COMPONENT);
        texture2DDepth_->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        texture2DDepth_->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        attach(osg::Camera::COLOR_BUFFER0, texture2DImage_, 0, 0);
        attach(osg::Camera::DEPTH_BUFFER, texture2DDepth_, 0, 0);
        setViewport(0, 0, sizeX, sizeY);
    }

    void createDisplayTextureCamera(osgViewer::Viewer& viewer)
    {
        osg::ref_ptr<DisplayTextureCamera> camera = new DisplayTextureCamera(texture2DImage_, texture2DDepth_);
        osg::GraphicsContext* gc = viewer.getCamera()->getGraphicsContext();
        camera->setGraphicsContext(gc);
        osg::Vec2d gcSize(gc->getTraits()->width, gc->getTraits()->height);
        //osg::DisplaySettings* ds = viewer.getDisplaySettings() ? viewer.getDisplaySettings() : osg::DisplaySettings::instance().get();
        //viewer.getTraits ();
        //camera->setViewport(0, 0, ds->getScreenWidth(), ds->getScreenHeight());
        camera->setViewport(0, 0, gcSize.x(), gcSize.y());
        camera->setup(812, 812, gcSize);
        viewer.addSlave(camera.get(), false);
    }
private:
    osg::ref_ptr<osg::Texture2D> texture2DImage_;
    osg::ref_ptr<osg::Texture2D> texture2DDepth_;
};

}

int main( int argc, char **argv )
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    if (arguments.argc()<2)
    {
        std::cout << arguments[0] <<": requires filename argument." << std::endl;
        return 1;
    }


    // construct the viewer.
    osgViewer::Viewer viewer;

    // add the state manipulator
    viewer.addEventHandler( new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()) );

    // add stats
    viewer.addEventHandler( new osgViewer::StatsHandler() );

    // load the scene.
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(arguments[1]);
    if (!loadedModel)
    {
        std::cout << argv[0] <<": No data loaded." << std::endl;
        return 2;
    }


    viewer.setUpViewInWindow(20, 30, 1280, 800);
    viewer.realize();
    viewer.setThreadingModel(osgViewer::ViewerBase::SingleThreaded);

    osg::ref_ptr<RTTCamera> rttCamera = new RTTCamera;
    rttCamera->setProjectionMatrix(viewer.getCamera()->getProjectionMatrix());
    rttCamera->setViewMatrix(viewer.getCamera()->getViewMatrix());
    rttCamera->setGraphicsContext(viewer.getCamera()->getGraphicsContext());
    rttCamera->setup(1024, 1024);
    rttCamera->setDisplaySettings(viewer.getCamera()->getDisplaySettings());
    viewer.setCamera(rttCamera.get());
    rttCamera->createDisplayTextureCamera(viewer);

    // pass the loaded model to the viewer.
    viewer.setSceneData(loadedModel.get());
    viewer.setCameraManipulator(new osgGA::TrackballManipulator());

    return viewer.run();
}

