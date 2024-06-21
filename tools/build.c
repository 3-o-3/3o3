/******************************************************************************
 *                       OS-3o3 operating system
 * 
 *               project builder using .vscode/tasks.json
 *
 *            19 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#include <process.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#include <unistd.h>
#else
#include <unistd.h>
#include <wait.h>
#endif

/*
 * types definition
 */
typedef long os_intn;
typedef unsigned long os_size;
typedef unsigned int os_uint32;
typedef unsigned char os_utf8;
typedef char os_bool;
typedef int os_result;


/**
 * maximal nested JSON memebers.
 */
#define json__MAX_DEPTH 20

/**
 * JSON object types
 */
enum
{
	json__undefined,
	json__object_null,
	json__boolean,
	json__boolean_true,
	json__boolean_false,
	json__number,
	json__bigint,
	json__string,
	json__symbol,
	json__function,
	json__object,
	json__object_array
};

/**
 * JSON state machine definition
 */
enum
{
	json__state_init,
	json__state_object_body,
	json__state_object_name,
	json__state_object_after_name,
	json__state_object_after_colon,
	json__state_object_after_comma,
	json__state_object_after_member,
	json__state_array_body,
	json__state_array_after_comma,
	json__state_array_after_member,
	json__state_end,
};

/**
 * JSON object structure
 */
struct json__object
{
	/** root tree */
	struct json__tree *tree;
	/** index in root tree stack*/
	os_intn depth;
	/**  JSON parser state of this object*/
	os_intn state;

	/** index of the start of the name of the current member in buffer */
	os_size child_name_s;
	/** index of the end of the name of the current member in buffer */
	os_size child_name_e;
	/** JSON type of the current member */
	os_intn child_type;

	/** type of this object */
	os_intn type;
	/** index of the start of the name of this object in buffer */
	os_size name_start;
	/** index of the end of the name of this object in buffer */
	os_size name_end;
	/** index of the start of the value of this object in buffer */
	os_size value_start;
	/** index of the end of the value of this object in buffer */
	os_size value_end;

	/** count of currenly parsed memeber of this object */
	os_intn count;
};

/**
 * root tree object (context) for JSON parser
 */
struct json__tree
{
	/** alloced buffer containing the full text of the JSON file */
	os_utf8 *buf;
	/** number of bytes in buffer */
	os_size len;
	/** JSON object stack. 
	 * At index 0 we have the "undefined" object.  
	 * At index 1 we have the root JSON object, 
	 * so members are at index 2 and above. */
	struct json__object stack[json__MAX_DEPTH + 2];
};


void json__dump(FILE *fp, struct json__object *obj, os_size len, os_utf8 *buf);

/**
 * display error message and context
 */
struct json__object *json__error(struct json__object *obj, os_size offset, os_utf8 *message)
{
	os_size line = 1;
	os_size col = 1;
	os_size i = 0;
	os_utf8 *p = obj->tree->buf;
	while (i < offset && i < obj->tree->len)
	{
		if (*p == '\n')
		{
			line++;
			col = 0;
		}
		col++;
		i++;
		p++;
	}
	fprintf(stderr, "JSON error : at line %lu col %lu : %s\n", line, col, message);
	return obj->tree->stack;
}

/**
 * copy the bytes at name_start to name_end from root tree buffer to buf.
 */
os_result json__copy_string(struct json__object *ret, os_size name_start,
							os_size name_end, os_size len, os_utf8 *buf)
{
	os_utf8 *q, *s;
	os_size j;
	q = buf;
	s = ret->tree->buf;
	*q = '\0';
	if (name_end - name_start >= len - 1)
	{
		fprintf(stderr, "buffer too small in copy string\n");
		exit(-10);
		return -1;
	}
	for (j = name_start; j < name_end; j++)
	{
		*q = s[j];
		q++;
	}
	*q = 0;
	return 0;
}

/**
 * go one level deeper in the tree stack.
 * \param obj the current parsed JSON object
 * \param offset byte offset of the beginning of the obj's value
 * \param out the new current object
 * \returns 0 on success
 */
os_result json__push(struct json__object *obj, os_size offset,
					 struct json__object **out)
{
	struct json__object *ret;
	if (obj->depth >= json__MAX_DEPTH)
	{
		*out = json__error(obj, offset, (os_utf8*)"too many nested level of objects");
		return -1;
	}

	obj->count++;

	ret = &obj->tree->stack[obj->depth + 1];
	ret->tree = obj->tree;
	ret->depth = obj->depth + 1;
	ret->value_start = offset + 1;
	ret->value_end = 0;
	ret->count = 0;

	ret->child_name_s = 0;
	ret->child_name_e = 0;
	ret->child_type = json__undefined;

	if (obj->child_type != json__undefined)
	{
		ret->name_start = obj->child_name_s;
		ret->name_end = obj->child_name_e;
		ret->type = obj->child_type;
		obj->child_type = json__undefined;
	}
	else
	{
		ret->type = 0;
		ret->name_start = 0;
		ret->name_end = 0;
	}

	*out = ret;
	return 0;
}

/**
 * end of parsed object.
 * \param obj the current parsed JSON object
 * \param offset byte offset of the end of the obj's value
 * \param out the new current object
 * \returns 0 on success
 */
os_result json__pop(struct json__object *obj, os_size offset,
					struct json__object **out)
{
	if (obj->depth < 1)
	{
		*out = json__error(obj, offset, (os_utf8*)"unmatched }");
		return -1;
	}
	obj->value_end = offset - 1;
	*out = &obj->tree->stack[obj->depth - 1];
	return 0;
}

/**
 * set the name and type of the upcomming memeber
 */
os_result json__prepare_member(struct json__object *ret, os_intn type, os_size name_start, os_size name_end)
{
	ret->child_name_s = name_start;
	ret->child_name_e = name_end;
	ret->child_type = type;
	return 0;
}

/**
 * add an member to the ret object.
 * The of the member must have been set by json__prepare_member()
 */
os_result json__add_member(struct json__object *ret, os_intn type, os_size value_start, os_size value_end)
{
	struct json__object *child;
	if (ret->child_type != json__undefined)
	{
		if (ret->child_type != type || ret->type != json__object)
		{
			json__error(ret, value_start, (os_utf8*)"data mismatch");
			return -1;
		}
	}
	child = &ret->tree->stack[ret->depth + 1];
	child->depth = ret->depth + 1;
	child->tree = ret->tree;
	child->name_start = ret->child_name_s;
	child->name_end = ret->child_name_e;
	child->type = type;
	child->value_start = value_start;
	child->value_end = value_end;
	ret->child_type = json__undefined;
	ret->count++;
	if (ret->state == json__state_array_body || ret->state == json__state_array_after_comma)
	{
		ret->state = json__state_array_after_member;
	}
	else
	{
		ret->state = json__state_object_after_member;
	}
	return 0;
}

/**
 * check if name match 
 * \returns 1 if name matches
 */
os_result json__match(struct json__object *ret, os_utf8 *name)
{
	os_size i;
	os_utf8 *p, *r;

	r = ret->tree->buf;
	p = name;
	for (i = ret->name_start; i < ret->name_end; i++)
	{
		if (*p != r[i])
		{
			return 0;
		}
		p++;
	}
	if (*p == '\0')
	{
		return 1;
	}
	return 0;
}

/* 
 * gets the next memmber after child in obj
 * \param obj the current parsed object
 * \param child the current member object or NULL 
 * \returns the found member or NULL
 */
struct json__object *json__get(struct json__object *obj, struct json__object *child)
{
	struct json__object *ret;
	os_utf8 *p;
	os_size i, end, start;
	os_size name_start = 0, name_end = 0;
	os_intn od;

	
	od = obj->depth;
	ret = obj;

	end = obj->value_end;
	if (child)
	{
		if (child->type == json__object || child->type == json__object_array)
		{
			start = child->value_end + 2;
		}
		else
		{
			start = child->value_end + 1;
		}
	}
	else
	{
		start = obj->value_start;
	}
	if (obj->depth >= json__MAX_DEPTH)
	{
		return json__error(obj, start, (os_utf8*)"too many nested elements");
	}
	ret->count = 0;
	p = obj->tree->buf + start;
	/* printf("get %ld at %ld\n", od, index); */
	for (i = start; i < end; i++)
	{

		switch (*p)
		{
		case '{':
		case '[':
			if (ret->state == json__state_init && *p != '{')
			{
				return json__error(obj, i, (os_utf8*)"{ expected at root");
			}
			if (ret->state == json__state_init)
			{
				ret->state = json__state_object_after_member;
			}
			else if (ret->state == json__state_object_after_colon)
			{
				if (*p == '{')
				{
					json__prepare_member(ret, json__object, name_start, name_end);
				}
				else
				{
					json__prepare_member(ret, json__object_array, name_start, name_end);
				}
				ret->state = json__state_object_after_member;
			}
			else if (ret->state == json__state_array_after_comma ||
					 ret->state == json__state_array_body)
			{
				ret->state = json__state_array_after_member;
			}
			else
			{
				if (*p == '{')
				{
					return json__error(obj, i, (os_utf8*)"unexpected {");
				}
				else
				{
					return json__error(obj, i, (os_utf8*)"unexpected [");
				}
			}
			if (json__push(ret, i, &ret))
			{
				return ret;
			}
			if (*p == '{')
			{
				ret->type = json__object;
				ret->state = json__state_object_body;
			}
			else
			{
				ret->type = json__object_array;
				ret->state = json__state_array_body;
			}
			break;
		case '}':
			if (ret->state != json__state_object_body &&
				ret->state != json__state_object_after_member &&
				ret->state != json__state_object_after_comma)
			{
				return json__error(obj, i, (os_utf8*)"unexpeced }");
			}
			if (json__pop(ret, i, &ret))
			{
				return ret;
			}
			if (ret->depth < od)
			{
				ret->state = json__state_end;
			}
			break;
		case ']':
			if (ret->state != json__state_array_body &&
				ret->state != json__state_array_after_member &&
				ret->state != json__state_array_after_comma)
			{
				return json__error(obj, i, (os_utf8*)"unexpeced ]");
			}
			if (json__pop(ret, i, &ret))
			{
				return ret;
			}
			if (ret->depth < od)
			{
				ret->state = json__state_end;
			}
			break;
		case ',':
			if (ret->state == json__state_array_after_member)
			{
				ret->state = json__state_array_after_comma;
			}
			else if (ret->state == json__state_object_after_member)
			{
				ret->state = json__state_object_after_comma;
			}
			else
			{
				return json__error(obj, i, (os_utf8*)" unexpected , ");
			}
			break;
		case ':':
			if (ret->state != json__state_object_after_name)
			{
				return json__error(obj, i, (os_utf8*)" unexpected : ");
			}
			ret->state = json__state_object_after_colon;
			break;
		case '"':
			i++;
			p++;
			if (ret->state == json__state_object_after_colon)
			{
				json__prepare_member(ret, json__string, name_start, name_end);
			}
			name_start = i;
			while (*p != '"' && i < end)
			{
				if (*p == '\\')
				{
					if (*(p + 1) == '"' || *(p + 1) == '\\')
					{
						p++;
						i++;
					}
				}
				p++;
				i++;
			}
			if (i > obj->value_end)
			{
				return json__error(obj, name_start, (os_utf8*)"unterminated quote");
			}
			name_end = i;
			if (ret->state == json__state_object_after_colon)
			{
				json__add_member(ret, json__string, name_start, name_end);
				ret->state = json__state_object_after_member;
			}
			else if (ret->state == json__state_object_after_comma)
			{
				ret->state = json__state_object_after_name;
			}
			else if (ret->state == json__state_array_body || ret->state == json__state_array_after_comma)
			{
				json__add_member(ret, json__string, name_start, name_end);
				ret->state = json__state_array_after_member;
			}
			else if (ret->state == json__state_object_body)
			{
				ret->state = json__state_object_after_name;
			}
			else
			{
				return json__error(obj, i, (os_utf8*)" unexpected string ");
			}
			break;
		case '\n':
		case ' ':
		case '\t':
		case '\r':
			break;
		case 'n':
			if (ret->state == json__state_object_after_colon)
			{
				json__prepare_member(ret, json__object_null, name_start, name_end);
			}
			if (p[1] == 'u' && p[2] == 'l' && p[3] == 'l')
			{
				json__add_member(ret, json__object_null, i, i + 4);
				p += 3;
				i += 3;
			}
			else
			{
				return json__error(obj, i, (os_utf8*)" unexpected keyword ");
			}
			break;
		case 't':
			if (ret->state == json__state_object_after_colon)
			{
				json__prepare_member(ret, json__boolean_true, name_start, name_end);
			}

			if (p[1] == 'r' && p[2] == 'u' && p[3] == 'e')
			{
				json__add_member(ret, json__boolean_true, i, i + 4);
				p += 3;
				i += 3;
			}
			else
			{
				return json__error(obj, i, (os_utf8*)" unexpected keyword ");
			}
			break;
		case 'f':
			if (ret->state == json__state_object_after_colon)
			{
				json__prepare_member(ret, json__boolean_false, name_start, name_end);
			}
			if (p[1] == 'a' && p[2] == 'l' && p[3] == 's' && p[4] == 'e')
			{
				json__add_member(ret, json__boolean_false, i, i + 5);
				p += 4;
				i += 4;
			}
			else
			{
				return json__error(obj, i, (os_utf8*)" unexpected keyword ");
			}
			break;
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (ret->state == json__state_object_after_colon)
			{
				json__prepare_member(ret, json__number, name_start, name_end);
			}
			name_start = i;
			if (*p == '-')
			{
				i++;
				p++;
			}
			while ((*p >= '0' && *p <= '9'))
			{
				i++;
				p++;
			}
			if (*p == '.')
			{
				i++;
				p++;
			}
			while ((*p >= '0' && *p <= '9'))
			{
				i++;
				p++;
			}
			if (*p == 'e' || *p == 'E')
			{
				i++;
				p++;
			}
			if (*p == '-' || *p == '+')
			{
				i++;
				p++;
			}
			while ((*p >= '0' && *p <= '9'))
			{
				i++;
				p++;
			}
			if (i == name_start + 1 && *(p - 1) == '-')
			{
				return json__error(obj, i, (os_utf8*)" unexpected - ");
			}
			json__add_member(ret, json__number, name_start, i);
			i--;
			p--;
			break;
		default:
			return json__error(obj, i, (os_utf8*)" unexpected character ");
		}
		
		if (ret->depth == od && ret->count == 1)
		{
			return &ret->tree->stack[ret->depth + 1];
		}
		if (ret->depth < od)
		{
			return 0;
		}
		if (ret->state == json__state_end)
		{
			return 0;
		}
		p++;
	}

	if (ret->depth == od && ret->count == 1)
	{
		return &ret->tree->stack[ret->depth + 1];
	}
	return 0;
}

/** 
 * gets the next memmber after child in obj
 * \param obj the current parsed object
 * \param child the current member object or NULL 
 * \returns the found member or NULL
 */
struct json__object *json__get_next(struct json__object *obj, struct json__object *child)
{
	struct json__object *ret;
	if (child)
	{
	}
	else if (obj->type == json__object_array)
	{
		obj->state = json__state_array_body;
	}
	else if (obj->type == json__object)
	{
		obj->state = json__state_object_body;
	}
	else
	{
		obj->state = json__state_init;
	}
	obj->count = 0;
	ret = json__get(obj, child);
	return ret;
}

/**
 * parse the full buffer
 * \param buf buffer
 * \param len # of byte in buffer
 * \param tree context for the parsing
 * \returns the JSON root object
 */
struct json__object *json__parse(os_utf8 *buf, os_size len, struct json__tree *tree)
{
	struct json__object *obj;
	tree->buf = buf;
	tree->len = len;
	obj = tree->stack;
	obj->tree = tree;
	obj->value_end = len;
	obj->value_start = 0;
	obj->depth = 0;
	obj->type = json__undefined;
	obj->state = json__state_init;
	obj->name_start = 0;
	obj->name_end = 0;
	obj->child_type = json__undefined;
	obj->count = 0;
	return json__get(obj, 0);
}

/**
 * call cb for each obj member or of obj itself 
 * if it is not a JSON object or JSON array
 * \param obj the current object
 * \param cb the callback
 * \param ctx data pointer for the callback
 * \returns the value returned by the callback (0 if no error)
 */
os_result json__visitor(struct json__object *obj,
						os_result (*cb)(struct json__object *obj, void *ctx), void *ctx)
{
	os_result r;
	struct json__object *child;

	if (obj->type != json__object && obj->type != json__object_array)
	{
		return cb(obj, ctx);
	}

	child = json__get_next(obj, 0);
	while (child)
	{
		r = json__visitor(child, cb, ctx);
		if (r)
		{
			return r;
		}
		child = json__get_next(obj, child);
	}
	return cb(obj, ctx);
}

/**
 * write JSON to file
 * \param fp fopen() file context
 * \param obj the root objext
 * \param len length of the temporary buffer
 * \param buf temporary buffer for object names
 */
void json__dump(FILE *fp, struct json__object *obj, os_size len, os_utf8 *buf)
{
	os_intn i;

	struct json__object *child = obj;

	if (obj->name_end > 0)
	{
		for (i = 0; i < obj->depth; i++)
		{
			fprintf(fp, " ");
		}
		if (json__copy_string(child, child->name_start, child->name_end, len, buf))
		{
			fprintf(stderr, "buffer too small1\n");
			exit(-10);
		}
		fprintf(fp, "\"%s\":", buf);
	}
	if (child->type == json__string ||
		child->type == json__object_null ||
		child->type == json__boolean_true ||
		child->type == json__boolean_false ||
		child->type == json__number)
	{
		if (obj->name_end <= 0)
		{
			for (i = 0; i < obj->depth; i++)
			{
				fprintf(fp, " ");
			}
		}
		if (json__copy_string(child, child->value_start, child->value_end, len, buf))
		{
			fprintf(stderr, "buffer too small2\n");
			exit(-10);
		}
		if (child->type == json__string)
		{
			fprintf(fp, "\"%s\"", buf);
		}
		else
		{
			fprintf(fp, "%s", buf);
		}
		return;
	}
	if (obj->name_end > 0)
	{
		fprintf(fp, "\n");
	}
	for (i = 0; i < obj->depth; i++)
	{
		fprintf(fp, " ");
	}
	if (obj->type != json__object && obj->type != json__object_array)
	{
		fprintf(stderr, "unknow member type\n");
		exit(-10);
	}

	if (obj->type == json__object)
	{
		fprintf(fp, "{\n");
	}
	else if (obj->type == json__object_array)
	{
		fprintf(fp, "[\n");
	}

	child = json__get_next(obj, 0);
	while (child)
	{
		json__dump(fp, child, len, buf);
		child = json__get_next(obj, child);
		if (child)
		{
			fprintf(fp, ",\n");
		}
	}
	fprintf(fp, "\n");
	for (i = 0; i < obj->depth; i++)
	{
		fprintf(fp, " ");
	}
	if (obj->type == json__object)
	{
		fprintf(fp, "}");
	}
	else if (obj->type == json__object_array)
	{
		fprintf(fp, "]");
	}
}

/**
 * load a full file in RAM
 * \param path file name
 * \param size return size of the file
 * \returns an alloced buffer (use free() to release it) containing the contenet of the file
 */
os_utf8 *json__load_file(os_utf8 *path, os_size *size)
{
	FILE *fp;
	os_utf8 *buf;
	os_size ret;

	fp = fopen((char *)path, "rb");
	if (!fp)
	{
		fprintf(stderr, "ERROR: cannot open %s\n", path);
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	*size = (os_size)ftell(fp);
	if (*size < 1)
	{
		fprintf(stderr, "ERROR: %s is empty\n", path);
		return 0;
	}
	fseek(fp, 0, SEEK_SET);
	buf = malloc(*size + 1);
	ret = fread(buf, 1, *size, fp);
	if (ret != *size)
	{
		fprintf(stderr, "ERROR: read fail on %s\n", path);
		return 0;
	}
	fclose(fp);
	buf[*size] = '\0';
	return buf;
}

/**
 * save buffer in file
 * \param path name of the file
 * \param size length of buf
 * \param buf buffer
 * \returns 0 on success
 */
os_result json__save_file(os_utf8 *path, os_size size, os_utf8 *buf)
{
	FILE *fp;
	os_size ret;
	fp = fopen((char *)path, "wb");
	if (!fp)
	{
		fprintf(stderr, "ERROR: cannot open %s\n", path);
		return -1;
	}

	ret = fwrite(buf, 1, size, fp);
	if (ret != size)
	{
		fprintf(stderr, "ERROR: write fail on %s\n", path);
		return -1;
	}
	fclose(fp);
	return 0;
}


/**
 * max size of a cmd 
 */
#define bld__MAX_PATH 250
/**
 * max amout of recusive dependent tasks
 */
#define bld__MAX_CHILDREN 60
/**
 * max size of a task label
 */
#define bld__MAX_LABEL 60

/**
 * build task
 */
struct bld__task
{
	/** unique id of the task */
	os_size id;

	os_utf8 label[bld__MAX_LABEL];
	os_utf8 command[bld__MAX_PATH];
	os_utf8 cwd[bld__MAX_PATH];
	os_utf8 depends_on[bld__MAX_CHILDREN + 1][bld__MAX_LABEL];
	os_utf8 args[bld__MAX_CHILDREN + 1][bld__MAX_PATH];

	/** set to 1 if we have "windows" specific args in the task */
	os_bool platform_args;

	/** previous task */
	struct bld__task *previous;
	/** next task */
	struct bld__task *next;

	/** set to 1 if task has already been generated */
	os_intn done;
};

/**
 * allocate a task struct
 * \param id unique id of the task
 * \returns the task
 */
struct bld__task *bld__task_new(os_size id)
{
	struct bld__task *r;
	r = malloc(sizeof(struct bld__task));
	if (!r)
	{
		fprintf(stderr, "out of memory\n");
		exit(-12);
	}
	memset(r, 0, sizeof(struct bld__task));
	r->id = id;
	return r;
}

/**
 * add task arg
 * \param obj the current parsed object
 * \param t the current task
 * \param platform 1 if we are in "windows":{"args":[]}
 * \returns 0 if an arg has been added
 */
os_result bld__args(struct json__object *obj, struct bld__task *t, os_bool platform)
{
	os_utf8 *s;
	os_intn i;
	
	if (t->platform_args && !platform)
	{
		/*we already have args for "windows"*/
		return 1;
	}
	if (!t->platform_args && platform && t->args[0])
	{
		/* clear generic args because there is special args for "windows" */
		for (i = 0; i < bld__MAX_CHILDREN; i++)
		{
			t->args[i][0] = 0;
		}
	}
	t->platform_args = platform;
	for (i = 0; i < bld__MAX_CHILDREN; i++)
	{
		s = t->args[i];
		if (!s[0])
		{
			break;
		}
	}
	if (i >= bld__MAX_CHILDREN)
	{
		fprintf(stderr, "too many args\n");
		exit(-10);
	}
	json__copy_string(obj, obj->value_start, obj->value_end, bld__MAX_PATH, s);
	return 0;
}


/**
 * callback to populate task values
 * \param obj the current parsed object
 * \param ctx pointer to root bld__task
 * \returns 0 
 */
os_result bld__gather_tasks(struct json__object *obj, void *ctx)
{
	struct bld__task *t = (struct bld__task *)ctx;
	struct json__object *stack = obj->tree->stack;
	os_utf8 *s;
	os_intn i;

	if (obj->depth < 3)
	{
		return 0;
	}
	if (json__match(&stack[2], (os_utf8*)"tasks"))
	{
		if (obj->depth > 3 && json__match(&stack[3], (os_utf8*)""))
		{
			if (t->id == 0)
			{
				t->id = stack[3].value_start;
			}
			else
			{
				while (t->next && t->id != stack[3].value_start)
				{
					t = t->next;
				}
				if (!t->next && t->id != stack[3].value_start)
				{
					t->next = bld__task_new(stack[3].value_start);
					t->next->previous = t->next;
					t = t->next;
				}
				else
				{
					t->id = stack[3].value_start;
				}
			}
			if (json__match(&stack[4], (os_utf8*)"command") && t->command[0] == 0)
			{
				json__copy_string(obj, obj->value_start, obj->value_end, sizeof(t->command), t->command);
			}
#ifdef _WIN32
			else if (obj->depth > 4 && json__match(&stack[4], (os_utf8*)"windows"))
			{
				if (json__match(&stack[5], (os_utf8*)"command"))
				{
					json__copy_string(obj, obj->value_start, obj->value_end, sizeof(t->command), t->command);
				} else if (obj->depth > 5 && json__match(&stack[5], (os_utf8*)"args")) {
					bld__args(obj, t, 1);
				}
			}
#elif defined(__APPLE__) && defined(__MACH__)
			else if (obj->depth > 4 && json__match(&stack[4], (os_utf8*)"osx"))
			{
				if (json__match(&stack[5], (os_utf8*)"command"))
				{
					json__copy_string(obj, obj->value_start, obj->value_end, sizeof(t->command), t->command);
				} else if (obj->depth > 5 && json__match(&stack[5], (os_utf8*)"args")) {
					bld__args(obj, t, 1);
				}
			}

#endif
			else if (json__match(&stack[4], (os_utf8*)"label"))
			{
				json__copy_string(obj, obj->value_start, obj->value_end, 1024, t->label);
			}
			else if (obj->depth > 4 && json__match(&stack[4], (os_utf8*)"args"))
			{
				bld__args(obj, t, 0);
			}
			else if (obj->depth > 4 && json__match(&stack[4], (os_utf8*)"dependsOn"))
			{
				for (i = 0; i < bld__MAX_CHILDREN; i++)
				{
					s = t->depends_on[i];
					if (!s[0])
					{
						break;
					}
				}
				if (i >= bld__MAX_CHILDREN)
				{
					fprintf(stderr, "too many dependsOn\n");
					exit(-10);
				}
				json__copy_string(obj, obj->value_start, obj->value_end, bld__MAX_PATH, s);
			}
			else if (obj->depth > 4 && json__match(&stack[4], (os_utf8*)"options") &&
					 json__match(&stack[5], (os_utf8*)"cwd"))
			{
				json__copy_string(obj, obj->value_start, obj->value_end, sizeof(t->cwd), t->cwd);
			}
		}
	}
	return 0;
}

#ifndef _WIN32
#define P_WAIT 1
extern char **environ;
/**
 * Execute command and wait until it terminates.
 */
int _spawnvp(int flags, const char *cmd, const char *const *args)
{
	pid_t pid;
	int status = 0;
	pid = fork();
	if (!pid) {
		return execve(cmd, (char *const*)args, environ);
	} else if (flags & P_WAIT) {
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)) {
			return WEXITSTATUS(status);
		}
		status = -1;
	}
	return status;
}

#endif

/**
 * exiecute vscode task.
 * \param t the task
 * \param workspace path to the root of the project
 * \returns 0 if success
 */
os_result bld__exec(struct bld__task *t, os_utf8 *workspace)
{
	os_utf8 *args[bld__MAX_CHILDREN + 5];
	os_utf8 cwd[1024];
	os_utf8 old[1024];
	os_intn n;
	os_intn i;
	if (t->done)
	{
		return 0;
	}
	getcwd((char*)old, 1024);
	if (!strncmp("${workspaceFolder}", (char *)t->cwd, 18))
	{
		snprintf((char*)cwd, 1020, "%s%s", workspace, t->cwd + 18);
	}
	else
	{
		snprintf((char*)cwd, 1020, "%s", t->cwd);
	}

	if (cwd[0])
	{
		chdir((char*)cwd);
	}
	getcwd((char*)cwd, 1024);
#ifdef _WIN32
	args[0] = (os_utf8 *)"cmd";
	args[1] = (os_utf8 *)"/c";
	args[2] = t->command;
	n = 3;
#else
	args[0] = (os_utf8 *)"/usr/bin/env";
	args[1] = (os_utf8 *)"-S";
	args[2] = t->command;
	n = 3;
#endif
	for (i = 0; i < bld__MAX_CHILDREN; i++)
	{
		if (t->args[i][0])
		{
			args[i + n] = t->args[i];
		}
		else
		{
			args[i + n] = 0;
		}
	}
	args[i + n] = 0;

	t->done = 1;
	printf("Execute task: \"%s\"\n", t->label);
	if (_spawnvp(P_WAIT, (const char *)args[0],
				 (const char *const *)args))
	{
		chdir((char*)old);
		fprintf(stderr, "in task '%s' command '%s' failed\n", t->label, t->command);
		return -1;
	}
	chdir((char*)old);
	return 0;
}

/**
 * dependence tree 
 */
struct bld__dep_tree
{
	struct bld__task *current;
	struct bld__dep_tree *parent;
};

/**
 * runs the dependent task and then run the current task
 * \param root task
 * \param t current task
 * \param p tree to check that we are not in infinite recursion
 * \param workspace path of the project's root dirictory
 * \returns 0 if success
 */
os_result bld__depends(struct bld__task *root, struct bld__task *t,
					   struct bld__dep_tree *p, os_utf8 *workspace)
{
	os_intn i;
	os_utf8 *s;
	struct bld__dep_tree *tr;
	struct bld__task *c;
	struct bld__dep_tree r;
	os_result ret;
	tr = p;
	while (tr)
	{
		if (p->current == t)
		{
			return 1;
		}
		tr = tr->parent;
	}
	r.current = t;
	r.parent = p;
	for (i = 0; i < bld__MAX_CHILDREN; i++)
	{
		s = t->depends_on[i];
		if (s[0])
		{
			c = root;
			while (c)
			{
				if (!strcmp((char *)s, (char *)c->label))
				{
					ret = bld__depends(root, c, &r, workspace);
					if (ret)
					{
						return ret;
					}
				}
				c = c->next;
			}
		}
		else
		{
			break;
		}
	}
	return bld__exec(t, workspace);
}

/**
 * run a task
 * \param root root task
 * \param workspace path of the project's root directory
 * \param name label of the task to run
 * \returns 0 if success
 */
os_result bld__make(struct bld__task *root, os_utf8 *workspace, os_utf8 *name)
{
	struct bld__task *t;
	t = root;
	while (t && strcmp((char *)t->label, (char *)name))
	{
		t = t->next;
	}
	if (t)
	{
		return bld__depends(root, t, 0, workspace);
	}
	else
	{
		fprintf(stderr, "Task \"%s\" not found", name);
	}
	return -1;
}

/**
 * free all the tasks
 */
void bld__free(struct bld__task *t) {
	if (!t) {
		return;
	}
	bld__free(t->next);
	free(t);
}

/**
 * main entry point
 */
int main(int argc, char *argv[])
{
	os_utf8 *json;
	os_size size;
	struct json__tree tree;
	struct json__object *obj;
	os_utf8 path[1024];
	struct bld__task *t;

	if (argc != 3)
	{
		fprintf(stderr, "usage: %s <workingfolder> <task>\n", argv[0]);
		exit(-1);
	}
	snprintf((char *)path, sizeof(path) - 1, "%s/.vscode/tasks.json", argv[1]);
	json = json__load_file(path, &size);
	if (!json)
	{
		exit(-1);
	}

	obj = json__parse(json, size, &tree);

	if (!obj)
	{
		fprintf(stderr, "ERROR: %s is invalid\n", path);
		exit(-2);
	}
	if (obj->type == json__undefined)
	{
		printf("cannot find root object\n");
	}

	/*
	json__dump(stdout, obj, 512, buf);
	*/
	t = bld__task_new(0); 
	json__visitor(obj, &bld__gather_tasks, t);
	bld__make(t, (os_utf8 *)argv[1], (os_utf8 *)argv[2]);
	bld__free(t);
	free(json);
	return 0;
}
