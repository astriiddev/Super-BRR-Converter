#ifndef __APPLE__

#if defined (_WIN32)

#include <Windows.h>
#include <shobjidl.h>

#elif defined (__linux__)

#include <gtk/gtk.h>

#endif

#include "sbc_filedialog.h"
#include "sbc_utils.h"

static char *last_dir = NULL;

static const char* get_dir_from_filepath(const char *file);

char *getLastDir(void) { return last_dir == NULL ? getWorkingDir() : last_dir; }

void setLastDir(const char* path)
{
    SBC_FREE(last_dir);

    if(path == NULL) return;

    last_dir = (char *) get_dir_from_filepath(path);
}

#if defined (_WIN32)

char *getHomeDir(void)
{
	char doc_dir[256];
	snprintf(doc_dir, 256, "%s%s", getenv("USERPROFILE"), "\\Documents");
	return _strndup(doc_dir, strlen(doc_dir) + strlen("Documents"));
}

void initFileDialog(int *argc, char ***argv) { (void) argc; (void) argv; }
void handleFileDialogEvents(void) { do{;} while(0); }

const char *getFileNameWithoutExt(const char * file) 
{
    char drive[_MAX_DRIVE], dir[_MAX_DIR], ext[_MAX_EXT], name[_MAX_FNAME];
    _splitpath_s(file, drive, _MAX_DRIVE, dir, _MAX_DIR, name, _MAX_FNAME, ext, _MAX_EXT);

    return _strndup(name, strlen(name));
}

const char *get_dir_from_filepath(const char *file)
{
    char drive[_MAX_DRIVE], dir[_MAX_DIR], ext[_MAX_EXT], name[_MAX_FNAME], *dir_name;
    size_t dir_len;

    _splitpath_s(file, drive, _MAX_DRIVE, dir, _MAX_DIR, name, _MAX_FNAME, ext, _MAX_EXT);

    dir_len  = strlen(drive) + strlen(dir);
    dir_name = calloc(1, dir_len + 1);

    snprintf(dir_name, dir_len + 1, "%s%s", drive, dir);

    return dir_name;
}

static void add_standard_file_filters(IFileDialog* pFileDialog)
{
    COMDLG_FILTERSPEC rgSpec[] = 
    {
        { L"WAV Files (*.wav)", L"*.wav" },
        { L"BRR Files (*.brr)", L"*.brr" },
        { L"IFF Files (*.iff), (*.8svx)", L"*.iff;*.8svx" },
        { L"AIF Files (*.aif), (*.aiff)", L"*.aif;*.aiff" },
        { L"All Files (*)", L"*" }
    };
    
    if(pFileDialog == NULL) return;
    
    pFileDialog->lpVtbl->SetFileTypes(pFileDialog, ARRAYSIZE(rgSpec), rgSpec);
}

const char *getFilePathResult(IFileDialog* pFileDialog)
{
    const char* filename = NULL;

    IShellItem* pItem;
    PWSTR pszFilePath;

    HRESULT hr = pFileDialog->lpVtbl->GetResult(pFileDialog, &pItem);
    
    if (FAILED(hr)) return NULL;

    hr = pItem->lpVtbl->GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);

    if (SUCCEEDED(hr))
    {
        const size_t path_len = wcslen(pszFilePath);

        filename = calloc(path_len + 1, sizeof *filename);
        wcstombs((char*) filename, pszFilePath, path_len);

        CoTaskMemFree(pszFilePath);
        setLastDir(filename);
    }

    pItem->lpVtbl->Release(pItem);

    return filename;
}

static void setDefaultDirectory(IFileDialog* pFileDialog, const char *dir_path)
{
    IShellItem *pCurFolder = NULL; 
    HRESULT hr;

    const size_t dir_len = strlen(dir_path);
    WCHAR* curr_dir = calloc(dir_len + 1, sizeof *curr_dir);

    if (curr_dir == NULL) return;

    mbstowcs(curr_dir, dir_path, dir_len + 1);

    hr = SHCreateItemFromParsingName(curr_dir, NULL, &IID_IShellItem, (LPVOID*) &pCurFolder);
    
    free(curr_dir);

    if(FAILED(hr)) return;

    pFileDialog->lpVtbl->SetFolder(pFileDialog, pCurFolder);
    pCurFolder->lpVtbl->Release(pCurFolder);
}

static void setDefaultFileName(IFileDialog* pFileDialog, const char *sample_name)
{
    const size_t name_len = strlen(sample_name);
    WCHAR *curr_file = calloc(name_len + 1, sizeof *curr_file);

    mbstowcs(curr_file, sample_name, name_len + 1);

    pFileDialog->lpVtbl->SetFileName(pFileDialog, curr_file);
    pFileDialog->lpVtbl->SetDefaultExtension(pFileDialog, L".wav");

    free(curr_file);
}

const char* openDialog(const char *dir_path)
{
    const char* filename = NULL;
    IFileDialog* pFileOpen;
    
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (FAILED(hr)) return NULL;

    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL, &IID_IFileOpenDialog, (LPVOID*) &pFileOpen);

    if (SUCCEEDED(hr))
    {
        setDefaultDirectory(pFileOpen, dir_path);
        add_standard_file_filters(pFileOpen);

        hr = pFileOpen->lpVtbl->Show(pFileOpen, NULL);

        if (SUCCEEDED(hr))
        {
            filename = getFilePathResult(pFileOpen);
        }

        pFileOpen->lpVtbl->Release(pFileOpen);

        CoUninitialize();
    }
    
    return filename;
}

const char *saveDialog(const char *dir_path, const char *sample_name)
{
    const char* filename = NULL;
    IFileDialog* pFileSave;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (FAILED(hr)) return NULL;

    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_ALL, &IID_IFileSaveDialog, (LPVOID*) &pFileSave);

    if (SUCCEEDED(hr))
    {
        if(dir_path != NULL) setDefaultDirectory(pFileSave, dir_path);
        if(sample_name != NULL) setDefaultFileName(pFileSave, sample_name);
        add_standard_file_filters(pFileSave);

        hr = pFileSave->lpVtbl->Show(pFileSave, NULL);

        if (SUCCEEDED(hr))
        {
            filename = getFilePathResult(pFileSave);
        }

        pFileSave->lpVtbl->Release(pFileSave);
    }

    CoUninitialize();

    return filename;
}

void soundErrorBell(void) { MessageBeep(MB_ICONERROR); }

#elif defined (__linux__)

void initFileDialog(int *argc, char ***argv) { gtk_init(argc, argv); }

void handleFileDialogEvents(void) { while (gtk_events_pending()) gtk_main_iteration(); }

char* getHomeDir(void) 
{ 
    char* home_dir = getenv("HOME");
    return _strndup(home_dir, strlen(home_dir)); 
}

static void add_gtk_file_filter(GtkFileChooser *chooser, const char* label, const char *type1, const char *type2)
{
    GtkFileFilter *filter = gtk_file_filter_new();

    gtk_file_filter_set_name(filter, label);
    gtk_file_filter_add_pattern(filter, type1);

    if(type2 != NULL) gtk_file_filter_add_pattern(filter, type2);

    gtk_file_chooser_add_filter(chooser, filter);
}

static void add_standard_file_filters(GtkFileChooser *chooser)
{
    add_gtk_file_filter(chooser, "WAV Files (*.wav)", "*.wav", NULL);
    add_gtk_file_filter(chooser, "BRR Files (*.brr)", "*.brr", NULL);
    add_gtk_file_filter(chooser, "IFF Files (*.iff), (*.8svx)", "*.iff", "*.8svx");
    add_gtk_file_filter(chooser, "AIF Files (*.aif), (*.aiff)", "*.aif", "*.aiff");
    add_gtk_file_filter(chooser, "All Files (*)", "*", NULL);
}

const char* openDialog(const char *dir_path)
{
    const char *filename = NULL, *working_dir;
    GtkWidget *dialog;
    gint res;

    dialog = gtk_file_chooser_dialog_new ("Open File", NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        ("_Cancel"), GTK_RESPONSE_CANCEL,
                                        ("_Open"), GTK_RESPONSE_ACCEPT, NULL);
    
    add_standard_file_filters(GTK_FILE_CHOOSER(dialog));

    working_dir = dir_path == NULL || strlen(dir_path) <= 0 ? getHomeDir() : dir_path;

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), working_dir);

    res = gtk_dialog_run (GTK_DIALOG (dialog));

    if (res == GTK_RESPONSE_ACCEPT)
    {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
        filename = gtk_file_chooser_get_filename (chooser);
        setLastDir(filename);
    }

    gtk_widget_destroy (dialog);
    
    return filename;
}

const char *saveDialog(const char *dir_path, const char *sample_name)
{
    const char *filename = NULL, *working_dir;
    GtkWidget *dialog;
    gint res;

    dialog = gtk_file_chooser_dialog_new ("Save File", NULL,
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        ("_Cancel"), GTK_RESPONSE_CANCEL,
                                        ("_Save"), GTK_RESPONSE_ACCEPT, NULL);
    
    add_standard_file_filters(GTK_FILE_CHOOSER(dialog));

    working_dir = dir_path == NULL || strlen(dir_path) <= 0 ? getHomeDir() : dir_path;
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), working_dir);

    if(sample_name != NULL && strlen(sample_name) > 0)
    {
        char *name_ext;
        const int sampname_len = (int) strlen(sample_name);

        name_ext = calloc(sampname_len + 5, sizeof *name_ext);
        snprintf(name_ext, sampname_len + 5, "%s%s", sample_name, ".wav");
        
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), name_ext);
        g_free(name_ext);
    }
    
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
    
    res = gtk_dialog_run (GTK_DIALOG (dialog));

    if (res == GTK_RESPONSE_ACCEPT)
    {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
        filename = gtk_file_chooser_get_filename (chooser);
        setLastDir(filename);
    }

    gtk_widget_destroy (dialog);
    
    return filename;
}

static const char* get_dir_from_filepath(const char *file)
{
    char* path = _strndup((char*) file, strlen(file));
    return g_path_get_dirname(path);
}

const char *getFileNameWithoutExt(const char * file) 
{
    char *bname = g_path_get_basename((char *) file), *ext;
    size_t name_len = strlen(bname);

    ext = strrchr(bname, '.');

    if(ext == NULL) return bname == NULL || name_len <= 0 ? NULL : _strndup(bname, name_len + 1);
    return bname == NULL || name_len <= 0 ? NULL : _strndup(bname, (ext - bname));
}

void soundErrorBell(void) { gdk_display_beep(gdk_display_get_default()); }  // TODO: make this async

#endif /* __linux__ */

#endif /* __APPLE__ */
