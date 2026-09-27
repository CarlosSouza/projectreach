# HaloPad

Private native ARM64 Halo PC/Custom Edition feasibility and implementation workspace. **No playable native client exists yet.**

Start with [status](docs/STATUS.md) and the [handoff](docs/HANDOFF.md). The active operating loop is [phase 2](docs/HaloPad-GOAL-LOOP-PHASE2.md). The assigned requirements are [the PRD](docs/HaloPad-PRD.md), [the original goal loop](docs/HaloPad-GOAL-LOOP.md) and [inputs/research](docs/HaloPad-INPUTS-AND-RESEARCH.md).

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r scripts/requirements-tools.txt
scripts/bootstrap-sources.sh
scripts/doctor.sh
.venv/bin/python -m unittest discover -s tests -v
```

Reference and game inputs remain under ignored `ref/`; generated products and raw evidence remain ignored. Do not run supplied installers or accept an input hash merely because extraction succeeds. See [input findings](docs/INPUTS.md), [source audit](docs/SOURCE-AUDIT.md) and [rights status](docs/RIGHTS-STATUS.md). No push or publication is authorized.
