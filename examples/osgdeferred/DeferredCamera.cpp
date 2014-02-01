# include "DeferredCamera.h"

# include <osg/Shader>
# include <osg/StateSet>
# include <osg/Texture2D>
# include <osg/TextureRectangle>

# include <deque>

using namespace osg;

namespace
{
    const uint8_t  NUM_TEXTURES = 4;
}

struct DeferredCamera::PImpl
{
    PImpl() {}
    void constuctStateSet(osg::Camera* camera, osg::StateSet* stateSet)
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
            camera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), textureRect);
            fboTextures_.push_back(textureRect);
        }
    }
    void resize(uint32_t tex_width, uint32_t tex_height)
    {
        for (auto textureIte(fboTextures_.cbegin()), textureEnd(fboTextures_.cend()); textureIte != textureEnd; ++textureIte)
        {
            (*textureIte)->setTextureSize(tex_width, tex_height);
        }
    }
    osg::ref_ptr<osg::StateSet>  mrtStateSet_;
    typedef std::deque<osg::ref_ptr<osg::TextureRectangle> > PBOTextures;
    PBOTextures     fboTextures_;
};

DeferredCamera::DeferredCamera()
    : Camera()
    , pImpl_(new PImpl())
{
    constructorInit();
}

DeferredCamera::DeferredCamera(const Camera& other,const CopyOp& copyop)
    : Camera(other, copyop)
    , pImpl_(new PImpl())
{
    constructorInit();
}

void DeferredCamera::constructorInit()
{
    osg::StateSet* stateset = getOrCreateStateSet();
    stateset->setGlobalDefaults();
    setClearColor(osg::Vec4(0.2f, 0.0f, 0.3f, 0.0f));
    setClearMask(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);
    pImpl_->constuctStateSet(this, stateset);
    pImpl_->resize(512, 512); // use default values for now
}

