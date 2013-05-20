#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/*system constants*/
#define BUF_SZ	256

/*io wrapper*/
#define myputc(X)	fputc(X,stdout);
#define print(X)	fprintf(stdout,"%s",X);
#define error(X)	do{									\
				fprintf(stderr,"file %s\tline %d\n",__FILE__,__LINE__);		\
				fprintf(stderr,"%s",X);						\
			}while(0)

enum obj_type{ S_NONE, S_LIST, S_STRING, S_SYMBOL, S_INT };

struct s_expr {
	enum obj_type type;
	size_t len;
	void *buf; /* S_NONE == ?, S_LIST == struct s_expr *X[], S_STRING || S_SYMBOL == char * */
};

struct s_expr* parse_string(char *s, char **ret_s){
	char buf[BUF_SZ]={0};
	int i=0;
	struct s_expr *se = malloc(sizeof(struct s_expr));

	if(se==NULL){
		error("malloc() call failed!\n");
		return NULL;
	}
	
	while(*s!='\0'){
		if(i>= BUF_SZ){
			error("String too long!\n");
			goto FAILURE;
		}

		switch(*s){
			case '"': /*Success!*/
				se->type = S_STRING;
				se->len = i; /*check this!*/
				se->buf = calloc(sizeof(char),i+1);
				if(se->buf==NULL){
					error("calloc() failed!\n");
					goto FAILURE;
				}
				memcpy(se->buf,buf,i);
				*ret_s = s + 1;

				return se;
			case '\\':
				switch(*++s){
					case '\\':
					case '"':
						buf[i++] = *s++;
						continue;
					case 'n':
						buf[i++] = '\n';
						s++;
						break;
					default:
						error("Not an escape character!\n");
						goto FAILURE;
				}
				break;
			default:
				buf[i++]=*s++;
		}

	}
FAILURE:
	error("Fail!\n");
	free(se);
	return NULL;
}
struct s_expr* parse_symbol(char *s, char **ret_s){
	char buf[BUF_SZ]={0};
	int i=0;
	struct s_expr *se = malloc(sizeof(struct s_expr));

	if(se==NULL){
		error("malloc() call failed!\n");
		return NULL;
	}

	if(s!=NULL) /*simple error checking*/
		while(*s!='\0'){
			if(i>= BUF_SZ){
				error("String too long!\n");
				goto FAILURE;
			}
						
			if(isspace(*s))
				goto SUCCESS;

			if(*s == ')' || *s == '('){
				s--;
				goto SUCCESS;
			}

			switch(*s){
				case '\\':
					switch(*++s){
						case '\\' : case '"': case '(': case ')':
							buf[i++] = *s++;
							continue;
						default:
							error("Not an escape character!\n");
							goto FAILURE;
					}
				case '"':
					/*Note, original marked this as a success but with an error message*/
					error("The '\"' is not allowed in symbol names!\n");
				       goto FAILURE;	
				default:
				       buf[i++] = *s++;

			}
		}

SUCCESS:
	*ret_s = s + 1;
	se->type = S_SYMBOL;
	se->len = i; /*check this!*/
	se->buf = calloc(sizeof(char),i+1);
	if(se->buf==NULL){
		error("calloc() failed!\n");
		goto FAILURE;
	}
	memcpy(se->buf,buf,i);
	return se;

FAILURE:
	free(se);
	return NULL;
}


int append(struct s_expr *list, struct s_expr *element)
{
	/*We have to realloc so it is gets a continuous piece of memory*/
	list->buf = realloc(list->buf, sizeof(struct s_expr *) * ++list->len);
	/*Check realloc!*/
	if(list->buf==NULL)
		return EXIT_FAILURE;
	else{
		((struct s_expr **)(list->buf))[list->len - 1] = element;
		return EXIT_SUCCESS;
	}
}

struct s_expr* parse_list(char *s, char **ret_s){
	struct s_expr *se = malloc(sizeof(struct s_expr));
	struct s_expr *child;
	char *next;

	if(se==NULL){
		error("malloc() call failed!\n");
		return NULL;
	}

	se->len = 0;

	while(*s!='\0'){
		if(isspace(*s)){
			s++;
			continue;
		}

		switch(*s){
			case ')':
				goto SUCCESS;
			case '(': /*parse list*/
				child = parse_list(s+1, &next);
				if(child==NULL){
					error("parse_list() failed!\n");
					goto FAILURE;
				}
				if(append(se,child)==EXIT_FAILURE){
					error("append() failed!\n");
					goto FAILURE;
				}
				s = next;
				continue;
			case '"': /*parse string*/
				child = parse_string(s+1, &next);
				if(child == NULL){
					error("parse_string() failed!\n");
					goto FAILURE;
				}
				if(append(se,child)==EXIT_FAILURE){
					error("append() failed!\n");
					goto FAILURE;
				}
				s = next;
				continue;
			default: /*parse symbol*/
				child = parse_symbol(s,&next);
				if(child == NULL){
					error("parse_symbol() failed!\n");
					goto FAILURE;
				}
				if(append(se,child)==EXIT_FAILURE){
					error("append() failed!\n");
					goto FAILURE;
				}
				s = next;
				continue;
		}

	}

SUCCESS:
	se->type=S_LIST;
	*ret_s = s + 1;
	return se;
FAILURE:
	free(se);
	return NULL;
}

struct s_expr* parse_sexpr(char *s, char **ret_s){
	if(s!=NULL) /*simple error checking*/
		while(*s!='\0'){
			if(isspace(*s)){
				s++;
				continue;
			}
			switch(*s){
				case '(':
					return parse_list(s+1,ret_s);
				case '"':
					return parse_string(s+1,ret_s);
				default:
					return parse_symbol(s,ret_s);
			}
		}
	return NULL;
}


void print_sexpr(struct s_expr *se){
	int i;
	struct s_expr *temp;
	
	if(se->type==S_SYMBOL)
		fprintf(stdout,"SYM: %s\n",(char *)se->buf);
	if(se->type==S_STRING)
		fprintf(stdout,"STR: %s", (char *) se->buf);
	if(se->type==S_LIST){
		fprintf(stdout,"LST: %d\n", se->len);
		for(i=0;i<se->len;i++){
			fprintf(stdout,"PTR:%ld\n",(long) (temp=&(((struct s_expr *)(se->buf))[i])));
			fprintf(stdout,"\tTYPE:%ld\n",(long) temp->type);
			fprintf(stdout,"\tLEN:%ld\n",(long) temp->len);
		}
	}

}

int main(int argc, char *argv[]){
	char *parse_me;
	char *ret_s;
	struct s_expr *se;

	if(argc!=2){
		error("Incorrect usage!\n");
		return EXIT_FAILURE;
	}

	parse_me = argv[1];
	ret_s = parse_me;

	print("Parsing:\n");
	print(argv[1]);
	myputc('\n');

	se=parse_sexpr(parse_me,&ret_s);

	if(se==NULL){
		fprintf(stdout,"SE == NULL\n");

	}
	else{
		fprintf(stdout,"LEN:%d\t%d\n",strlen(parse_me),(int)(ret_s - parse_me));
		print_sexpr(se);

	}

	return EXIT_SUCCESS;
}
