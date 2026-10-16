GROOVE GARDEN - build on Windows without installing anything
1. Make a free account at github.com, click New repository (name: GrooveGarden).
2. Upload ALL files from this folder (keep the folder structure, including the
   hidden .github folder) - easiest: unzip, then drag everything into the repo page.
3. Open the repo's Actions tab, wait for "Build VST3" to turn green (~5-10 min).
4. Open that run, download the "GrooveGarden-VST3-Windows" artifact, unzip it.
5. Copy "Groove Garden.vst3" (the whole folder) into
   C:\Program Files\Common Files\VST3
6. FL Studio: Options > Manage plugins > Find plugins (Verify plugins on).
   Load it on a mixer insert slot or in a Patcher chain after your sound.
