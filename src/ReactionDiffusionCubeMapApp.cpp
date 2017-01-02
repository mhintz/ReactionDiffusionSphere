#include <memory>
#include <vector>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Color.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"

#include "glm/gtc/constants.hpp"

#include "MeshHelpers.h"

using namespace ci;
using namespace ci::app;

class ReactionDiffusionCubeMapApp : public App {
public:
	static void prepSettings(Settings * settings);
	void setup() override;

	gl::GlslProgRef setupRenderShader();
	void uploadRates(float * ratePair);

	void update() override;
	void draw() override;

	void keyUp(KeyEvent evt) override;

	void drawVectorToFBO(std::vector<Color> const & pixelBuffer);
	void setupRoundedSquareRD(float side);
	void setupCircleRD(float rad);
	void setupSquareRD(float side);

	void updateRD();
	void drawRD();

	gl::TextureCubeMapRef mSourceTex;
	GLuint mSourceFbo;
	gl::TextureCubeMapRef mDestTex;
	GLuint mDestFbo;
	gl::GlslProgRef mRDProgram;
	gl::VboMeshRef mPointMesh;

	CameraPersp mCamera;
	CameraUi mCameraUi;

	gl::GlslProgRef mRenderRDProgram;
	gl::VboMeshRef mNormalizedCubeMesh;
	gl::BatchRef mNormalizedCubeBatch;

	bool mPauseSimulation = false;

	int mRDReadFboBinding = 0;
	int mRDRenderTextureBinding = 1;
};

static float typeAlpha[] = { 0.010, 0.047 };
static float typeDelta[] = { 0.042, 0.059 };
static float typeBeta[] = { 0.014, 0.039 };
static float typeGamma[] = { 0.022, 0.051 };
// optional: 0.024, 0.058
static float typeEpsilon[] = { 0.018, 0.055 };
static float typeZeta[] = { 0.022, 0.061 };
static float typeEta[] = { 0.034, 0.063 };
static float typeTheta[] = { 0.038, 0.061 };
// static float typeKappa[] = { 0.050, 0.063 };
static float typeKappa[] = { 0.0545, 0.062 };
static float typeLambda[] = { 0.034, 0.065 };
// optional: 0.0545, 0.062
static float typeXi[] = { 0.014, 0.047 };
static float typePi[] = { 0.062, 0.061 };

// Not so interesting, or there are better versions elsewhere:
// static float typeRho[] = { 0.090, 0.059 };
// static float typeSigma[] = { 0.110, 0.0523 };

static std::map<int, float *> availableTypes = {
	{1, typeAlpha},
	{2, typeBeta},
	{3, typeGamma},
	{4, typeEpsilon},
	{5, typeTheta},
	{6, typeXi},
	{7, typePi},
	{8, typeLambda},
	{9, typeKappa},
};

static int mInitialType = 9;
static int updatesPerFrame = 10;
static int cubeMapSide = 512;
// static int cubeMapSide = 256;

void ReactionDiffusionCubeMapApp::prepSettings(Settings * settings) {
	settings->setFullScreen();
	settings->setTitle("Reaction Diffusion Interpretation");
	settings->setHighDensityDisplayEnabled();
}

void ReactionDiffusionCubeMapApp::setup()
{
	auto colorTextureFormat = gl::TextureCubeMap::Format()
		.internalFormat(GL_RGB32F)
		.wrap(GL_CLAMP_TO_EDGE)
		.minFilter(GL_NEAREST)
		.magFilter(GL_NEAREST)
		.mipmap(false);

	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };

	{
		mSourceTex = gl::TextureCubeMap::create(cubeMapSide, cubeMapSide, colorTextureFormat);
		glGenFramebuffers(1, & mSourceFbo);
		gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mSourceFbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mSourceTex->getId(), 0);
		glDrawBuffers(1, drawBuffers);
		gl::clear(Color(0, 1, 0)); // Clear to all "A"
	}

	{
		mDestTex = gl::TextureCubeMap::create(cubeMapSide, cubeMapSide, colorTextureFormat);
		glGenFramebuffers(1, & mDestFbo);
		gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mDestFbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mDestTex->getId(), 0);
		glDrawBuffers(1, drawBuffers);
		gl::clear(Color(0, 1, 0)); // Clear to all "A"
	}

	mRDProgram = gl::GlslProg::create(loadAsset("reactionDiffusion_v.glsl"), loadAsset("reactionDiffusion_f.glsl"), loadAsset("reactionDiffusion_g.glsl"));
	mRDProgram->uniform("gridSideLength", cubeMapSide);
	mRDProgram->uniform("uPrevFrame", mRDReadFboBinding);
	uploadRates(availableTypes[mInitialType]);

	mCamera.lookAt(vec3(0, 0, 4), vec3(0), vec3(0, 1, 0));
	mCameraUi = CameraUi(& mCamera, getWindow());

	{
		vec3 pointBuf[1] = { vec3(cubeMapSide / 2, cubeMapSide / 2, 0) };
		auto pointVbo = gl::Vbo::create(GL_ARRAY_BUFFER, sizeof(vec3), pointBuf, GL_STATIC_DRAW);
		auto pointVboLayout = geom::BufferLayout({ geom::AttribInfo(geom::POSITION, 3, 0, 0) });
		mPointMesh = gl::VboMesh::create(1, GL_POINTS, { { pointVboLayout, pointVbo } });
	}

	mRenderRDProgram = setupRenderShader();
	mNormalizedCubeMesh = makeNormalizedCubeSphere();
	mNormalizedCubeBatch = gl::Batch::create(mNormalizedCubeMesh, mRenderRDProgram, { { geom::CUSTOM_0, "aFaceSide" } });

	setupCircleRD(20);
	// setupSquareRD(40);
	// setupRoundedSquareRD(40);

	gl::enableFaceCulling();
	gl::cullFace(GL_BACK);
}

gl::GlslProgRef ReactionDiffusionCubeMapApp::setupRenderShader() {
	auto renderProgram = gl::GlslProg::create(loadAsset("renderGrid_v.glsl"), loadAsset("renderGrid_f.glsl"));
	renderProgram->uniform("uGridSampler", mRDRenderTextureBinding);
	return renderProgram;
}

void ReactionDiffusionCubeMapApp::uploadRates(float * ratePair) {
	mRDProgram->uniform("feedRateA", ratePair[0]);
	mRDProgram->uniform("killRateB", ratePair[1]);
}

void ReactionDiffusionCubeMapApp::update()
{
	if (!mPauseSimulation) {
		gl::ScopedDepth scpDepth(false);

		gl::ScopedViewport scpView(0, 0, cubeMapSide, cubeMapSide);

		gl::ScopedMatrices scpMat;
		gl::setMatricesWindow(cubeMapSide, cubeMapSide);

		gl::ScopedGlslProg scpShader(mRDProgram);

		// Update the reaction-diffusion system multiple times per frame to speed things up
		for (int i = 0; i < updatesPerFrame; i++) {
			// Bind the source texture to read the previous state
			gl::ScopedTextureBind scpTex(mSourceTex, mRDReadFboBinding);
			// Bind the destination FBO (with the destination texture attached) to write the new state
			gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mDestFbo);
			
			// Draw a single point, using the GS to expand this one vertex into the six layers of the CubeMap texture
			gl::draw(mPointMesh);

			// Swap both the Texture pointers and the Fbo IDs to ping pong the buffers
			std::swap(mSourceTex, mDestTex);
			std::swap(mSourceFbo, mDestFbo);
		}
	}
}

void ReactionDiffusionCubeMapApp::draw()
{
	{
		gl::ScopedDepth scpDepth(true);

		gl::ScopedMatrices scpMat;
		gl::setMatrices(mCamera);

		gl::clear(Color(0, 0, 0));
		
		// // Draws the reaction-diffusion Cubemap on the sphere
		gl::ScopedTextureBind scpTex(mDestTex, mRDRenderTextureBinding);

		mNormalizedCubeBatch->draw();
	}

	// Debug Zone
	{
		// Draw the framerate
		gl::drawString(std::to_string(getAverageFps()), vec2(10.0f, 20.0f), ColorA(1.0f, 1.0f, 1.0f, 1.0f));
	}
}

void ReactionDiffusionCubeMapApp::keyUp(KeyEvent evt) {
	char keyChar = evt.getChar();
	if (keyChar == 'r') {
		try {
			mRenderRDProgram = setupRenderShader();
		} catch (gl::GlslProgCompileExc exp) {
			console() << exp.what() << std::endl;
		}
	} else if (keyChar == 'p') {
		mPauseSimulation = !mPauseSimulation;
	} else if ('0' < keyChar && keyChar <= '0' + availableTypes.size()) {
		uploadRates(availableTypes[keyChar - '0']);
	} else if (evt.getCode() == KeyEvent::KEY_ESCAPE) {
		quit();
	}
}

// void ReactionDiffusionCubeMapApp::drawVectorToFBO(std::vector<Color> const & pixelBuffer) {
// 	auto initTexFmt = gl::Texture2d::Format().dataType(GL_FLOAT).internalFormat(GL_RGB32F);
// 	auto initTexture = gl::Texture2d::create(pixelBuffer.data(), GL_RGB, cubeMapSide, cubeMapSide, initTexFmt);

// 	gl::ScopedViewport scpView(0, 0, cubeMapSide, cubeMapSide);
// 	gl::ScopedDepth scpDepth(false);

// 	gl::ScopedMatrices scpMat;
// 	gl::setMatricesWindow(cubeMapSide, cubeMapSide);

// 	mSourceFbo->bindFramebufferFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);

// 	gl::draw(initTexture, Rectf(0, 0, cubeMapSide, cubeMapSide));

// 	mSourceFbo->unbindFramebuffer();
// }

// void ReactionDiffusionCubeMapApp::setupRoundedSquareRD(float side) {
// 	vec2 center(cubeMapSide / 2.0f, cubeMapSide / 2.0f);
// 	float halfSide = side / 2.0f;

// 	gl::ScopedViewport scpView(0, 0, cubeMapSide, cubeMapSide);
// 	gl::ScopedDepth scpDepth(false);

// 	gl::ScopedMatrices scpMat;
// 	gl::setMatricesWindow(cubeMapSide, cubeMapSide);

// 	mSourceFbo->bindFramebufferFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);

// 	// Clear to all A
// 	gl::clear(Color(0, 1, 0));

// 	// Initial state in a certain area is all B
// 	gl::ScopedColor scpC(Color(0, 0, 1));

// 	// Not sure why setting line width here doesn't work... seems a bit like a bug to me :(
// 	// I think the reason why this doesn't work is because modern systems don't support glLineWidth any more... :(
// 	gl::ScopedLineWidth scpLW(8.0f);
// 	gl::drawStrokedRoundedRect(Rectf(center.x - halfSide, center.y - halfSide, center.x + halfSide, center.y + halfSide), 10);

// 	mSourceFbo->unbindFramebuffer();
// }

void ReactionDiffusionCubeMapApp::setupCircleRD(float rad) {
	gl::ScopedViewport scpView(0, 0, cubeMapSide, cubeMapSide);
	gl::ScopedMatrices scpMat;
	gl::setMatricesWindow(cubeMapSide, cubeMapSide);

	GLuint tempFbo;
	glGenFramebuffers(1, & tempFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, mSourceTex->getId(), 0);

	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	gl::clear(Color(0, 1, 0));

	gl::ScopedColor scpC(Color(0, 0, 1));

	gl::drawStrokedCircle(vec2(cubeMapSide / 2.0f, cubeMapSide / 2.0f), rad, 8.0f);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, & tempFbo);
}

// void ReactionDiffusionCubeMapApp::setupSquareRD(float side) {
// 	vec2 center(cubeMapSide / 2.0f, cubeMapSide / 2.0f);
// 	float halfSide = side / 2.0f;

// 	gl::ScopedViewport scpView(0, 0, cubeMapSide, cubeMapSide);
// 	gl::ScopedDepth scpDepth(false);

// 	gl::ScopedMatrices scpMat;
// 	gl::setMatricesWindow(cubeMapSide, cubeMapSide);

// 	mSourceFbo->bindFramebufferFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);

// 	// All A
// 	gl::clear(Color(0, 1, 0));

// 	// All B
// 	gl::ScopedColor scpC(Color(0, 0, 1));

// 	gl::drawStrokedRect(Rectf(center.x - halfSide, center.y - halfSide, center.x + halfSide, center.y + halfSide), 8);

// 	mSourceFbo->unbindFramebuffer();
// }

CINDER_APP( ReactionDiffusionCubeMapApp, RendererGl, & ReactionDiffusionCubeMapApp::prepSettings )
