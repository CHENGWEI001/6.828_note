#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

// Simplifed xv6 shell.

#define MAXARGS 10
#define MAX_EXEC_DIR_CANDIDATES 100

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // arguments to the command to be exec-ed
};

struct redircmd {
  int type;          // < or > 
  struct cmd *cmd;   // the command to be run (e.g., an execcmd)
  char *file;        // the input/output file
  int flags;         // flags for open() indicating read or write
  int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

struct sepcmd {
  int type;          // ;
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

int fork1(void);  // Fork but exits on failure.
struct cmd *parsecmd(char*);
void setup_redirection(struct redircmd *rcmd);
void dup_wrapper(int old_fd, int new_fd);
int my_exec(struct execcmd *cmd);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;
  struct sepcmd *scmd;
  char bin_dir[] = "/bin/";
  char *final_path;
  char me = 'p';
  int backUpStdOut = 10;

  if(cmd == 0)
    _exit(0);
  
  switch(cmd->type){
  default:
    fprintf(stderr, "unknown runcmd\n");
    _exit(-1);

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      _exit(0);
    // fprintf(stderr, "exec not implemented\n");
    // Your code here ...
    if (my_exec(ecmd) < 0) {
      fprintf(stderr, "exec %s failed\n", ecmd->argv[0]);
    }
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;
    // fprintf(stderr, "redir not implemented\n");
    // Your code here ...
    setup_redirection(rcmd);
    runcmd(rcmd->cmd);
    break;

  case ';':
    scmd = (struct sepcmd*)cmd;
    if (fork() == 0) {
      runcmd(scmd->left);
      break;
    }
    wait(&r);
    if (fork() == 0) {
      runcmd(scmd->right);
      break;
    }
    wait(&r);   
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;
    // fprintf(stderr, "pipe not implemented\n");
    // Your code here ...
    pipe(p);
    r = 0;
    // for left cmd, need to dup STDOUT with p[1]
    if (fork() == 0) {
      close(p[0]);
      // dup_wrapper(STDOUT_FILENO, backUpStdOut);      
      dup_wrapper(p[1], STDOUT_FILENO);
      close(p[1]);
      runcmd(pcmd->left);
      // dup_wrapper(backUpStdOut, STDOUT_FILENO);      
      // close(backUpStdOut);
      me = 'l';
      break;
    } else {
      r++;
    }
    // for right cmd, need to dup STDIN with p[0]
    if (fork() == 0) {
      close(p[1]);
      dup_wrapper(p[0], STDIN_FILENO);
      close(p[0]);
      runcmd(pcmd->right);
      me = 'r';
      break;
    } else {
      r++;
    }
    // remember to close the fd for parent process as well
    close(p[0]);
    close(p[1]);
    while (r) {
      // fprintf(stdout, "[%d]%c waiting r:%d\n",getpid(), me, r);
      wait(NULL);
      r--;
    }
    
    break;
  }    
  _exit(0);
}

int my_exec_from_dir(struct execcmd *cmd, char *dir) {
  int ret;
  char *final_path;
  if ((final_path = malloc(strlen(cmd->argv[0]) + strlen(dir) + 2)) == NULL) {
    fprintf(stderr, "fail to allocate memory for %s %s\n", cmd->argv[0], dir);
    _exit(-1);
  }  
  final_path = strcat(final_path, dir);
  final_path = strcat(final_path, "/");
  final_path = strcat(final_path, cmd->argv[0]);
  ret = execv(final_path, cmd->argv);
  free(final_path);
  return ret;
}
int my_exec(struct execcmd *cmd) {
  int ret;
  char *env_path = getenv("PATH");
  char *delim = ":";
  char *token = strtok(env_path, delim);
  // first try curr working dir
  if ((ret = my_exec_from_dir(cmd, "")) >= 0) {
    return ret;
  }  
  // if not try PATH
  while (token != NULL) {
    if ((ret = my_exec_from_dir(cmd, token)) >= 0) {
      return ret;
    }      
    token = strtok(NULL, delim);
  }  
  return -1;
}

int my_exec_old(struct execcmd *cmd) {
  char *dirs[MAX_EXEC_DIR_CANDIDATES];
  int num_dirs = 0;
  dirs[num_dirs++] = "";
  char *env_path = getenv("PATH");
  char *delim = ":";
  char *token = strtok(env_path, delim);
  char *exe = cmd->argv[0];
  char *final_path;
  int i;
  while (token != NULL && num_dirs < MAX_EXEC_DIR_CANDIDATES) {
    dirs[num_dirs++] = token;
    token = strtok(NULL, delim);
  }  
  for (i = 0; i < MAX_EXEC_DIR_CANDIDATES; i++) {
    if (my_exec_from_dir(cmd, dirs[i]) >= 0) {
      break;
    }    
  }
  return -1;
}

void dup_wrapper(int old_fd, int new_fd) {
  if (dup2(old_fd, new_fd) < 0) {
    fprintf(stderr, "failed to dup file descriptors: %s\n", \
                    strerror(errno));
    exit(EXIT_FAILURE);        
  }
}

void 
setup_redirection(struct redircmd *cmd) {
  int tmp_fd = 0;
  if (cmd->type == '>') {
    tmp_fd = open(cmd->file, cmd->flags, S_IRWXU);
  } else {
    tmp_fd = open(cmd->file, cmd->flags);
  }
  if (tmp_fd < 0) {
    fprintf(stderr, "failed to open file for redirection: %s\n",\
                    strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (dup2(tmp_fd, cmd->fd) < 0) {
    fprintf(stderr, "failed to dup file descriptors: %s\n", \
                    strerror(errno));
    exit(EXIT_FAILURE);    
  }
  close(tmp_fd);
}

// char *
// search_path(char *exe, char *path_buf, unsigned path_buf_size)
// {
//   DIR *d;
//   struct dirent *dir
//   char *env_paths;
//   char *delim = ":";
//   char *path_dir;
//   unsigned int final_path_size;
//   char *env_paths_candidate[] = {
//     "PWD",
//     "PATH"
//   };
//   int i;
//   unsigned int exe_len = (strlen(exe);
  
//   for (i = 0; i < sizeof(env_paths_candidate)/sizeof(char*); i++) {
//     env_paths = getenv(env_paths_candidate[i]);
//     path_dir = strtok(env_paths, delim);
//     // loop through all possible path in PATH env
//     while (path_dir != NULL) {
//       d = opendir(path_dir);
//       if (d == NULL) {
//         fprintf(stderr, "cannot open dir: %s", strerror(errno));
        
//       } 
//       // len of exe file name, dir path, one additional '/', and '\0' 
//       else if (exe_len + strlen(path_dir) + 2 <= path_buf_size) {
//         // loop DIR stream and check each file see any match
//         while ((dir = readdir(d)) != NULL) {
//           // if found 
//           if (strcmp(dir->d_name, exe) == 0) {
//             path_buf = strcat(path_buf, path_dir);
//             path_buf = strcat(path_buf, "/");
//             path_buf = strcat(path_buf, exe);
//             return 0;
//           }
//         }   
//       }
//       path_dir = strtok(NULL, delim);
//     }
//   }
//   return -1;
// }

int
getcmd(char *buf, int nbuf)
{
  if (isatty(fileno(stdin)))
    fprintf(stdout, "6.828$ ");
  memset(buf, 0, nbuf);
  if(fgets(buf, nbuf, stdin) == 0)
    return -1; // EOF
  return 0;
}

int
main(void)
{
  static char buf[100];
  int fd, r;

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf+3);
      continue;
    }
    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait(&r);
  }
  exit(0);
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->flags = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd*
sepcmd(struct cmd *left, struct cmd *right)
{
  struct sepcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ';';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *parsesep(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  struct sepcmd *scmd = (struct sepcmd *)cmd;
  struct sepcmd *slcmd = (struct sepcmd *)(scmd->left);
  struct sepcmd *srcmd = (struct sepcmd *)(scmd->right);
  // fprintf(stdout, "parsecmd: %c\n", scmd->type);
  // fprintf(stdout, "parsecmd: %c\n", scmd->left->type);
  // fprintf(stdout, "parsecmd: %c\n", scmd->right->type);
  return cmd;
}

void printchars(char *s, char *e) {
  fprintf(stdout, "[s:%p, e:%p:", s, e);
  while (s < e) {
    fprintf(stdout, "%c", *s);
    s++;
  }
  fprintf(stdout, "]\n");
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  // printchars(*ps, es);
  cmd = parsesep(ps, es);
  return cmd;
}

struct cmd*
parsesep(char **ps, char *es)
{
  struct cmd *cmd;
  char *p = *ps;
  char *leftStart = *ps;
  char *rightStart;
  int foundSep = 0;
  // printchars(*ps, es);
  int lvl = 0; // purpose of lvl is to ignore colon inside subshell
  while (p < es) {
    if (lvl == 0 && *p == ';') {
      foundSep = 1;
      rightStart = p + 1;
      break;
    }
    if (*p == '(') {
      lvl++;
    } else if (*p == ')') {
      lvl--;
    }
    p++;
  }
  if (foundSep) {
    cmd = sepcmd(parsepipe(&leftStart, p), parsesep(&rightStart, es));
    *ps = es;
  } else {
    cmd = parsepipe(ps, es);
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  char *subLeft, *subRight;
  int parenCnt = 0;
  
  // if we have subshell, parse that cmd and return
  if (peek(ps, es, "(")) {
    subLeft = *ps;
    subRight = *ps;
    while (subRight < es) {
      // fprintf(stdout, "[debug subdebugRight:%p, es:%p] %c\n", subRight, es, *subRight);
      if (*subRight == '(') {
        parenCnt++;
      } else if (*subRight == ')') {
        parenCnt--;
      }
      if (parenCnt == 0) {
        break;
      }
      subRight++;
    }
    subLeft++;
    ret = parseline(&subLeft, subRight);
    subRight++;
    ret = parseredirs(ret, &subRight, es);
    *ps = subRight;
    return ret;
  }
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while((*ps < es) && !peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
