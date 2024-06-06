#include <sys/stat.h>

#if defined (_WIN32)
#include <Windows.h>
#else

#include <unistd.h>
#include <dirent.h>

#if defined (__linux__)
#include <linux/limits.h>
#elif defined (__APPLE__)
#include <sys/syslimits.h>
#endif

#endif

#include "sbc_utils.h"
#include "sbc_audio.h"
#include "sbc_conf.h"

#if defined (_WIN32)

#ifndef PATH_MAX
#define PATH_MAX _MAX_DIR
#endif

/*
*   getline slightly modified from:
*       https://dev.w3.org/libwww/Library/src/vms/getline.c
*/
static int getline(char **lineptr, size_t *n, FILE *stream)
{
    static char line[256];
    char* ptr;
    size_t len;

    if (lineptr == NULL || n == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if (ferror(stream))
        return -1;

    if (feof(stream))
        return -1;

    fgets(line, 256, stream);

    ptr = strchr(line, '\n');
    if (ptr) *ptr = '\0';

    len = strlen(line);
    if (ptr) *ptr = '\n';

    if ((len + 1) < 256)
    {
        ptr = realloc(*lineptr, 256);
        if (ptr == NULL)
            return(-1);
        *lineptr = ptr;
        *n = 256;
    }

    strcpy(*lineptr, line);
    return (int) len;
}

#endif

bool check_path_exists(const char* path)
{
    struct stat sb;
    assert(path != NULL);
    return (stat(path, &sb) == 0);
}

static char* get_conf_dir(void)
{
    char *conf_dir = NULL;
    SBC_CALLOC(PATH_MAX, sizeof *conf_dir, conf_dir);

#if defined (_WIN32)
    if(getenv("APPDATA") != NULL) snprintf(conf_dir, PATH_MAX, "%s", getenv("APPDATA"));
    else snprintf(conf_dir, PATH_MAX, "%s\\AppData\\Roaming", getenv("USERPROFILE"));
#elif defined (__linux__)
    snprintf(conf_dir, PATH_MAX, "%s/.config", getenv("HOME"));
#elif defined (__APPLE__)
    snprintf(conf_dir, PATH_MAX, "%s/Library/Application Support", getenv("HOME"));
#else
    SBC_FREE(conf_dir);
    conf_dir = NULL;
#endif

    return conf_dir;
}

static bool check_config_dir(void)
{
    char *conf_dir = NULL;
    bool dir_exists = true;

    conf_dir = get_conf_dir();
    dir_exists = check_path_exists(conf_dir);

    SBC_FREE(conf_dir);

    return dir_exists;
}

static char *get_prog_path(void)
{
    char *prog_dir = NULL;
    SBC_CALLOC(PATH_MAX, sizeof *prog_dir, prog_dir);

#if defined (_WIN32)
    if(!GetCurrentDirectoryA(PATH_MAX, prog_dir))
#else
    if(getcwd(prog_dir, PATH_MAX) == NULL)
#endif
    {
        SBC_FREE(prog_dir);
        return NULL;
    }

    return prog_dir;
}

static char *get_conf_path(const bool test_exist)
{
    char *conf_path = NULL, *prog_dir = NULL;
#if defined (_WIN32)
    const char *conf_file = "\\sbc.conf";
#else
    const char *conf_file = "/sbc.conf";
#endif
    size_t conf_len = PATH_MAX + strlen(conf_file);

    SBC_CALLOC(conf_len, sizeof *conf_path, conf_path);

    if(check_config_dir())
    {
        prog_dir = get_conf_dir();

        snprintf(conf_path, conf_len, "%s%s", prog_dir, conf_file);

        SBC_FREE(prog_dir);

        if(!test_exist) return conf_path;

        if(check_path_exists(conf_path)) return conf_path;
    }

    if((prog_dir = get_prog_path()) == NULL) 
    {
        SBC_FREE(prog_dir);
        SBC_FREE(conf_path);
        return NULL;
    }

    memset(conf_path, 0, conf_len);
    snprintf(conf_path, conf_len, "%s%s", prog_dir, conf_file);

    SBC_FREE(prog_dir);
    return conf_path;
}

void loadConfig(void)
{
    FILE *conf_file;
    char *line = NULL, *conf_path = NULL;
    size_t len = 0;

    if((conf_path = get_conf_path(true)) == NULL) return;

    SBC_LOG(CONFIG FILE PATH, % s, conf_path);
    if((conf_file = fopen(conf_path, "r")) == NULL) 
    {
        SBC_FREE(conf_path);
        return;
    }

    SBC_FREE(conf_path);

    while(getline(&line, &len, conf_file) != -1)
    {
        char *ptr, *val_ptr;
        int val= 0;

        if(strchr(line, '#')) 
        {
            SBC_FREE(line);
            continue;
        }

#ifdef DEBUG
        printf("%s", line);
#endif

        if((val_ptr = strchr(line, ':')) == NULL)
        {
            SBC_FREE(line);
            continue;
        };

        val = (int) strtol(val_ptr + 1, &ptr, 10);

        if(_strcasestr(line, "Driver Num: ")) setAudioDriver(val);
        else if(_strcasestr(line, "Output Device Num: ")) setOutputDevice(val);
        else if(_strcasestr(line, "Input Device Num: "))  setInputDevice(val);
        else if(_strcasestr(line, "Sample Rate Selection: ")) setDeviceSampleRate(val);
        else if(_strcasestr(line, "Buffer Size Selection: ")) setAudioBufferSize(val);
        else if(_strcasestr(line, "Interpolation Selection: ")) setInterpolationType(val);
        else if(_strcasestr(line, "Default Dir: ")) 
        {
            const size_t line_len = strlen(line), dhdr_len = strlen("Default Dir: ");

            if(line_len - dhdr_len > 0) 
            {
                char *dir = NULL, 
                     *endline = strchr(val_ptr + 2, '\n'); 
                
                *(endline) = '\0';
                dir = _strndup(val_ptr + 2, line_len - dhdr_len - 1);
                setWorkingDir(dir);
                SBC_FREE(dir);
            }
        }

        SBC_FREE(line);
    }

    fclose(conf_file);
}

static bool write_header(FILE *conf_file)
{
    char *header =  "########################################################################\n" \
                    "#          Configuration file for SBC700 Super BRR Converter           #\n" \
                    "#           Used for internally loading and saving settings            #\n" \
                    "#    Changing settings in a text editor may cause potential issues!    #\n" \
                    "#       SBC700 by _astriid_ (Finley Baker) astriiddev@gmail.com        #\n" \
                    "########################################################################\n\n";

    assert(conf_file != NULL);

    if (fwrite(header, sizeof *header, strlen(header), conf_file) < strlen(header)) 
        return false;

    return true;
}

static bool write_devices(FILE *conf_file)
{
    bool success = true;

    char* header = "# Audio driver/device numbers\n";
    char driver[32], out_dev[32], in_dev[32];

    assert(conf_file != NULL);

    snprintf(driver,  32, "Driver Num: %d\n", getCurrentAudioDriver());
    snprintf(out_dev, 32, "Output Device Num: %d\n", getCurrentOutDev());
    snprintf(in_dev,  32, "Input Device Num: %d\n\n", getCurrentInDev());

    if (fwrite(header,  sizeof *header,  strlen(header),  conf_file) < strlen(header))  success = false;
    if (fwrite(driver,  sizeof *driver,  strlen(driver),  conf_file) < strlen(driver))  success = false;
    if (fwrite(out_dev, sizeof *out_dev, strlen(out_dev), conf_file) < strlen(out_dev)) success = false;
    if (fwrite(in_dev,  sizeof *in_dev,  strlen(in_dev),  conf_file) < strlen(in_dev))  success = false;

    return success;
}

static bool write_dev_settings(FILE *conf_file)
{
    bool success = true;

    char* header = "# Audio device settings\n";
    char samp_rate[32], buffer_size[32], interpolation[32];

    assert(conf_file != NULL);

    snprintf(samp_rate,      32, "Sample Rate Selection: %d\n",     getDeviceSampleRate());
    snprintf(buffer_size,    32, "Buffer Size Selection: %d\n",     getAudioBufferSize());
    snprintf(interpolation,  32, "Interpolation Selection: %d\n\n", getCurrentInterpolationType());

    if (fwrite(header,         sizeof *header,         strlen(header),         conf_file) < strlen(header))         success = false;
    if (fwrite(samp_rate,      sizeof *samp_rate,      strlen(samp_rate),      conf_file) < strlen(samp_rate))      success = false;
    if (fwrite(buffer_size,    sizeof *buffer_size,    strlen(buffer_size),    conf_file) < strlen(buffer_size))    success = false;
    if (fwrite(interpolation,  sizeof *interpolation,  strlen(interpolation),  conf_file) < strlen(interpolation))  success = false;

    return success;
}

static bool write_default_dir(FILE *conf_file)
{
    bool success = true;

    char *header = "# Miscellaneous settings\n", *dir_header = "Default Dir: ";
    char *default_dir = NULL, *dir = NULL;
    size_t dir_len = 0;
    
    assert(conf_file != NULL);
    
    dir = strdup(getWorkingDir());

    dir_len = strlen(dir_header) + strlen(dir) + 3;
    SBC_CALLOC(dir_len, sizeof *default_dir, default_dir);

    snprintf(default_dir, dir_len, "%s%s\n\n", dir_header, dir);

    if (fwrite(header,  sizeof *header,  strlen(header),  conf_file) < strlen(header))  success = false;
    if (fwrite(default_dir,  sizeof *default_dir,  strlen(default_dir),  conf_file) < strlen(default_dir))  success = false;

    SBC_FREE(dir);
    SBC_FREE(default_dir);

    return success;
}

void saveConfig(void)
{
    bool success = true;
    FILE *conf_file = NULL;
    char *conf_path = NULL;

    if((conf_path = get_conf_path(false)) == NULL) return;
    printf("Conf path: %s\n", conf_path);
    
    if((conf_file = fopen(conf_path, "w")) == NULL)
    {
        showErrorMsgBox("File Save Error!", "Unable to open config file for saving!", strerror(errno));
        SBC_FREE(conf_path);
        return;
    }
    
    SBC_FREE(conf_path);
    
    if(!write_header(conf_file)) success = false;
    if(!write_devices(conf_file)) success = false;
    if(!write_dev_settings(conf_file)) success = false;
    if(!write_default_dir(conf_file)) success = false;

    fclose(conf_file);

    if(!success) showErrorMsgBox("File Save Error!", "Error while saving config file!", strerror(errno));
}
