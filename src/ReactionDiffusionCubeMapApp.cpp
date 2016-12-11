#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ReactionDiffusionCubeMapApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void ReactionDiffusionCubeMapApp::setup()
{
}

void ReactionDiffusionCubeMapApp::mouseDown( MouseEvent event )
{
}

void ReactionDiffusionCubeMapApp::update()
{
}

void ReactionDiffusionCubeMapApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( ReactionDiffusionCubeMapApp, RendererGl )
