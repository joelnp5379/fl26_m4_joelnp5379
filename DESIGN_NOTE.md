# M4 Design Note
1. Components and Threads

GUI Thread: Manages user interactions, increments the workspace version, generates the background work, and receives the built corpus through invokeMethod.
Background Work: Executes rebuild() to simulate the delayed build, while having no direct access to GUI widgets

2. Corpus States and Transitions
Empty: Initial state or post-Reset. (Event: App launch, reset click)
Stale: Document added, build not yet completed. (Event: Add Document click)
Building: Background thread is running rebuild() (Event: Worker thread generated)
Current: Build finished successfully and matches the workspace. (Event: GUI receives corpus)
Failed: Build encounters an error. (Event: GUI receives an error. (Event: GUI receives error)

3. Threat Analysis (R1-R4)
Threat to R1 (Responsiveness): Build blocks UI. Handling: Execute rebuild() on a detached thread.
Threat to R2 (Correctness): Handling: MainWindow manages a workspace id increment on changes. The worker thread grabs the ID at the start and returns it upon completion. GUI discards the corpus if the returned ID does not match the current ID. 
Step by step walkthrough 1: 
User adds Doc A: Thread 1 spawns with ID 1
User adds DOc B. Thread w spawns with ID 2
Thread 1 finishes, passes corpus and ID 1 to GUI. GUI sees 1 != 2 and discards the corpus.
Threat to R3 (Failure): Build fails silently. Handling: Worker passes an error flag to the GUI slot and the GUI enters failed state.
Threat to R4 (Lifecycle): Handling: MainWindow tracks the thread. When destruction is called it signals a cancellation flag and calls join().
Step by Step walkthrough 2:
The user initiates the build and worker thread begins processing.
When the user clicks exit, MainWIndow destructor is executed.
Destructor sets cancel flag equal to true and blocks on worker_thread.join(). Then the worker checks the flag and aborts the application safely.

4. Corpus Ownership Path
Created locally in the background thread then wrapped in a unique pointer then moved into a lambda captured by QMetaObject, this is lastly executed on the GUI thread. Which will move the unique pointer into the current corpus.

5. Rejected Alternative
Polling std::future through QTimer: This is rejected because continuously checking on the GUI thread wastes CPU cycles and adds latency.

Due Wed Oct 28, 11:59 pm. See specification Section 7.
