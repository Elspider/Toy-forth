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
#define TFERR_TYPE 2
#define TFERR_NOFUNC 3
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
	tfobj* name;
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
	FuncTab func_table;
	int error;
}tfctx;

/*--------------------Allocation wrappers------------------------------*/

void *xmalloc(size_t size){
	void *ptr = malloc(size);
	if(ptr == NULL && size != 0){
		fprintf(stderr, "Out of memory allocating %zu Bytes\n",size);
		exit(1);
	}
	return ptr;
}
void *xrealloc(void * old, size_t size){
	void *ptr = realloc(old, size);
	if(ptr == NULL && size != 0){
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
	o->str.ptr[len] = 0;
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
			printf("\"%s\" ",o->str.ptr);
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
/*===============Necessary forward declaration==================*/
void deleteObject(tfobj *o);
void relese(tfobj *o);
void retain(tfobj *o);
int isSymbolChar(int c);
/*Turn a null-terminated buffer into a List
 * object containing the list*/
tfobj *compile(char *prg);
int executeList(tfctx *ctx, tfobj *prg);

/*-----TF function forwars declaration -------*/
int basicMathFunctions(tfctx *ctx, tfobj *name);


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
			free(o->list.elem);
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

/*This function use memcmp on the string
 * of 2 object, and return 0 if two string object 
 * have the same string, 1 if the first string is greather
 * than the second according to memcmp() and -1 if 
 * the second is. 
 * If one string is an exact prefixe of another, meaning it is 
 * shorter but matching for all its character, it is considered
 * lesser than.
 * WARNING: Function does not check object type*/
int stringObjCompare(tfobj *obj1, tfobj *obj2){
	int minlen = ( obj1->str.len < obj2->str.len ) ? obj1->str.len : obj2->str.len;
	int res = memcmp(obj1->str.ptr, obj2->str.ptr, minlen);
	if(res > 0) return  1;
	else if( res < 0) return -1;
	else{
		res = (obj1->str.len > obj2->str.len ) ? 1 : -1;
	}
	return res;
}

/* -----------List object function-------------*/

/*Append an element to the list and hence
 * increase the reference count of the object */
void listPush(tfobj *o, tfobj *list){
	assert(list != NULL);
	assert(list->type == TFOBJ_TYPE_LIST );
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
	assert(list->list.len > 0);
	list->list.len--;
	o = list->list.elem[list->list.len];
	list->list.elem[list->list.len] = NULL;
	//Check if list is mostly empty
	if(list->list.max_capacity > 4 * list->list.len && list->list.len != 0)
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
	if(c == 0) return 0;
	char buf[] = "+-*/%<>=";
	return ( 
			(strchr(buf, c) != NULL) ||
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
	int listcount = 1;
	while(p->p[0] != 0 && listcount > 0){
		p->p++;
		if(p->p[0] == '[') listcount++;
		else if(p->p[0] == ']') listcount--;
	}
	end = p->p;

	if(end[0] == 0) return NULL;
	else p->p++;

	char *buf = xmalloc((end-start+1)*sizeof(char));
	memcpy(buf, start, end - start);
	buf[end-start] = 0;
	tfobj *o = compile(buf);

	free(buf);

	return o;
}

tfobj *parseString(tfparser *p){
	char *start = ++p->p;
	char *end;

	while(p->p[0] != 0 && p->p[0] != '"')
		p->p++;
	end = p->p;
	if(end[0] == 0) return NULL;

	tfobj *o = createStringObject(start, end-start);
	//Go over the string
	p->p++;
	return o;
}
tfobj *parseSymbol(tfparser *p){
	char *start = p->p;
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
		else o = NULL;
		//appending the object
		if(o == NULL){
			fprintf(stderr,"Sintax error at %s", startToken);
			relese(parsed);
			return NULL;
		}
		else{
			listPush(o, parsed);
			relese(o);
		}
	}
	return parsed;
}
/*========Basic Functions managment=========*/
FuncEntry *getFunctionEntry(tfctx *ctx, tfobj *name){
	assert(name->type == TFOBJ_TYPE_SYMBOL || name->type == TFOBJ_TYPE_STR);
	for(size_t j = 0; j < ctx->func_table.len; j++){
		if(stringObjCompare(name, ctx->func_table.elem[j]->name))
			return ctx->func_table.elem[j];
	}
	return NULL;
}
void registerFunction(tfctx *ctx, FuncEntry *fe){
	ctx->func_table.len++;
	ctx->func_table.elem = xrealloc(ctx->func_table.elem, (ctx->func_table.len) * sizeof(FuncEntry*));
	ctx->func_table.elem[ctx->func_table.len - 1] = fe;
}

void registerCFunction(tfctx *ctx, char *name, int (*callback)(tfctx *ctx, tfobj *name)){
	tfobj *oname = createSymbolObject(name, strlen(name));
	FuncEntry *fe = xmalloc(sizeof(FuncEntry));
	fe->name = oname;
	fe->type = F_TYPE_NATIVE;
	fe->callback = callback;
	registerFunction(ctx, fe);
}
/*==============Context and execution===========*/


/*-------Context related function---------*/
tfctx *createContext(){
	tfctx *ctx = xmalloc(sizeof(tfctx));
	ctx->stack = createListObject();
	ctx->func_table.len = 0;
	ctx->func_table.elem = NULL;
	ctx->error = TFERR_OK;
	registerCFunction(ctx, "+", basicMathFunctions);
	registerCFunction(ctx, "-", basicMathFunctions);
	registerCFunction(ctx, "*", basicMathFunctions);
	registerCFunction(ctx, "/", basicMathFunctions);
	registerCFunction(ctx, "%", basicMathFunctions);
	return ctx;
}
void deleteContext(tfctx *ctx){
	FuncEntry *curr;
	relese(ctx->stack);
	for(size_t j = 0; j < ctx->func_table.len; j++){
		curr = ctx->func_table.elem[j];
		switch (curr->type) {
			case F_TYPE_USER:
				relese(curr->userList);
				break;
			case F_TYPE_NATIVE:
				break;
		}
		free(curr);
	}
	free(ctx);
}
/*return the last element from the context
 * and erase it from the stack*/
tfobj *ctxStackPop(tfctx *ctx, int type){
	tfobj *o = listPop(ctx->stack);
	if(o->type != type) ctx->error = TFERR_TYPE;
	return o;
}
/*return the last element from the context
 * without erasing it from the stack*/
tfobj *ctxStackPeek(tfctx *ctx, int type){
	tfobj *o = listPeek(ctx->stack);
	if(o->type != type) ctx->error = TFERR_TYPE;
	return o;

}
void ctxStackPush(tfctx *ctx, tfobj *o){
	listPush(o, ctx->stack);
}
/*-----------Standard library------------*/
int basicMathFunctions(tfctx *ctx, tfobj *name){
	int res;
	tfobj *b = ctxStackPop(ctx, TFOBJ_TYPE_INT);
	tfobj *a = ctxStackPop(ctx, TFOBJ_TYPE_INT);
	if(ctx->error != TFERR_OK) goto cleanup;
	switch(name->str.ptr[0]){
		case '+': res = a->num + b->num; break;
		case '-': res = a->num - b->num; break;
		case '*': res = a->num * b->num; break;
		case '/': res = a->num / b->num; break;
		case '%': res = a->num % b->num; break;
	}
	tfobj *o = createIntObject(res);
	ctxStackPush(ctx, o);

//TODO: Maybe adding an option to sum string?
cleanup:
	relese(a);
	relese(b);
	return ctx->error;
}
/*------------Control structure----------*/

/*----------Execution related function------------*/

void executeSymbol(tfctx *ctx, tfobj *name){
	FuncEntry *fe = getFunctionEntry(ctx, name);
	if(fe == NULL){
		ctx->error = TFERR_NOFUNC;
	}

	switch(fe->type){
		case F_TYPE_NATIVE:
			ctx->error = fe->callback(ctx, name);
			break;
		case F_TYPE_USER:
			ctx->error = executeList(ctx, fe->userList);
			break;
	}
}

int executeList(tfctx *ctx, tfobj *list){
	tfobj *word;
	tfobj *sublist;
	tfctx *subctx;
	for(size_t j = 0; j < list->list.len; j++){
		word = list->list.elem[j];
		switch (word->type) {
			case TFOBJ_TYPE_SYMBOL:
				executeSymbol(ctx, word);
				break;
			case TFOBJ_TYPE_LIST:
				subctx = createSubContext(ctx);
				executeList(subctx, sublist);
				ctxStackPush(ctx, sublist);
			default:
				ctxStackPush(ctx, word);
				break;
		}
		if(ctx->error != TFERR_OK){
			//TODO: ADD error Location here
			return ctx->error;
		}
	}
	return TFERR_OK;
}
/*This function execute the program and print
 * an output if there are some error*/
void executeProgram(tfctx *ctx, tfobj *prg){
	executeList(ctx, prg);
	switch (ctx->error) {
		case TFERR_NOFUNC:
			fprintf(stderr, "Error: No matching function found\n");
			break;
		case TFERR_TYPE:
			fprintf(stderr,"Error: Wrong type for function \n"); 
			break;
		case TFERR_UNDERFLOW:
			fprintf(stderr, "Error: Missing argumen for function \n");
			break;
		case TFERR_OK:
			printf("Execution of the program did not produce any error\n");
			break;
		default: break;
	}
	return;
}
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
		fprintf(stderr, "error while compiling the program\n");
			return 1;
	}
	free(prgtext);
	dumpobj(prg);
	putchar('\n');
	
	printf("Program after execution\n");
	tfctx *ctx = createContext();
	executeProgram(ctx, prg);
	dumpobj(ctx->stack);
	putchar('\n');
	deleteContext(ctx);
	relese(prg);
	return 0;
}
