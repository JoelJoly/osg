# include "DeferredCamera.h"

# include <osg/Geode>
# include <osg/Geometry>
# include <osg/GraphicsContext>
# include <osg/Shader>
# include <osg/StateSet>
# include <osg/Texture2D>
# include <osg/TextureRectangle>

# include <osgViewer/Renderer>
# include <osgViewer/View>

# include <deque>
# include <cstdint>

using namespace osg;

namespace
{
    const uint8_t   NUM_TEXTURES = 4;
    // use width and height big enough so that the view will never bigger than this
    const uint32_t  defaultWidth = 2048;//800;
    const uint32_t  defaultHeight = 2048;//450;
}

struct DeferredCamera::PImpl : public osg::GraphicsContext::ResizedCallback
{
    PImpl(DeferredCamera& camera) : mainCamera_(camera), previousResizedCallback_(nullptr) {}
    ~PImpl() {}
    void constuctStateSet(osg::StateSet* stateSet)
    {
        mrtStateSet_ = stateSet;
        static const char *vertexShaderSource = {
            "varying vec3 normal;\n"
            "varying vec3 world_space_position;\n"

            "void main(void)\n"
            "{\n"
            "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
            "    normal = gl_NormalMatrix * gl_Normal;\n"
            "    world_space_position = gl_Position.xyz;\n"
            "    gl_FrontColor = gl_Color;\n"
            "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
            "};\n"
        };
        osg::ref_ptr<osg::Shader> vshader = new osg::Shader(osg::Shader::VERTEX, vertexShaderSource);
        static const char *fragmentShaderSource = {
            "varying vec3 normal;\n"
            "varying vec3 world_space_position;\n"

            "void main(void)\n"
            "{\n"
            "    gl_FragData[0] = vec4(world_space_position, 1);\n"
            "    gl_FragData[1] = gl_Color;\n"
            "    gl_FragData[2] = vec4(normalize(normal), 1);\n"
            "    gl_FragData[3] = gl_TexCoord[0];\n"
            "}\n"
        };
        osg::ref_ptr<osg::Shader> fshader = new osg::Shader(osg::Shader::FRAGMENT, fragmentShaderSource);
        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader(fshader.get());
        program->addShader(vshader.get());
        mrtStateSet_->setAttributeAndModes(program.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        for (auto i = 0; i < NUM_TEXTURES; ++i)
        {
            osg::TextureRectangle* textureRect = new osg::TextureRectangle();
            textureRect->setInternalFormat(GL_RGBA);
            textureRect->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
            textureRect->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
            mainCamera_.attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), textureRect);
            fboTextures_.push_back(textureRect);
        }
    }
    void resize(uint32_t tex_width, uint32_t tex_height)
    {
        for (auto textureIte(fboTextures_.cbegin()), textureEnd(fboTextures_.cend()); textureIte != textureEnd; ++textureIte)
        {
            (*textureIte)->setTextureSize(tex_width, tex_height);
            auto i = std::distance(fboTextures_.cbegin(), textureIte);
            mainCamera_.attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), *textureIte);
        }
        osgViewer::Renderer* renderer = dynamic_cast<osgViewer::Renderer*>(mainCamera_.getRenderer());
        if (renderer)
            renderer->getSceneView(0)->getRenderStage()->setCameraRequiresSetUp(true);
    }
    void prepareSecondPass()
    {
        secondPassCamera_ = new osg::Camera();
        secondPassCamera_->setRenderOrder(osg::Camera::NESTED_RENDER);
        secondPassCamera_->setComputeNearFarMode(osg::CullSettings::ComputeNearFarMode::DO_NOT_COMPUTE_NEAR_FAR);
        // add subcene to camera to render
        secondPassCamera_->addChild(createRTTQuad());

        // set up the background color and clear mask.
        secondPassCamera_->setClearColor(osg::Vec4(0.1f,0.1f,0.3f,1.0f));
        secondPassCamera_->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // the camera is going to look at our input quad
        secondPassCamera_->setProjectionMatrix(osg::Matrix::ortho2D(0, 1, 0, 1));
        secondPassCamera_->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        secondPassCamera_->setViewMatrix(osg::Matrix::identity());

        secondPassCamera_->setViewport(0, 0, defaultWidth, defaultHeight);
    }
    void resizeRTTQuad(uint32_t tex_width, uint32_t tex_height)
    {
        osg::ref_ptr<osg::Vec2Array> quadTexCoords = new osg::Vec2Array(); // texture coords
        quadTexCoords->push_back(osg::Vec2(0, 0));
        quadTexCoords->push_back(osg::Vec2(tex_width, 0));
        quadTexCoords->push_back(osg::Vec2(tex_width, tex_height));
        quadTexCoords->push_back(osg::Vec2(0, tex_height));

        fullScreenQuad_->setTexCoordArray(0, quadTexCoords.get());
        osg::StateSet* stateset = fullScreenQuad_->getOrCreateStateSet();
        stateset->getUniform("width")->set(tex_width);
        stateset->getUniform("height")->set(tex_height);

    }
    // The quad geometry is used by the render to texture camera to generate multiple textures.
    osg::Group* createRTTQuad()
    {
        osg::ref_ptr<osg::Group> topGroup = new osg::Group();
        osg::ref_ptr<osg::Geode> quadGeode = new osg::Geode();

        osg::ref_ptr<osg::Vec3Array> quadCoords = new osg::Vec3Array(); // vertex coords
        // counter-clockwise
        quadCoords->push_back(osg::Vec3d(0, 0, 0));
        quadCoords->push_back(osg::Vec3d(1, 0, 0));
        quadCoords->push_back(osg::Vec3d(1, 1, 0));
        quadCoords->push_back(osg::Vec3d(0, 1, 0));

        fullScreenQuad_ = new osg::Geometry();
        fullScreenQuad_->setSupportsDisplayList(false);

        fullScreenQuad_->setVertexArray(quadCoords.get());
        fullScreenQuad_->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
        osg::ref_ptr<osg::Vec4Array> quadColors = new osg::Vec4Array;
        quadColors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        fullScreenQuad_->setColorArray(quadColors.get(), osg::Array::BIND_OVERALL);
        quadGeode->addDrawable(fullScreenQuad_.get());

        topGroup->addChild(quadGeode.get());
        osg::StateSet* stateset = fullScreenQuad_->getOrCreateStateSet();
        {
            static const char *shaderSource = {
                "#extension GL_ARB_texture_rectangle : enable\n"
                "uniform uint width;\n"
                "uniform uint height;\n"
                "uniform sampler2DRect textureID0;\n"
                "uniform sampler2DRect textureID1;\n"
                "uniform sampler2DRect textureID2;\n"
                "uniform sampler2DRect textureID3;\n"
                "void main(void)\n"
                "{\n"
                "    gl_FragData[0] = \n"
                "          vec4(gl_TexCoord[0].s / width, gl_TexCoord[0].s / height, 0.2, 1) + \n"
                "          vec4(texture2DRect(textureID2, gl_TexCoord[0].st).rgb, 1);  \n"
                "}\n"
            };
            osg::ref_ptr<osg::Shader> fshader = new osg::Shader(osg::Shader::FRAGMENT, shaderSource);
            osg::ref_ptr<osg::Program> program = new osg::Program();
            program->addShader(fshader.get());
            stateset->setAttributeAndModes(program.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);
            // attach the textures to use
            for (auto textureIte(fboTextures_.cbegin()), textureEnd(fboTextures_.cend()); textureIte != textureEnd; ++textureIte)
            {
                stateset->setTextureAttributeAndModes(std::distance(fboTextures_.cbegin(), textureIte), textureIte->get(), osg::StateAttribute::ON);
            }
            stateset->addUniform(new osg::Uniform("width", defaultWidth));
            stateset->addUniform(new osg::Uniform("height", defaultHeight));
            stateset->addUniform(new osg::Uniform("textureID0", 0));
            stateset->addUniform(new osg::Uniform("textureID1", 1));
            stateset->addUniform(new osg::Uniform("textureID2", 2));
            stateset->addUniform(new osg::Uniform("textureID3", 3));
        }
        return topGroup.release();
    }
    void installResizeHandler(osg::GraphicsContext& graphicsContext)
    {
        previousResizedCallback_ = graphicsContext.getResizedCallback();
        graphicsContext.setResizedCallback(this);
    }
    osg::Camera&                    mainCamera_; // don't hold a pointer to our own object
    osg::ref_ptr<osg::StateSet>     mrtStateSet_;
    osg::ref_ptr<osg::Geometry>     fullScreenQuad_;
    osg::ref_ptr<osg::Camera>       secondPassCamera_;
    typedef std::deque<osg::ref_ptr<osg::TextureRectangle> > PBOTextures;
    PBOTextures                     fboTextures_;
    osg::GraphicsContext::ResizedCallback*      previousResizedCallback_;
private:
    void resizedImplementation(GraphicsContext* gc, int x, int y, int width, int height)
    {
        resize(width, height);
        resizeRTTQuad(width, height);
        // default implementation does not resize cameras bound to FBO
        mainCamera_.setViewport(0, 0, width, height);

        if (previousResizedCallback_)
            previousResizedCallback_->resizedImplementation(gc, x, y, width, height);
        else
            gc->resizedImplementation(x, y, width, height);
        // we don't want to change the projection matrix based on the aspect ratio, it is already handled
        secondPassCamera_->setProjectionMatrix(osg::Matrix::ortho2D(0, 1, 0, 1));
    }

};

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

DeferredCamera::~DeferredCamera()
{
    // NOTHING
}

osg::Camera* DeferredCamera::getSlaveCamera()
{
    return pImpl_->secondPassCamera_;
}

void DeferredCamera::handleResize(osg::GraphicsContext& graphicsContext)
{
    pImpl_->installResizeHandler(graphicsContext);
}

void DeferredCamera::constructorInit()
{
    pImpl_ = new PImpl(*this);
    osg::StateSet* stateset = getOrCreateStateSet();
    stateset->setGlobalDefaults();
    setClearColor(osg::Vec4(0.2f, 0.0f, 0.3f, 0.0f));
    setClearMask(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);
    setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    setRenderOrder(osg::Camera::PRE_RENDER);
    pImpl_->constuctStateSet(stateset);
    pImpl_->prepareSecondPass();
    // set viewport
//    pImpl_->secondPassCamera_->setViewport(0, 0, defaultWidth, defaultHeight);
    pImpl_->resize(defaultWidth, defaultHeight); // use default values for now
    pImpl_->resizeRTTQuad(defaultWidth, defaultHeight);
}

namespace
{
    osg::ref_ptr<DeferredCamera> createDeferredCamera()
    {
        osg::ref_ptr< osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
        traits->x = 20; traits->y = 30;
        traits->width = 800; traits->height = 450;
        traits->windowDecoration = true;
        traits->doubleBuffer = true;
        traits->depth = true;
        osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext( traits.get() );
        if (! gc.valid())
        {
            osg::notify( osg::FATAL ) << "Unable to create OpenGL context." << std::endl;
            return nullptr;
        }

        // Create a Camera that uses the above OpenGL context.
        osg::ref_ptr<DeferredCamera> deferredCamera = new DeferredCamera();

        deferredCamera->setGraphicsContext(gc.get());
        deferredCamera->getSlaveCamera()->setGraphicsContext(gc.get());
        // Must set perspective projection for fovy and aspect.
        deferredCamera->setProjectionMatrix(osg::Matrix::perspective(30., (double)traits->width/(double)traits->height, 1., 100.));
        deferredCamera->handleResize(*gc);
        // Unlike OpenGL, OSG viewport does *not* default to window dimensions.
        deferredCamera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
        deferredCamera->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
        return deferredCamera;
    }
}
DeferredCamera* install(osgViewer::View& view)
{
    osg::ref_ptr<DeferredCamera> camera = createDeferredCamera();
    // use the deferred camera for the viewr
    view.setCamera(camera.get());
    view.addSlave(camera->getSlaveCamera(), false);
    return camera.get();
}
