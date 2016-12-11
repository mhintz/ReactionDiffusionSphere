#include <memory>
#include <vector>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Color.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"

#include "glm/gtc/constants.hpp"

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

	void mouseDrag(MouseEvent evt) override;
	void keyUp(KeyEvent evt) override;

	void drawVectorToFBO(std::vector<Color> const & pixelBuffer);
	void setupRoundedSquareRD(float side);
	void setupCircleRD(float rad);
	void setupSquareRD(float side);

	void updateRD();
	void drawRD();

	gl::FboCubeMapRef mSourceFbo;
	gl::FboCubeMapRef mDestFbo;

	gl::TriMeshRef mSphereMesh;

	Camera mCamera;
	CameraUi mCameraUi;

	gl::FboRef mRenderFbo;

	gl::GlslProgRef mRDProgram;
	gl::GlslProgRef mRenderRDProgram;

	bool mPauseSimulation = false;

	int mRDReadFboBinding = 0;
	int mRDRenderFboBinding = 1;
	int mRenderFboBinding = 2;
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
static int updatesPerFrame = 5;
static int cubeMapSide = 500;

void ReactionDiffusionCubeMapApp::prepSettings(Settings * settings) {
	settings->setFullScreen();
	settings->setTitle("Reaction Diffusion Interpretation");
	settings->setHighDensityDisplayEnabled();
}

void ReactionDiffusionCubeMapApp::setup()
{
	gl::Texture2d::Format colorTextureFormat = gl::Texture2d::Format().internalFormat(GL_RGB32F).wrap(GL_REPEAT);
	gl::Fbo::Format gridFboFmt = gl::Fbo::Format().colorTexture(colorTextureFormat);

	mSourceFbo = gl::FboCubeMap::create(cubeMapSide, cubeMapSide, gridFboFmt);
	mDestFbo = gl::FboCubeMap::create(cubeMapSide, cubeMapSide, gridFboFmt);

	mRenderFbo = gl::Fbo::create(getWindowWidth(), getWindowHeight());

	mCamera.lookAt(vec3(0, 0, 4), vec3(0), vec3(0, 1, 0));
	mCameraUi = CameraUi(& mCamera, getWindow());

	mRDProgram = gl::GlslProg::create(loadAsset("v_passThrough.glsl"), loadAsset("f_reactionDiffusion.glsl"));
	mRDProgram->uniform("gridSide", cubeMapSide);
	mRDProgram->uniform("uPrevFrame", mRDReadFboBinding);
	uploadRates(availableTypes[mInitialType]);

	mRenderRDProgram = setupRenderShader();

	gl::enableDepth();
	gl::enableFaceCulling();
	gl::cullface(GL_BACK);

	setupCircleRD(20);
	// setupSquareRD(40);
	// setupRoundedSquareRD(40);
}

gl::GlslProgRef ReactionDiffusionCubeMapApp::setupRenderShader() {
	auto renderProgram = gl::GlslProg::create(loadAsset("v_passThrough.glsl"), loadAsset("f_renderGrid.glsl"));
	renderProgram->uniform("uGridSampler", mRDRenderFboBinding);
	return renderProgram;
}

void ReactionDiffusionCubeMapApp::uploadRates(float * ratePair) {
	mRDProgram->uniform("feedRateA", ratePair[0]);
	mRDProgram->uniform("killRateB", ratePair[1]);
}

void ReactionDiffusionCubeMapApp::update()
{
	if (!mPauseSimulation) {	
		// Update the reaction-diffusion system multiple times per frame to speed things up
		for (int i = 0; i < updatesPerFrame; i++) {
			// Bind the source FBO to read the previous state
			gl::ScopedTextureBind scpTex(mSourceFbo->getColorTexture(), mRDReadFboBinding);
			// Bind the destination FBO to receive the new state
			gl::ScopedFramebuffer scpFB(mDestFbo);
			// Bind the update program
			gl::ScopedGlslProg scpShader(mRDProgram);
			// Draw a full screen rectangle
			gl::drawSolidRect(getWindowBounds());
			// Swap the source and destination FBOs for the next frame
			std::swap(mSourceFbo, mDestFbo);
		}
	}
}

void ReactionDiffusionCubeMapApp::draw()
{
	gl::clear(Color(0, 0, 0));

	// For debugging initial state
	// gl::ScopedTextureBind scpTex(mSourceFbo->getColorTexture(), mRDRenderFboBinding);

	{	
		gl::ScopedFramebuffer scpFB(mRenderFbo);
		// Draws the reaction-diffusion FBO
		gl::ScopedTextureBind scpTex(mDestFbo->getColorTexture(), mRDRenderFboBinding);
		// The reaction-diffusion render shader
		gl::ScopedGlslProg scpShader(mRenderRDProgram);
		// Draw a full screen rectangle
		gl::drawSolidRect(getWindowBounds());
	}

	{
		gl::draw(mRenderFbo->getColorTexture());
	}

	// Draw the framerate
	gl::drawString(std::to_string(getAverageFps()), vec2(10.0f, 20.0f), ColorA(0.0f, 0.0f, 0.0f, 1.0f));
}

void ReactionDiffusionCubeMapApp::mouseDrag(MouseEvent evt) {
	gl::ScopedFramebuffer scpFB(mSourceFbo);
	Color setValue = evt.isMetaDown() ? Color(0, 1, 0) : Color(0, 0, 1);
	float radius = evt.isMetaDown() ? 50.0f : 10.0f;
	gl::ScopedColor scpC(setValue);
	gl::drawSolidCircle(evt.getPos(), radius);
}

void ReactionDiffusionCubeMapApp::keyUp(KeyEvent evt) {
	char keyChar = evt.getChar();
	if (keyChar == 'r') {
		mRenderRDProgram = setupRenderShader();
	} else if (keyChar == 'p') {
		mPauseSimulation = !mPauseSimulation;
	} else if ('0' < keyChar && keyChar <= '0' + availableTypes.size()) {
		uploadRates(availableTypes[keyChar - '0']);
	}
}

Color & idxGrid(std::vector<Color> * grid, int width, int x, int y) {
	return (*grid)[x + width * y];
}

void ReactionDiffusionCubeMapApp::drawVectorToFBO(std::vector<Color> const & pixelBuffer) {
	auto initTexFmt = gl::Texture2d::Format().dataType(GL_FLOAT).internalFormat(GL_RGB32F);
	auto initTexture = gl::Texture2d::create(pixelBuffer.data(), GL_RGB, mWidth, mHeight, initTexFmt);

	gl::ScopedFramebuffer scpFB(mSourceFbo);
	gl::draw(initTexture, Rectf(0, 0, mWidth, mHeight));
}

void ReactionDiffusionCubeMapApp::setupRoundedSquareRD(float side) {
	vec2 center(mWidth / 2.0f, mHeight / 2.0f);
	float halfSide = side / 2.0f;

	gl::ScopedFramebuffer scpFB(mSourceFbo);

	// Clear to all A
	gl::clear(Color(0, 1, 0));

	// Initial state in a certain area is all B
	gl::ScopedColor scpC(Color(0, 0, 1));

	// Not sure why setting line width here doesn't work... seems a bit like a bug to me :(
	gl::ScopedLineWidth scpLW(8.0f);
	gl::drawStrokedRoundedRect(Rectf(center.x - halfSide, center.y - halfSide, center.x + halfSide, center.y + halfSide), 10);
}

void ReactionDiffusionCubeMapApp::setupCircleRD(float rad) {
	gl::ScopedFramebuffer scpFB(mSourceFbo);

	gl::clear(Color(0, 1, 0));

	gl::ScopedColor scpC(Color(0, 0, 1));

	gl::drawStrokedCircle(vec2(mWidth / 2.0f, mHeight / 2.0f), rad, 8.0f);
}

void ReactionDiffusionCubeMapApp::setupSquareRD(float side) {
	vec2 center(mWidth / 2.0f, mHeight / 2.0f);
	float halfSide = side / 2.0f;

	gl::ScopedFramebuffer scpFB(mSourceFbo);

	// All A
	gl::clear(Color(0, 1, 0));

	// All B
	gl::ScopedColor scpC(Color(0, 0, 1));

	gl::drawStrokedRect(Rectf(center.x - halfSide, center.y - halfSide, center.x + halfSide, center.y + halfSide), 8);
}

CINDER_APP( ReactionDiffusionCubeMapApp, RendererGl, & ReactionDiffusionCubeMapApp::prepSettings )
