/** @file       liblisp_xml.c
 *  @brief      Interface to mxml XML parser
 *  see: http://www.msweet.org/documentation/project3/Mini-XML.html
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *  @todo 	Improve this module; XML writer, mxmlFindPath, Parse tree,
 *  Customized parsing (as integers, no attributes, ...)
 *  @todo	Test this module and try it out on the newer version
 *  @todo	Better error status need to be returned, instead of the
 *              generic gsym_error()
 *  @todo       An error callback should be made
 *  @bug        The lengths need to be calculated correctly for lists that
 *              have been made.
 **/
#include <assert.h>
#include <liblisp.h>
#include <mxml.h>

#define SUBROUTINE_XLIST\
	X("xml-parse-file",      subr_xml_parse_file,     "Z",       "parse an XML document given a file name")\
	X("xml-parse-string",    subr_xml_parse_string,   "Z",       "parse an XML document given a string")\
	X("xml-write-file",      subr_xml_write_file,     "Z c",     "write an S-Expression list as an XML document to a file")\
	X("xml-write-string",    subr_xml_write_string,   "c",       "write an S-Expression list to a string")

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING), SUBR },
static lisp_module_subroutines_t primitives[] = {
	SUBROUTINE_XLIST		/*all of the subr functions */
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};
#undef X

/*I might want to export this function*/
static lisp_cell_t *xml2lisp(lisp_t *l, mxml_node_t *node) 
{
	if(!node)
		return gsym_nil();
	switch(node->type) {
	case MXML_ELEMENT: 
	{
		mxml_node_t *t = node;
		hash_table_t *ht;
		lisp_cell_t *e, *ehead, *ename, *hash = NULL;
		int i;
		size_t j;

		ename = mk_str(l, lisp_strdup(l, node->value.element.name));

		if(t->value.element.num_attrs) {
			/* process attributes into a hash */
			if(!(ht = hash_create(16)))
				LISP_HALT(l, "\"%s\"", "out of memory");

			for(i = 0; i < t->value.element.num_attrs; i++) {
				char *key, *val;
				lisp_cell_t *keyc, *valc;
				key = lisp_strdup(l, (t->value.element.attrs)[i].name);
				val = lisp_strdup(l, (t->value.element.attrs)[i].value);
				keyc = mk_str(l, key);
				valc = mk_str(l, val);
				if(hash_insert(ht, key, cons(l, keyc, valc)) < 0)
					LISP_HALT(l, "\"%s\"", "out of memory");
			}
			hash = mk_hash(l, ht);
		}

		/*elements*/
		ehead = e = cons(l, gsym_nil(), gsym_nil());
		for(j = 1;(t = mxmlWalkNext(t, node, MXML_DESCEND));) {
			if(t->parent == node) {
				lisp_cell_t *r = xml2lisp(l, t);
				if(is_nil(r))
					continue;
				set_cdr(e, cons(l, r, gsym_nil()));
				e = cdr(e);
				j++;
			}
		}
		fix_list_len(ehead, j);

		if(hash)
			return mk_list(l, ename, hash, cdr(ehead), NULL);
		else
			return mk_list(l, ename, cdr(ehead), NULL);
	}	
	case MXML_INTEGER:
		return mk_int(l, node->value.integer);
	case MXML_REAL:
		return mk_float(l, node->value.real);
	case MXML_TEXT:
		/**@warning The parser seems to put empty strings everywhere, so
		 * we return nil to signal this, this cannot occur in the text
		 * ("" gets parsed as "\"\""), for the moment */
		if(strcmp(node->value.text.string, ""))
			return mk_str(l, lisp_strdup(l, node->value.text.string));
		return gsym_nil();
	case MXML_OPAQUE:
	case MXML_CUSTOM:
	default:
		return gsym_error();
	}
	return gsym_error();
}

static mxml_node_t *_lisp2xml(lisp_t *l, mxml_node_t *parent, lisp_cell_t *x);
static mxml_node_t *xnode(lisp_t *l, mxml_node_t *parent, lisp_cell_t *x)
{ /**@todo add hash -> xml*/
	if(is_cons(x)) {
		return _lisp2xml(l, parent, car(x));
	} else if(is_sym(x)) {
		return mxmlNewText(parent, 1, get_sym(x));
	} else if(is_str(x)) {
		return mxmlNewText(parent, 1, get_str(x));
	} else if(is_int(x)) {
		return mxmlNewInteger(parent, get_int(x));
	} else if(is_floating(x)) {
		return mxmlNewReal(parent, get_float(x));
	} else {
		return mxmlNewText(parent, 1, "CANNOT-SERIALIZE");
	}
}

/**@todo Error handling!
 * @todo handle hashes and attributes*/
static mxml_node_t *_lisp2xml(lisp_t *l, mxml_node_t *parent, lisp_cell_t *x)
{
	mxml_node_t *ret = NULL;
	if(is_cons(x)) {
		lisp_cell_t *y;
		if(is_sym(car(x))) {
			ret = mxmlNewElement(parent, get_str(car(x)));
		} else {
			ret = mxmlNewElement(parent, ""); /* valid xml? */
		}
		for(y = cdr(x); is_cons(y); y = cdr(y)) {
			if(!xnode(l, ret, y))
				return NULL;
		}
		if(!is_nil(y)) {
			if(!xnode(l, ret, y))
				return NULL;
		}
		return ret;
	} else {
		if(!(ret = xnode(l, parent, x)))
			return NULL;
		return ret;
	}
	return ret;
}

/**@todo Error handling!*/
/*I might want to export this function*/
static mxml_node_t *lisp2xml(lisp_t *l, lisp_cell_t *x)
{
	mxml_node_t *xml;
	assert(l && x);
	xml = mxmlNewXML("1.0");
	if(!_lisp2xml(l, xml, x))
		return NULL;
	return xml;
}

static lisp_cell_t *subr_xml_parse_file(lisp_t * l, lisp_cell_t *args)
{
	mxml_node_t *tree;
	FILE *input = fopen(get_str(car(args)), "rb");
	lisp_cell_t *ret;
	if(!input)
		return gsym_error();
	tree = mxmlLoadFile(NULL, input, MXML_TEXT_CALLBACK);
	if(!tree)
		return gsym_error();
	fclose(input);
	ret = xml2lisp(l, tree);
	mxmlDelete(tree);
	return ret;
}

static lisp_cell_t *subr_xml_parse_string(lisp_t * l, lisp_cell_t *args)
{
	mxml_node_t *tree;
	char *str = get_str(car(args));
	lisp_cell_t *ret;
	tree = mxmlLoadString(NULL, str, MXML_TEXT_CALLBACK);
	if(!tree)
		return gsym_error();
	ret = xml2lisp(l, tree);
	mxmlDelete(tree);
	return ret;
}

static lisp_cell_t *subr_xml_write_file(lisp_t *l, lisp_cell_t *args)
{
	mxml_node_t *tree;
	FILE *output;
	if(!(output = fopen(get_str(car(args)), "wb")))
		return gsym_error();
	if(!(tree = lisp2xml(l, car(args))))
		return gsym_error();
	if(mxmlSaveFile(tree, output, MXML_TEXT_CALLBACK) < 0)
		return gsym_error();
	fclose(output);
	mxmlDelete(tree);
	return gsym_tee();
}

static lisp_cell_t *subr_xml_write_string(lisp_t *l, lisp_cell_t *args)
{
	mxml_node_t *tree;
	char *s;
	if(!(tree = lisp2xml(l, car(args))))
		return gsym_error();
	if(!(s = mxmlSaveAllocString(tree, MXML_NO_CALLBACK)))
		return gsym_error();
	mxmlDelete(tree);
	return mk_str(l, s);
}

int lisp_module_initialize(lisp_t *l)
{
	assert(l);
	if(lisp_add_module_subroutines(l, primitives, 0) < 0)
		goto fail;
	return 0;
 fail:	
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void) { }
static void destruct(void) { }
#elif _WIN32
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNUSED(hinstDLL);
	UNUSED(lpvReserved);
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}
#endif
