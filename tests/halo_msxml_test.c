/* MSXML 4.0 test (G3): the translated msxml4.dll through its COM interfaces, as Keystone.dll uses
 * it for the chat UI. A DOMDocument40 parses a string, loads a file, and validates a KSML
 * document against Halo's content schema (KSML.xsd), as Keystone wraps each .ksml file. Parse
 * errors are printed with msxml's reason text.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_msxml_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
void halopad_protect_image(uint32_t image_base);
void halopad_modules_init(void);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static uint32_t wstr(const char *s) { uint32_t n = (uint32_t)strlen(s), g = halopad_heap_alloc(2 * n + 2, 1); for (uint32_t i = 0; i < n; i++) ((uint16_t *)halopad_guest_ptr(g))[i] = (uint8_t)s[i]; return g; }
static void narrow(uint32_t w, char *out, int n)
{
    int i = 0;
    if (w) for (; i < n - 1; i++) { uint16_t c = ((uint16_t *)halopad_guest_ptr(w))[i]; if (!c) break; out[i] = c < 0x80 ? (char)c : '?'; }
    out[i] = 0;
}
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-66s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static void checks(const char *what, const char *got, const char *want)
{
    int ok = !strcmp(got, want);
    printf("%-66s %s (got \"%s\", want \"%s\")\n", what, ok ? "PASS" : "FAIL", got, want);
    failures += !ok;
}
static uint32_t method(uint32_t obj, uint32_t index, uint32_t n, const uint32_t *args)
{
    uint32_t a[12] = {obj};
    memcpy(a + 1, args, 4 * n);
    return halopad_call_guest(rd(rd(obj) + 4 * index), n + 1, a);
}
#define M(obj, index, ...) method(obj, index, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
static uint32_t oleaut, ole;
static uint32_t call(uint32_t mod, const char *name, uint32_t n, const uint32_t *args)
{
    uint32_t va = GetProcAddress_c(mod, str(name));
    if (!va) { printf("no %s\n", name); exit(2); }
    return halopad_call_guest(va, n, args);
}
#define C(mod, name, ...) call(mod, name, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
static uint32_t bstr(const char *s) { return C(oleaut, "SysAllocString", wstr(s)); }
static uint32_t guid(const char *text)          /* "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" */
{
    uint32_t g = halopad_heap_alloc(16, 0);
    uint8_t *b = halopad_guest_ptr(g);
    unsigned d1, d2, d3, x[8];
    sscanf(text, "{%8x-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x}", &d1, &d2, &d3, &x[0], &x[1], &x[2], &x[3], &x[4], &x[5], &x[6], &x[7]);
    memcpy(b, &d1, 4); b[4] = (uint8_t)d2; b[5] = (uint8_t)(d2 >> 8); b[6] = (uint8_t)d3; b[7] = (uint8_t)(d3 >> 8);
    for (int i = 0; i < 8; i++) b[8 + i] = (uint8_t)x[i];
    return g;
}

/* IXMLDOMDocument methods (msxml2.h vtable order) */
enum { D_documentElement = 45, D_load = 58, D_parseError = 60, D_put_async = 63, D_loadXML = 65, D_put_validateOnParse = 68,
       N_nodeName = 7, N_childNodes = 12, N_attributes = 17, N_xml = 34, NM_getNamedItem = 8, N_text = 26,
       E_errorCode = 7, E_reason = 9, E_line = 11, E_linepos = 12, E_srcText = 10 };

static void report(uint32_t doc)
{
    uint32_t pe = halopad_heap_alloc(4, 1), v = halopad_heap_alloc(4, 1);
    if (M(doc, D_parseError, pe) || !rd(pe)) { printf("    (no parse error object)\n"); return; }
    uint32_t e = rd(pe), code, line, pos;
    M(e, E_errorCode, v); code = rd(v);
    M(e, E_line, v); line = rd(v);
    M(e, E_linepos, v); pos = rd(v);
    M(e, E_reason, v);
    char reason[400], src[200];
    narrow(rd(v), reason, sizeof reason);
    M(e, E_srcText, v);
    narrow(rd(v), src, sizeof src);
    printf("    parse error 0x%08x at line %u, column %u: %s    source: %s\n", code, line, pos, reason, src);
}

static uint32_t load_string(uint32_t doc, const char *xml)
{
    uint32_t ok = halopad_heap_alloc(4, 1);
    uint32_t hr = M(doc, D_loadXML, bstr(xml), ok);
    if (hr || (rd(ok) & 0xFFFF) != 0xFFFF) report(doc);
    return hr ? hr : rd(ok) & 0xFFFF;
}

static uint32_t read_file(const char *path, char *out, size_t n)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    size_t k = fread(out, 1, n - 1, f);
    fclose(f);
    out[k] = 0;
    return (uint32_t)k;
}

int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE"), *game = getenv("HALOPAD_GAME_ROOT");
    if (!image || !game) return 2;
    setvbuf(stdout, NULL, _IONBF, 0);
    halopad_guest_harness_heap = 0;
    uint32_t top = halopad_guest_init(image, 0x400000);
    halopad_thread_init(0x00300000, 0x00100000, 0x400000);
    halopad_vm_mark(0x00100000, 0x00200000);
    halopad_vm_mark(0x400000, 0x42C000);
    halopad_vm_mark(0x7FFD0000, 0x00030000);
    halopad_protect_image(0x400000);
    halopad_modules_init();
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;
    uint32_t crt_arg = 1;
    halopad_call_guest_ex(0x5d6ba6, 1, &crt_arg, 0, 0);     /* Halo's CRT heap and thread data, as the UI test */
    halopad_call_guest_ex(0x5cf966, 0, NULL, 0, 0);
    ole = LoadLibraryA_c(str("ole32.dll"));
    oleaut = LoadLibraryA_c(str("oleaut32.dll"));
    check("CoInitialize", C(ole, "CoInitialize", 0), 0);

    uint32_t pdoc = halopad_heap_alloc(4, 1);
    check("CoCreateInstance(DOMDocument40, IXMLDOMDocument2)",
          C(ole, "CoCreateInstance", guid("{88D969C0-F192-11D4-A65F-0040963251E5}"), 0, 1, guid("{2933BF95-7B36-11D2-B20E-00C04F983E60}"), pdoc), 0);
    uint32_t doc = rd(pdoc);
    if (!doc) return 1;
    check("put_async(false)", M(doc, D_put_async, 0), 0);
    printf("  (DOMDocument vtable: loadXML at 0x%08x, load at 0x%08x)\n", rd(rd(doc) + 4 * D_loadXML), rd(rd(doc) + 4 * D_load));

    check("loadXML(<a><b x='1'>t</b></a>)", load_string(doc, "<a><b x='1'>t</b></a>"), 0xFFFF);
    uint32_t pel = halopad_heap_alloc(4, 1), v = halopad_heap_alloc(4, 1);
    char s[200];
    M(doc, D_documentElement, pel);
    if (rd(pel)) {
        M(rd(pel), N_nodeName, v); narrow(rd(v), s, sizeof s);
        checks("  documentElement.nodeName", s, "a");
        M(rd(pel), N_text, v); narrow(rd(v), s, sizeof s);
        checks("  documentElement.text", s, "t");
        M(rd(pel), N_xml, v); narrow(rd(v), s, sizeof s);
        checks("  documentElement.xml", s, "<a><b x=\"1\">t</b></a>");
    } else check("  documentElement", 0, 1);
    check("loadXML(<a><b></a>) fails with S_FALSE", load_string(doc, "<a><b></a>"), 1);

    /* a KSML document as Keystone.dll wraps it: the schema from the install directory */
    char path[1200], raw[8192], body[8192], xml[16384];
    snprintf(path, sizeof path, "%s/content/480editbox.ksml", game);
    uint32_t n = read_file(path, raw, sizeof raw);
    check("read content/480editbox.ksml", n > 0, 1);
    /* UTF-16LE with a byte-order mark: keep the ASCII text from <body> on */
    size_t k = 0;
    for (uint32_t i = 2; i + 1 < n && k < sizeof body - 1; i += 2) body[k++] = raw[i + 1] ? '?' : raw[i];
    body[k] = 0;
    char *b = strstr(body, "<body>");
    char *end = strstr(body, "</ksml>");
    if (end) *end = 0;
    snprintf(xml, sizeof xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<ksml xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
             "xsi:noNamespaceSchemaLocation=\"C:\\Program Files\\Microsoft Games\\Halo Custom Edition\\ksml.xsd\"><head/>%s</ksml>", b ? b : "");
    check("loadXML(480editbox.ksml, validated against KSML.xsd)", load_string(doc, xml), 0xFFFF);

    /* As Keystone.dll's 0x10245890 does it: a schema cache holding KSML.xsd for no namespace, the
       document validating against it with external resolution off, and loadXML given the file's
       UTF-16 text in a plain heap buffer, its byte-order mark skipped as Keystone does (0x1021e32a) */
    uint32_t pcache = halopad_heap_alloc(4, 1);
    check("CoCreateInstance(XMLSchemaCache40, IXMLDOMSchemaCollection)",
          C(ole, "CoCreateInstance", guid("{88D969C2-F192-11D4-A65F-0040963251E5}"), 0, 1, guid("{373984C8-B845-449B-91E7-45AC83036ADE}"), pcache), 0);
    uint32_t cache = rd(pcache);
    if (cache) {
        uint32_t xsd = bstr("C:\\Program Files\\Microsoft Games\\Halo Custom Edition\\ksml.xsd");
        uint32_t hr = M(cache, 7, bstr(""), 8, 0, xsd, 0);
        check("  add(\"\", KSML.xsd)", hr, 0);
        if (hr) {
            uint32_t pinfo = halopad_heap_alloc(4, 1);
            if (!C(oleaut, "GetErrorInfo", 0, pinfo) && rd(pinfo)) {
                M(rd(pinfo), 5, v); narrow(rd(v), s, sizeof s);           /* IErrorInfo::GetDescription */
                printf("    error: %s\n", s);
            }
        }
        uint32_t pdoc2 = halopad_heap_alloc(4, 1);
        C(ole, "CoCreateInstance", guid("{88D969C0-F192-11D4-A65F-0040963251E5}"), 0, 1, guid("{2933BF95-7B36-11D2-B20E-00C04F983E60}"), pdoc2);
        uint32_t d2 = rd(pdoc2);
        M(d2, D_put_async, 0);
        check("  putref_schemas(cache)", M(d2, 78, 9, 0, cache, 0), 0);
        check("  put_validateOnParse(true)", M(d2, D_put_validateOnParse, 0xFFFF), 0);
        check("  put_resolveExternals(false)", M(d2, 70, 0), 0);
        uint32_t skip = n >= 2 && (uint8_t)raw[0] == 0xFF && (uint8_t)raw[1] == 0xFE ? 2 : 0;
        uint32_t text = halopad_heap_alloc(n + 2, 1);
        memcpy(halopad_guest_ptr(text), raw + skip, n - skip);
        uint32_t okp = halopad_heap_alloc(4, 1);
        uint32_t lhr = M(d2, D_loadXML, text, okp);
        if (lhr || (rd(okp) & 0xFFFF) != 0xFFFF) report(d2);
        check("  loadXML(480editbox.ksml as read)", lhr ? lhr : rd(okp) & 0xFFFF, 0xFFFF);
        /* the schema is enforced: an element KSML.xsd does not define is rejected */
        lhr = M(d2, D_loadXML, bstr("<ksml><head/><body><nosuch/></body></ksml>"), okp);
        check("  loadXML(<ksml> with an element KSML.xsd lacks) fails validation", lhr, 1);
        report(d2);
    }
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}
