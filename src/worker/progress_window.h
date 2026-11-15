// progress_window.h: the small window shown while jobs run, with a progress bar and a Cancel button.
// Globals: RunProgressWindow.
// Doc: docs/notes/src/worker/progress_window.md
#pragma once
#include "progress.h"

// Runs a message loop on the calling thread until progress.finished is true. The window appears
// after 500 ms, and a quick job finishes with no window. Cancel, Esc and the close box set progress.cancel.
void RunProgressWindow(Progress& progress);
