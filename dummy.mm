#import <Foundation/Foundation.h>

#include <string>

std::string stringFromNS(NSString * str) {
  const char * cstr = [str UTF8String];
  return { cstr };
}
