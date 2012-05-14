#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <string>
#import <iostream>
#import "Interfaces.hpp"

using namespace std;

class ResourceManager : public IResourceManager {
private:
    ivec2 m_imageSize;
    CFDataRef m_imageData;
public:
    string GetResourcePath() const
    {
        NSString* bundlePath = [[NSBundle mainBundle] resourcePath];
        return [bundlePath UTF8String];
    }
    void LoadPngImage(const string& name)
    {
        NSString* basePath = [NSString stringWithUTF8String:name.c_str()];
        NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
        NSString* fullPath = [resourcePath stringByAppendingPathComponent:basePath]; 
        UIImage* uiImage = [UIImage imageWithContentsOfFile:fullPath];
        CGImageRef cgImage = uiImage.CGImage;
        m_imageSize.x = CGImageGetWidth(cgImage);
        m_imageSize.y = CGImageGetHeight(cgImage);
        m_imageData = CGDataProviderCopyData(CGImageGetDataProvider(cgImage));
    }
    void* GetImageData()
    {
        return (void*) CFDataGetBytePtr(m_imageData);
    }
    ivec2 GetImageSize()
    {
        return m_imageSize;
    }
    void UnloadImage()
    {
        CFRelease(m_imageData);
    }
};

IResourceManager* CreateResourceManager()
{
    return new ResourceManager();
}