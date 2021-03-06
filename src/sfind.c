#include "sfind.h"

/**
* Creates a subpath
*/
void createSubpath(const char *originalPath, struct dirent *directory, char *finalPath){
  char newPath[1024];
  //  Create new path
  strcpy(newPath, originalPath);
  strcat(newPath, "/");
  strcat(newPath, directory->d_name);
  //  Modify final path
  strcpy(finalPath, newPath);
}

/**
*  Founded file action handler
*/
void handleFoundFile(struct dirent dirent, const char path[], const char command[], const char execute[]){
  char newPath[1024];

  //  Create new path
  createSubpath(path, &dirent, newPath);

  if (strcmp(command, CMD_PRINT) == 0){
    printf("%s\n", newPath);
  }
  else if (strcmp(command, CMD_DELETE) == 0){
    remove(newPath);
  }
  else if (strcmp(command, CMD_EXECUTE) == 0) {
    char newExecCommand[1024];
    strcpy(newExecCommand, execute);

    //  Replace '{}' ocurrencies with file path
    str_replace(newExecCommand, "{}", newPath);

    //  Execute system command
    if (system(newExecCommand) != 0) {
      exit(1);
    }
  }
}

/**
* Handle fork action
*/
void handleFork(const char *searchedText,const char pathname[], const char command[], const char searchParameter[], const char execute[]){
  //  FORK process
  pid_t pid = fork();

  if (pid < 0 ) {
    perror("error forking");
    abort();
    
  } else if (pid > 0) {
    /* parent */
    int status;
    waitpid(pid, &status, 0);

  } else  if (pid == 0){
    /* child */
    //  remove SIGINT signal from child process
    signal(SIGINT, SIG_DFL);
    //  call find recursively
    sfind(searchedText, pathname, command, searchParameter, execute);
    exit(0);
  }
}

/**
* Returns 1 if it is the same type
*/
int isStatType(const struct stat stat, const char*searchedText){
  //  Regular files
  if (*searchedText == DT_REG && S_ISREG(stat.st_mode)) {
    return 1;
  }
  else if (*searchedText == DT_DIR && S_ISDIR(stat.st_mode)) {
    return 1;
  }
  else if (*searchedText == DT_LNK && S_ISLNK(stat.st_mode)) {
    return 1;
  }
  return 0;
}

/**
* Returns 1 if it is the same permission
*/
int isStatPerm(const struct stat stat, const char *searchedText){
  //  create another string to avoid memory and pointer problems
  char intermedium[1024];
  //  copy seached text to intermedium string
  strcpy(intermedium, searchedText);

  int a = (stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
  int b = strtol((char*)intermedium, (char**)&intermedium, 8);

  if (a == b) {
    return 1;
  } else {
    return 0;
  }
}

void fileLogic(const char *searchedText, const char pathname[], const char command[], const char searchParameter[], struct dirent *dirent, struct stat statsbuf, const char execute[]){
  //  If dirent name is equal searched name text, handle it.
  if (strcmp(PARAM_NAME, searchParameter)== 0 && strcmp(dirent->d_name, (char*)searchedText) == 0) {
    handleFoundFile(*dirent, pathname, command, execute);
  }
  //  If dirent type is equal file searched type
  else if (strcmp(PARAM_TYPE, searchParameter) == 0 && isStatType(statsbuf, searchedText)) {
    handleFoundFile(*dirent, pathname, command, execute);
  }
  //  If dirent perm is equal file searched perm
  else if (strcmp(PARAM_PERM, searchParameter) == 0 && isStatPerm(statsbuf, searchedText)){
    handleFoundFile(*dirent, pathname, command, execute);
  }
  //  Check if dirent is a directory, and is diferent from "." and ".."
  if (S_ISDIR(statsbuf.st_mode) && strcmp(dirent->d_name, "..") != 0 && strcmp(dirent->d_name, ".") != 0){
    //  Found a directory, search in directory
    //  create file path
    char filePath[1024];

    createSubpath(pathname, dirent, filePath);

    //printf("%s\n", filePath);

    handleFork(searchedText, filePath, command, searchParameter, execute);
  }
}

int sfind(const char *searchedText, const char pathname[], const char command[], const char searchParameter[], const char execute[]){
  struct dirent *dirent = NULL;
  struct stat statbuf;
  DIR *directory;

  //  Open directory
  if((directory = opendir(pathname)) == NULL){
    perror("opendir() error");
    exit(1);
  }

  //  change directory to actual path
  chdir(pathname);

  //  Read directory files
  while ((dirent = readdir(directory)) != NULL) {
    //  printf("Analyzing: %s/%s\n", pathname, dirent->d_name);

    //  Read file status
    if (lstat(dirent->d_name, &statbuf) == -1) {
      //  File couldn't be read
      perror("stat() error");
    } else {
      //  File could be read
      fileLogic(searchedText, pathname, command, searchParameter, dirent, statbuf, execute);
    }
  }

  closedir(directory);
  exit(0);
}
