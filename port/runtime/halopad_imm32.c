/* HaloPad IMM32 (G3): the input method manager of the reference machine, English Windows XP
 * without East Asian language support, where IMM is not enabled (SM_IMMENABLED = 0). No input
 * context or IME window exists: ImmGetContext and ImmGetDefaultIMEWnd give NULL, the keyboard
 * layout is not an IME, and every call on the NULL context fails or reports no data, as on that
 * machine. ksimeui.dll (Keystone's IME support, translated) probes these at start-up; the chat
 * .ksml files also turn the IME off. A non-NULL context can only be a context HaloPad never
 * gave out, so it stops the program with its value. */
#include "halopad_win32.h"

static void no_context(const char *service, uint32_t himc)
{
    if (himc) hp_unsupported(service, "input context 0x%08x (IMM is not enabled on the reference machine)", himc);
}

uint32_t ImmGetContext_c(uint32_t hwnd) { (void)hwnd; return 0; }
uint32_t ImmReleaseContext_c(uint32_t hwnd, uint32_t himc) { (void)hwnd; no_context("ImmReleaseContext", himc); return 1; }
uint32_t ImmGetDefaultIMEWnd_c(uint32_t hwnd) { (void)hwnd; return 0; }
uint32_t ImmIsIME_c(uint32_t hkl) { (void)hkl; return 0; }
uint32_t ImmGetIMEFileNameA_c(uint32_t hkl, uint32_t buf, uint32_t len)
{
    (void)hkl;
    if (buf && len) *(char *)G(buf) = 0;
    return 0;
}
uint32_t ImmAssociateContext_c(uint32_t hwnd, uint32_t himc) { (void)hwnd; no_context("ImmAssociateContext", himc); return 0; }
uint32_t ImmGetOpenStatus_c(uint32_t himc) { no_context("ImmGetOpenStatus", himc); return 0; }
uint32_t ImmSetOpenStatus_c(uint32_t himc, uint32_t open) { (void)open; no_context("ImmSetOpenStatus", himc); return 0; }
uint32_t ImmGetConversionStatus_c(uint32_t himc, uint32_t conv, uint32_t sent) { (void)conv; (void)sent; no_context("ImmGetConversionStatus", himc); return 0; }
uint32_t ImmSetConversionStatus_c(uint32_t himc, uint32_t conv, uint32_t sent) { (void)conv; (void)sent; no_context("ImmSetConversionStatus", himc); return 0; }
uint32_t ImmGetCompositionStringA_c(uint32_t himc, uint32_t index, uint32_t buf, uint32_t len) { (void)index; (void)buf; (void)len; no_context("ImmGetCompositionStringA", himc); return 0; }
uint32_t ImmGetCompositionStringW_c(uint32_t himc, uint32_t index, uint32_t buf, uint32_t len) { (void)index; (void)buf; (void)len; no_context("ImmGetCompositionStringW", himc); return 0; }
uint32_t ImmGetCandidateListA_c(uint32_t himc, uint32_t index, uint32_t list, uint32_t len) { (void)index; (void)list; (void)len; no_context("ImmGetCandidateListA", himc); return 0; }
uint32_t ImmGetCandidateListW_c(uint32_t himc, uint32_t index, uint32_t list, uint32_t len) { (void)index; (void)list; (void)len; no_context("ImmGetCandidateListW", himc); return 0; }
uint32_t ImmNotifyIME_c(uint32_t himc, uint32_t action, uint32_t index, uint32_t value) { (void)action; (void)index; (void)value; no_context("ImmNotifyIME", himc); return 0; }
uint32_t ImmSimulateHotKey_c(uint32_t hwnd, uint32_t id) { (void)hwnd; (void)id; return 0; }
uint32_t ImmLockIMC_c(uint32_t himc) { no_context("ImmLockIMC", himc); return 0; }
uint32_t ImmUnlockIMC_c(uint32_t himc) { no_context("ImmUnlockIMC", himc); return 0; }
uint32_t ImmLockIMCC_c(uint32_t imcc) { no_context("ImmLockIMCC", imcc); return 0; }
uint32_t ImmUnlockIMCC_c(uint32_t imcc) { no_context("ImmUnlockIMCC", imcc); return 0; }
uint32_t ImmDisableTextFrameService_c(uint32_t tid) { (void)tid; return 1; }
