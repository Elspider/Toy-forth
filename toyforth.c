#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#define debug
/*Definition of object type*/
#define TFOBJ_TYPE_INT 0
#define TFOBJ_TYPE_STR 1
#define TFOBJ_TYPE_BOOL 2
#define TFOBJ_TYPE_LIST 3
#define TFOBJ_TYPE_SYMBOL 4
/*Definition of Function Type*/
#define F_TYPE_NATIVE 0
#define F_TYPE_USER 1
/*definition os standard error*/
#define TFERR_OK 0
#define TFERR_UNDERFLOW 1
#define TFERR_TIPE 2
/*--------------------Basic Data structures*------------------------------*/
typedef struct tfobj tfobj;
typedef struct tfparser tfparser;
typedef struct funcEntry FuncEntry;
typedef struct funcTab FuncTab;
typedef struct tfctx tfctx;

typedef struct tfobj{
	int refcount;
	int type; //TFOBG_TYPE_*
	union{
		//Type int
		int num;
		//type string
		struct{
			char *ptr;
			size_t len;
		}str;
		//type list
		struct{
			struct tfobj **elem;
			size_t len;
			size_t max_capacity;
		}list;
	};
}tfobj;

typedef struct tfparser{
	char *prg; //the program to compile into a list. It is alwais null-termineted
	char *p; //the next token to parse
}tfparser;
typedef struct funcEntry{
	int type;//F_TYPE_* 
	union{
		tfobj *userList;
	    int (*callback)(tfctx *ctx, tfobj *name);

	};
}FuncEntry;
typedef struct funcTab{
	size_t len;
	FuncEntry **elem;
}FuncTab;
//Context of execution
typedef struct tfctx{
	tfobj *stack;
	FuncTab *func_table;
}tfctx;

/*--------------------Allocation wrappers------------------------------*/

void *xmalloc(size_t size){
	void *ptr = malloc(size);
	if(ptr == NULL){
		fprintf(stderr, "Out of memory allocating %zu Bytes\n",size);
		exit(1);
	}
	return ptr;
}
void *xrealloc(void * old, size_t size){
	void *ptr = realloc(old, size);
	if(ptr == NULL){
		fprintf(stderr, "Out of memory reallocating %zu Bytes\n",size);
		exit(1);
	}
	return ptr;
}

/*--------------------Object related function------------------------------*/

/*Allocate and initialize a new toyfoth object*/
tfobj *createObject(int type){
	tfobj *o = xmalloc(sizeof(tfobj));
	o->type = type;
	o->refcount = 1;
	return o;
}

/*The following functions allocate different kind of ToyFort Object*/
tfobj *createStringObject(char *s, size_t len){
	tfobj *o = createObject(TFOBJ_TYPE_STR);
	o->str.ptr = xmalloc(len);
	memcpy(o->str.ptr,s,len);
	o->str.len = len;
	return o;
}
tfobj *createBoolObject(int i){
	tfobj *o = createObject(TFOBJ_TYPE_BOOL);
	o->num = i;
	return o;
}
tfobj *createIntObject(int num){
	tfobj *o = createObject(TFOBJ_TYPE_INT);
	o->num = num;
	return o;
}
tfobj *createSymbolObject(char *s, size_t len){
	tfobj *o = createStringObject(s, len);
	o->type = TFOBJ_TYPE_SYMBOL;
	return o;
}
tfobj *createListObject(){
	tfobj *o = createObject(TFOBJ_TYPE_LIST);
	o->list.elem = NULL;
	o->list.len = 0;
	o->list.max_capacity = 0;
	return o;
}

void dumpobj(tfobj *o){
	if(o == NULL){
		fprintf(stderr, "\nTried to print a null object\n");
		exit(1);
	}
	switch (o->type){
		case TFOBJ_TYPE_BOOL:
			(o->num) ? printf("True ") : printf("False");
			break;
		case TFOBJ_TYPE_INT:
			printf("%d ", o->num);
			break;
		case TFOBJ_TYPE_SYMBOL:
			printf("%s ",o->str.ptr);
			break;
		case TFOBJ_TYPE_STR:
			printf("\" %s\" ",o->str.ptr);
			break;
		case TFOBJ_TYPE_LIST:
			putchar('[');
			for(size_t j = 0; j < o->list.len; j++){
				dumpobj(o->list.elem[j]);
			}
			printf("] ");
			break;
		default: break;
	}
}
/*===============Necessary function prototype==================*/
void deleteObject(tfobj *o);
void relese(tfobj *o);
void retain(tfobj *o);
int isSymbolChar(int c);
/*Turn a null-terminated buffer into a List
 * object containing the list*/
tfobj *compile(char *prg);

/*===============Function Implementation=======================*/
/*-------------Memory managment function---------*/
void deleteObject(tfobj *o){
	if(o == NULL){
#ifdef debug
		fprintf(stderr,"Tried to delete a NULL object\n");
#endif
		return;
	}
	tfobj *current = NULL;
	switch (o->type){
		case TFOBJ_TYPE_SYMBOL:
		case TFOBJ_TYPE_STR:
			free(o->str.ptr);
			break;
		case TFOBJ_TYPE_LIST:
			for(size_t j = 0; j < o->list.len; j++){
				current = o->list.elem[j];
				relese(current);
			}
			break;
		default: break;
	}
	free(o);
}

void retain(tfobj *o){
	assert(o->refcount > 0);
	o->refcount++;
}
void relese(tfobj *o){
	assert(o->refcount > 0);
	o->refcount--;
	if(o->refcount == 0) deleteObject(o);
}
/*------------String Object Function-----------*/
/* -----------List object function-------------*/

/*Append an element to the list and hence
 * increase the reference count of the object */
void listPush(tfobj *o, tfobj *list){
	assert(list != NULL && list->type == TFOBJ_TYPE_LIST );
	if(list->list.max_capacity <= list->list.len){
		list->list.max_capacity = (list->list.max_capacity == 0) ? 1 : list->list.max_capacity * 2;
		list->list.elem = xrealloc(list->list.elem, list->list.max_capacity * sizeof(tfobj*));
	}
	list->list.elem[list->list.len] = o;
	list->list.len++;
	retain(o);
}
/* Pop out the last element from the list*/
tfobj *listPop(tfobj *list){
	tfobj *o;
	list->list.len--;
	o = list->list.elem[list->list.len];
	list->list.elem[list->list.len] = NULL;
	//Check if list is mostly empty
	if(list->list.max_capacity > 4 * list->list.len)
		list->list.elem = xrealloc(list->list.elem , sizeof(tfobj*) * list->list.len);
	return o;
}
/*This function return the last element on the list.
 * if the object is stored, it must be retained by the caller
 * This is made to allow rapid inline comparison*/
 //TODO: Add an offset option to the command?
tfobj *listPeek(tfobj *list){
	tfobj *o = list->list.elem[list->list.len - 1 ];
	return o;
}

/*------------Parsing Related function----------*/
int isSymbolChar(int c){
	char buf[] = "+-*/%<>=";
	return ( 
			(strchr(buf, c) == NULL) ||
			 isalpha(c) );
}
#define MAX_INT_LEN 64
tfobj *parseNumber(tfparser *p){
	int num;
	char *start = p->p;
	char *end;
	char buf[MAX_INT_LEN];

	//Using do while to avoid minus exception
	do{
		p->p++;
	}while(isdigit(p->p[0]));
	end = p->p;
	memcpy(buf, start, end - start);
	buf[end-start] = 0;
	num = atoi(buf);
	
	tfobj *o = createIntObject(num);
	return o;
}
tfobj *parseList(tfparser *p){
	char *start = ++p->p;
	char *end;

	while(p->p[0] != 0 && p->p[0] != ']')
		p->p++;
	end = p->p;
	if(end[0] == 0) return NULL;
	else(p->p++);

	char * buf = xmalloc((end-start+1)*sizeof(char));
	buf[end-start] = 0;
	memcpy(buf, start, end - start);
	tfobj *o = compile(buf);


	free(buf);
	return o;
}

tfobj *parseString(tfparser *p){
	char *start = ++p->p;
	char *end;

	while(p->p[0] != 0 && p->p[0] != '\"')
		p->p++;
	end = p->p;
	if(end[0] == 0) return NULL;

	tfobj *o = createStringObject(start, end-start);
	//Go over the string
	p->p++;
	return o;
}
tfobj *parseSymbol(tfparser *p){
	char *start = ++p->p;
	char *end;

	while(isSymbolChar(p->p[0]))
		p->p++;
	end = p->p;

	tfobj *o = createSymbolObject(start, end-start);

	return o;
}
void parseSpace(tfparser *p){
	while(isspace(p->p[0])) p->p++;
}
/*parse the string containing program and 
 * return it as a Toyforth List*/
tfobj *compile(char *prg){
	tfparser parser;
	tfobj *parsed = createListObject();
	parser.p = parser.prg = prg;
	char *startToken;
	tfobj *o = NULL;
	while(parser.p[0]){
		o = NULL;
		parseSpace(&parser);
		if(parser.p[0] == 0) break;
		startToken = parser.p;
		if(isdigit(parser.p[0]) ||
		  (parser.p[0] == '-' && isdigit(parser.p[1]))){
			o =parseNumber(&parser);
		}
		else if(parser.p[0] == '['){
			o = parseList(&parser);
		}
		else if(parser.p[0] == '"'){
			o = parseString(&parser);
		}
		else if(isSymbolChar(parser.p[0])){
			o = parseSymbol(&parser);
		}
		//appending the object
		if(o == NULL){
			fprintf(stderr,"Sintax error at %s", startToken);
			return NULL;
		}
		else{
			listPush(o, parsed);
		}
	}
	return parsed;
}
/*==========Basic Function==============*/
/*-----------Standard library------------*/
/*------------Control structure----------*/
/*=======Main========*/
int main(int argc, char **argv){
	if(argc !=2){
		fprintf(stderr,"Badly written request\n usage: %s <filename>\n",argv[0]);
		return 1;
	}
	//Read the file into prgtext
	FILE *testfile;
	testfile = fopen(argv[1], "r");
	fseek(testfile, 0, SEEK_END);
	long file_size = ftell(testfile);
	printf("file size: %lu\n", file_size);
	char *prgtext = xmalloc((size_t)file_size+1);
	fseek(testfile, 0, SEEK_SET);
	fread(prgtext, file_size, 1, testfile);
	prgtext[file_size] = 0;
	if(testfile == NULL){
		fprintf(stderr, "Invalid file selected\n");
		return 1;
	}
	fclose(testfile);
#ifdef debug
	printf("\"%s\"\n",prgtext);
	//print prtext
#endif
	tfobj *prg = compile(prgtext);
	if(prg == NULL){
		fprintf(stderr, "error while complinig the program\n");
			return 1;
	}
	dumpobj(prg);
	putchar('\n');
	return 0;
}
