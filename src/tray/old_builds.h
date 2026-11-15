// old_builds.h: removes the binaries of earlier builds from the AddToTarGz folder.
// Globals: RemoveOldBuilds.
// Doc: docs/notes/src/tray/old_builds.md
#pragma once

// Installer.iss puts each build into its own folder, %LOCALAPPDATA%\AddToTarGz\build-<date>-<time>. Deletes the
// binaries of every other build folder and of the AddToTarGz folder itself (builds before build folders), and each
// build folder left empty. A file another program has loaded stays, and the next start tries again. Does nothing
// when this exe is not in a build folder.
void RemoveOldBuilds();
