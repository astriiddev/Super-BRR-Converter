#ifndef __SBC_FILE_DIALOG_H
#define __SBC_FILE_DIALOG_H

char *getLastDir(void);
char *getHomeDir(void);
void setLastDir(const char* path);

void initFileDialog(int *argc, char ***argv);
void handleFileDialogEvents(void);

const char* getFileNameWithoutExt(const char *file);
const char* openDialog(const char *dir_path);
const char* saveDialog(const char *dir_path, const char* sample_name);

void soundErrorBell(void);

#endif /* __SBC_FILE_DIALOG_H */
