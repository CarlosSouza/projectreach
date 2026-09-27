# HaloPad rights status

State: **private-engineering-authorized; publication-not-authorized**.

Chris explicitly assigned the HaloPad goal loop on 2026-09-06 and subsequently instructed use of the supplied `ref/` folder. This authorizes the private engineering and input inspection performed here. It does not establish the installer's historical source, original-PC-key provenance, or distribution rights. No credential was requested or read. No source, game input, generated code, evidence or package was published.

| Material | Inspection / current treatment | Outstanding decision |
|---|---|---|
| Supplied Halo installer and extracted executable | Private ignored input; inspected without execution | Accepted acquisition provenance and original installation/key route; exact patched profile |
| Translated game logic, initialized data, assets and prepared packages | Not generated; must remain ignored | Separate source, binary and data distribution decisions at G12 |
| SR at locked revision | README offers MIT or GPLv2+/LGPLv2.1+ for original code; SRW SConstruct includes MIT grant | Private tool builds now use a recorded patch with adjacent MIT notice; preserve file-specific and bundled library notices |
| SR bundled udis86 1.7.2 | Inspected permissive source notices; built locally with notices retained in ignored copy | Retain all component notices if distribution is later authorized |
| xboxrecomp at locked revision | Top-level MIT; runtime source inspected only | File/component provenance audit before adoption; root license is not blanket clearance |
| UTP at locked revision | `RIGHTS_AND_LICENSES.md` states no top-level license grant; read as reference | No UTP implementation copied; explicit applicable rights decision before copying |
| HaloPad scripts and documentation added in this session | New local integration work | No project-wide public license grant or release authorization inferred |

No permission is needed to continue ordinary authorized local engineering. G12 remains a separate gate for exact artifacts. This file is a project decision record, not a legal opinion.

LDC 1.42.0 is a private build-time tool from the official release, locked by archive/tree hashes. Its bundled LICENSE identifies BSD, Boost, LLVM and other component boundaries; no compiler package or runtime is being redistributed. SCons 4.8.1 package metadata declares MIT. No Halo logic is contained in the self-authored synthetic arithmetic fixture.
