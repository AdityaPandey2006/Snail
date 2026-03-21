#ifndef FILEDUMP_H
#define FILEDUMP_H


int makeDumpFolder();
int cleanDump();
int fileRestore();
int sendToDump(char* entryPath);// both files and folders can be sent to the dump. Calling files and folders together as entries
#endif




/*
The fileDump folder has the following structure:

~/.snailDump/
    files/
        Group16_Lab3-4.pdf
        hi.2.c
        hi.3.c
        yokoso.mp3
    info/
        Group16_Lab3-4.pdf.snailInfo
        hi.2.c.snailInfo
        hi.3.c.snailInfo
        yokoso.mp3.snailInfo
*/
/*
Each .snailInfo file looks like the folowing:
[Trash Info]
Path:
/home/ancientmachine/Downloads/test_case_template.xlsx
DeletionDate:
2026-03-18T19:01:26
*/