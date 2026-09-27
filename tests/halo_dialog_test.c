/* Dialog test (G3): Halo's own warning and error dialog on HaloPad's dialog manager.
 *
 * Halo reports startup problems with 0x582060(text id, link id, fatal): string resources from
 * strings.dll, dialog template 0x66 ("Halo - Warning") shown with DialogBoxIndirectParamA
 * through 0x5817e0, and its dialog procedure 0x581b90, which centres the dialog, fills it in,
 * subclasses the dialog and the "more information" link (0x581ab0: hyperlink colour, underlined
 * font), and ends it with the player's button. This test is the host (halopad_host_dialog): it
 * records what the player would see and answers as a player would.
 *
 *  1. The Ctrl-key warning (0x87, link 0x7e, not fatal): the player follows the link, ticks
 *     "Don't show this warning again" and continues. The link opens string 0x7e's URL; Halo
 *     records the choice under HKCU and returns.
 *  2. The same warning again: Halo does not show it (its own registry check).
 *  3. The product-key error as Halo's start-up raises it (0xa0, link 0x7e, fatal), through
 *     0x5817e0 directly because 0x582060 exits the process after a fatal one: only Exit is
 *     enabled, and Exit ends the dialog with 2.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_dialog_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"
#include "../port/runtime/halopad_dialog.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
void halopad_protect_image(uint32_t image_base);
void halopad_modules_init(void);
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t LoadStringA_c(uint32_t instance, uint32_t id, uint32_t buf, uint32_t size);
extern int (*halopad_shell_open_hook)(const char *url);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static void wr(uint32_t g, uint32_t v) { memcpy(halopad_guest_ptr(g), &v, 4); }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static void check(const char *what, uint32_t got, uint32_t want)
{
    fprintf(stderr, "HALOPAD TEST: %s\n", what);
    printf("%-72s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static void check_text(const char *what, const char *got, const char *want)
{
    int ok = got && !strcmp(got, want);
    fprintf(stderr, "HALOPAD TEST: %s\n", what);
    printf("%-72s %s (\"%s\")\n", what, ok ? "PASS" : "FAIL", got ? got : "(none)");
    if (!ok) printf("    want \"%s\"\n", want);
    failures += !ok;
}

/* ---- the host: records each showing and answers from a list of control ids ---- */
static halopad_dialog_view shown[16];
static int nshown;
static const int32_t *answers;
int halopad_host_dialog(const halopad_dialog_view *v)
{
    if (nshown < 16) shown[nshown] = *v;
    nshown++;
    int32_t id = *answers++;
    if (id == HPD_CLOSE) return HPD_CLOSE;
    for (int i = 0; i < v->count; i++) if ((int32_t)v->item[i].id == id) return i;
    printf("    host: no control %d in the dialog\n", id);
    return HPD_CLOSE;
}
static char opened[1024];
static int open_count;
static int shell_open(const char *url) { snprintf(opened, sizeof opened, "%s", url); open_count++; return 1; }

static const halopad_dialog_item *item(const halopad_dialog_view *v, uint32_t id)
{
    for (int i = 0; i < v->count; i++) if (v->item[i].id == id) return &v->item[i];
    return NULL;
}
static char *resource_string(uint32_t module, uint32_t id)
{
    static char out[4][1024];
    static int k;
    uint32_t buf = halopad_heap_alloc(1024, 1);
    LoadStringA_c(module, id, buf, 1024);
    k = (k + 1) & 3;
    snprintf(out[k], sizeof out[k], "%s", (const char *)halopad_guest_ptr(buf));
    return out[k];
}

enum { G_INSTANCE = 0x6e1480, G_STRINGS = 0x6bde88, G_FATAL = 0x68b3ac, G_TEXT = 0x68afa8, G_TITLE = 0x68b3b0, G_LINK = 0x68b430, G_MAINWND = 0x6e1484 };

int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) return 2;
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
    check("CRT _heap_init (0x5d6ba6)", halopad_call_guest_ex(0x5d6ba6, 1, &crt_arg, 0, 0) != 0, 1);
    check("CRT _mtinit (0x5cf966)", halopad_call_guest_ex(0x5cf966, 0, NULL, 0, 0) != 0, 1);
    halopad_call_guest_ex(0x5d3ac3, 0, NULL, 0, 0);
    check("CRT _ioinit (0x5cf3f2)", (int32_t)halopad_call_guest_ex(0x5cf3f2, 0, NULL, 0, 0) >= 0, 1);

    uint32_t strings = LoadLibraryA_c(str("strings.dll"));
    check("LoadLibraryA(strings.dll), as Halo's 0x582590", strings, 0x3F800000);
    wr(G_STRINGS, strings);
    wr(G_INSTANCE, 0x400000);                                     /* WinMain's hInstance */
    wr(0x6bde7c, 2400);                                           /* what start-up measures: CPU MHz (0x580e70) */
    wr(0x6bde78, 512);                                            /* and memory in MB */
    halopad_shell_open_hook = shell_open;
    const char *ctrl_text = resource_string(strings, 0x87), *url = resource_string(strings, 0x7e);

    /* 1. the Ctrl-key warning: follow the link, tick the box, continue */
    static const int32_t a1[] = {1007, 1001, 1004};
    answers = a1;
    uint32_t args[3] = {0x87, 0x7e, 0};
    halopad_call_guest_ex(0x582060, 3, args, 0, 0);
    check("warning: shown once per player action (link, checkbox, Continue)", (uint32_t)nshown, 3);
    const halopad_dialog_view *v = &shown[0];
    printf("    title \"%s\", %dx%d client pixels, %d controls\n", v->title, v->w, v->h, v->count);
    check_text("  title (string 0x7f)", v->title, resource_string(strings, 0x7f));
    check("  client size from the template's dialog units (305x102 DLU)", (uint32_t)(v->w << 16 | v->h), 458u << 16 | 166u);
    const halopad_dialog_item *t = item(v, 1006);
    check_text("  problem text (string 0x87)", t ? t->text : NULL, ctrl_text);
    const halopad_dialog_item *cont = item(v, 1004), *safe = item(v, 3), *quit = item(v, 1005), *box = item(v, 1001), *link = item(v, 1007);
    check("  Continue Anyway: an enabled default button", cont && cont->kind == HPD_BUTTON && cont->enabled && cont->is_default, 1);
    check_text("  its caption", cont ? cont->text : NULL, "Continue Anyway");
    check("  Continue in 'Safe Mode' and Exit enabled", safe && quit && safe->enabled && quit->enabled, 1);
    check("  the checkbox, unticked", box && box->kind == HPD_CHECKBOX && box->enabled && !box->checked, 1);
    check("  the link: a notifying static (subclassed by Halo's 0x581ab0)", link && link->kind == HPD_LINK, 1);
    check("  in Halo's link colour, set in its WM_CTLCOLORSTATIC (0x581890)", link ? link->color : 0, 0xC00000);
    const halopad_dialog_item *info = item(v, 1009);
    check_text("  machine info, as Halo formats it (\"%dMHz, %dMB\")", info ? info->text : NULL, "2400MHz, 512MB");
    int icons = 0;
    for (int i = 0; i < v->count; i++) icons += v->item[i].kind == HPD_ICON && !strcmp(v->item[i].text, "#105");
    check("  the warning icon (#105)", (uint32_t)icons, 1);
    check_text("the link opened string 0x7e's URL through ShellExecuteA", opened, url);
    check("  once", (uint32_t)open_count, 1);
    const halopad_dialog_item *box2 = item(&shown[2], 1001);
    check("the checkbox shows ticked after the player's click", box2 && box2->checked, 1);
    check("Halo read it (0x6bde90: don't show again)", rd(0x6bde90), 1);

    /* 2. the same warning: suppressed by Halo's HKCU record */
    int before = nshown;
    halopad_call_guest_ex(0x582060, 3, args, 0, 0);
    check("the same warning again is not shown", (uint32_t)(nshown - before), 0);

    /* 3. the product-key error, fatal, as 0x582060 prepares it (0x5817e0 with esi = strings.dll,
          ebx = the dialog procedure; 0x582060 would exit the process after it) */
    const char *key_text = resource_string(strings, 0xa0);
    strcpy(halopad_guest_ptr(G_TEXT), key_text);
    strcpy(halopad_guest_ptr(G_TITLE), resource_string(strings, 0x80));
    strcpy(halopad_guest_ptr(G_LINK), url);
    wr(G_FATAL, 1);
    wr(G_MAINWND, 0);
    static const int32_t a3[] = {1004, 1005};                     /* Continue is disabled: the host asks for it, nothing happens */
    answers = a3;
    before = nshown;
    uint32_t esi0 = cpu._esi, ebx0 = cpu._ebx;
    cpu._esi = strings; cpu._ebx = 0x581b90;
    uint32_t a[2] = {0x66, 0};
    uint32_t r = halopad_call_guest_ex(0x5817e0, 2, a, 0, 0);
    cpu._esi = esi0; cpu._ebx = ebx0;
    check("fatal error: DialogBoxIndirectParamA returns Exit's 2", r, 2);
    v = &shown[before];
    check_text("  title (string 0x80)", v->title, resource_string(strings, 0x80));
    t = item(v, 1006);
    check_text("  \"Your product key is invalid...\" (string 0xa0)", t ? t->text : NULL, key_text);
    cont = item(v, 1004); safe = item(v, 3); quit = item(v, 1005); box = item(v, 1001);
    check("  Continue, Safe Mode and the checkbox disabled; Exit enabled",
          cont && safe && box && quit && !cont->enabled && !safe->enabled && !box->enabled && quit->enabled, 1);
    check("  the disabled Continue did nothing (shown again, then Exit)", (uint32_t)(nshown - before), 2);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}
