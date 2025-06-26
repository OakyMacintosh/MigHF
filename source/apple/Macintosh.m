#import "mighf/Qadra.h"
#import "mighf/Macintosh.h"

@implementation Macintosh
    - (instancetype)init {
    self = [super init];
    if (self) {
        _name = @"Macintosh";
        _icon = @"macintosh";
        _type = @"Apple";
        _manufacturer = @"Apple Inc.";
        _year = 1984;
        _cpu = @"Motorola 68000";
        _ram = @"128 KB";
        _storage = @"400 KB floppy disk";
    }
    return self;
    
}

- (NSString *)description {
    return [NSString stringWithFormat:@"%@ (%@) - %@, %@, %@, %@, %@",
            self.name, self.type, self.manufacturer, self.year, self.cpu, self.ram, self.storage];
}

- (NSString *)icon {
    return [NSString stringWithFormat:@"icon-%@", self.icon];
}

- (NSString *)type {
    return [NSString stringWithFormat:@"type-%@", self.type];
}

- (NSString *)manufacturer {
    return [NSString stringWithFormat:@"manufacturer-%@", self.manufacturer];
}

- (NSString *)year {
    return [NSString stringWithFormat:@"year-%ld", (long)self.year];
}

- (NSString *)cpu {
    return [NSString stringWithFormat:@"cpu-%@", self.cpu];
}

- (void) setRam:(NSString *)ram {
    _ram = ram;
}

// Check MacOS version, and give an error if it's higher than 10.8
- (NSString *)checkMacOSVersion {
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    return [NSString stringWithFormat:@"MacOS Version: %ld.%ld.%ld", (long)version.majorVersion, (long)version.minorVersion, (long)version.patchVersion];
}

