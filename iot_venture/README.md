# Local Quality Gates & CI for Zephyr Projects

This section explains how to:

1.	Enable local Git hooks to block pushes when tests fail (Option B: default .git/hooks/),

2.	Set up a GitHub Actions CI pipeline that builds & runs your Zephyr tests in Docker, and

3.	Optionally switch to Twister to run many tests at once.

Repo layout assumed (update paths if your folder structure differs):

```zsh
.
├── LAB_CI/                      # your app & tests live here
│   └── tests/SUM_UNIT_TEST/     # ztest-based unit test
├── zephyr/                      # Zephyr source (checked-in or fetched by west)
└── .githooks/                   # optional custom hooks folder
```


⸻

## 1 Local pre-push hook

Use a pre-push hook so that *git push* fails if the test does not build or run successfully within the Zephyr Docker image.

### 1.1 Create the hook

Place this file at .git/hooks/pre-push (no file extension) and make it executable.

For macOS/Linux:

```c
chmod +x .git/hooks/pre-push
```

For Windows (PowerShell):

```c
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
bash -c "chmod +x .git/hooks/pre-push"
```

### 1.2 Update directory paths based on your project structure

If your test folder or project layout is different, open .git/hooks/pre-push and update:

APP_DIR – path to your Zephyr test or app directory, e.g.:

```c
APP_DIR="LAB_CI/tests/SUM_UNIT_TEST"
```

If your tests are under myproject/tests/my_test, then:

```c
APP_DIR="myproject/tests/my_test"
```

BOARD – the board to target for build. Common options:

```c
BOARD="native_sim/native/64"      # host simulation
BOARD="nrf7002dk/nrf5340/cpuapp" # Nordic hardware
```

IMAGE – the Zephyr CI Docker image:

```zsh
IMAGE="ghcr.io/zephyrproject-rtos/zephyr-build:main"
```

### 1.3 How the hook works (high level)
- Verifies your repo has a zephyr/ directory.
- Runs a Docker container, mounts the repo at /workdir, and executes:
- `west --version` (sanity check)
- `west build -p always -b "$BOARD" /workdir/$APP_DIR --pristine` (builds for the board)
- `west build -t run /workdir/$APP_DIR` (run tests)
- If any command fails, the hook exits non-zero and blocks the push.

### 1.4 Test/bypass
- Dry-run the hook manually to test it out:

.git/hooks/pre-push

- Bypass the hook for one push (use sparingly):

```zsh
git push --no-verify
```

## Git Hooks - Option B

Why Option B? It uses Git’s default hooks location (.git/hooks/), so every clone works immediately with additional setup.

(Optional) Option A — use a custom hooks folder

If you prefer to store hooks under version control (.githooks/):

```zsh
git config core.hooksPath .githooks
chmod +x .githooks/pre-push
```

Verify with:

```zsh
git config --get core.hooksPath   # should print .githooks
```

⸻

## 2 GitHub Actions — CI pipeline

GitHub Actions automatically builds and tests your firmware on every push or pull request. It can also store the results of testing and stash your build output files (such as the .hex files). All of this is done in a consistent build environment, such as using Docker images.

### 2.1 Where to put it

Create: `.github/workflows/ta_test.yml` (Or whatever file name you'd like)

### 2.2 What it does (high level)
- Checks out your repo.
- Frees disk space (GitHub runners have limited storage).
- Pulls the official Zephyr Docker image.
- Runs west build and run in a clean Docker environment.
- Uploads build artifacts (like .elf or logs).

### 2.3 Things to modify for your project
- APP_DIR → same as in your hook: LAB_CI/tests/SUM_UNIT_TEST
- BOARD → either native_sim/native/64 for simulation or your real board.
- Artifacts path → where your build output lives, e.g., LAB_CI/tests/SUM_UNIT_TEST/build/
- Disk cleanup scope → adjust if your build needs any of the removed tools.

## 3 Running multiple tests with Twister

Twister is Zephyr’s native test runner that can handle multiple test cases and platforms.

### 3.1 Replace single west build steps with Twister commands
- Run one test on one platform:

```zsh
west twister -p native_sim/native/64 -s sum_log.test
```

- Run all tests under a folder:
  
```zsh
west twister -p native_sim/native/64 -T LAB_CI/tests
````

- Generate reports (for CI uploads):
  
```zsh
west twister -p native_sim/native/64 -T LAB_CI/tests \
  --report-junit twister.xml \
  --report-json twister.json
```


### 3.2 Common Twister customizations
- Platform – use -p qemu_cortex_m3 for QEMU or -p nrf7002dk/nrf5340/cpuapp for Nordic.
- Selective tests – target multiple test IDs using multiple -s options.
- Parallel jobs – use -j <N> to speed up builds.

## Conclusion

With this setup, you now have:
- Local pre-push testing that blocks broken code before pushing.
- Cloud CI verification on every commit.
- Twister automation for scaling to multiple tests and boards effortlessly.

## Appendix

### Windows-specific notes
- Docker Desktop must be running, and your repo must be under a shared drive (C:/Users/...).
- Paths are automatically translated by Docker, but ensure volumes are quoted properly:

```zsh
docker run --rm -v "${PWD}:/workdir" -w /workdir ghcr.io/zephyrproject-rtos/zephyr-build:main bash -lc 'west build -p always -b native_sim/native/64 LAB_CI/tests/SUM_UNIT_TEST'
```

- Use PowerShell for commands, or prefix Bash commands with bash -c if needed.

### Troubleshooting quick hits
- “west: unknown command build” → You’re not in a west workspace. Run west init && west update once.
- SDK errors → For native_sim, use the host toolchain inside Docker; don’t install Zephyr SDK manually.
- “Path not found” → Check that APP_DIR matches your repo’s folder names.
- Low space in CI → Keep the cleanup step or use actions/cache to reuse builds.
