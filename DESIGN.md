# M4 Decision Record

1. Threads, Ownership, and Constraint
This application implements two background threads. The corpus building thread is owned and managed by the MainWindow class. While the POSIX socket listener thread is owned and managed by the IpcServer class, which is instatiated by main.cpp. The constraint statement entails that background threads never interact with the Qt GUI directly. All UI updates that come from background operations are pushed to the main thread's event loop. All thread objects are signaled to stop and join in the destructors of their owning classes to ensure clean shutdowns.

2. Validating Finished Builds
To determine if a corpus build is valid, MainWindow keepos a workspace version counter. This counter increments every time the workspace is modified. When the user clicks on build corpus, the current version integer is captured by value into the background labda. Once the build is finished, it is then handeled by a function and compared against the integer of the live workspace verison, if they are a match the corpus is valid and deployed. If they are differnet the user modified the workspace during the build and the corpus is discarded.

3. Chnages since the checkpoint
Since the initial checkpoint, the IPC architecture was rebuild, the socket initialization and management were removed from MainWindow and isolated into an IPC server class. This decoupled the GUI from the networking implementation. Ownership of IPCserver was moved to main which now injects a callback to trigger document additions.

4. CR-1 Impact & Message Format
Impacted code: main.cpp, ipc_server.cpp, ipc_server.hpp, and aiws_send.cpp
The rest didnt change since the GUI widgets were not modified and the IPC server verifies file readbility.

5. Fault Switches and Testing
AIWS_FAULT_ACCEPT_STALE: Located in MainWindow::handle_build_finished. The application skips the workspace_version_ check and incorrectly accepts a corpus that was built from an outdated workspace. This is caught by StudentTests::test_r2_stale_corpus_rejection

AIWS_FAULT_LOSE_FAILURE: Located in MainWindow::handle_build_failed. The application catches the exception from the background thread but fails to update the UI status label or unlock the buttons, leaving the app stuck in a Building state.

6. AI-Use disclosure
AI was used to help debug code, run the workspace, test remote adding, and target syntax errors.

Partner Name: Arkadiy Fusyak
Sessions: 1. 11/8/2026 12:00PM - 5:00PM  Worked on Arkadiy’s code
	     2. 11/8/2026 5:30PM - 10:30PM Worked on Joel’s code

Swapped every 30-45 minutes

Role Contributions: When working on MainWindow after moving the IPC server initialization to main I ran into a compiler error “no matching function for call to IpcServer::IpcServer(MainWindow*)” My partner noticed that there was leftover code trying to call the IpcServer inside the main window. This reminded me that the new constructor required two arguments and that the old instance needed to be deleted.

Best Interaction: Me and my partner interacted through discord screen sharing and discussion of each of our codes.
