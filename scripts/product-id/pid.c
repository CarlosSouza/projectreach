/* Calls the Halo CE installer's own PIDGen.dll exactly as its mgspid.dll does:
   PIDGenSimpA(key, MPC "69771", SKU "Z08-00030", OEM "", retail). Key is read from
   stdin (25 characters, dashes ignored). Prints "PID <pid2>" and "DPID <hex>". */
typedef unsigned long DWORD;
typedef void *HANDLE;
__declspec(dllimport) void *__stdcall LoadLibraryA(const char *);
__declspec(dllimport) void *__stdcall GetProcAddress(void *, const char *);
__declspec(dllimport) HANDLE __stdcall GetStdHandle(DWORD);
__declspec(dllimport) int __stdcall ReadFile(HANDLE, void *, DWORD, DWORD *, void *);
__declspec(dllimport) int __stdcall WriteFile(HANDLE, const void *, DWORD, DWORD *, void *);
__declspec(dllimport) void __stdcall ExitProcess(unsigned);
typedef DWORD(__stdcall *pidgen_simp)(char *, const char *, const char *, const char *, int,
	char *, unsigned char *, DWORD *, int *);
static HANDLE out;
static void put(const char *s) { DWORD n = 0, w; while (s[n]) n++; WriteFile(out, s, n, &w, 0); }
void __stdcall start(void)
{
	char raw[128], key[32], pid2[64] = {0}, hex[3] = {0}, num[12];
	unsigned char dpid[256] = {0};
	DWORD got = 0, seq = 0, i, k = 0, r;
	int ccp = 0;
	void *dll;
	pidgen_simp gen;
	out = GetStdHandle((DWORD)-11);
	ReadFile(GetStdHandle((DWORD)-10), raw, sizeof raw - 1, &got, 0);
	for (i = 0; i < got && k < 25; i++)
		if ((raw[i] >= '0' && raw[i] <= '9') || (raw[i] >= 'A' && raw[i] <= 'Z')) key[k++] = raw[i];
	key[k] = 0;
	if (k != 25) { put("ERROR key length\n"); ExitProcess(2); }
	dll = LoadLibraryA("PIDGen.dll");
	if (!dll) { put("ERROR no PIDGen.dll\n"); ExitProcess(3); }
	gen = (pidgen_simp)GetProcAddress(dll, "PIDGenSimpA");
	if (!gen) { put("ERROR no PIDGenSimpA\n"); ExitProcess(4); }
	*(DWORD *)dpid = 0x100;
	r = gen(key, "69771", "Z08-00030", "", 0, pid2, dpid, &seq, &ccp);
	for (i = 0; i < 25; i++) key[i] = 0;
	if (r) {
		for (i = 0; i < 8; i++) num[7 - i] = "0123456789abcdef"[(r >> (4 * i)) & 15];
		num[8] = 0; put("ERROR PIDGen rejected the key, code 0x"); put(num); put("\n"); ExitProcess(5);
	}
	put("PID "); put(pid2); put("\nDPID ");
	for (i = 0; i < dpid[0]; i++) { hex[0] = "0123456789abcdef"[dpid[i] >> 4]; hex[1] = "0123456789abcdef"[dpid[i] & 15]; put(hex); }
	put("\n");
	ExitProcess(0);
}
