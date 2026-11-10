# START HERE - how to get the AFTER TRUE installer (no coding needed)

You do **not** need to install Visual Studio, Qt, FFmpeg or anything else on your PC.
GitHub's free Windows computers build the program for you. You only click buttons in a web browser.

> **Honest warning:** this is Milestone 1 (version 0.1.0). The core logic was tested on my side, but the
> Windows/Qt/FFmpeg part could only be compiled by GitHub's servers - never by me. So the **first build may fail** with a
> compile error. That is normal. If it happens, follow "If the build fails" at the bottom and send me the error text; I will fix it.

---------------------------------------------------------------------------------------------------

## Step 1 - Make a free GitHub account
1. Go to https://github.com and click **Sign up**. (Skip if you already have one.)

## Step 2 - Create an empty repository
1. Click the **+** at the top right, then **New repository**.
2. Repository name: `AfterTrue`
3. Choose **Public** (recommended: free, unlimited build time). *Private* also works but has a monthly build-minute limit.
4. Do **not** tick "Add a README". Click **Create repository**.

## Step 3 - Upload the project
1. Unzip `AfterTrue.zip` on your PC (right-click, **Extract All**).
2. Open the extracted folder. You should see files like `CMakeLists.txt`, `README.md` and folders like `src`, `tests`, `.github`.
   (`.github` is a normal folder - it is the most important one.)
3. On your new GitHub page, click the link **uploading an existing file**.
4. Select **everything inside** the folder (Ctrl+A) and **drag it into the browser page**.
   Wait until all files finish uploading (a few moments).
5. Scroll down and click the green **Commit changes** button.

## Step 3b - If GitHub says "too many files"
Browser upload accepts at most 100 files at a time. If you see that message, upload in two rounds:
first the folders `src` and `tests`, commit; then everything else, commit.
(Or use the free **GitHub Desktop** app: File > Add local repository > publish; it has no limit.)

## Step 4 - Watch GitHub build it
1. Click the **Actions** tab at the top of your repository.
2. If you see a green button *"I understand my workflows, go ahead and enable them"*, click it.
3. Click **Build AFTER TRUE for Windows** on the left, then the newest run. (If nothing is running, click **Run workflow** > **Run workflow**.)
4. A yellow circle = working. The **first build takes about 20-40 minutes** (it compiles FFmpeg). Later builds take ~10 minutes.
5. Green check mark = success. Red X = failed (see the bottom of this page).

## Step 5 - Download your installer
1. Click the finished (green) run.
2. Scroll to the bottom to the **Artifacts** section.
3. Click **AFTER-TRUE-Installer** - a ZIP downloads. (**AFTER-TRUE-Portable** is a no-install version.)
4. Unzip it and double-click `AFTER-TRUE-Installer-0.1.0.exe`.
5. Windows may say **"Windows protected your PC"** because the installer is not code-signed (signing costs money).
   Click **More info**, then **Run anyway**. This is expected for any unsigned program.
6. Finish the installer. Start **AFTER TRUE** from the Start menu or desktop.

## Optional - a permanent download link ("Release")
1. On your repository page click **Releases** > **Create a new release**.
2. In *Choose a tag* type `v0.1.0`, click **Create new tag**, then **Publish release**.
3. The build runs again and attaches the installer to the release page automatically.

---------------------------------------------------------------------------------------------------

## Using the program (what works today)
* **Import:** File > Import Footage (Ctrl+I) or drag files into the window. Video, images and audio files are recognised.
* **Make a layer:** double-click footage in the *Project* panel. Newest layers go on top. Stack as many as you like.
* **Move in time:** click/drag the ruler on the Timeline. Space = play/pause, Left/Right = one frame.
* **Edit layers:** drag a bar to move it, drag its edges to trim. Boxes **V / S / L** = visible / solo / lock.
  Right-click a layer for Duplicate, Split, Move Up/Down, Delete.
* **Animate:** select a layer, tick **Animate** next to a property (Position, Scale, Rotation, Opacity...), move the playhead,
  change the value - a yellow diamond (keyframe) appears. Pick Linear / Easy Ease / Hold for new keyframes.
* **Solid layers:** Layer > New Solid (Ctrl+Y).
* **Save / open:** Ctrl+S / Ctrl+O (`.atrue` files). Autosave runs every minute; after a crash the app offers recovery.
* **Export a picture:** File > Export Current Frame as PNG.
* Timeline mouse wheel: scroll rows; **Ctrl+wheel** zoom; **Shift+wheel** scroll time. `=` / `-` zoom, `;` fit.

## NOT available yet (see docs/FEATURES.md for the complete honest list)
Effects, masks, blend modes, text/shape layers, graph editor, audio playback, **video export (MP4/MOV)**, AI features,
background removal, transitions, undo/redo, GPU acceleration. These are the next milestones.

---------------------------------------------------------------------------------------------------

## If the build fails (red X)
1. Click the failed run, then click the step with the red X (probably **Configure** or **Build**).
2. Click the little arrow to expand it, scroll to the red lines containing `error`.
3. Copy that text (a screenshot is fine too) and send it to me. I will correct the code, give you a new ZIP, and you re-upload
   (on GitHub: **Add file > Upload files**, drop the changed files, **Commit changes**; the build restarts by itself).
