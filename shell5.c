#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

typedef struct list {
    char* word;
    struct list* next;
    int key;
} list; // список слов, разделенных пробелами

typedef struct commands {
	int priority; //3-conv; 2- AND; 2- Or; 0 - Parallels
    int type; //3-conv; 2 -AND;1-Or;0-Parallels
    list* word;
    struct commands* next;
} commands;

enum state {finish,getword,execute,error};
static int quote=0;
static int redir=0; //перенаправление ввода/вывода
static int ConvFlag=0; // флаг конвейера
static int BackgroundMode=0; // флаг фонового режима
static enum state flag;
static int FlagAnd=0;
static int FlagOr=0;
static int FlagParallels=0;
pid_t* BackPidsOpen = NULL;
int BackLenOpen=0;
pid_t* BackPidsClosed = NULL;
int BackLenClosed=0;

list* Add_word_to_List(list* mas);
commands* AddList_to_commands(commands* arr,list* lis,int prior,int type);
char* get_Word();
void PrintWords(list* mas);
void Clean(list* mas);
list* Insert(list* mas,char* p);
list* CreateNode(char*p);
char** CreateMas(list* lis,int* length);
void PrintMasWords(char** mas,int length);
void CleanMas(char** mas,int length);
void CleanCommands(commands* arr);
int CheckCMD(char** mas);
int MakeCMD(list* mas);
void MakeCommand(commands* arr,int Length);
void PrintCommands(commands* arr);
list* CopyList(list* mas);
int ChangeDirection();
void Conv(commands* arr,int PipL);
void Error(int pids[], int n);
void ChangeMode();
void sig_chld(int sig);
void PrintClosedProcesses();
void killOpenProcess();

char* get_Word()
{
    int len=0;
    char* p=(char*) malloc(sizeof(char)*(len+1));
    int c;
    while (((c=getchar())!=EOF) && (c!='\n')&& (c!=' ') && (c!='\t')&&(c!='>')&&(c!='<')&&(c!='|')&& (c!='&')&&(c!=';'))
    {
        if (c=='"') 
        {
            quote=!quote;
        }
        else 
        {
            p= (char*) realloc(p,sizeof(char)*(len+1));
            p[len]=c;
            len++;
        }
    }
    switch (c)
    {
    case EOF:
        flag = finish;
        break;
    case '>':
        if ((c=getchar())=='>')
        {
            redir = 3;
        }
        else
        {
            ungetc(c,stdin);
            redir=1;
        }
        break;
    case '<':
        redir=2;
        break;
    case '|':
		if ((c=getchar())=='|')
		{
			FlagOr=1;
		}
		else
		{
		    ConvFlag=1;
			ungetc(c,stdin);
		}
    break;
    case '&':
        if ((c=getchar())=='\n')
        {
            BackgroundMode=1;
        }
        else 
        {
			if (c=='&')
			{
				FlagAnd=1;
			}
			else
			{
				ungetc(c,stdin);
				printf("Erorr\n");
			    free(p);
			    return NULL;
			}
		}
    break;  
    case ';':
		FlagParallels=1;
		break;
    case '\n':
        flag= execute;
        break;
    default:
				flag = getword;
        break;
    }
    if (len!=0)
    {
        p=(char*) realloc(p,sizeof(char)*(len+1));
        p[len]=0;
        return p;
    }
    else
    {
        free(p);
        return NULL;
    }
}

list* Add_word_to_List(list* mas) // вариант первый
{
    char* p= get_Word();
    list* prev=NULL;
    list* cur=NULL;
    if (p!=NULL)
    {
        if (mas == NULL)
        {
            mas = (list*) malloc(sizeof(list));
            mas->word=p;
            if ((strcmp(p,"&&\0")==0) || (strcmp(p,"||\0")==0))
            {
				mas->key=1;
			}
			else
			{
				mas->key=0;
			}
            mas->next=NULL;
        }
        else
        {
            prev= mas;
            while (prev->next!=NULL)
            {
                prev=prev->next;
            }
            cur=(list*) malloc(sizeof(list));
            cur->word=p;
            if ((strcmp(p,"&&\0")==0) || (strcmp(p,"||\0")==0))
            {
				cur->key=1;
			}
			else
			{
				cur->key=0;
			}
            cur->next=NULL;
            prev->next=cur;
        }
    }
    return mas;
}

commands* AddList_to_commands(commands* arr,list* mas,int prior,int type)
{
    commands* prev;
    commands* cur;
    if (mas!=NULL)
    {
		if (arr==NULL)
		{
			arr=(commands*) malloc(sizeof(commands)+1);
			arr->word=CopyList(mas);
			arr->priority=prior;
            arr->type=type;
			arr->next=NULL;
		}
		else
		{
			prev=arr;
			while (prev->next!=NULL)
			{
				prev=prev->next;
			}
			cur=(commands*) malloc(sizeof(commands)+1);
			cur->word=CopyList(mas);
			cur->priority=prior;
            cur->type=type;
			cur->next=NULL;
			prev->next=cur;
		}
	}
    return arr;
}

list* CopyList(list* mas)
{
    list *q, *tmp;
	list *nlist = NULL;
	while (mas) {
		if (!(nlist))
		{
			nlist = (list*)malloc(sizeof(list));
			nlist -> word = (char*)malloc(sizeof(char)*(strlen(mas -> word) + 1));
			strcpy(nlist -> word, mas-> word);
            nlist -> key = mas->key;
			nlist -> next = NULL;
		}
		else
		{
			tmp = nlist;
			q = nlist;
			q = q -> next;
			while (q)
			{
				q = q -> next;
				tmp = tmp -> next;
			}
			q = (list*)malloc(sizeof(list));
			q -> word = (char*)malloc(sizeof(char) * (strlen(mas -> word) + 1));
			strcpy(q -> word, mas -> word);
			q -> next = NULL;
            q -> key = mas->key;
			tmp->next = q;
		}
		mas = mas -> next;
	}
	return nlist;
}

list* Insert(list* mas,char* p) 
{
    if (mas!=NULL)
    {
        mas->next=Insert(mas->next,p);
    }
    else 
    {
       return CreateNode(p);
    }
    return mas;
}

list* CreateNode(char* p) 
{
    list* mas = (list*) malloc (sizeof(list));
    mas->word=p;
    mas->next=NULL;
    return mas;
}

void CleanCommands(commands* arr)
{
    if (arr!=NULL)
    {
        CleanCommands(arr->next);
        Clean(arr->word);
        free(arr);
    }
}

void Clean(list* mas)
{
    if (mas!=NULL)
    {
        Clean(mas->next);
        free(mas->word);
        free(mas);
    }
}

void PrintCommands(commands* arr)
{
    while (arr!=NULL)
    {
		printf("приоритет = %d ",arr->priority);
        PrintWords(arr->word);
        printf("\n");
        arr=arr->next;
    }
}

void PrintWords(list* mas)
{
    while (mas!=NULL)
    {
        printf("Слово: %s    тип = %d\n",mas->word,mas->key);
        mas=mas->next;
    }
}

char** CreateMas(list* lis,int* length)
{
	if (lis!=NULL)
	{
		int len=0;
		char** mas=(char**) malloc(sizeof(char*)*(len+1));
		while (lis!=NULL)
		{
			mas[len] = (char*) malloc(sizeof(char)*(strlen(lis->word)+1));
			strcpy(mas[len],lis->word);
			lis=lis->next;
			len++;
			mas=(char**) realloc(mas,sizeof(char*)*(len+1));
    }
    mas[len]=NULL;
    *length=len;
    return mas;
	}
	else
	{
		return NULL;
	}
}

void PrintMasWords(char** mas,int length)
{
    int i=0;
    for (i=0;i<length;i++)
    {
        printf("%s\n",mas[i]);
    }
}

void CleanMas(char** mas,int length)
{
    int i=0;
    for (i=0;i<length;i++)
    {
        free(mas[i]);
    }
    free(mas);
}

int CheckCMD(char** mas)
{
    char* cd="cd\0";
    char* ex="exit\0";
    if (mas==NULL)
    {
        fprintf(stderr,"No commands\n");
        return 3;
    }
    else
    {
        if (strcmp(cd,mas[0])==0)
        {
            if (mas[1]!=NULL)
            {
                chdir(mas[1]);
            }
            else
            {
                chdir(getenv("HOME"));
            }
            return 1;
        }
        else
        {
            if (strcmp(ex,mas[0])==0)
            { 
                //printf("Программа завершила работу\n");
                return 2;
            }
            else
            {
                return 0;
            }
        }
    }
}

int MakeCMD(list* lis)
{
    pid_t pid;
    int status;
    int ckCMD;
    int length;
    if (lis==NULL)
    {
		fprintf(stderr,"Текст пустой\n");
		return 1;
	}
    else
    {
        char** mas=CreateMas(lis,&length);
        if ((ckCMD=CheckCMD(mas))==0)
        {
            if ((pid=fork())==0)
            {
                if (BackgroundMode==1)
                {
                    ChangeMode();
                }
                execvp(mas[0],mas);
                fprintf(stderr,"Incorrect input\n");
                exit(1);
            }
            else
            {
                if (pid<0)
                {
                    fprintf(stderr,"Process error\n");
                    exit(1); 
                }
                else
                {
                    if (BackgroundMode==1)
                    {
                        printf("Procces %d in the Background mode\n",pid);
                        BackPidsOpen=(pid_t*) realloc(BackPidsOpen,sizeof(pid_t)*(BackLenOpen+1));
                        BackPidsOpen[BackLenOpen]=pid;
                        BackLenOpen++;
                        BackgroundMode=0;
                        return 0;
                    }
                    else
                    {
                        wait(&status);
                        if (WIFEXITED(status)==0)
						{
							printf("Процесс завершился плохо с точки зрения ОС\n");
							return 1;
						}
						else
						{
							if (WEXITSTATUS(status)!=0)
							{
								printf("С точки зрения программиста что-то не так\n");
								perror("It's better to stop");
							}
							return 0;
						}
                    }
                }
            }		
        }
        else
        {
            if (ckCMD==2) //ввели exit
            {
                CleanMas(mas,length);
                Clean(lis);
                printf("Программа завершила работу\n");
                exit(1);
            }
        }
        CleanMas(mas,length);
    }
}

void MakeCommand(commands* arr,int Length)
{
	int i=0;
	commands* result=arr;
	int check;
    int skip=0;
	for (i=0;i<Length;i++)
	{
		int max=-1;
		commands* maxc=NULL;
		commands* p=result;
		while (p!=NULL)
		{
			if (max<(p->priority))
			{
				maxc=p;
				max=p->priority;
			}
			p=p->next;
		}
        maxc->priority=-1;
        if (skip==1) continue;
		check=MakeCMD(maxc->word);
		switch (check)
        {
		case 1:
			if ((maxc->type)==2) skip=1;
		break;
        case 0:
            if ((maxc->type)==1) skip=1;
        break;
        }
	}
}

void sig_chld(int sig)
{
    pid_t pid;
    int status;
    int i=0;
    pid=waitpid(-1,&status,WNOHANG);
    for (i=0;i<BackLenOpen;i++)
    {
        if (pid == BackPidsOpen[i])
        {
            BackPidsOpen[i]=-1;
            BackPidsClosed=(pid_t*) realloc(BackPidsClosed,sizeof(pid_t)*(BackLenClosed+1));
            BackPidsClosed[BackLenClosed]=pid;
            BackLenClosed++;
        }
    }
}

void killOpenProcess()
{
    int i=0;
    for (i=0;i<BackLenOpen;i++)
    {
        if (BackPidsOpen[i]>0)
        {
            kill(BackPidsOpen[i],SIGKILL);
            printf("Process %d was killed\n",BackPidsOpen[i]);
        }
    }
}

void PrintClosedProcesses()
{
    int i=0; 
    for (i=0;i<BackLenClosed;i++)
    {
        printf("Фоновый процесс с pid %d завершился.\n",BackPidsClosed[i]);
    }
}

void ChangeMode()
{
    int fd1=open("/dev/null",O_WRONLY);
    int fd0=open("/dev/null",O_RDONLY);
    dup2(fd1,1);
    dup2(fd0,0);
    close(fd0);
    close(fd1);
}

int ChangeDirection()
{
    int fd;
    int c;
    char* word;
    while ((c = getchar()) == ' ');
	ungetc(c, stdin);
    word= get_Word();
    if (word==NULL)
       {
        fprintf(stderr,"Error\n");
        redir =0;
        return 1;
    }
    switch (redir)
    {
    case 1:
    {
        fd= open(word,O_CREAT|O_WRONLY | O_TRUNC, 0664);
        if (fd < 0) 
		{
			fprintf(stderr, "File can't be open\n");
			close(fd);
			//exit(1);
		}
        dup2(fd,1);
        close(fd);
    }
    break;
    case 2:
    {
        fd= open(word, O_RDONLY);
        if (fd < 0) 
		{
			fprintf(stderr, "File can't be open\n");
			close(fd);
			//exit(1);
		}
        dup2(fd,0);
        close(fd);
    }
    break;
    case 3:
    {
        fd =open(word,O_CREAT | O_WRONLY | O_APPEND,0644);
        if (fd < 0) 
		{
			fprintf(stderr, "File can't be open\n");
			close(fd);
			//exit(1);
		}
        dup2(fd,1);
        close(fd);
    }
    break;
    default: break;
    }
    redir=0;
    free(word);
    return 0;
}
void Conv(commands* arr,int PipL)
{
    int pids[PipL]; 
	int i = 0;
    int fd1[2]; 
    int fd2[2];
    pid_t pid;
	char** coms;
    int len=0;
	fd1[0] = dup(0);
	fd1[1] = dup(1);
	while (i < PipL) 
	{
		if (i + 1 < PipL) 
		{
        	if (pipe(fd2) == -1) 
            {
                Error(pids, i);
            }
        } 
		else 
        {
		    fd2[0] = dup(0); 
		    fd2[1] = dup(1);
		}
		coms = CreateMas(arr->word,&len);
		if ((pid = fork()) < 0) 
		{
            Error(pids, i);
        } 
		else 
        {
            if (pid == 0) 
            {
        		dup2(fd1[0], 0);
        		dup2(fd2[1], 1);
           		close(fd1[0]);
           		close(fd2[0]);
           		close(fd1[1]);
           		close(fd2[1]);
           		execvp((arr->word)->word,coms);
                exit(1);
            }
        }
		pids[i] = pid;
       	if (close(fd1[0]) || close(fd1[1])) {
       		Error(pids, i + 1);
       	}
       	fd1[0] = fd2[0]; 
       	fd1[1] = fd2[1];
		i++;
        arr=arr->next;
		CleanMas(coms,len);
        len=0;
		coms = NULL;
	}
	if (close(fd1[0]) || close(fd1[1]))
    {
        Error(pids, PipL);
   	}
	while (wait(NULL) != -1) { };
	if (errno != ECHILD) 
      {
		Error(pids, PipL);
	}
	CleanMas(coms,len);
}

void Error(int pids[], int n)
{
	int i;
	for (i = 1; i < n; i++) 
    {
		kill(pids[i], SIGKILL);
	}
	while (wait(NULL) != -1) { } 
}

int main()
{
    int fd1=dup(1);
    int fd0=dup(0);
    flag=getword;
    int prior=0;
    int PipL=0;
    list* lis= NULL;
    commands* arr=NULL;
    int ComLen=0;
    int ckCMD=0;
    BackPidsOpen=(pid_t*)malloc(sizeof(pid_t)+1);
    BackPidsClosed=(pid_t*)malloc(sizeof(pid_t)+1);
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD,sig_chld);
    printf("Enter the command\n");
    do
    {
        lis=Add_word_to_List(lis);
        if (redir!=0) // перенаправление ввода/вывода
        {
            if (ChangeDirection())
            {
				printf("Enter the command\n");
				Clean(lis);
				lis=NULL;
                PipL=0;
                dup2(fd1,1);// возвращает поток вывода на привычное место
                dup2(fd0,0);//возвращает поток ввода на привычное место
				continue;
			}
        }
        if (ConvFlag!=0)
        {
            if (lis!=NULL)
            {
                arr=AddList_to_commands(arr,lis,3,3);
                ComLen++;
                PipL++;
                Clean(lis);
                lis=NULL;
                ConvFlag=0;
            }
        }
        if (FlagAnd!=0)
        {
            if (lis!=NULL)
            {
                prior=2;
			    arr=AddList_to_commands(arr,lis,2,2);
			    ComLen++;
			    Clean(lis);
			    lis=NULL;
			    FlagAnd=0;
            }
		}
		if (FlagOr!=0)
		{
            if (lis!=NULL)
            {
                arr=AddList_to_commands(arr,lis,2,1);
			    ComLen++;
		    	Clean(lis);
		    	lis=NULL;
		    	FlagOr=0;
            }
		}
		if (FlagParallels!=0)
		{
            if (lis!=NULL)
            {
                arr=AddList_to_commands(arr,lis,1,0);
			    ComLen++;
			    Clean(lis);
			    lis=NULL;
			    FlagParallels=0;
            }
		}
        if (flag==execute)
        {
            if (quote!=0)
            {
                fprintf(stderr,"ERROR: Quote wasn't closed!\n");
                exit(1);
            }
            if (PipL>0)
            {// звпустить конвейер
                if (lis!=NULL)
                {
                    arr=AddList_to_commands(arr,lis,3,3);
                    PipL++;
                    Clean(lis);
                    lis=NULL;
                }
                Conv(arr,PipL);
            }
            else
            {//запуск процессов
                if (lis!=NULL)
                {
                    arr=AddList_to_commands(arr,lis,0,0);
                    ComLen++;
                }
                MakeCommand(arr,ComLen);
                //printf("Введенные слова\n");
                //PrintCommands(arr);
                Clean(lis);
                lis=NULL;
            }
            CleanCommands(arr);   
            arr=NULL;
            lis=NULL;
            ComLen=0;
            PipL=0;
            BackgroundMode=0;
            dup2(fd1,1);// возвращает поток вывода на привычное место
            dup2(fd0,0);//возвращает поток ввода на привычное место
            PrintClosedProcesses();
            flag=getword;
            printf("Enter the command\n");
        }
    } while (flag!=finish); 
    killOpenProcess();
    free(BackPidsClosed);
    free(BackPidsOpen);
    return 0;
}
