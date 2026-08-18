#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define debug
/*Definition of object type*/
#define TFOBJ_TYPE_INT 0
#define TFOBJ_TYPE_STR 1
#define TFOBJ_TYPE_BOOL 2
#define TFOBJ_TYPE_LIST 3
#define TFOBJ_TYPE_SIMBOL 4
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
	tfobj *o = malloc(sizeof(tfobj));
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
tfobj *createSimbolObject(char *s, size_t len){
	tfobj *o = createStringObject(s, len);
	o->type = TFOBJ_TYPE_SIMBOL;
	return o;
}
tfobj *createListObject(){
	tfobj *o = createObject(TFOBJ_TYPE_LIST);
	o->list.elem = NULL;
	o->list.len = 0;
	return o;
}

void dumpobj(tfobj *o){
	
}
/*===============function description==================*/
tfobj *parseProgram(char *prg);
/*Print the object, return 1 if the operation went well*/
/*===============Function Implementation==============*/
/*------------String Object Function-----------*/
/* -----------List object function-------------*/
/*------------Parsing Related function----------*/
tfobj *parseProgram(char *prg);
/*==========Basic Function==============*/
/*-----------Standard library------------*/
/*------------Control structure----------*/
/*=======Main========*/
int main(int argc, char **argv){
	if(argc !=2){
		fprintf(stderr, "Badly written request\n usage: %s <filename>\n",argv[0]);
		
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
	fclose(testfile);
#ifdef debug
	printf("\"%s\\n",prgtext);
	//print prtext
#endif
	return 0;
}
