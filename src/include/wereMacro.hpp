#define XSTR(x) #x
#define STR(x) XSTR(x)
#define EMBED(file) STR(EMBED_ROOT/file)