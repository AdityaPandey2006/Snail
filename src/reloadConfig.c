#include "reloadConfig.h"
#include "config.h"
#include <stdio.h>

executorResult reloadConfigCommand(Command *newCommand){
    executorResult result = {0, 0};

    if(newCommand->argCount > 1){
        fprintf(stderr, "reloadConfig: too many arguments\n");
        result.statusCode = 1;
        return result;
    }

    if(!loadSnailConfig()){
        fprintf(stderr, "reloadConfig: could not reload config\n");
        result.statusCode = 1;
        return result;
    }

    const char *configPath = getSnailConfigPath();
    if(configPath != NULL){
        printf("Reloaded config from %s\n", configPath);
    }
    else{
        printf("Reloaded config\n");
    }

    return result;
}
