#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <string>
#import <iostream>
#import "Interfaces.hpp"

using namespace std;

class ResourceManager : public IResourceManager {
public:
    string GetResourcePath() const
    {
        NSString* bundlePath = [[NSBundle mainBundle] resourcePath];
        return [bundlePath UTF8String];
    }
};

IResourceManager* CreateResourceManager()
{
    return new ResourceManager();
}