#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define KMAG  "\x1B[35m"
#define KYEL  "\x1B[33m"
#define KWHT  "\x1B[37m"

/* Splits the string by space and returns the array of tokens
 *
 */
char **tokenize(char *line)
{
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;

	for(i =0; i < strlen(line); i++){

		char readChar = line[i];

		if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
			token[tokenIndex] = '\0';
			if (tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0; 
			}
		} else {
			token[tokenIndex++] = readChar;
		}
	}

	free(token);
	tokens[tokenNo] = NULL ;
	return tokens;
}

char ***pTokenize(char *line,int *n)
{
	char ***pTokens = (char ***)malloc(10 * sizeof(char **));
	char *pToken = (char *)malloc(100* sizeof(char));
	int i, tokenIndex = 0,j =0,k=0;

	for(j = 0; j < 10; ++j){
		pTokens[j] = (char **)malloc(100 * sizeof(char *));
	}
	j = 0;
	for(i =0; i < strlen(line); i++){

		char readChar = line[i];

		if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
			pToken[tokenIndex] = '\0';
			if (tokenIndex != 0){
				pTokens[*n][k] = (char*)malloc(100*sizeof(char));
				strcpy(pTokens[*n][k++], pToken);
				tokenIndex = 0; 
			}
		} else if(readChar == '|'){
			++(*n);
			k = 0;
			
		}else {
			pToken[tokenIndex++] = readChar;
		}
	}
	k = 0;
	++(*n);
	free(pToken);
	pTokens[*n][k] = NULL;
	return pTokens;
}


void builtProc(int i, int o, char **pToken){
	if(!fork()){
		dup2(i,0);
		close(i);
		dup2(o,1);
		close(o);
		execvp(pToken[0],pToken);
	}
}

void builtPipe(int n, char ***pTokens){
	int i=0,j,o,fd[2];
	for(j = 0; j < n-1; ++j){
		pipe(fd);
		builtProc(i,fd[1],pTokens[j]);
		close(fd[1]);
		i = fd[0];
	}
	dup2(i,0);
	execvp(pTokens[n-1][0],pTokens[n-1]);
}

int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	FILE* fp;
	if(argc == 2) {
		fp = fopen(argv[1],"r");
		if(fp < 0) {
			printf("File doesn't exists.");
			return -1;
		}
	}
	char *path = (char *) malloc(sizeof(char) * 100);
	while(1) {			
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		DIR *dir = NULL;
		struct dirent *sd = NULL;
		path = getcwd(path, 100);

		if(argc == 2) { // batch mode
			if(fgets(line, sizeof(line), fp) == NULL) { // file reading finished
				break;	
			}
			line[strlen(line) - 1] = '\0';
		} else { // interactive mode
			printf("%ssohan%s:%s~%s$%s ",KYEL,KWHT,KMAG,path,KWHT);
			scanf("%[^\n]", line);
			getchar();
		}
		// printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

		int j = 0,flag = 0;
		while(tokens[j] != NULL){
			if(strcmp(tokens[j],"|")==0){
				flag = 1;
				break;
			}
			++j;
		}
		char ***pTokens; 
		//do whatever you want with the commands, here we just print the
		if(tokens[0] == NULL){
			continue;
		}
		else if(flag == 1){
			int n = 0;
			pTokens = pTokenize(line,&n);
			builtPipe(n,pTokens);
			
		}
		else if(strcmp(tokens[0],"exit")==0){
			exit(0);
		}
		else if(strcmp(tokens[0],"pwd")==0){
			printf("%s\n",path);
		}
		else if(strcmp(tokens[0],"ls")==0){
			dir = opendir((const char *)path);
			while((sd = readdir(dir))!= NULL){
				if(strcmp(sd->d_name,".") == 0 || strcmp(sd->d_name,"..") == 0 || sd->d_name[0]== '.'){
					continue;
				} else{
					printf("%s\t",sd->d_name);
				}
			}
			printf("\n");
		}
		else if(strcmp(tokens[0],"cd")==0){
			int result = chdir(tokens[1]);
		  	if(result != 0){
		    	switch(result){
		      		case EACCES: perror("Permission denied");
				    break;
				    case EIO: perror("An input output error occured");
				    break;
				    case ENAMETOOLONG: perror("Path is to long");
				    break;
				    case ENOTDIR: perror("A component of path not a directory"); 
				    break;
				    case ENOENT: perror("No such file or directory");
				    break;
				    default: perror("Couldn't find directory");
				    break;
		    	}
			}
		}
		else if(strcmp(tokens[0],"clear")==0){
			system("clear");
		}
		else if(strcmp(tokens[0],"mkdir")==0){
			char *p = tokens[1];
			int r = mkdir(p,0777);
		}
		else if(strcmp(tokens[0],"rm")==0){
			remove(tokens[1]);
		}
		// else if(strcmp(tokens[0],"sleep")==0){
		// 	int time;
		// 	sscanf(tokens[1],"%d",&time);
		// 	sleep(time);
		// }
		else if(strcmp(tokens[0],"echo")==0){
			i = 1;
			while(tokens[i]!=NULL){
				printf("%s ",tokens[i]);
				++i;
			}
			printf("\n");
		}
		// else if(strcmp(tokens[2],"|")==0){
		// 	char line3[200];
		// 	memset(line3,0,sizeof(line3));
		// 	FILE *fr = fopen(tokens[1],"r");
		// 	//FILE *fw = fopen("sample.txt", "w"); 
		// 	if(fr == NULL){
		// 		perror("File not find");
		// 	}
		// 	else if(strcmp(tokens[3],"grep")){
		// 		while (fgets(line3, sizeof(line3), fr)) {
		// 		   if (strstr(line3, tokens[4])) {
		// 		   		printf("%s",line3);
		// 		   }
		// 		   memset(line3,0,sizeof(line3));
		// 		}
		// 	} else{
		// 		FILE *fw = fopen(tokens[3],"w");
		// 		while(fgets(line3,sizeof(line3),fr)){
		// 			fprintf(fw,"%s\n",line3);
		// 		}
		// 	}

		// }
		else if(strcmp(tokens[0],"cat")==0){
			int f = open(tokens[1],O_RDONLY);
			char c;
			if(f == -1){
				perror("File not found");
			}
			else{
				while(read(f,&c,1)){
					write(STDOUT_FILENO,&c,1);
				}
				close(f);
			}
			printf("\n");
		}
		// else if(strcmp(tokens[0],"grep")==0){
		// 	char line2[200];
		// 	memset(line,0,sizeof(line2));
		// 	FILE *file = fopen(tokens[2],"r");
		// 	if(file == NULL){
		// 		perror("File not find");
		// 	}
		// 	else{
		// 		while (fgets(line2, sizeof(line2), file)) {
		// 		   if (strstr(line2, tokens[1])) {
		// 		   		printf("%s",line2);
		// 		   }
		// 		   memset(line,0,sizeof(line2));
		// 		}
		// 	}
			
		// }

		
		else {
			//execlp("vi","vi","1.txt",NULL);
			int n = 0;
			while(tokens[n]){
				n++;
			}

			int frk = fork();
			if(frk < 0){
				printf("Command couldn't execute");
			}else if(frk > 0){
				if(strcmp(tokens[n-1],"&")==0){
					continue;
				}
				else{
					wait(NULL);
				}
			} 
			else if(frk==0){
				if(strcmp(tokens[n-1],"&")==0){
					tokens[n-1] = NULL;
				}
				execvp(tokens[0],tokens);
			}
		}
		// for(i=0;tokens[i]!=NULL;i++){
		// 	printf("found token %s (remove this debug output later)\n", tok

		
		
		// for(i=0;tokens[i]!=NULL;i++){
		// 	printf("found token %s (remove this debug output later)\n", tokens[i]);
		// }

		// Freeing the allocated memory	
		// }
		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);
	}
	return 0;

}