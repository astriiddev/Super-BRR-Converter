#if defined (__APPLE__)

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <Availability.h>

#if  MACOS_X_VERSION_MAX_ALLOWED >= 1100
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

#import <libgen.h>

#import "sbc_filedialog.h"
#import "sbc_utils.h"

char* getHomeDir(void) 
{ 
	char doc_dir[256];
	snprintf(doc_dir, 256, "%s%s", getenv("HOME"), "/Documents");
    return _strndup(doc_dir, strlen(doc_dir)); 
}

const char *getFileNameWithoutExt(const char * file) 
{
    char *bname = basename((char *) file), *ext;
    size_t name_len = strlen(bname);

    ext = strrchr(bname, '.');
    if(ext == NULL) return bname == NULL || name_len <= 0 ? NULL : _strndup(bname, name_len + 1);

    return bname == NULL || name_len <= 0 ? NULL : _strndup(bname, (ext - bname));
}

const char* saveDialog(const char *dir_path, const char* sample_name)
{
    char *filename = NULL;
    NSSavePanel *saveDlg = [NSSavePanel savePanel];
    
    NSMutableString *sample_file = [NSMutableString stringWithUTF8String:sample_name];

#if  MACOS_X_VERSION_MAX_ALLOWED >= 1100
    UTType *const UTTypeBRR = [UTType typeWithFilenameExtension:(@"brr")],
           *const UTTypeIFF = [UTType typeWithFilenameExtension:(@"iff")];

    NSArray<UTType*> *fileTypesArray = @[UTTypeWAV, UTTypeBRR, UTTypeIFF, UTTypeAIFF];
    [saveDlg setAllowedContentTypes:fileTypesArray];
#else
    NSArray<NSString*> *fileTypesArray = @[@"wav", @"brr", @"iff", @"aif"];
    [saveDlg setAllowedFileTypes:fileTypesArray];
#endif
    
    [sample_file appendString:@".wav"];
    [saveDlg setNameFieldStringValue:sample_file];
    
    [saveDlg setAllowsOtherFileTypes:YES];

    // Display the dialog box.  If OK is pressed, process the files.
    if ([saveDlg runModal] == NSModalResponseOK) 
    {
        // Get list of all files selected
        NSURL* file = [saveDlg URL];

        const char *fd= [[file path] UTF8String];
        filename = _strndup((char*) fd, strlen(fd));
    }
    
    (void) dir_path;

    return filename;
}

const char * openDialog(const char * path)
{
    char *filename = NULL;
    NSOpenPanel *openDlg = [NSOpenPanel openPanel];
    
#if  MACOS_X_VERSION_MAX_ALLOWED >= 1100
    UTType *const UTTypeBRR = [UTType typeWithFilenameExtension:(@"brr")],
           *const UTTypeIFF = [UTType typeWithFilenameExtension:(@"iff")];
    
    NSArray<UTType*> *fileTypesArray = @[UTTypeWAV, UTTypeBRR, UTTypeIFF, UTTypeAIFF];
    [openDlg setAllowedContentTypes:fileTypesArray];
#else
    NSArray<NSString*> *fileTypesArray = @[@"wav", @"brr", @"iff", @"aif"];
    [openDlg setAllowedFileTypes:fileTypesArray];
#endif

    // Enable options in the dialog.
    [openDlg setCanChooseFiles:YES];
    [openDlg setAllowsMultipleSelection:NO];
    [openDlg setAllowsOtherFileTypes:YES];

    // Display the dialog box.  If OK is pressed, process the files.
    if ([openDlg runModal] == NSModalResponseOK) 
    {
        // Get list of all files selected
        NSArray<NSURL*> *files = [openDlg URLs];

        const char *fd= [[files[0] path] UTF8String];
        filename = _strndup((char*) fd, strlen(fd));
    }

    (void) path;
    
    return filename;
}

void soundErrorBell(void) { NSBeep(); }

/*
*======================================================================================

	Unused in Mac version

*======================================================================================
*/

void setLastDir(const char* path) { (void) path; return; }
char *getLastDir(void) { return getHomeDir(); }
void initFileDialog(int *argc, char ***argv) { (void) argc; (void) argv; }
void handleFileDialogEvents(void) { return; }

#endif /* __APPLE__ */
