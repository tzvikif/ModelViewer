#import "Interfaces.hpp"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

@interface GLView : UIView {
@private
    IApplicationEngine* m_applicationEngine;
    IRenderingEngine* m_renderingEngine;
    EAGLContext* m_context;
    float m_timestamp;
    IResourceManager* m_resourceManager;
}

- (void) drawView: (CADisplayLink*) displayLink;

@end
