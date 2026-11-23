# M5 Decision Record

1. M4 Fixes:
I fixed reset_application() since it never invalidated an in flight build so now Reset cancels it using Building::reset_requested -> BuildWorker::cancel. Build_corpus() was also fixed since it joined a thread on the GUI thread, and build corpus stayed enabled during a build, I made it so there is only one worker so there is no join. IpcServer::stop() kept closing the server_socket_ while the server thread was reading it and could wait on a stalled client. I fixed this so it now joings first and closes after, read_data now checks active_ every 100ms. The M4 tests were also fixed, some of them were always passing and some did not test some of the constraints, I replaced these with R1-R4 tests.

2. Threads and Shared Data:
The GUI thread runs the Qt event loop, every widget and state object live in this loop and are only touched here, it is also the only thread that calls BuildWorker and cancel(). The BuildWorker thread is called once the build worker constructor joins the destructor then its run() loop waits for the condition variable until pending_ or stopping_ has a request. The request is then taken and the lock is released so the GUI is never blocked during a build, then after checking wanted(generation) it hands the finished processingcore to the GUI. Lastly the IPC Server thread accepts aiws send connections, it then validates the path and posts add_document_file() to the GUI thread.

Some shared data includes pending_, stopping_, idle_ which are used by GUI writes and worker reads always under the mutex. Wanted_generation- which is also used by GUI writes this is atomic and the GUI only writes it while holding mutex_. on_done_ and on_failed_ are used by worker calls and are const after construction.

3. States:
|   State   |   Build   |   Workspace Changed   |   Reset   |   Finished / Failed   |   exit

Not Built      Building            stay              stay              ignored         nothing

Building       Ignored            Cancel            Cancel      Matches Generation     Cancel

Current        Building          Not Build          Not Built            Ignored       nothing

4. Trace: Reset during a build, then rebuild
The GUI runs build_corpus -> NotBuilt::build_requested -> MainWindow::start_build -> BuildWorker::submit; state becomes Building(1).
The Worker run() wakes, takes the request, and build() checks wanted(1) before each document.\
GUI then calls Reset -> Building::reset_requested -> cancel_build -> BuildWorker::cancel sets wanted_generation_ to 0 -> state NotBuilt.
The Worker responds by setting the next wanted(1) to false, so build() returns and the worker waits again.
GUI finally build agains -> submit. The worker starts at once; on_done_ queues handle_build_finished -> Building(2)::build_finished -> Current.

5. Decisions:
Build during a build: While a build is running, a second Build Corpus request does nothing. I chose this instead of canceling the running build because the workspace has not changed, so the running build is still wanted, and restarting it would only waste time.

Remote add mid-build: A remote add through aiws_sen is a Workspace change, exactly like Add Document. The IPC thread posts add_document_file to the GUI thread with a call, which adds the document. In Building, wanted_generation is set to 0 and the corpus is discarded. Then calls report_status, and enters NotBuilt. Build Corpus is enabled again, and the old corpus is not shown. The worker stops and the cancelled build never becomes current.

Smallest shared data: 
The GUI thread and the worker share a request slot and a stop flag guarded the mutex including pending_, stopping_, idle_, and one atomic, wanted_generation_. The idle worker has to sleep until work or a stop request arrives, and a condition variable needs a predicate to avoid a missed wakeup. The atomic cannot be removed either, because the worker needs to learn that its build is no longer wanted between documents.

6. Fault Switches
AIWS_FAULT_NO_CANCEL: BuildWorker::wanted() in build_worker.cpp. Caught by test_r3_cancelled_build_stops_early and test_r2_outdated_request_never_built.
AIWS_FAULT_SILENT_SHUTDOWN: ~BuildWorker skips notify_all. Caught by test_r4_idle_shutdown_joins

7. AI use
I used AI to help debug my code and find where I needed to fix things from M4, also used it to help fix the tests.