#ifndef ICONTITLE_H
#define ICONTITLE_H

#define BOX_PX				40
#define BOX_PY				37

void iconTitleInit();
void loadConsoleIcons();
void getGameInfo(int num, bool isDir, const char* name);
void iconUpdate(int num, bool isDir, const char* name);
void titleUpdate(int num, bool isDir, const char* name, const bool highlighted);
void drawIconFolder(int Xpos, int Ypos, s32 scale);
void drawIcon(int num, int Xpos, int Ypos, s32 scale);

#endif // ICONTITLE_H