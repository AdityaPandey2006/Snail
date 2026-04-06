// #include "dumpList.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <dirent.h>
// #include <linux/limits.h>

// executorResult dumpList(Command* newCommand) {
//     executorResult result;
//     result.shouldExit = 0;
//     result.statusCode = 0;

//     char* home = getenv("HOME"); // gives current user's home directory path 
//     if (!home) {
//         fprintf(stderr, "dumplist: Could not get HOME environment variable\n");
//         result.statusCode = 1; // command execution failed 
//         return result;
//     }

//     char dumpPath[PATH_MAX];  // PATH_MAX is max. char length allowed as path (in Linux)
//     char dumpInfoPath[PATH_MAX];

//     sprintf(dumpPath, "%s/.snailDump/", home);  //   dumpPath = home/.snailDump/
//     sprintf(dumpInfoPath, "%s/info/", dumpPath);  // dumpInfoPath = dumpPath/info/

  
//     if (access(dumpPath, F_OK) != 0 || access(dumpInfoPath, F_OK) != 0)  // to check if file/folder exists
//     // if either of dumpPath  or dumpInfoPath is missing 
//     {
//         printf("The snailDump is currently empty or not created yet.\n");
//         return result;
//     }

//     DIR* dumpInfoDir = opendir(dumpInfoPath);
//     if (!dumpInfoDir) {
//         perror("dumplist: Failed to open dump info directory");
//         result.statusCode = 1; // command execution failed
//         return result;
//     }

//     struct dirent* entry;
//     int count = 0;

//     printf("\n%-50s | %s\n", "ORIGINAL FILE PATH", "DELETION DATE");
//     printf("---------------------------------------------------+----------------------\n");

//     while ((entry = readdir(dumpInfoDir)) != NULL) {
//         // to Skip current and parent directory pointers
//         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
//             continue;
//         }

//         char infoFilePath[PATH_MAX];
//         sprintf(infoFilePath, "%s%s", dumpInfoPath, entry->d_name);

//         FILE* fp = fopen(infoFilePath, "r");
//         if (fp) {
//             char buffer[1024];
//             char origPath[PATH_MAX] = "";
//             char delDate[25] = "";
            
//             // State trackers: 0 = searching, 1 = header found (grab next line), 2 = data saved
//             int pFound = 0; 
//             int dFound = 0; 

//             while (fgets(buffer, sizeof(buffer), fp)) {
               
//                 buffer[strcspn(buffer, "\n")] = 0; // create a 0(null terminator) at the end of line

//                 if (pFound == 1) {
//                     strncpy(origPath, buffer, sizeof(origPath) - 1);
//                     pFound = 2; 
//                 } else if (dFound == 1) {
//                     strncpy(delDate, buffer, sizeof(delDate) - 1);
//                     dFound = 2;
//                 }

//                 // Check if the current line is a header
//                 if (strcmp(buffer, "Path:") == 0) pFound = 1;
//                 if (strcmp(buffer, "DeletionDate:") == 0) dFound = 1;
//             }
//             fclose(fp);

//             // If we successfully extracted both pieces of data, print the row
//             if (origPath[0] != '\0' && delDate[0] != '\0') {
//                 // %-50.50s forces the string to take up exactly 50 characters, to keep the table aligned
//                 printf("%-50.50s | %s\n", origPath, delDate);
//                 count++;
//             }
//         }
//     }
//     closedir(dumpInfoDir);

//     if (count == 0) {
//         printf("The dump is completely empty.\n");
//     } else {
//         printf("---------------------------------------------------+----------------------\n");
//         printf("Total items in dump: %d\n\n", count);
//     }

//     return result;
// }
