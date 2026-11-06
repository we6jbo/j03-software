# Findagrave Qt Proxy: annotated rewrite (draft)

This is a complete annotated `main.cpp` based on the 2.2.5 upload, **not a tested production release**. I did not receive `InfoArtRecorder.h`, its implementation, or the existing build files, and Qt development headers were unavailable here, so compilation and integration remain unverified. The code preserves the existing architecture and network behavior where possible.

## Locating the requested behavior

- `recommend()` : blue-button highlighting and `QTimer::singleShot`. Set *Auto-push the blue (recommended) button* and 5 seconds to schedule a click roughly five seconds after recommendation.
- Optional second check: uncomment `if(!settings.value("auto_push_blue_enabled").toBool(false))return;` inside the timer callback. Without this, turning the setting off after scheduling does **not** cancel an already queued click.
- `parseNewestHtml()` : uncomment `writeHumanInteractionReview(imp);` to write `/home/we6jbo/.treeresearchoct2326/human_interaction.json` automatically when parsing. In `recommend()`, ALSO uncomment `if(safeButton==hoBtn)return;` to require a *manual* Submit. These are two separate changes.
- `writeHumanInteractionReview()` merges family groups by source URL and includes name, birthdate/place, death date/place, father, mother, spouses, their `children` arrays, plus children not safely assigned to a spouse and related people with unknown relationships. Unknown fields remain null. The old parser only reliably extracts a few relations and does **not** verify that a child belongs to a particular spouse. Never invent a connection.
- Fetch button text now explains that the protocol requires automatic pushing **when enabled**. HTTP/site restrictions still apply.

## Git tab (corrected existing-repository workflow)

- **Local source:** `/home/we6jbo/.treeresearchoct2326/sourcecode/proxy/`
- **Existing Git checkout:** `/home/we6jbo/.j03-software/`
- **GitHub origin:** `https://github.com/we6jbo/j03-software.git` (or equivalent SSH remote).
- **Destination inside repository:** `nov42026-proxy/`.
- **Init now commits AND pushes.** It does *not* run `git init`, create a repository, or ask you to click a second button to upload. It copies only explicitly allowlisted C++ source and build files, creates a project-scoped commit, then runs `git push origin main`.
- Init refuses to act if the local checkout is missing, the remote or branch differs, the checkout is dirty, local and remote histories diverge, or unexpected files would be overwritten. You must approve a confirmation dialog which warns the existing GitHub repo may be **public**. The program will not change its visibility.
- **Sync changes + push** runs the same guarded workflow for subsequent source updates.
- **Previous/Next** browse commits touching `nov42026-proxy/` only. They restore files into the checkout and do not change branch HEAD or push. Review and resolve any restored differences before syncing again.
- All automation pauses when the Git tab is selected, with timestamps in `/home/we6jbo/.treeresearchoct2326/paused_to.log`. A `wget` process launched before the pause might still finish.
- Git command logs and a proxy-specific two-hour commit freshness indicator are shown on the Git tab.
- For GitHub authentication, configure ordinary `git` credentials or GitHub CLI outside the application. Never put a token in the source code.

**Privacy warning:** `we6jbo/j03-software` may be public. Upload only source files you have reviewed for secrets, private genealogy information, and licensing obligations. Your earlier requirement to keep a repository private cannot be guaranteed when uploading into an existing repository with potentially different visibility.

## Licensing

The user said "Apache2, MIT, GPT3". `GPT3` is not treated as a standard software license. The source **provisionally** interprets that as `GPL-3.0-only` and presents the choices as `Apache-2.0 OR MIT OR GPL-3.0-only`, in that order. The Git initializer writes a `LICENSE-CHOICES.md` notice, not full license texts. Before distribution, confirm that GPLv3 was intended, confirm source ownership, license compatibility and add the complete texts for each chosen license. Private-repo creation is not legal proof of licensing.

## Security and release notes

The original source had a hardcoded proxy password. This rewrite changes it to `qgetenv("FINDAGRAVE_PROXY_PASSWORD")`; set that environment variable before launching. The password in the uploaded original should be treated as exposed and rotated anywhere it is in use. Git never stages the live `~/.treeresearchoct2326` directory. Verify all files for credentials before pushing even to a private repo.

## Suggested minimal checks

1. Back up original `main.cpp` and ensure Qt Widgets, Qt Network, your original build configuration and `InfoArtRecorder` dependencies are present.
2. Build locally, review compile errors, and test `--version` and `--check-port` before using the GUI.
3. With a dummy handoff, test five-second auto-fetch, toggle-off during countdown, and Git-tab pausing. Do not test against real people or public servers until validated.
4. Enable the optional JSON review code only after confirming its output from local test HTML and ensuring submission is truly manual.
5. Validate Git status, history restoration, repository visibility and the two-hour freshness label in a disposable local repo before connecting to your real GitHub account.

No Git command, GitHub repository creation, or actual `wget` has been executed for the user.
