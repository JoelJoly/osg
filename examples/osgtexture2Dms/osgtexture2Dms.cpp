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

class RTTCamera : public osg::Camera
{
public:
    void setup(unsigned int sizeX, unsigned int sizeY)
    {
        setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        setRenderOrder(osg::Camera::PRE_RENDER);
        texture2DImage_ = new osg::Texture2D();
        texture2DImage_->setTextureSize(sizeX, sizeY);
        texture2DDepth_ = new osg::Texture2D();
        texture2DDepth_->setTextureSize(sizeX, sizeY);
        attach(osg::Camera::COLOR_BUFFER0, texture2DImage_, 0, 0);
        attach(osg::Camera::DEPTH_BUFFER, texture2DDepth_, 0, 0);
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

    // pass the loaded model to the viewer.
    viewer.setSceneData(loadedModel.get());
    viewer.setCameraManipulator(new osgGA::TrackballManipulator());

    viewer.setUpViewInWindow(20, 30, 1280, 800);
    viewer.realize();

    osg::ref_ptr<RTTCamera> rttCamera = new RTTCamera();
//    viewer.setCamera(rttCamera.get());

    return viewer.run();
}

